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
///
/// Thread-safe: all methods are const and keep their scratch data on the stack.
class TerminalEvaluator {
 public:
  /// Keeps a reference to `hands`, which must outlive the evaluator.
  explicit TerminalEvaluator(const Hands& hands);

  int hole_cards() const noexcept { return hole_cards_; }

  /// out[h] = total reach of the opponent hands compatible with hand h.
  void compatible_reach(int player, const float* opponent_reach, float* out) const;

  /// Every compatible opponent hand pays `payoff`.
  void fold(int player, double payoff, const float* opponent_reach, float* out) const;

  /// Weaker opponent hands pay `win`, equal ones `tie`, stronger ones `lose`.
  void showdown(const ShowdownTable& table, int player, double win, double tie, double lose,
                const float* opponent_reach, float* out) const;

 private:
  const Hands* hands_;
  int hole_cards_ = 0;
  /// Index of the identical hand in the opponent's set, or -1.
  std::array<std::vector<std::int32_t>, 2> same_hand_;
};

}  // namespace gto
