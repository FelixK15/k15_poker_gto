// Hand abstraction: bucket functions per street.

#include "gto/mc/abstraction.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <numeric>

#include "gto/hand_evaluator.h"
#include "gto/range.h"
#if GTO_HAVE_OMAHA
#include "gto/omaha.h"
#endif

namespace gto::mc {

namespace {

/// binom[n][k] for n <= 52, k <= 5.
struct Binomials {
  std::array<std::array<std::uint64_t, 6>, 53> v{};
  Binomials() {
    for (int n = 0; n <= 52; ++n) {
      v[n][0] = 1;
      for (int k = 1; k <= 5; ++k) v[n][k] = n == 0 ? 0 : v[n - 1][k - 1] + v[n - 1][k];
    }
  }
};
const Binomials& binomials() {
  static const Binomials b;
  return b;
}
std::uint64_t choose(int n, int k) { return n < k || n < 0 ? 0 : binomials().v[n][k]; }

/// Colex rank of a set of distinct positions (any order) among 0..m-1.
std::uint64_t colex_rank(std::uint8_t* pos, int k) {
  std::sort(pos, pos + k);
  std::uint64_t rank = 0;
  for (int j = 0; j < k; ++j) rank += choose(pos[j], j + 1);
  return rank;
}

/// Position of each card in the deck without the starting board.
struct DeckIndex {
  std::array<std::int8_t, 52> pos{};
  int size = 0;
  explicit DeckIndex(std::span<const Card> start_board) {
    std::uint64_t used = 0;
    for (const Card c : start_board) used |= c.mask();
    for (int c = 0; c < 52; ++c) {
      if (used & (std::uint64_t{1} << c)) {
        pos[c] = -1;
      } else {
        pos[c] = static_cast<std::int8_t>(size++);
      }
    }
  }
};

/// Ways the cards dealt between the start street and `street` can fall,
/// counted as the lossless abstraction indexes them.
std::uint64_t deal_slots(Street start, Street street, int deck) {
  std::uint64_t slots = 1;
  for (int s = static_cast<int>(start) + 1; s <= static_cast<int>(street); ++s) {
    slots *= s == static_cast<int>(Street::Flop) ? choose(deck, 3) : static_cast<std::uint64_t>(deck);
  }
  return slots;
}

// --- Bucket functions -------------------------------------------------------

class ToyBuckets final : public BucketFunction {
 public:
  explicit ToyBuckets(int deck) : deck_(deck) {}
  int num_buckets(int) const override { return deck_; }
  int bucket(int, const HoleHand& hand, int, const std::uint8_t*) const override {
    return hand.cards[0];
  }
  std::string describe() const override { return "toy deck: one bucket per card"; }
  std::size_t memory_bytes() const override { return 0; }

 private:
  int deck_;
};

class Holdem169 final : public BucketFunction {
 public:
  int num_buckets(int) const override { return 169; }
  int bucket(int, const HoleHand& hand, int, const std::uint8_t*) const override {
    return holdem_class(Card(hand.cards[0]), Card(hand.cards[1]));
  }
  std::string describe() const override { return "169 hand classes"; }
  std::size_t memory_bytes() const override { return 0; }
};

class LosslessBuckets final : public BucketFunction {
 public:
  LosslessBuckets(const GameConfig& config, Street street, std::span<const std::size_t> live)
      : deck_(config.board), start_(config.start_street), street_(street),
        start_size_(board_size(config.start_street)) {
    slots_ = deal_slots(start_, street_, deck_.size);
    for (std::size_t p = 0; p < live.size(); ++p) counts_[p] = static_cast<int>(live[p] * slots_);
  }
  int num_buckets(int player) const override { return counts_[static_cast<std::size_t>(player)]; }
  int bucket(int, const HoleHand&, int live_index, const std::uint8_t* board) const override {
    std::uint64_t deal = 0;
    int at = start_size_;
    for (int s = static_cast<int>(start_) + 1; s <= static_cast<int>(street_); ++s) {
      if (s == static_cast<int>(Street::Flop)) {
        std::uint8_t pos[3];
        for (int j = 0; j < 3; ++j) pos[j] = static_cast<std::uint8_t>(deck_.pos[board[at + j]]);
        deal = deal * choose(deck_.size, 3) + colex_rank(pos, 3);
        at += 3;
      } else {
        deal = deal * static_cast<std::uint64_t>(deck_.size) +
               static_cast<std::uint64_t>(deck_.pos[board[at]]);
        at += 1;
      }
    }
    return static_cast<int>(static_cast<std::uint64_t>(live_index) * slots_ + deal);
  }
  std::string describe() const override {
    return slots_ == 1 ? "lossless (one bucket per hand)"
                       : "lossless (hand x " + std::to_string(slots_) + " deals)";
  }
  std::size_t memory_bytes() const override { return 0; }

