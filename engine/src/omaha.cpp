#include "gto/omaha.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cmath>
#include <functional>
#include <numeric>

#include "gto/range.h"

namespace gto {

namespace {

/// value[n][k] = C(n, k) for n ≤ 52, k ≤ 4.
struct Binomials {
  int value[53][5] = {};
  constexpr Binomials() {
    for (int n = 0; n <= 52; ++n) {
      value[n][0] = 1;
      for (int k = 1; k <= 4; ++k) value[n][k] = n == 0 ? 0 : value[n - 1][k - 1] + value[n - 1][k];
    }
  }
};
constexpr Binomials kBinomial;

/// Index of a two-card holding as Range::combo_index() numbers it.
constexpr int pair_index(int a, int b) noexcept {
  return a > b ? a * (a - 1) / 2 + b : b * (b - 1) / 2 + a;
}

std::string_view trim(std::string_view text) {
  const auto is_space = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
  while (!text.empty() && is_space(text.front())) text.remove_prefix(1);
  while (!text.empty() && is_space(text.back())) text.remove_suffix(1);
  return text;
}

bool equals_ignore_case(std::string_view a, std::string_view b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
           return std::tolower(static_cast<unsigned char>(x)) ==
                  std::tolower(static_cast<unsigned char>(y));
         });
}

bool parse_number(std::string_view text, double& out) {
  text = trim(text);
  if (text.empty()) return false;
  const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
  return ec == std::errc() && end == text.data() + text.size();
}

enum class SuitFilter { Any, DoubleSuited, SingleSuited, Rainbow };

bool passes(SuitFilter filter, const std::array<Card, 4>& cards) {
  if (filter == SuitFilter::Any) return true;
  std::array<int, 4> count{};
  for (const Card card : cards) ++count[static_cast<std::size_t>(card.suit())];
  std::sort(count.begin(), count.end(), std::greater<>());
  switch (filter) {
    case SuitFilter::DoubleSuited: return count[0] == 2 && count[1] == 2;
    case SuitFilter::SingleSuited: return count[0] == 2 && count[1] == 1;
    case SuitFilter::Rainbow: return count[0] == 1;
    case SuitFilter::Any: break;
  }
  return true;
}

// ---------------------------------------------------------------------------
// Preflop ranking: a fixed point-count heuristic, NOT computed from equities.
//
// Points (in half points, so that everything stays integral and the ranking
// is the same on every machine):
//   pairs        a pair of rank r scores 12 + 2r (AA 36, 22 12); trips count
//                as a weaker pair (the third card is dead), quads as almost
//                nothing
//   suitedness   each suit held exactly twice scores 4 + the rank of its
//                higher card (A-suited 16); a suit held three times scores
//                less, since it kills one's own flush outs
//   high cards   A 8, K 6, Q 5, J 4, T 3 per card (nut potential)
//   connectedness  every two distinct ranks at most four apart score
//                2·(5 − gap), the ace also counting low with 2-5 at half
//                value; four ranks within a five-rank window add a rundown
//                bonus. The total is scaled by the top rank, since low
//                straights are rarely the nuts
//   danglers     a card that is neither paired, suited with another card,
//                nor within four ranks of another card costs 4
// Ties are broken by combo index.
// ---------------------------------------------------------------------------

int preflop_score(const std::array<Card, 4>& cards) {
  std::array<int, 13> rank_count{};
  std::array<int, 4> suit_count{};
  std::array<int, 4> suit_high{-1, -1, -1, -1};
  for (const Card card : cards) {
    ++rank_count[static_cast<std::size_t>(card.rank())];
    const auto s = static_cast<std::size_t>(card.suit());
    ++suit_count[s];
    suit_high[s] = std::max(suit_high[s], card.rank());
  }

  int score = 0;
  for (int r = 0; r < 13; ++r) {
    switch (rank_count[static_cast<std::size_t>(r)]) {
      case 2: score += 12 + 2 * r; break;
      case 3: score += 4 + r; break;
      case 4: score += r / 2; break;
      default: break;
    }
  }

  for (std::size_t s = 0; s < 4; ++s) {
    if (suit_count[s] == 2) score += 4 + suit_high[s];
    if (suit_count[s] == 3) score += 1 + suit_high[s] / 2;
  }

  static constexpr int kHighCard[13] = {0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 8};
  for (const Card card : cards) score += kHighCard[card.rank()];

  // Distinct ranks, ascending.
  int ranks[4] = {0, 0, 0, 0};
  int distinct = 0;
  for (int r = 0; r < 13; ++r) {
    if (rank_count[static_cast<std::size_t>(r)] > 0) ranks[distinct++] = r;
  }
  const int top = ranks[distinct - 1];
  int connected = 0;
  for (int i = 0; i < distinct; ++i) {
    for (int j = i + 1; j < distinct; ++j) {
      const int gap = ranks[j] - ranks[i];
      if (gap <= 4) connected += 2 * (5 - gap);
    }
    if (top == 12 && ranks[i] <= 3) connected += 5 - (ranks[i] + 1);
  }
  if (distinct == 4 && ranks[3] - ranks[0] <= 4) connected += 8;
  score += connected * (6 + top) / 18;

  for (const Card card : cards) {
    const int r = card.rank();
    if (rank_count[static_cast<std::size_t>(r)] > 1) continue;
    if (suit_count[static_cast<std::size_t>(card.suit())] > 1) continue;
    bool near = false;
    for (int i = 0; i < distinct; ++i) {
      if (ranks[i] != r && std::abs(ranks[i] - r) <= 4) near = true;
      if (r == 12 && ranks[i] <= 3) near = true;
      if (r <= 3 && ranks[i] == 12) near = true;
    }
    if (!near) score -= 4;
  }
  return score;
}

}  // namespace

