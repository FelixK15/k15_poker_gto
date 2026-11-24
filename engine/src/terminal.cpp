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

TerminalEvaluator::TerminalEvaluator(const HandSets& sets) : sets_(&sets) {
  for (int player = 0; player < 2; ++player) {
    const auto& hands = sets.hands[player];
    const auto& strength = sets.strength[player];
    assert(hands.size() == strength.size());
    if (!hands.empty()) hole_cards_ = hands.front().num_cards;

    auto& order = sorted_[player];
    order.resize(hands.size());
    std::iota(order.begin(), order.end(), 0u);
    std::sort(order.begin(), order.end(),
              [&](std::uint32_t a, std::uint32_t b) { return strength[a] < strength[b]; });
  }
  assert(hole_cards_ == 1 || hole_cards_ == 2);

  for (int player = 0; player < 2; ++player) {
    std::unordered_map<std::uint64_t, std::int32_t> opponent_index;
    const auto& opponent_hands = sets.hands[1 - player];
    for (std::size_t i = 0; i < opponent_hands.size(); ++i) {
      opponent_index.emplace(opponent_hands[i].mask, static_cast<std::int32_t>(i));
    }
    auto& same = same_hand_[player];
    same.reserve(sets.hands[player].size());
    for (const Hand& hand : sets.hands[player]) {
      const auto it = opponent_index.find(hand.mask);
      same.push_back(it == opponent_index.end() ? -1 : it->second);
    }
  }
}

void TerminalEvaluator::compatible_reach(int player, const float* opponent_reach,
                                         float* out) const {
  const auto& hands = sets_->hands[player];
  const auto& opponent_hands = sets_->hands[1 - player];

  double total = 0;
  double card_sum[kMaxCardIds] = {};
  for (std::size_t j = 0; j < opponent_hands.size(); ++j) {
    const double reach = opponent_reach[j];
    if (reach == 0) continue;
    total += reach;
    const Hand& hand = opponent_hands[j];
    for (int c = 0; c < hand.num_cards; ++c) card_sum[hand.cards[c]] += reach;
  }

  const auto& same = same_hand_[player];
  for (std::size_t i = 0; i < hands.size(); ++i) {
    const Hand& hand = hands[i];
    double value = total;
    for (int c = 0; c < hand.num_cards; ++c) value -= card_sum[hand.cards[c]];
    // With two hole cards the identical opponent hand was subtracted twice.
    if (hole_cards_ == 2 && same[i] >= 0) value += opponent_reach[same[i]];
    out[i] = static_cast<float>(value);
  }
}

void TerminalEvaluator::fold(int player, double payoff, const float* opponent_reach,
                             float* out) const {
  compatible_reach(player, opponent_reach, out);
  const auto scale = static_cast<float>(payoff);
  const std::size_t count = sets_->hands[player].size();
  for (std::size_t i = 0; i < count; ++i) out[i] *= scale;
}

void TerminalEvaluator::showdown(int player, double win, double tie, double lose,
                                 const float* opponent_reach, float* out) const {
  const int opponent = 1 - player;
  const auto& hands = sets_->hands[player];
  const auto& opponent_hands = sets_->hands[opponent];
  const auto& strength = sets_->strength[player];
  const auto& opponent_strength = sets_->strength[opponent];
  const auto& order = sorted_[player];
  const auto& opponent_order = sorted_[opponent];

  // value = win·W + tie·T + lose·L with T = compatible − W − L
  //       = tie·compatible + (win − tie)·W + (lose − tie)·L
  compatible_reach(player, opponent_reach, out);
  for (std::size_t i = 0; i < hands.size(); ++i) out[i] = static_cast<float>(tie * out[i]);

  // An opponent hand of different strength can share at most one card with
  // ours in a way the per-card sums count exactly once, so no correction for
  // the identical hand is needed in the two passes below.
  const auto accumulate = [&](double& sum, double* card_sum, std::uint32_t opponent_hand) {
    const double reach = opponent_reach[opponent_hand];
    if (reach == 0) return;
    sum += reach;
    const Hand& hand = opponent_hands[opponent_hand];
    for (int c = 0; c < hand.num_cards; ++c) card_sum[hand.cards[c]] += reach;
  };
  const auto blocked = [](const Hand& hand, const double* card_sum) {
    double value = 0;
    for (int c = 0; c < hand.num_cards; ++c) value += card_sum[hand.cards[c]];
    return value;
  };

  // Ascending: opponent hands we beat.
  {
    double sum = 0;
    double card_sum[kMaxCardIds] = {};
    std::size_t j = 0;
    for (const std::uint32_t i : order) {
      while (j < opponent_order.size() && opponent_strength[opponent_order[j]] < strength[i]) {
        accumulate(sum, card_sum, opponent_order[j++]);
      }
      out[i] += static_cast<float>((win - tie) * (sum - blocked(hands[i], card_sum)));
    }
  }

  // Descending: opponent hands that beat us.
  {
    double sum = 0;
    double card_sum[kMaxCardIds] = {};
    std::size_t j = opponent_order.size();
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
      const std::uint32_t i = *it;
      while (j > 0 && opponent_strength[opponent_order[j - 1]] > strength[i]) {
        accumulate(sum, card_sum, opponent_order[--j]);
      }
      out[i] += static_cast<float>((lose - tie) * (sum - blocked(hands[i], card_sum)));
    }
  }
}

}  // namespace gto