 private:
  DeckIndex deck_;
  Street start_;
  Street street_;
  int start_size_;
  std::uint64_t slots_ = 1;
  std::array<int, kMaxPlayers> counts_{};
};

/// Lazily filled, thread-safe table of per-board data keyed by the set of
/// cards dealt since the start of the spot.
template <typename T>
class BoardCache {
 public:
  BoardCache(std::span<const Card> start_board, int k)
      : deck_(start_board), k_(k), slots_(choose(deck_.size, k)),
        cells_(std::make_unique<std::atomic<const T*>[]>(slots_)) {
    for (std::uint64_t i = 0; i < slots_; ++i) cells_[i].store(nullptr, std::memory_order_relaxed);
  }
  ~BoardCache() {
    for (std::uint64_t i = 0; i < slots_; ++i) delete cells_[i].load(std::memory_order_relaxed);
  }
  BoardCache(const BoardCache&) = delete;
  BoardCache& operator=(const BoardCache&) = delete;

  std::uint64_t slot(const std::uint8_t* dealt) const {
    std::uint8_t pos[5];
    for (int j = 0; j < k_; ++j) pos[j] = static_cast<std::uint8_t>(deck_.pos[dealt[j]]);
    return colex_rank(pos, k_);
  }

  template <typename Make>
  const T& get(const std::uint8_t* dealt, Make&& make) const {
    const std::uint64_t i = slot(dealt);
    const T* have = cells_[i].load(std::memory_order_acquire);
    if (have != nullptr) return *have;
    T* made = new T(make());
    const T* expected = nullptr;
    if (cells_[i].compare_exchange_strong(expected, made, std::memory_order_acq_rel)) {
      filled_.fetch_add(1, std::memory_order_relaxed);
      return *made;
    }
    delete made;
    return *expected;
  }

  std::uint64_t slots() const noexcept { return slots_; }
  std::uint64_t filled() const noexcept { return filled_.load(std::memory_order_relaxed); }

 private:
  DeckIndex deck_;
  int k_;
  std::uint64_t slots_;
  std::unique_ptr<std::atomic<const T*>[]> cells_;
  mutable std::atomic<std::uint64_t> filled_{0};
};

class HoldemStrengthBuckets final : public BucketFunction {
 public:
  HoldemStrengthBuckets(const GameConfig& config, Street street)
      : start_size_(board_size(config.start_street)), size_(board_size(street)),
        cfg_(config.abstraction.streets[static_cast<std::size_t>(street)]),
        cache_(config.board, size_ - start_size_) {}
  int num_buckets(int) const override { return cfg_.strength_bins * cfg_.potential_bins; }
  int bucket(int, const HoleHand& hand, int, const std::uint8_t* board) const override {
    const auto& table = cache_.get(board + start_size_, [&] {
      std::vector<Card> cards;
      for (int i = 0; i < size_; ++i) cards.emplace_back(board[i]);
      return holdem_strength_table(cards, cfg_.strength_bins, cfg_.potential_bins);
    });
    return table[static_cast<std::size_t>(hand.combo)];
  }
  std::string describe() const override {
    if (size_ == 5) return std::to_string(num_buckets(0)) + " strength buckets (HS)";
    return std::to_string(cfg_.strength_bins) + " EHS x " + std::to_string(cfg_.potential_bins) +
           " EHS2 strength buckets";
  }
  std::size_t memory_bytes() const override {
    return cache_.slots() * sizeof(void*) + cache_.filled() * (1326 * 2 + 32);
  }

