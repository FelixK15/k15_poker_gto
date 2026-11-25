#pragma once

#include <array>
#include <cstddef>
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

  /// Works out a board's showdown steps once, which makes every later
  /// showdown() on it several times faster. Optional; two-card games only.
  void prepare(ShowdownTable& table) const;

  /// out[h] = total reach of the opponent hands compatible with hand h.
  void compatible_reach(int player, const float* opponent_reach, float* out) const;

  /// Every compatible opponent hand pays `payoff`.
  void fold(int player, double payoff, const float* opponent_reach, float* out) const;

  /// Weaker opponent hands pay `win`, equal ones `tie`, stronger ones `lose`.
  void showdown(const ShowdownTable& table, int player, double win, double tie, double lose,
                const float* opponent_reach, float* out) const;

 private:
  static constexpr std::uint8_t kSpareCard = 63;
  /// Most entries a prepared showdown's running sums may have: the total
  /// and two per opponent hand for a full range, plus a zero for each.
  static constexpr std::size_t kMaxShowdownSlots = 4096;

  void scaled_compatible_reach(int player, double scale, const float* opponent_reach,
                               float* out) const;
  void showdown_general(const ShowdownTable& table, int player, double beat, double beat_or_tie,
                        double lose, const float* opponent_reach, float* out) const;

  const Hands* hands_;
  int hole_cards_ = 0;
  /// Index of the identical hand in the opponent's set, or -1.
  std::array<std::vector<std::int32_t>, 2> same_hand_;
  // The same facts laid out flat for the inner loops: each hand's two cards,
  // and its identical opponent hand as an index (0 if none) with a weight of
  // 1 or 0 that switches it on or off without a branch.
  std::array<std::vector<std::uint8_t>, 2> card1_;
  std::array<std::vector<std::uint8_t>, 2> card2_;
  std::array<std::vector<std::uint32_t>, 2> same_index_;
  std::array<std::vector<float>, 2> same_weight_;
};

}  // namespace gto
