#include "gto/terminal.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <unordered_map>

#include "gto/card.h"
#include "gto/omaha.h"

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

std::string hand_name(const Hand& hand) {
  if (hand.num_cards == 2) return holdem_hand_name(hand);
  if (hand.num_cards == 4) {
    return omaha_hand_name({Card(hand.cards[0]), Card(hand.cards[1]), Card(hand.cards[2]),
                            Card(hand.cards[3])});
  }
  std::string name;
  for (int c = 0; c < hand.num_cards; ++c) name += Card(hand.cards[c]).to_string();
  return name;
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
  assert(hole_cards_ == 1 || hole_cards_ == 2 || hole_cards_ == 4);

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
      const bool has_same = hole_cards_ >= 2 && same.back() >= 0;
      same_index_[player].push_back(has_same ? static_cast<std::uint32_t>(same.back()) : 0u);
      same_weight_[player].push_back(has_same ? 1.0f : 0.0f);
    }
  }

  if (hole_cards_ == 4) build_four_card_slots();
}

namespace {

// Ids of the card subsets of a four-card hand: its cards (0-51), pairs
// (52 + pair index) and triples (52 + 1326 + triple index).
constexpr std::size_t kPairBase = 52;
constexpr std::size_t kTripleBase = 52 + 1326;
constexpr std::size_t kSubsetIds = 52 + 1326 + 22100;

std::array<std::uint32_t, 14> subset_ids(const Hand& hand) {
  std::array<std::uint32_t, 4> c = {hand.cards[0], hand.cards[1], hand.cards[2], hand.cards[3]};
  std::sort(c.begin(), c.end());
  const auto pair = [](std::uint32_t a, std::uint32_t b) {  // a < b
    return static_cast<std::uint32_t>(kPairBase) + b * (b - 1) / 2 + a;
  };
  const auto triple = [](std::uint32_t a, std::uint32_t b, std::uint32_t d) {  // a < b < d
    return static_cast<std::uint32_t>(kTripleBase) + a + b * (b - 1) / 2 +
           d * (d - 1) * (d - 2) / 6;
  };
  return {c[0],
          c[1],
          c[2],
          c[3],
          pair(c[0], c[1]),
          pair(c[0], c[2]),
          pair(c[0], c[3]),
          pair(c[1], c[2]),
          pair(c[1], c[3]),
          pair(c[2], c[3]),
          triple(c[0], c[1], c[2]),
          triple(c[0], c[1], c[3]),
          triple(c[0], c[2], c[3]),
          triple(c[1], c[2], c[3])};
}

}  // namespace

