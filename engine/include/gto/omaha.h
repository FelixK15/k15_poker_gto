#pragma once

// Pot-Limit Omaha with four hole cards (PLO4): combo indexing, hand
// evaluation (exactly two hole cards and three board cards) and ranges.

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "gto/card.h"
#include "gto/hand_evaluator.h"

namespace gto {

/// Four-card combos out of 52 cards: C(52, 4).
inline constexpr int kOmahaCombos = 270'725;

/// Index of a four-card combo in 0 … kOmahaCombos-1. The cards must differ;
/// their order does not matter.
int omaha_combo_index(const std::array<Card, 4>& cards) noexcept;

/// The four cards of a combo, ascending by card index.
std::array<Card, 4> omaha_combo_cards(int index) noexcept;

/// Text form, cards ordered by rank (highest first), e.g. "AsKsQhJh".
std::string omaha_hand_name(const std::array<Card, 4>& cards);

/// Best Omaha hand: exactly two of the four hole cards with exactly three of
/// the three to five board cards. Straightforward reference implementation.
HandStrength evaluate_omaha(const std::array<Card, 4>& hole, std::span<const Card> board) noexcept;

/// Fast Omaha evaluation on one fixed board of three to five cards. The
/// constructor evaluates every two-card holding with every three-card subset
/// of the board once; evaluate() then takes the best of at most 60 table
/// lookups. Thread-safe after construction.
class OmahaBoardEvaluator {
 public:
  explicit OmahaBoardEvaluator(std::span<const Card> board);

  /// Same result as evaluate_omaha(hole, board). Hole cards must not be on
  /// the board.
  HandStrength evaluate(const std::array<Card, 4>& hole) const noexcept;

 private:
  /// [pair index 0…1325] = best strength of that pair with any board triple.
  std::vector<HandStrength> best_with_pair_;
};

/// A PLO4 range: one weight in [0, 1] per four-card combo.
class OmahaRange {
 public:
  OmahaRange() : weights_(static_cast<std::size_t>(kOmahaCombos), 0.0f) {}

  /// Every combo with weight 1.
  static OmahaRange full();

  /// Parses a comma-separated list. Items, each with an optional ":weight":
  ///   "*" or "all"      every combo
  ///   "AsKsQhJh"        one exact combo
  ///   "AAKK", "AKQJ"    every combo with these four ranks
  ///   "AAxx", "KQJx"    'x' stands for any rank
  ///   suffix "ds", "ss", "r"   only double-suited, single-suited (exactly
  ///                     one suited pair, the other two cards of other
  ///                     suits) or rainbow combos, e.g. "AAxxds"
  ///   "top15%"          the strongest 15% of combos by a fixed preflop
  ///                     ranking (see omaha_preflop_rank)
  /// Later items override earlier ones. An empty string is an empty range.
  static std::optional<OmahaRange> parse(std::string_view text, std::string* error = nullptr);

  float weight(int combo) const noexcept { return weights_[static_cast<std::size_t>(combo)]; }
  void set_weight(int combo, float weight) noexcept {
    weights_[static_cast<std::size_t>(combo)] = weight;
  }
  const std::vector<float>& weights() const noexcept { return weights_; }
  double total_weight() const noexcept;

 private:
  std::vector<float> weights_;
};

/// A fixed preflop strength ranking of all PLO4 combos, strongest first
/// (index 0 is the best combo). Used by "top N%" ranges and as a coarse
/// preflop bucketing. Computed once on first use.
const std::vector<int>& omaha_preflop_rank();

}  // namespace gto
