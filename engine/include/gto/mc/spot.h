#pragma once

// Multiway postflop spots (2-9 players, NLH or PLO4) for the mc engine: the
// form a user describes a spot in, turned into a GameConfig and a Solver.

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gto/mc/game.h"
#include "gto/mc/solver.h"
#include "gto/omaha.h"
#include "gto/range.h"

namespace gto::mc {

struct MultiwaySpot {
  Variant variant = Variant::Holdem;
  /// Three, four or five cards: the spot starts on the flop, turn or river.
  std::vector<Card> board;
  /// One range per player, in postflop action order (player 0 acts first).
  /// Hold'em uses `ranges`, PLO `omaha_ranges`.
  std::vector<Range> ranges;
  std::vector<OmahaRange> omaha_ranges;
  /// Pot at the start of the spot (dead money for the payoff convention).
  double pot = 0;
  /// Chips behind for each player.
  std::vector<double> stacks;
  /// Bet sizes per street (index by Street; Flop..River used).
  std::array<StreetSizing, kNumStreets> sizing;
  int raise_cap = 3;
  double allin_threshold = 0.67;
  /// Pot limit; PLO spots are always pot limit.
  bool pot_limit = false;
  RakeConfig rake;
  AbstractionConfig abstraction;
  /// Last street with betting (River for a normal spot).
  Street last_street = Street::River;
  std::size_t max_nodes = 20'000'000;
};

/// The GameConfig of a spot (validated).
std::optional<GameConfig> make_game_config(const MultiwaySpot& spot, std::string* error = nullptr);
/// Size of a spot before building anything.
std::optional<GameEstimate> estimate_spot(const MultiwaySpot& spot, std::string* error = nullptr);
/// A solver for a spot.
std::unique_ptr<Solver> make_solver(const MultiwaySpot& spot, const SolverConfig& config = {},
                                    std::string* error = nullptr);

}  // namespace gto::mc
