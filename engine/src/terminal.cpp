#include "gto/terminal.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <unordered_map>

#include "gto/card.h"

namespace gto {

Hand make_hand(std::initializer_list<int> card_ids, float weight) {
  assert(card_ids.size() <= Hand::kMaxCards);
  Hand hand;
  for (const int id : card_ids) {
    assert(id >= 0 && id < 64);
    hand.cards[hand.num_cards++] = static_cast<std::uint8_t>(id);
    hand.mask |= std::uint64_t{1} << id;
  }
  hand.weight = weight;
  return hand;
}

std::string holdem_hand_name(const Hand& hand) {
  Card a(hand.cards[0]);
  Card b(hand.cards[1]);
  if (b > a) std::swap(a, b);
  return a.to_string() + b.to_string();
}

ShowdownTable make_showdown_table(std::array<std::vector<HandStrength>, 2> strength) {
  ShowdownTable table;
  table.strength = std::move(strength);
  for (std::size_t player = 0; player < 2; ++player) {
    const auto& values = table.strength[player];
    auto& order = table.order[player];
    order.resize(values.size());
    std::iota(order.begin(), order.end(), 0u);
    std::sort(order.begin(), order.end(),
              [&](std::uint32_t a, std::uint32_t b) { return values[a] < values[b]; });
  }
  return table;
}

TerminalEvaluator::TerminalEvaluator(const Hands& hands) : hands_(&hands) {
  for (const auto& player_hands : hands) {
    if (!player_hands.empty()) hole_cards_ = player_hands.front().num_cards;
  }
  assert(hole_cards_ == 1 || hole_cards_ == 2);

  for (std::size_t player = 0; player < 2; ++player) {
    std::unordered_map<std::uint64_t, std::int32_t> opponent_index;
    const auto& opponent_hands = hands[1 - player];
    for (std::size_t i = 0; i < opponent_hands.size(); ++i) {
      opponent_index.emplace(opponent_hands[i].mask, static_cast<std::int32_t>(i));
    }
    auto& same = same_hand_[player];
    same.reserve(hands[player].size());
    for (const Hand& hand : hands[player]) {
      const auto it = opponent_index.find(hand.mask);
      same.push_back(it == opponent_index.end() ? -1 : it->second);

      // Flat copies for the inner loops. A one-card hand uses the spare slot
      // 63 as its second card; nothing ever adds to it.
      card1_[player].push_back(hand.cards[0]);
      card2_[player].push_back(hole_cards_ == 2 ? hand.cards[1] : kSpareCard);
      const bool has_same = hole_cards_ == 2 && same.back() >= 0;
      same_index_[player].push_back(has_same ? static_cast<std::uint32_t>(same.back()) : 0u);
      same_weight_[player].push_back(has_same ? 1.0f : 0.0f);
    }
  }
}

void TerminalEvaluator::prepare(ShowdownTable& table) const {
  // One-card games are toys; they keep the general path.
  if (hole_cards_ != 2) return;

  for (std::size_t p = 0; p < 2; ++p) {
    const auto& strength = table.strength[p];
    const auto& opponent_strength = table.strength[1 - p];
    const auto& order = table.order[p];
    const auto& opponent_order = table.order[1 - p];
    // Indices are stored in 16 bits.
    if (order.size() > 0xFFFF || opponent_order.size() > 0xFFFF) return;

    auto& events = table.events[p];
    events.clear();
    events.reserve(2 * order.size() + opponent_order.size());
    const auto add = [&](std::uint32_t opponent_hand) {
      events.push_back({static_cast<std::uint16_t>(opponent_hand), 0, card1_[1 - p][opponent_hand],
                        card2_[1 - p][opponent_hand], ShowdownEvent::kAdd});
    };
    const auto emit = [&](std::uint32_t hand, std::uint8_t kind) {
      const bool same = kind == ShowdownEvent::kBeatOrTie && same_weight_[p][hand] != 0;
      events.push_back({static_cast<std::uint16_t>(same ? same_index_[p][hand] : 0u),
                        static_cast<std::uint16_t>(hand), card1_[p][hand], card2_[p][hand],
                        same ? static_cast<std::uint8_t>(ShowdownEvent::kBeatOrTieSame) : kind});
    };

    // The same walk the general path does, written down once instead of
    // being rediscovered, with its unpredictable branches, at every showdown.
    std::size_t j = 0, i = 0;
    while (i < order.size()) {
      const HandStrength level = strength[order[i]];
      std::size_t end = i + 1;
      while (end < order.size() && strength[order[end]] == level) ++end;

      while (j < opponent_order.size() && opponent_strength[opponent_order[j]] < level) {
        add(opponent_order[j++]);
      }
      for (std::size_t k = i; k < end; ++k) emit(order[k], ShowdownEvent::kBeat);
      while (j < opponent_order.size() && opponent_strength[opponent_order[j]] == level) {
        add(opponent_order[j++]);
      }
      for (std::size_t k = i; k < end; ++k) emit(order[k], ShowdownEvent::kBeatOrTie);
      i = end;
    }
    while (j < opponent_order.size()) add(opponent_order[j++]);
  }
}

namespace {

/// Total reach and reach per card of the opponent's hands.
///
/// Consecutive hands share their higher card, so adding straight into one
/// table would make every addition wait for the one before. Four tables used
/// in turn keep the processor busy on four independent chains.
struct CardSums {
  double total = 0;
  double by_card[64] = {};
};

inline void card_sums(const float* reach, const std::uint8_t* card1, const std::uint8_t* card2,
                      std::size_t count, CardSums& out) {
  double lane[4][64] = {};
  double total[4] = {0, 0, 0, 0};
  std::size_t j = 0;
  for (; j + 4 <= count; j += 4) {
    for (std::size_t k = 0; k < 4; ++k) {
      const double r = reach[j + k];
      total[k] += r;
      lane[k][card1[j + k]] += r;
      lane[k][card2[j + k]] += r;
    }
  }
  for (; j < count; ++j) {
    const double r = reach[j];
    total[0] += r;
    lane[0][card1[j]] += r;
    lane[0][card2[j]] += r;
  }
  out.total = (total[0] + total[1]) + (total[2] + total[3]);
  for (std::size_t c = 0; c < 64; ++c) {
    out.by_card[c] = (lane[0][c] + lane[1][c]) + (lane[2][c] + lane[3][c]);
  }
}

}  // namespace

void TerminalEvaluator::compatible_reach(int player, const float* opponent_reach,
                                         float* out) const {
  scaled_compatible_reach(player, 1.0, opponent_reach, out);
}

void TerminalEvaluator::fold(int player, double payoff, const float* opponent_reach,
                             float* out) const {
  scaled_compatible_reach(player, payoff, opponent_reach, out);
}

void TerminalEvaluator::scaled_compatible_reach(int player, double scale,
                                                const float* opponent_reach, float* out) const {
  const auto p = static_cast<std::size_t>(player);
  CardSums sums;
  card_sums(opponent_reach, card1_[1 - p].data(), card2_[1 - p].data(), card1_[1 - p].size(),
            sums);
  // A one-card hand must not be charged for the spare slot.
  sums.by_card[kSpareCard] = 0;

  const std::uint8_t* card1 = card1_[p].data();
  const std::uint8_t* card2 = card2_[p].data();
  const std::uint32_t* same = same_index_[p].data();
  const float* same_weight = same_weight_[p].data();
  const std::size_t count = card1_[p].size();
  for (std::size_t i = 0; i < count; ++i) {
    // With two hole cards the identical opponent hand was subtracted twice.
    const double value = sums.total - sums.by_card[card1[i]] - sums.by_card[card2[i]] +
                         static_cast<double>(opponent_reach[same[i]]) * same_weight[i];
    out[i] = static_cast<float>(scale * value);
  }
}

void TerminalEvaluator::showdown(const ShowdownTable& table, int player, double win, double tie,
                                 double lose, const float* opponent_reach, float* out) const {
  const auto p = static_cast<std::size_t>(player);
  // With W the reach of the compatible opponent hands we beat, WT of those we
  // beat or tie, and C of all of them:
  //   value = win·W + tie·(WT − W) + lose·(C − WT)
  //         = (win − tie)·W + (tie − lose)·WT + lose·C
  // One walk up both players' hands in order of strength gives W and WT; C is
  // what has been added up once the walk is over.
  const double beat = win - tie;
  const double beat_or_tie = tie - lose;

  if (table.events[p].empty()) {
    showdown_general(table, player, beat, beat_or_tie, lose, opponent_reach, out);
    return;
  }

  const std::size_t count = card1_[p].size();
  // Results are collected in a local buffer with one spare slot at the end.
  constexpr std::size_t kMaxHands = 1326;
  if (count > kMaxHands) {
    showdown_general(table, player, beat, beat_or_tie, lose, opponent_reach, out);
    return;
  }

  // Every event does the same arithmetic. Per kind of event, these tables
  // say whether it adds an opponent hand, what it multiplies into a result
  // and whether it keeps what the result already holds; the parts an event
  // does not need become additions of zero.
  //
  // They also say where it writes. An addition updates the real sums and
  // puts its (meaningless) result in the spare slot; a result is stored for
  // real and its (unchanged) sums go to the spare halves of the sum tables.
  // Writing somewhere harmless instead of not writing keeps the loop free of
  // branches, and keeping those writes away from what later events read
  // keeps the events from waiting for one another.
  const double adds[4] = {1, 0, 0, 0};
  const double same[4] = {0, 0, 0, 1};
  const double factor[4] = {0, beat, beat_or_tie, beat_or_tie};
  const float keeps[4] = {0, 0, 1, 1};
  const std::size_t spare_sum[4] = {0, 1, 1, 1};
  const std::size_t spare_cards[4] = {0, 64, 64, 64};
  const std::size_t spare_result[4] = {count, 0, 0, 0};

  float result[kMaxHands + 1];
  std::fill(result, result + count + 1, 0.0f);
  double sums[2] = {0, 0};
  double card_sum[128] = {};

  for (const ShowdownEvent& event : table.events[p]) {
    const unsigned kind = event.kind;
    const double reach = opponent_reach[event.source];
    const double added = reach * adds[kind];

    const double sum = sums[0] + added;
    sums[spare_sum[kind]] = sum;
    const double first = card_sum[event.card1] + added;
    card_sum[event.card1 + spare_cards[kind]] = first;
    const double second = card_sum[event.card2] + added;
    card_sum[event.card2 + spare_cards[kind]] = second;

    // The identical hand ties with ours; it was subtracted once per card.
    const double value = sum - first - second + reach * same[kind];
    result[event.target + spare_result[kind]] =
        result[event.target] * keeps[kind] + static_cast<float>(factor[kind] * value);
  }

  if (lose == 0) {
    std::copy(result, result + count, out);
    return;
  }
  const std::uint8_t* card1 = card1_[p].data();
  const std::uint8_t* card2 = card2_[p].data();
  const std::uint32_t* same_index = same_index_[p].data();
  const float* same_weight = same_weight_[p].data();
  for (std::size_t h = 0; h < count; ++h) {
    const double all = sums[0] - card_sum[card1[h]] - card_sum[card2[h]] +
                       static_cast<double>(opponent_reach[same_index[h]]) * same_weight[h];
    out[h] = result[h] + static_cast<float>(lose * all);
  }
}

void TerminalEvaluator::showdown_general(const ShowdownTable& table, int player, double beat,
                                         double beat_or_tie, double lose,
                                         const float* opponent_reach, float* out) const {
  const auto p = static_cast<std::size_t>(player);
  const Hand* hands = (*hands_)[p].data();
  const Hand* opponent_hands = (*hands_)[1 - p].data();
  const HandStrength* strength = table.strength[p].data();
  const HandStrength* opponent_strength = table.strength[1 - p].data();
  const std::uint32_t* order = table.order[p].data();
  const std::uint32_t* opponent_order = table.order[1 - p].data();
  const std::size_t count = table.order[p].size();
  const std::size_t opponent_count = table.order[1 - p].size();
  const std::int32_t* same = same_hand_[p].data();
  const bool two_cards = hole_cards_ == 2;

  double sum = 0;
  double card_sum[64] = {};
  const auto add = [&](std::uint32_t opponent_hand) {
    const double reach = opponent_reach[opponent_hand];
    if (reach == 0) return;
    sum += reach;
    const Hand& hand = opponent_hands[opponent_hand];
    for (int c = 0; c < hand.num_cards; ++c) card_sum[hand.cards[c]] += reach;
  };
  // Reach so far of the opponent hands that share a card with `hand`.
  const auto blocked = [&](const Hand& hand) {
    double value = 0;
    for (int c = 0; c < hand.num_cards; ++c) value += card_sum[hand.cards[c]];
    return value;
  };

  std::size_t j = 0;
  std::size_t i = 0;
  while (i < count) {
    const HandStrength level = strength[order[i]];
    std::size_t end = i + 1;
    while (end < count && strength[order[end]] == level) ++end;

    // Everything weaker than this group of equal hands. The identical hand
    // is as strong as ours, so it is not among them and each blocked hand
    // shares exactly one card: the per-card sums count it once.
    while (j < opponent_count && opponent_strength[opponent_order[j]] < level) {
      add(opponent_order[j++]);
    }
    for (std::size_t k = i; k < end; ++k) {
      const std::uint32_t h = order[k];
      out[h] = static_cast<float>(beat * (sum - blocked(hands[h])));
    }

    // Now also what ties with the group. The identical hand is in there and
    // was subtracted once per card, so with two cards it goes back in once.
    while (j < opponent_count && opponent_strength[opponent_order[j]] == level) {
      add(opponent_order[j++]);
    }
    for (std::size_t k = i; k < end; ++k) {
      const std::uint32_t h = order[k];
      double reach = sum - blocked(hands[h]);
      if (two_cards && same[h] >= 0) reach += opponent_reach[same[h]];
      out[h] += static_cast<float>(beat_or_tie * reach);
    }
    i = end;
  }

  // Whatever beats all of our hands completes the totals.
  while (j < opponent_count) add(opponent_order[j++]);
  if (lose != 0) {
    for (std::size_t h = 0; h < count; ++h) {
      double reach = sum - blocked(hands[h]);
      if (two_cards && same[h] >= 0) reach += opponent_reach[same[h]];
      out[h] += static_cast<float>(lose * reach);
    }
  }
}

}  // namespace gto
