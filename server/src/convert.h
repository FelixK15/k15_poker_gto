#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "gto/solver.h"
#include "gto/v1/common.pb.h"

namespace gto::server {

/// The kind of game a spot asks for, read from the wire without checking it.
struct SpotShape {
  /// Players at the start: PreflopSettings.players for a preflop solve, else
  /// Spot.players (two when it is empty).
  std::size_t players = 2;
  bool preflop = false;
  v1::Variant variant = v1::VARIANT_HOLDEM;

  bool heads_up_postflop() const noexcept { return !preflop && players == 2; }
};
SpotShape spot_shape(const v1::Spot& spot);

/// Reads a heads-up postflop spot from the wire (oop_range / ip_range, or two
/// Spot.players). Checks what can be checked without building the tree: the
/// board, the variant and both ranges must be readable. Pot-limit defaults
/// to on for PLO4.
std::optional<SpotConfig> to_spot_config(const v1::Spot& spot, std::string* error);

/// A key that two spots share exactly when they make the same game: the flop
/// cards in any order, ranges compared by their weights (combos the board
/// blocks ignored; PLO ranges by a 128-bit hash of those weights), tree
/// defaults filled in, variant and pot limit. Bet sizes count in the order
/// given, since that is the order of the actions.
std::string spot_key(const SpotConfig& config);

/// Position names of a preflop table of 2 to 9 players, in the mc engine's
/// seat order, which is the order of preflop action: UTG … BTN, SB, BB
/// (heads-up BTN, BB; the button posts the small blind).
std::vector<std::string> preflop_positions(std::size_t players);

/// Players of a PreflopSettings.preset ("hu", "6max", "9max" and aliases),
/// or 0 for an unknown name.
std::size_t preset_players(std::string_view preset);

/// A position name can appear in paths ("BTN:raise2.5"): 1 to 12 letters,
/// digits, '+', '-' or '_'.
bool valid_position(std::string_view name);

/// Checks that `names` are valid and unique.
bool check_positions(const std::vector<std::string>& names, std::string* error);

/// Solve settings with the documented defaults filled in.
struct ResolvedSettings {
  int max_iterations = 1000;
  double target_exploitability_pct = 0.3;
  /// 0: no time limit.
  double max_seconds = 0;
  std::uint64_t mc_iterations = 0;
};
/// Monte Carlo budget when SolveSettings.mc_iterations is 0.
inline constexpr std::uint64_t kDefaultMcIterations = 100'000'000;
ResolvedSettings resolve(const v1::SolveSettings& settings);

}  // namespace gto::server