 private:
  int start_size_;
  int size_;
  StreetAbstraction cfg_;
  BoardCache<std::vector<std::uint16_t>> cache_;
};

#if GTO_HAVE_OMAHA

class OmahaPreflopBuckets final : public BucketFunction {
 public:
  explicit OmahaPreflopBuckets(int buckets) : buckets_(buckets), table_(kOmahaCombos) {
    const std::vector<int>& rank = omaha_preflop_rank();
    const auto n = static_cast<std::uint64_t>(rank.size());
    for (std::uint64_t k = 0; k < n; ++k) {
      table_[static_cast<std::size_t>(rank[k])] =
          static_cast<std::uint16_t>(k * static_cast<std::uint64_t>(buckets_) / n);
    }
  }
  int num_buckets(int) const override { return buckets_; }
  int bucket(int, const HoleHand& hand, int, const std::uint8_t*) const override {
    return table_[static_cast<std::size_t>(hand.combo)];
  }
  std::string describe() const override {
    return std::to_string(buckets_) + " preflop rank percentile buckets";
  }
  std::size_t memory_bytes() const override { return table_.size() * 2; }

 private:
  int buckets_;
  std::vector<std::uint16_t> table_;
};

/// Per-board PLO data: an evaluator and the sorted strengths of a fixed
/// sample of opponent holdings.
struct OmahaBoard {
  std::unique_ptr<OmahaBoardEvaluator> eval;
  std::vector<HandStrength> sample;  // sorted

  double hs(const std::array<Card, 4>& hole) const {
    const HandStrength s = eval->evaluate(hole);
    const auto lo = std::lower_bound(sample.begin(), sample.end(), s);
    const auto hi = std::upper_bound(lo, sample.end(), s);
    return (static_cast<double>(lo - sample.begin()) + 0.5 * static_cast<double>(hi - lo)) /
           static_cast<double>(sample.size());
  }
};

/// Shared per-board caches for all PLO strength streets (the potential on a
/// street looks at boards of the next street).
class OmahaBoards {
 public:
  OmahaBoards(std::span<const Card> start_board, int samples)
      : start_(start_board.begin(), start_board.end()), samples_(samples) {
    for (int size = std::max<int>(3, static_cast<int>(start_.size())); size <= 5; ++size) {
      caches_[static_cast<std::size_t>(size)] = std::make_unique<BoardCache<OmahaBoard>>(
          start_board, size - static_cast<int>(start_.size()));
    }
  }

  const OmahaBoard& get(const std::uint8_t* board, int size) const {
    return caches_[static_cast<std::size_t>(size)]->get(board + start_.size(), [&] {
      std::vector<Card> cards;
      std::uint64_t used = 0;
      for (int i = 0; i < size; ++i) {
        cards.emplace_back(board[i]);
        used |= cards.back().mask();
      }
      OmahaBoard b;
      b.eval = std::make_unique<OmahaBoardEvaluator>(cards);
      // Deterministic sample: a fixed LCG seeded by the board.
      std::uint64_t state = used * 0x9E3779B97F4A7C15ull + 12345;
      b.sample.reserve(static_cast<std::size_t>(samples_));
      while (static_cast<int>(b.sample.size()) < samples_) {
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        const int combo = static_cast<int>((state >> 33) % static_cast<std::uint64_t>(kOmahaCombos));
        const auto hole = omaha_combo_cards(combo);
        std::uint64_t m = 0;
        for (const Card c : hole) m |= c.mask();
        if (m & used) continue;
        b.sample.push_back(b.eval->evaluate(hole));
      }
      std::sort(b.sample.begin(), b.sample.end());
      return b;
    });
  }
  std::size_t memory_bytes() const {
    std::size_t total = 0;
    for (const auto& c : caches_) {
      if (c) total += c->slots() * sizeof(void*) +
                      c->filled() * (1326 * 4 + static_cast<std::size_t>(samples_) * 4 + 64);
    }
    return total;
  }