int omaha_combo_index(const std::array<Card, 4>& cards) noexcept {
  int c[4] = {cards[0].index(), cards[1].index(), cards[2].index(), cards[3].index()};
  // Sorting network for four values.
  const auto order = [&](int i, int j) {
    if (c[i] > c[j]) std::swap(c[i], c[j]);
  };
  order(0, 1);
  order(2, 3);
  order(0, 2);
  order(1, 3);
  order(1, 2);
  assert(c[0] < c[1] && c[1] < c[2] && c[2] < c[3]);
  // Combinatorial number system (colex order).
  return kBinomial.value[c[0]][1] + kBinomial.value[c[1]][2] + kBinomial.value[c[2]][3] +
         kBinomial.value[c[3]][4];
}

std::array<Card, 4> omaha_combo_cards(int index) noexcept {
  assert(index >= 0 && index < kOmahaCombos);
  std::array<Card, 4> cards;
  int c = 51;
  for (int k = 4; k >= 1; --k) {
    while (kBinomial.value[c][k] > index) --c;
    index -= kBinomial.value[c][k];
    cards[static_cast<std::size_t>(k - 1)] = Card(static_cast<std::uint8_t>(c));
    --c;
  }
  return cards;
}

std::string omaha_hand_name(const std::array<Card, 4>& cards) {
  std::array<Card, 4> sorted = cards;
  std::sort(sorted.begin(), sorted.end(), std::greater<>());
  std::string name;
  for (const Card card : sorted) name += card.to_string();
  return name;
}

HandStrength evaluate_omaha(const std::array<Card, 4>& hole, std::span<const Card> board) noexcept {
  assert(board.size() >= 3 && board.size() <= 5);
  HandStrength best = 0;
  std::array<Card, 5> five;
  const std::size_t n = board.size();
  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = i + 1; j < 4; ++j) {
      five[0] = hole[i];
      five[1] = hole[j];
      for (std::size_t a = 0; a < n; ++a) {
        for (std::size_t b = a + 1; b < n; ++b) {
          for (std::size_t c = b + 1; c < n; ++c) {
            five[2] = board[a];
            five[3] = board[b];
            five[4] = board[c];
            best = std::max(best, evaluate_hand(five));
          }
        }
      }
    }
  }
  return best;
}

OmahaBoardEvaluator::OmahaBoardEvaluator(std::span<const Card> board)
    : best_with_pair_(static_cast<std::size_t>(Range::kNumCombos), 0) {
  assert(board.size() >= 3 && board.size() <= 5);
  std::uint64_t board_mask = 0;
  for (const Card card : board) board_mask |= card.mask();

  std::vector<std::array<Card, 3>> triples;
  const std::size_t n = board.size();
  for (std::size_t a = 0; a < n; ++a) {
    for (std::size_t b = a + 1; b < n; ++b) {
      for (std::size_t c = b + 1; c < n; ++c) triples.push_back({board[a], board[b], board[c]});
    }
  }

  std::array<Card, 5> five;
  for (int high = 1; high < Card::kDeckSize; ++high) {
    const Card ch(static_cast<std::uint8_t>(high));
    if ((board_mask & ch.mask()) != 0) continue;
    for (int low = 0; low < high; ++low) {
      const Card cl(static_cast<std::uint8_t>(low));
      if ((board_mask & cl.mask()) != 0) continue;
      five[0] = cl;
      five[1] = ch;
      HandStrength best = 0;
      for (const auto& triple : triples) {
        five[2] = triple[0];
        five[3] = triple[1];
        five[4] = triple[2];
        best = std::max(best, evaluate_hand(five));
      }
      best_with_pair_[static_cast<std::size_t>(pair_index(high, low))] = best;
    }
  }
}

HandStrength OmahaBoardEvaluator::evaluate(const std::array<Card, 4>& hole) const noexcept {
  const int c0 = hole[0].index(), c1 = hole[1].index(), c2 = hole[2].index(),
            c3 = hole[3].index();
  const HandStrength* best = best_with_pair_.data();
  return std::max({best[pair_index(c0, c1)], best[pair_index(c0, c2)], best[pair_index(c0, c3)],
                   best[pair_index(c1, c2)], best[pair_index(c1, c3)], best[pair_index(c2, c3)]});
}

