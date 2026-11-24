#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gto {

/// A playing card, stored as an index 0..51 = rank * 4 + suit.
/// Rank: 0 = deuce … 12 = ace. Suit: 0 = clubs, 1 = diamonds, 2 = hearts, 3 = spades.
class Card {
 public:
  static constexpr int kDeckSize = 52;
  static constexpr int kNumRanks = 13;
  static constexpr int kNumSuits = 4;

  constexpr Card() noexcept = default;
  constexpr explicit Card(std::uint8_t index) noexcept : index_(index) {}
  constexpr Card(int rank, int suit) noexcept
      : index_(static_cast<std::uint8_t>(rank * kNumSuits + suit)) {}

  constexpr std::uint8_t index() const noexcept { return index_; }
  constexpr int rank() const noexcept { return index_ / kNumSuits; }
  constexpr int suit() const noexcept { return index_ % kNumSuits; }
  /// Single-bit mask for use in 64-bit card sets.
  constexpr std::uint64_t mask() const noexcept { return std::uint64_t{1} << index_; }

  /// Two-character form, e.g. "As", "Td", "2c".
  std::string to_string() const;

  /// Parses "As", "td", "2C" … Returns nullopt for anything else.
  static std::optional<Card> parse(std::string_view text) noexcept;

  friend constexpr bool operator==(Card, Card) noexcept = default;
  friend constexpr auto operator<=>(Card, Card) noexcept = default;

 private:
  std::uint8_t index_ = 0;
};

/// Rank index (0 = deuce … 12 = ace) for '2'…'9', 'T', 'J', 'Q', 'K', 'A' in
/// either case, or -1.
int rank_from_char(char c) noexcept;

/// Inverse of rank_from_char for 0…12.
char rank_to_char(int rank) noexcept;

/// Parses a run of cards such as "AsKd7c" or "As Kd 7c".
/// Returns nullopt on any malformed or duplicated card.
std::optional<std::vector<Card>> parse_cards(std::string_view text);

}  // namespace gto