 private:
  std::vector<Card> start_;
  int samples_;
  std::array<std::unique_ptr<BoardCache<OmahaBoard>>, 6> caches_;
};

class OmahaStrengthBuckets final : public BucketFunction {
 public:
  OmahaStrengthBuckets(const GameConfig& config, Street street, std::shared_ptr<OmahaBoards> boards)
      : size_(board_size(street)), cfg_(config.abstraction.streets[static_cast<std::size_t>(street)]),
        potential_cards_(config.abstraction.omaha_potential_cards), boards_(std::move(boards)) {}
  int num_buckets(int) const override { return cfg_.strength_bins * cfg_.potential_bins; }
  int bucket(int, const HoleHand& hand, int, const std::uint8_t* board) const override {
    const std::array<Card, 4> hole{Card(hand.cards[0]), Card(hand.cards[1]), Card(hand.cards[2]),
                                   Card(hand.cards[3])};
    const int total = num_buckets(0);
    if (size_ == 5) {
      const double hs = boards_->get(board, 5).hs(hole);
      return std::min(total - 1, static_cast<int>(hs * total));
    }
    std::uint64_t used = hand.mask;
    for (int i = 0; i < size_; ++i) used |= std::uint64_t{1} << board[i];
    std::uint8_t next[5];
    std::copy(board, board + size_, next);
    double sum = 0, sum2 = 0;
    int count = 0;
    // Deterministic, evenly spread next cards.
    const int step = std::max(1, 52 / potential_cards_);
    int card = static_cast<int>((used * 0x9E3779B97F4A7C15ull) >> 58) % 52;
    for (int tries = 0; tries < 52 && count < potential_cards_; ++tries) {
      card = (card + step) % 52;
      while (used & (std::uint64_t{1} << card)) card = (card + 1) % 52;
      used |= std::uint64_t{1} << card;
      next[size_] = static_cast<std::uint8_t>(card);
      const double hs = boards_->get(next, size_ + 1).hs(hole);
      sum += hs;
      sum2 += hs * hs;
      ++count;
    }
    const double ehs = sum / count;
    const double sd = std::sqrt(std::max(0.0, sum2 / count - ehs * ehs));
    const int a = std::min(cfg_.strength_bins - 1, static_cast<int>(ehs * cfg_.strength_bins));
    const int b = std::min(cfg_.potential_bins - 1, static_cast<int>(sd / 0.5 * cfg_.potential_bins));
    return a * cfg_.potential_bins + b;
  }
  std::string describe() const override {
    return std::to_string(num_buckets(0)) + " PLO strength buckets";
  }
  std::size_t memory_bytes() const override { return boards_->memory_bytes(); }