OmahaRange OmahaRange::full() {
  OmahaRange range;
  std::fill(range.weights_.begin(), range.weights_.end(), 1.0f);
  return range;
}

double OmahaRange::total_weight() const noexcept {
  double total = 0;
  for (const float w : weights_) total += w;
  return total;
}

std::optional<OmahaRange> OmahaRange::parse(std::string_view text, std::string* error) {
  const auto fail = [&](std::string message) -> std::optional<OmahaRange> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };

  OmahaRange range;
  if (trim(text).empty()) return range;

  std::size_t start = 0;
  while (start <= text.size()) {
    std::size_t end = text.find(',', start);
    if (end == std::string_view::npos) end = text.size();
    std::string_view item = trim(text.substr(start, end - start));
    start = end + 1;
    if (item.empty()) return fail("empty item in the range");

    float weight = 1.0f;
    if (const std::size_t colon = item.find(':'); colon != std::string_view::npos) {
      double value = 0;
      if (!parse_number(item.substr(colon + 1), value) || !(value >= 0 && value <= 1)) {
        return fail("bad weight in '" + std::string(item) + "'");
      }
      weight = static_cast<float>(value);
      item = trim(item.substr(0, colon));
    }

    if (item == "*" || equals_ignore_case(item, "all")) {
      std::fill(range.weights_.begin(), range.weights_.end(), weight);
      continue;
    }

    if (item.size() > 4 && equals_ignore_case(item.substr(0, 3), "top") && item.back() == '%') {
      double percent = 0;
      if (!parse_number(item.substr(3, item.size() - 4), percent) ||
          !(percent >= 0 && percent <= 100)) {
        return fail("bad percentage in '" + std::string(item) + "'");
      }
      const auto& rank = omaha_preflop_rank();
      const auto count = static_cast<std::size_t>(std::llround(percent / 100.0 * kOmahaCombos));
      for (std::size_t i = 0; i < count; ++i) {
        range.weights_[static_cast<std::size_t>(rank[i])] = weight;
      }
      continue;
    }

    if (item.size() == 8) {
      if (const auto cards = parse_cards(item); cards && cards->size() == 4) {
        range.set_weight(omaha_combo_index({(*cards)[0], (*cards)[1], (*cards)[2], (*cards)[3]}),
                         weight);
        continue;
      }
    }

    // Four ranks or 'x', optionally followed by a suit filter.
    SuitFilter filter = SuitFilter::Any;
    std::string_view ranks = item;
    if (ranks.size() == 6 && equals_ignore_case(ranks.substr(4), "ds")) {
      filter = SuitFilter::DoubleSuited;
      ranks = ranks.substr(0, 4);
    } else if (ranks.size() == 6 && equals_ignore_case(ranks.substr(4), "ss")) {
      filter = SuitFilter::SingleSuited;
      ranks = ranks.substr(0, 4);
    } else if (ranks.size() == 5 && (ranks[4] == 'r' || ranks[4] == 'R')) {
      filter = SuitFilter::Rainbow;
      ranks = ranks.substr(0, 4);
    }
    if (ranks.size() != 4) return fail("cannot read '" + std::string(item) + "'");
    std::array<int, 13> needed{};
    for (const char c : ranks) {
      if (c == 'x' || c == 'X') continue;
      const int rank = rank_from_char(c);
      if (rank < 0) return fail("cannot read '" + std::string(item) + "'");
      ++needed[static_cast<std::size_t>(rank)];
    }
    for (int combo = 0; combo < kOmahaCombos; ++combo) {
      const auto cards = omaha_combo_cards(combo);
      std::array<int, 13> have{};
      for (const Card card : cards) ++have[static_cast<std::size_t>(card.rank())];
      bool match = true;
      for (std::size_t r = 0; r < 13; ++r) match = match && have[r] >= needed[r];
      if (match && passes(filter, cards)) range.weights_[static_cast<std::size_t>(combo)] = weight;
    }
  }
  return range;
}

const std::vector<int>& omaha_preflop_rank() {
  static const std::vector<int> rank = [] {
    std::vector<int> score(static_cast<std::size_t>(kOmahaCombos));
    for (int combo = 0; combo < kOmahaCombos; ++combo) {
      score[static_cast<std::size_t>(combo)] = preflop_score(omaha_combo_cards(combo));
    }
    std::vector<int> order(static_cast<std::size_t>(kOmahaCombos));
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
      const int sa = score[static_cast<std::size_t>(a)];
      const int sb = score[static_cast<std::size_t>(b)];
      return sa != sb ? sa > sb : a < b;
    });
    return order;
  }();
  return rank;
}

}  // namespace gto
