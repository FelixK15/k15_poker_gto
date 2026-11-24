#pragma once

#include <array>
#include <cstdint>
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

/// What the solver needs to know about both players' private hands.
struct HandSets {
  std::array<std::vector<Hand>, 2> hands;
  /// Showdown strength of each hand; larger wins, equal ties.
  std::array<std::vector<HandStrength>, 2> strength;
};

}  // namespace gto