 private:
  int size_;
  StreetAbstraction cfg_;
  int potential_cards_;
  std::shared_ptr<OmahaBoards> boards_;
};

#endif  // GTO_HAVE_OMAHA

/// Assigns quantile bins to values; equal values share the bin of their
/// middle rank.
void quantile_bins(std::vector<std::pair<float, int>>& items, int bins, std::vector<int>& out) {
  std::sort(items.begin(), items.end());
  const std::size_t n = items.size();
  out.assign(n, 0);
  std::size_t i = 0;
  while (i < n) {
    std::size_t j = i;
    while (j < n && items[j].first == items[i].first) ++j;
    const double mid = 0.5 * static_cast<double>(i + j - 1);
    const int bin = std::min(bins - 1, static_cast<int>(mid * bins / static_cast<double>(n)));
    for (std::size_t k = i; k < j; ++k) out[k] = bin;
    i = j;
  }
}

}  // namespace

// --- Public helpers ---------------------------------------------------------

HoleHand decode_combo(const GameConfig& config, int combo) {
  HoleHand hand;
  hand.combo = combo;
  if (config.toy_deck_size > 0) {
    hand.count = 1;
    hand.cards[0] = static_cast<std::uint8_t>(combo);
  } else if (config.variant == Variant::Holdem) {
    const auto [a, b] = Range::combo_cards(combo);
    hand.count = 2;
    hand.cards[0] = a.index();
    hand.cards[1] = b.index();
  } else {
#if GTO_HAVE_OMAHA
    const auto cards = omaha_combo_cards(combo);
    hand.count = 4;
    for (int i = 0; i < 4; ++i) hand.cards[static_cast<std::size_t>(i)] = cards[static_cast<std::size_t>(i)].index();
#endif
  }
  for (int i = 0; i < hand.count; ++i) hand.mask |= std::uint64_t{1} << hand.cards[static_cast<std::size_t>(i)];
  return hand;
}

std::vector<PlayerHands> live_hands(const GameConfig& config) {
  std::uint64_t board = 0;
  for (const Card c : config.board) board |= c.mask();
  const int combos = config.num_combos();
  std::vector<PlayerHands> result(static_cast<std::size_t>(config.num_players));
  for (int p = 0; p < config.num_players; ++p) {
    PlayerHands& ph = result[static_cast<std::size_t>(p)];
    ph.index_of_combo.assign(static_cast<std::size_t>(combos), -1);
    for (int c = 0; c < combos; ++c) {
      const float w = config.weight(p, c);
      if (!(w > 0)) continue;
      HoleHand hand = decode_combo(config, c);
      if (hand.mask & board) continue;
      hand.weight = w;
      ph.index_of_combo[static_cast<std::size_t>(c)] = static_cast<std::int32_t>(ph.hands.size());
      ph.hands.push_back(hand);
    }
  }
  return result;
}

std::vector<std::size_t> live_counts(const GameConfig& config) {
  std::uint64_t board = 0;
  for (const Card c : config.board) board |= c.mask();
  const int combos = config.num_combos();
  std::vector<std::size_t> counts(static_cast<std::size_t>(config.num_players), 0);
  for (int p = 0; p < config.num_players; ++p) {
    for (int c = 0; c < combos; ++c) {
      if (!(config.weight(p, c) > 0)) continue;
      if (board != 0 && (decode_combo(config, c).mask & board) != 0) continue;
      ++counts[static_cast<std::size_t>(p)];
    }
  }
  return counts;
}

int holdem_class(Card a, Card b) noexcept {
  int hi = a.rank(), lo = b.rank();
  if (hi < lo) std::swap(hi, lo);
  if (hi == lo) return (12 - hi) * 13 + (12 - hi);
  if (a.suit() == b.suit()) return (12 - hi) * 13 + (12 - lo);
  return (12 - lo) * 13 + (12 - hi);
}

std::string holdem_class_name(int cls) {
  const int row = cls / 13, col = cls % 13;
  std::string name;
  if (row == col) {
    name = {rank_to_char(12 - row), rank_to_char(12 - row)};
  } else if (row < col) {
    name = {rank_to_char(12 - row), rank_to_char(12 - col), 's'};
  } else {
    name = {rank_to_char(12 - col), rank_to_char(12 - row), 'o'};
  }
  return name;
}

int holdem_class_combos(int cls) noexcept {
  const int row = cls / 13, col = cls % 13;
  return row == col ? 6 : (row < col ? 4 : 12);
}

void holdem_hand_strength(std::span<const Card> board, float* out) {
  std::uint64_t used = 0;
  for (const Card c : board) used |= c.mask();
  struct Entry {
    HandStrength strength;
    std::uint16_t combo;
    std::uint8_t a, b;
  };
  Entry list[1326];
  int n = 0;
  Card cards[7];
  const int nb = static_cast<int>(board.size());
  for (int i = 0; i < nb; ++i) cards[i] = board[static_cast<std::size_t>(i)];
  for (int combo = 0; combo < Range::kNumCombos; ++combo) {
    const auto [a, b] = Range::combo_cards(combo);
    if (((a.mask() | b.mask()) & used) != 0) {
      out[combo] = -1;
      continue;
    }
    cards[nb] = a;
    cards[nb + 1] = b;
    list[n++] = {evaluate_hand(std::span<const Card>(cards, static_cast<std::size_t>(nb + 2))),
                 static_cast<std::uint16_t>(combo), a.index(), b.index()};
  }
  std::sort(list, list + n, [](const Entry& x, const Entry& y) { return x.strength < y.strength; });
  int count[52] = {};
  for (int i = 0; i < n; ++i) {
    ++count[list[i].a];
    ++count[list[i].b];
  }
  int below_total = 0;
  int below[52] = {};
  int group[52] = {};
  int i = 0;
  while (i < n) {
    int j = i;
    while (j < n && list[j].strength == list[i].strength) {
      ++group[list[j].a];
      ++group[list[j].b];
      ++j;
    }
    const int size = j - i;
    for (int k = i; k < j; ++k) {
      const Entry& e = list[k];
      const int beaten = below_total - below[e.a] - below[e.b];
      const int ties = size - group[e.a] - group[e.b] + 1;
      const int total = n - count[e.a] - count[e.b] + 1;
      out[e.combo] = total > 0 ? (static_cast<float>(beaten) + 0.5f * static_cast<float>(ties)) /
                                     static_cast<float>(total)
                               : 0.5f;
    }
    for (int k = i; k < j; ++k) {
      group[list[k].a] = 0;
      group[list[k].b] = 0;
      ++below[list[k].a];
      ++below[list[k].b];
    }
    below_total += size;
    i = j;
  }
}

std::vector<std::uint16_t> holdem_strength_table(std::span<const Card> board, int strength_bins,
                                                 int potential_bins) {
  std::vector<std::uint16_t> table(1326, 0);
  std::vector<float> hs(1326);
  holdem_hand_strength(board, hs.data());
  std::vector<int> bins;
  if (board.size() >= 5) {
    std::vector<std::pair<float, int>> items;
    for (int c = 0; c < 1326; ++c) {
      if (hs[static_cast<std::size_t>(c)] >= 0) items.push_back({hs[static_cast<std::size_t>(c)], c});
    }
    quantile_bins(items, strength_bins * potential_bins, bins);
    for (std::size_t k = 0; k < items.size(); ++k) {
      table[static_cast<std::size_t>(items[k].second)] = static_cast<std::uint16_t>(bins[k]);
    }
    return table;
  }
  std::uint64_t used = 0;
  for (const Card c : board) used |= c.mask();
  std::vector<double> sum(1326, 0.0), sum2(1326, 0.0);
  std::vector<int> cnt(1326, 0);
  std::vector<Card> next(board.begin(), board.end());
  next.push_back(Card());
  std::vector<float> hs2(1326);
  for (int x = 0; x < 52; ++x) {
    if (used & (std::uint64_t{1} << x)) continue;
    next.back() = Card(static_cast<std::uint8_t>(x));
    holdem_hand_strength(next, hs2.data());
    for (int c = 0; c < 1326; ++c) {
      const float v = hs2[static_cast<std::size_t>(c)];
      if (v < 0 || hs[static_cast<std::size_t>(c)] < 0) continue;
      sum[static_cast<std::size_t>(c)] += v;
      sum2[static_cast<std::size_t>(c)] += static_cast<double>(v) * v;
      ++cnt[static_cast<std::size_t>(c)];
    }
  }
  std::vector<std::pair<float, int>> items;
  for (int c = 0; c < 1326; ++c) {
    if (hs[static_cast<std::size_t>(c)] < 0) continue;
    items.push_back({static_cast<float>(sum[static_cast<std::size_t>(c)] / cnt[static_cast<std::size_t>(c)]), c});
  }
  quantile_bins(items, strength_bins, bins);
  // Group by primary bin, then quantiles of EHS² inside.
  std::vector<std::vector<std::pair<float, int>>> groups(static_cast<std::size_t>(strength_bins));
  for (std::size_t k = 0; k < items.size(); ++k) {
    const int c = items[k].second;
    groups[static_cast<std::size_t>(bins[k])].push_back(
        {static_cast<float>(sum2[static_cast<std::size_t>(c)] / cnt[static_cast<std::size_t>(c)]), c});
  }
  std::vector<int> sub;
  for (int g = 0; g < strength_bins; ++g) {
    auto& group = groups[static_cast<std::size_t>(g)];
    if (group.empty()) continue;
    quantile_bins(group, potential_bins, sub);
    for (std::size_t k = 0; k < group.size(); ++k) {
      table[static_cast<std::size_t>(group[k].second)] =
          static_cast<std::uint16_t>(g * potential_bins + sub[k]);
    }
  }
  return table;
}

// --- Planning and construction ----------------------------------------------

std::array<StreetPlan, kNumStreets> plan_abstraction(const GameConfig& config,
                                                     std::span<const std::size_t> live) {
  std::array<StreetPlan, kNumStreets> plans;
  const int n = config.num_players;
  const int start_size = board_size(config.start_street);
  const int deck = 52 - start_size;
  for (int s = static_cast<int>(config.start_street); s <= static_cast<int>(config.last_street); ++s) {
    const Street street = static_cast<Street>(s);
    StreetPlan& plan = plans[static_cast<std::size_t>(s)];
    const StreetAbstraction& a = config.abstraction.streets[static_cast<std::size_t>(s)];
    const auto fill = [&](std::uint64_t b) {
      for (int p = 0; p < n; ++p) plan.buckets[static_cast<std::size_t>(p)] = b;
    };
    if (config.toy_deck_size > 0) {
      plan.kind = AbstractionKind::Lossless;
      fill(static_cast<std::uint64_t>(config.toy_deck_size));
      plan.description = "toy deck: one bucket per card";
      continue;
    }
    if (street == Street::Preflop) {
      if (config.variant == Variant::Holdem) {
        plan.kind = AbstractionKind::Lossless;
        fill(169);
        plan.description = "169 hand classes";
      } else {
        plan.kind = AbstractionKind::Strength;
        fill(static_cast<std::uint64_t>(config.abstraction.omaha_preflop_buckets));
        plan.cache_bytes = 270725 * 2;
        plan.description = std::to_string(config.abstraction.omaha_preflop_buckets) +
                           " preflop rank percentile buckets";
      }
      continue;
    }
    const std::uint64_t slots = deal_slots(config.start_street, street, deck);
    std::uint64_t largest = 0;
    for (int p = 0; p < n; ++p) largest = std::max<std::uint64_t>(largest, live[static_cast<std::size_t>(p)] * slots);
    bool lossless = a.kind == AbstractionKind::Lossless ||
                    (a.kind == AbstractionKind::Auto && largest <= config.abstraction.lossless_limit);
    if (lossless && largest > 0x7FFFFFFFull) lossless = false;
    if (lossless) {
      plan.kind = AbstractionKind::Lossless;
      for (int p = 0; p < n; ++p) plan.buckets[static_cast<std::size_t>(p)] = live[static_cast<std::size_t>(p)] * slots;
      plan.description = slots == 1 ? "lossless (one bucket per hand)"
                                    : "lossless (hand x " + std::to_string(slots) + " deals)";
    } else {
      plan.kind = AbstractionKind::Strength;
      fill(static_cast<std::uint64_t>(a.strength_bins * a.potential_bins));
      const std::uint64_t boards = choose(deck, board_size(street) - start_size);
      if (config.variant == Variant::Holdem) {
        plan.cache_bytes = boards * (1326 * 2 + 32 + 8);
        plan.description = board_size(street) == 5
                               ? std::to_string(a.strength_bins * a.potential_bins) + " strength buckets (HS)"
                               : std::to_string(a.strength_bins) + " EHS x " +
                                     std::to_string(a.potential_bins) + " EHS2 strength buckets";
      } else {
        plan.cache_bytes = boards * (1326 * 4 + static_cast<std::uint64_t>(config.abstraction.omaha_strength_samples) * 4 + 72);
        plan.description = std::to_string(a.strength_bins * a.potential_bins) + " PLO strength buckets";
      }
    }
  }
  return plans;
}

std::optional<Abstraction> Abstraction::create(const GameConfig& config,
                                               const std::vector<PlayerHands>& hands,
                                               std::string* error) {
  std::vector<std::size_t> live;
  for (const PlayerHands& ph : hands) live.push_back(ph.hands.size());
  Abstraction result;
  result.plan_ = plan_abstraction(config, live);
#if GTO_HAVE_OMAHA
  std::shared_ptr<OmahaBoards> omaha_boards;
#endif
  for (int s = static_cast<int>(config.start_street); s <= static_cast<int>(config.last_street); ++s) {
    const Street street = static_cast<Street>(s);
    const StreetPlan& plan = result.plan_[static_cast<std::size_t>(s)];
    std::shared_ptr<const BucketFunction> fn;
    if (config.toy_deck_size > 0) {
      fn = std::make_shared<ToyBuckets>(config.toy_deck_size);
    } else if (street == Street::Preflop) {
      if (config.variant == Variant::Holdem) {
        fn = std::make_shared<Holdem169>();
      } else {
#if GTO_HAVE_OMAHA
        fn = std::make_shared<OmahaPreflopBuckets>(config.abstraction.omaha_preflop_buckets);
#endif
      }
    } else if (plan.kind == AbstractionKind::Lossless) {
      fn = std::make_shared<LosslessBuckets>(config, street, live);
    } else if (config.variant == Variant::Holdem) {
      fn = std::make_shared<HoldemStrengthBuckets>(config, street);
    } else {
#if GTO_HAVE_OMAHA
      if (!omaha_boards) {
        omaha_boards = std::make_shared<OmahaBoards>(config.board, config.abstraction.omaha_strength_samples);
      }
      fn = std::make_shared<OmahaStrengthBuckets>(config, street, omaha_boards);
#endif
    }
    if (!fn) {
      if (error) *error = "PLO support is not built into this engine yet";
      return std::nullopt;
    }
    result.streets_[static_cast<std::size_t>(s)] = std::move(fn);
  }
  return result;
}

std::size_t Abstraction::memory_bytes() const {
  std::size_t total = 0;
  for (const auto& s : streets_) {
    if (s) total += s->memory_bytes();
  }
  return total;
}

}  // namespace gto::mc
