#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "gto/card.h"

namespace gto {

/// A Hold'em range: one weight in [0, 1] for each of the 1326 two-card combos.
class Range {
 public:
  static constexpr int kNumCombos = 1326;

  /// Index of the combo {a, b} in 0…1325. The cards must differ.
  static int combo_index(Card a, Card b) noexcept;

  /// The two cards of a combo, lower card index first.
  static std::pair<Card, Card> combo_cards(int index) noexcept;

  /// Parses the text format used by PioSOLVER and others, e.g.
  ///   "QQ+, AKs, AQo:0.5, A5s-A2s, 98s, KhQh:0.25"
  /// Supported: pairs "77", "AKs", "AKo", "AK" (suited and offsuit), exact
  /// combos "AhKh", "+" ("TT+", "ATs+"), "-" ("QQ-TT", "A5s-A2s"), and an
  /// optional ":weight" on any item. Later items override earlier ones.
  /// An empty string is an empty range.
  static std::optional<Range> parse(std::string_view text, std::string* error = nullptr);

  /// Text form that parse() reads back to the same weights (to 4 digits).
  std::string to_string() const;

  float weight(int combo) const noexcept { return weights_[static_cast<std::size_t>(combo)]; }
  float weight(Card a, Card b) const noexcept { return weight(combo_index(a, b)); }
  void set_weight(int combo, float weight) noexcept {
    weights_[static_cast<std::size_t>(combo)] = weight;
  }
  void set_weight(Card a, Card b, float weight) noexcept { set_weight(combo_index(a, b), weight); }

  const std::array<float, kNumCombos>& weights() const noexcept { return weights_; }

  /// Sum of all weights, i.e. the number of combos in the range.
  double total_weight() const noexcept;

 private:
  std::array<float, kNumCombos> weights_{};
};

/// "AA", "AKs" or "T9o" for a two-card hand.
std::string hand_class_name(Card a, Card b);

}  // namespace gto
