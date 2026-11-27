#include "gto/mc/spot.h"

namespace gto::mc {

std::optional<GameConfig> make_game_config(const MultiwaySpot& spot, std::string* error) {
  const auto fail = [&](std::string message) -> std::optional<GameConfig> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };
  GameConfig config;
  config.variant = spot.variant;
  const std::size_t players = spot.stacks.size();
  config.num_players = static_cast<int>(players);
  switch (spot.board.size()) {
    case 3: config.start_street = Street::Flop; break;
    case 4: config.start_street = Street::Turn; break;
    case 5: config.start_street = Street::River; break;
    default: return fail("the board needs three, four or five cards");
  }
  config.board = spot.board;
  config.last_street = spot.last_street;
  config.stacks = spot.stacks;
  config.dead_pot = spot.pot;
  config.sizing = spot.sizing;
  config.raise_cap = spot.raise_cap;
  config.allin_threshold = spot.allin_threshold;
  config.pot_limit = spot.pot_limit || spot.variant == Variant::Omaha4;
  config.rake = spot.rake;
  config.abstraction = spot.abstraction;
  config.max_nodes = spot.max_nodes;
  if (spot.variant == Variant::Holdem) {
    if (spot.ranges.size() != players) return fail("need one range per player");
    for (const Range& range : spot.ranges) {
      config.ranges.emplace_back(range.weights().begin(), range.weights().end());
    }
  } else {
    if (spot.omaha_ranges.size() != players) return fail("need one range per player");
    for (const OmahaRange& range : spot.omaha_ranges) config.ranges.push_back(range.weights());
  }
  if (!config.validate(error)) return std::nullopt;
  return config;
}

std::optional<GameEstimate> estimate_spot(const MultiwaySpot& spot, std::string* error) {
  const auto config = make_game_config(spot, error);
  if (!config) return std::nullopt;
  return estimate_game(*config, error);
}

std::unique_ptr<Solver> make_solver(const MultiwaySpot& spot, const SolverConfig& solver,
                                    std::string* error) {
  const auto config = make_game_config(spot, error);
  if (!config) return nullptr;
  return Solver::create(*config, solver, error);
}

}  // namespace gto::mc
