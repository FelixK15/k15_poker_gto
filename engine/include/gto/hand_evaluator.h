#pragma once

#include <cstdint>
#include <span>

#include "gto/card.h"

namespace gto {

/// Strength of a poker hand. A larger value beats a smaller one; equal values tie.
/// Layout: category in bits 20+, then up to five 4-bit ranks, most significant first.
using HandStrength = std::uint32_t;

enum class HandCategory : std::uint8_t {
  HighCard,
  Pair,
  TwoPair,
  Trips,
  Straight,
  Flush,
  FullHouse,
  Quads,
  StraightFlush,
};

/// Best five-card hand out of 5, 6 or 7 distinct cards.
HandStrength evaluate_hand(std::span<const Card> cards) noexcept;

constexpr HandCategory hand_category(HandStrength strength) noexcept {
  return static_cast<HandCategory>(strength >> 20);
}

}  // namespace gto