void TerminalEvaluator::build_four_card_slots() {
  std::array<std::vector<std::uint32_t>, 2> slot_of;
  for (std::size_t player = 0; player < 2; ++player) {
    slot_of[player].assign(kSubsetIds, 0);
    std::uint32_t next = 1;
    auto& add = add_slots_[player];
    add.reserve((*hands_)[player].size() * kFourCardSubsets);
    for (const Hand& hand : (*hands_)[player]) {
      assert(hand.num_cards == 4);
      for (const std::uint32_t id : subset_ids(hand)) {
        assert(id < kSubsetIds);
        std::uint32_t& slot = slot_of[player][id];
        if (slot == 0) slot = next++;
        add.push_back(slot);
      }
    }
    slot_count_[player] = next;
  }
  for (std::size_t player = 0; player < 2; ++player) {
    auto& read = read_slots_[player];
    read.reserve((*hands_)[player].size() * kFourCardSubsets);
    for (const Hand& hand : (*hands_)[player]) {
      for (const std::uint32_t id : subset_ids(hand)) read.push_back(slot_of[1 - player][id]);
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
    const std::size_t opponents = opponent_order.size();

    // Where each sum starts: the total first, then one per card that some
    // opponent hand holds. A card no opponent hand holds reads slot 0, the
    // total's leading zero.
    std::array<std::size_t, 64> held{};
    for (std::size_t o = 0; o < opponents; ++o) {
      ++held[card1_[1 - p][o]];
      ++held[card2_[1 - p][o]];
    }
    std::array<std::size_t, 64> start{};
    std::size_t size = opponents + 1;
    std::vector<std::uint16_t> lengths = {static_cast<std::uint16_t>(opponents)};
    for (std::size_t c = 0; c < 64; ++c) {
      if (held[c] == 0) continue;
      start[c] = size;
      size += held[c] + 1;
      lengths.push_back(static_cast<std::uint16_t>(held[c]));
    }
    // Slots are stored in 16 bits, and showdown() keeps the sums on the stack.
    if (size > kMaxShowdownSlots) continue;

    ShowdownPlan& plan = table.plans[p];
    plan.size = size;
    plan.lengths = std::move(lengths);
    plan.source.assign(size - plan.lengths.size(), 0);
    plan.slots.assign(order.size(), {});

    // `added[c]` counts the opponent hands with card c summed so far, so the
    // running sum for card c up to here sits at start[c] + added[c].
    std::array<std::size_t, 64> added{};
    std::array<std::size_t, 64> source_start{};
    {
      std::size_t at = opponents;
      for (std::size_t c = 0; c < 64; ++c) {
        if (held[c] == 0) continue;
        source_start[c] = at;
        at += held[c];
      }
    }
    std::size_t j = 0;
    const auto add = [&](std::uint32_t o) {
      const auto source = static_cast<std::uint16_t>(o);
      plan.source[j] = source;
      for (const std::uint8_t c : {card1_[1 - p][o], card2_[1 - p][o]}) {
        plan.source[source_start[c] + added[c]] = source;
        ++added[c];
      }
      ++j;
    };
    const auto here = [&](std::uint32_t h) {
      const std::uint8_t c1 = card1_[p][h];
      const std::uint8_t c2 = card2_[p][h];
      return std::array<std::uint16_t, 3>{static_cast<std::uint16_t>(j),
                                          static_cast<std::uint16_t>(start[c1] + added[c1]),
                                          static_cast<std::uint16_t>(start[c2] + added[c2])};
    };

    std::size_t i = 0;
    while (i < order.size()) {
      const HandStrength level = strength[order[i]];
      std::size_t end = i + 1;
      while (end < order.size() && strength[order[end]] == level) ++end;

      while (j < opponents && opponent_strength[opponent_order[j]] < level) add(opponent_order[j]);
      for (std::size_t k = i; k < end; ++k) plan.slots[order[k]].beat = here(order[k]);
      while (j < opponents && opponent_strength[opponent_order[j]] == level) add(opponent_order[j]);
      for (std::size_t k = i; k < end; ++k) plan.slots[order[k]].beat_or_tie = here(order[k]);
      i = end;
    }
    while (j < opponents) add(opponent_order[j]);
    for (std::size_t h = 0; h < order.size(); ++h) {
      const auto last = here(static_cast<std::uint32_t>(h));
      plan.slots[h].all = {last[1], last[2]};
    }
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
  if (hole_cards_ == 4) {
    scaled_compatible_reach_four(player, scale, opponent_reach, out);
    return;
  }
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
  // Running sums of the opponent's reach in order of strength give W, WT and
  // C for every hand: where to read them was worked out by prepare().
  const double beat = win - tie;
  const double beat_or_tie = tie - lose;
  if (hole_cards_ == 4) {
    showdown_four(table, player, beat, beat_or_tie, lose, opponent_reach, out);
    return;
  }

  const ShowdownPlan& plan = table.plans[p];
  if (plan.slots.empty()) {
    showdown_general(table, player, beat, beat_or_tie, lose, opponent_reach, out);
    return;
  }

  // The running sums: for each sum a zero, then the reach of every hand in
  // it added up in order of strength. Each sum is one chain of additions,
  // but different sums do not wait for one another.
  double sums[kMaxShowdownSlots];
  {
    const std::uint16_t* source = plan.source.data();
    double* slot = sums;
    for (const std::uint16_t length : plan.lengths) {
      double running = 0;
      *slot++ = 0;
      for (std::uint16_t k = 0; k < length; ++k) {
        running += opponent_reach[*source++];
        *slot++ = running;
      }
    }
  }
  const double total = sums[plan.lengths[0]];

  // Every hand now reads its totals off the sums, independently of the
  // others. The hands that share a card with ours were subtracted per card;
  // the opponent's identical hand, which ties with ours, shares both, so it
  // was subtracted twice and goes back in once.
  const ShowdownSlots* slots = plan.slots.data();
  const std::uint32_t* same_index = same_index_[p].data();
  const float* same_weight = same_weight_[p].data();
  const std::size_t count = plan.slots.size();
  for (std::size_t h = 0; h < count; ++h) {
    const ShowdownSlots& s = slots[h];
    const double same = static_cast<double>(opponent_reach[same_index[h]]) * same_weight[h];
    const double weaker = sums[s.beat[0]] - sums[s.beat[1]] - sums[s.beat[2]];
    const double weaker_or_equal =
        sums[s.beat_or_tie[0]] - sums[s.beat_or_tie[1]] - sums[s.beat_or_tie[2]] + same;
    const double all = total - sums[s.all[0]] - sums[s.all[1]] + same;
    out[h] = static_cast<float>(beat * weaker + beat_or_tie * weaker_or_equal + lose * all);
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

namespace {

/// Reach of the opponent hands added so far that share a card with a hand
/// whose subsets read `slot`: inclusion-exclusion over cards, pairs and
/// triples. The identical hand, if added, is left out: the caller adds it.
inline double blocked_four(const double* sums, const std::uint32_t* slot) {
  const double cards = (sums[slot[0]] + sums[slot[1]]) + (sums[slot[2]] + sums[slot[3]]);
  const double pairs = (sums[slot[4]] + sums[slot[5]]) + (sums[slot[6]] + sums[slot[7]]) +
                       (sums[slot[8]] + sums[slot[9]]);
  const double triples = (sums[slot[10]] + sums[slot[11]]) + (sums[slot[12]] + sums[slot[13]]);
  return cards - pairs + triples;
}

/// Zeroed scratch for the per-subset sums, one buffer per thread.
std::vector<double>& four_card_scratch(std::size_t size) {
  thread_local std::vector<double> sums;
  sums.assign(size, 0.0);
  return sums;
}

}  // namespace

void TerminalEvaluator::scaled_compatible_reach_four(int player, double scale,
                                                     const float* opponent_reach,
                                                     float* out) const {
  const auto p = static_cast<std::size_t>(player);
  std::vector<double>& buffer = four_card_scratch(slot_count_[1 - p]);
  double* sums = buffer.data();

  const std::uint32_t* add = add_slots_[1 - p].data();
  const std::size_t opponents = (*hands_)[1 - p].size();
  double total = 0;
  for (std::size_t o = 0; o < opponents; ++o) {
    const double r = opponent_reach[o];
    if (r == 0) continue;
    total += r;
    const std::uint32_t* slot = add + o * kFourCardSubsets;
    for (std::size_t k = 0; k < kFourCardSubsets; ++k) sums[slot[k]] += r;
  }

  const std::uint32_t* read = read_slots_[p].data();
  const std::uint32_t* same = same_index_[p].data();
  const float* same_weight = same_weight_[p].data();
  const std::size_t count = (*hands_)[p].size();
  for (std::size_t h = 0; h < count; ++h) {
    const double value = total - blocked_four(sums, read + h * kFourCardSubsets) +
                         static_cast<double>(opponent_reach[same[h]]) * same_weight[h];
    out[h] = static_cast<float>(scale * value);
  }
}

void TerminalEvaluator::showdown_four(const ShowdownTable& table, int player, double beat,
                                      double beat_or_tie, double lose,
                                      const float* opponent_reach, float* out) const {
  const auto p = static_cast<std::size_t>(player);
  const HandStrength* strength = table.strength[p].data();
  const HandStrength* opponent_strength = table.strength[1 - p].data();
  const std::uint32_t* order = table.order[p].data();
  const std::uint32_t* opponent_order = table.order[1 - p].data();
  const std::size_t count = table.order[p].size();
  const std::size_t opponent_count = table.order[1 - p].size();
  const std::uint32_t* add_slots = add_slots_[1 - p].data();
  const std::uint32_t* read = read_slots_[p].data();
  const std::uint32_t* same = same_index_[p].data();
  const float* same_weight = same_weight_[p].data();

  std::vector<double>& buffer = four_card_scratch(slot_count_[1 - p]);
  double* sums = buffer.data();
  double sum = 0;
  const auto add = [&](std::uint32_t o) {
    const double r = opponent_reach[o];
    if (r == 0) return;
    sum += r;
    const std::uint32_t* slot = add_slots + static_cast<std::size_t>(o) * kFourCardSubsets;
    for (std::size_t k = 0; k < kFourCardSubsets; ++k) sums[slot[k]] += r;
  };
  const auto compatible = [&](std::uint32_t h) {
    return sum - blocked_four(sums, read + static_cast<std::size_t>(h) * kFourCardSubsets);
  };
  const auto same_reach = [&](std::uint32_t h) {
    return static_cast<double>(opponent_reach[same[h]]) * same_weight[h];
  };

  // As in showdown_general(): the identical hand ties with ours, so it is
  // among the added hands only from the ties on.
  std::size_t j = 0;
  std::size_t i = 0;
  while (i < count) {
    const HandStrength level = strength[order[i]];
    std::size_t end = i + 1;
    while (end < count && strength[order[end]] == level) ++end;

    while (j < opponent_count && opponent_strength[opponent_order[j]] < level) {
      add(opponent_order[j++]);
    }
    for (std::size_t k = i; k < end; ++k) {
      const std::uint32_t h = order[k];
      out[h] = static_cast<float>(beat * compatible(h));
    }
    while (j < opponent_count && opponent_strength[opponent_order[j]] == level) {
      add(opponent_order[j++]);
    }
    for (std::size_t k = i; k < end; ++k) {
      const std::uint32_t h = order[k];
      out[h] += static_cast<float>(beat_or_tie * (compatible(h) + same_reach(h)));
    }
    i = end;
  }

  while (j < opponent_count) add(opponent_order[j++]);
  if (lose != 0) {
    for (std::size_t h = 0; h < count; ++h) {
      const auto hand = static_cast<std::uint32_t>(h);
      out[h] += static_cast<float>(lose * (compatible(hand) + same_reach(hand)));
    }
  }
}

}  // namespace gto
