#pragma once

#include <cstdint>

namespace gto {

/// The poker game being played.
enum class Variant : std::uint8_t {
  /// No-Limit Hold'em: two hole cards, any five of seven.
  Holdem = 0,
  /// Pot-Limit Omaha with four hole cards: exactly two hole cards and three
  /// board cards.
  Omaha4 = 1,
};

constexpr int hole_card_count(Variant variant) noexcept {
  return variant == Variant::Holdem ? 2 : 4;
}

/// Number of distinct starting hands: C(52, 2) or C(52, 4).
constexpr int combo_count(Variant variant) noexcept {
  return variant == Variant::Holdem ? 1326 : 270'725;
}

}  // namespace gto
