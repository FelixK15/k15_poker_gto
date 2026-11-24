#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "gto/hands.h"

namespace gto {

/// Values at the end of a hand for every hand of one player, given how likely
/// each opponent hand is to be there ("opponent reach").
///
/// All results are counterfactual values: sums over the opponent hands that do
/// not share a card with the player's hand, weighted by their reach. Each call
/// runs in O(hands) rather than O(hands²): opponent reach is accumulated once
/// in total and per card, and the hands blocked by the player's own cards are
/// subtracted.
///
/// Supports one or two hole cards per hand. Four (PLO) needs a longer
/// inclusion-exclusion and is not implemented yet.
class TerminalEvaluator {
 public:
  explicit TerminalEvaluator(const HandSets& sets);

  /// out[h] = total reach of the opponent hands compatible with hand h.
  void compatible_reach(int player, const float* opponent_reach, float* out) const;

  /// Every compatible opponent hand pays `payoff`.
  void fold(int player, double payoff, const float* opponent_reach, float* out) const;

  /// Weaker opponent hands pay `win`, equal ones `tie`, stronger ones `lose`.
  void showdown(int player, double win, double tie, double lose, const float* opponent_reach,
                float* out) const;

 private:
  static constexpr int kMaxCardIds = 64;

  const HandSets* sets_;
  int hole_cards_ = 0;
  /// Hand indices ordered by ascending strength.
  std::array<std::vector<std::uint32_t>, 2> sorted_;
  /// Index of the identical hand in the opponent's set, or -1.
  std::array<std::vector<std::int32_t>, 2> same_hand_;
};

}  // namespace gto
