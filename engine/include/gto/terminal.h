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
/// Supports one, two or four hole cards per hand. With four (PLO) the hands
/// blocked by ours are found by inclusion-exclusion over the subsets of our
/// cards: subtract the reach summed per card, add it back per pair of cards,
/// subtract it per triple, and add the identical hand.
///
/// Thread-safe: all methods are const and keep their scratch data on the
/// stack or, for the larger four-card sums, in thread-local buffers.
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
  void showdown_four(const ShowdownTable& table, int player, double beat, double beat_or_tie,
                     double lose, const float* opponent_reach, float* out) const;
  void build_four_card_slots();
  void scaled_compatible_reach_four(int player, double scale, const float* opponent_reach,
                                    float* out) const;

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

  // Four hole cards. Each player's hands, as opponent hands, add their reach
  // to 14 running sums: one per card, per pair and per triple of their
  // cards. Only the subsets some hand holds get a sum, numbered from 1 in
  // the player's own slot space; slot 0 stays zero.
  static constexpr std::size_t kFourCardSubsets = 14;  // 4 cards, 6 pairs, 4 triples
  /// [hand * 14 + k]: the slots a player's hand adds to.
  std::array<std::vector<std::uint32_t>, 2> add_slots_;
  /// [hand * 14 + k]: where a player's hand reads the opponent's sums for
  /// its own cards (k < 4), pairs (k < 10) and triples, in the opponent's
  /// slot space; 0 when no opponent hand holds that subset.
  std::array<std::vector<std::uint32_t>, 2> read_slots_;
  /// Number of slots in each player's slot space, slot 0 included.
  std::array<std::size_t, 2> slot_count_{0, 0};
};

}  // namespace gto
