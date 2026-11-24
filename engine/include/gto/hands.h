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

/// One step of a showdown, worked out in advance for a board: either add an
/// opponent hand to the running totals, or read off a result for one of ours.
struct ShowdownEvent {
  enum Kind : std::uint8_t {
    kAdd = 0,            // add opponent hand `source`
    kBeat = 1,           // our hand `target` beats everything added so far
    kBeatOrTie = 2,      // … beats or ties it
    kBeatOrTieSame = 3,  // … and the opponent's identical hand is `source`
  };

  std::uint16_t source = 0;
  std::uint16_t target = 0;
  /// Cards of the hand the event is about.
  std::uint8_t card1 = 0;
  std::uint8_t card2 = 0;
  std::uint8_t kind = kAdd;
};

/// Who beats whom on one complete board.
struct ShowdownTable {
  /// Strength of each hand; larger wins, equal ties. Hands that the board
  /// makes impossible may hold any value: they never reach a showdown.
  std::array<std::vector<HandStrength>, 2> strength;
  /// Hand indices ordered by ascending strength.
  std::array<std::vector<std::uint32_t>, 2> order;
  /// Per player, the showdown as a fixed list of steps. Filled in by
  /// TerminalEvaluator::prepare(); empty means "work it out each time".
  std::array<std::vector<ShowdownEvent>, 2> events;
};

ShowdownTable make_showdown_table(std::array<std::vector<HandStrength>, 2> strength);

}  // namespace gto
