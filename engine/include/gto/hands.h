#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include "gto/hand_evaluator.h"

namespace gto {

/// One private hand a player can hold. Generic over the number of hole cards
/// so that the same solver serves Hold'em (2), toy games (1) and later PLO (4).
struct Hand {
  static constexpr int kMaxCards = 4;

  /// Card ids, each below 64. For Hold'em these are Card indices.
  std::array<std::uint8_t, kMaxCards> cards{};
  std::uint8_t num_cards = 0;
  /// One bit per card id; two hands conflict when their masks overlap.
  std::uint64_t mask = 0;
  /// Weight of the hand in the player's starting range.
  float weight = 0;
};

/// Builds a hand from card ids.
Hand make_hand(std::initializer_list<int> card_ids, float weight);

/// Text form of a Hold'em hand, higher rank first, e.g. "AhKd".
std::string holdem_hand_name(const Hand& hand);

/// Both players' private hands, indexed by player.
using Hands = std::array<std::vector<Hand>, 2>;

/// Where one hand of ours reads its showdown totals in the running sums a
/// ShowdownPlan describes (see TerminalEvaluator::showdown).
struct ShowdownSlots {
  /// Opponent hands weaker than ours: the running total, minus the running
  /// totals of the hands holding our first and our second card.
  std::array<std::uint16_t, 3> beat{};
  /// The same up to and including the hands as strong as ours.
  std::array<std::uint16_t, 3> beat_or_tie{};
  /// End of the running totals of the hands holding each of our cards.
  std::array<std::uint16_t, 2> all{};
};

/// One player's showdown on one board, worked out in advance.
///
/// The opponent's hands, in order of strength, are summed up once in total
/// and once per card; each sum starts with a zero. `source` names the
/// opponent hand behind each entry of those sums after the leading zeros,
/// sum by sum, and `lengths` how many entries each sum has. The first sum is
/// the total; the rest belong to cards.
struct ShowdownPlan {
  std::vector<std::uint16_t> source;
  std::vector<std::uint16_t> lengths;
  /// Per hand of ours.
  std::vector<ShowdownSlots> slots;
  /// Entries in all the sums together, leading zeros included.
  std::size_t size = 0;
};

/// Who beats whom on one complete board.
struct ShowdownTable {
  /// Strength of each hand; larger wins, equal ties. Hands that the board
  /// makes impossible may hold any value: they never reach a showdown.
  std::array<std::vector<HandStrength>, 2> strength;
  /// Hand indices ordered by ascending strength.
  std::array<std::vector<std::uint32_t>, 2> order;
  /// Per player, the showdown worked out in advance. Filled in by
  /// TerminalEvaluator::prepare(); empty means "work it out each time".
  std::array<ShowdownPlan, 2> plans;
};

ShowdownTable make_showdown_table(std::array<std::vector<HandStrength>, 2> strength);

}  // namespace gto
