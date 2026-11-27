#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gto/mc/game.h"
#include "gto/mc/solver.h"
#include "solve_engine.h"

namespace gto::server {

/// Whether this build has the Monte Carlo engine.
inline constexpr bool kMonteCarloAvailable = true;

/// Multiway postflop (2 to 9 players, NLH and PLO4), preflop solves and
/// heads-up spots too large for the exact engine, through the gto::mc
/// external-sampling MCCFR engine.
///
/// Seats: Job.positions and every per-player array follow the mc engine's
/// numbering. Postflop spots: Spot.players order, player 0 acts first on
/// every street. Preflop solves: UTG … BTN, SB, BB (heads-up SB, BB), the
/// order of preflop action.
///
/// Paths are the mc engine's: the same as the exact engine's for postflop
/// spots (turn and river one card each); in a preflop solve the flop is one
/// entry of three cards ("raise2.5/call/Ks7d2c/check"). Preflop segments may
/// also be written "POS:action" (players before POS fold).
class McEngine final : public SolveEngine {
 public:
  /// Checks everything that can be checked without building the game.
  /// InvalidArgument if bad.
  static Error validate(const v1::Spot& spot);

  /// The mc game of a spot (validated first).
  static std::optional<mc::GameConfig> game_config(const v1::Spot& spot,
                                                   const EngineLimits& limits, std::string* error);

  /// Fills EstimateTree: engine MONTE_CARLO, the exploitability kind for the
  /// player count, players, positions, info sets, memory, abstraction.
  static Error estimate(const v1::Spot& spot, const EngineLimits& limits,
                        v1::EstimateTreeResponse* out);

  /// The reuse key: a canonical form of the mc game (flop in any order,
  /// ranges by weight with blocked combos ignored, defaults filled in).
  static std::string key(const v1::Spot& spot, std::string* error);

  static std::unique_ptr<McEngine> create(const v1::Spot& spot, const EngineLimits& limits,
                                          std::string* error);

  v1::Engine kind() const noexcept override { return v1::ENGINE_MONTE_CARLO; }
  std::size_t player_count() const noexcept override { return positions_.size(); }

  std::uint64_t iterations() const noexcept override { return solver_->iterations(); }
  /// About a quarter of a second of sampling.
  void run_chunk(std::uint64_t budget) override;
  /// Time based: evaluations cost real time (sampled best responses), so
  /// they run every few seconds and take at most about a fifth of it.
  bool measurement_due(int check_interval) const override;
  EngineEvaluation evaluate() const override;

  Error node(std::string_view path, const NodeOptions& options,
             v1::GetNodeResponse* out) const override;
  Error runouts(std::string_view path, v1::GetRunoutReportResponse* out) const override;
  Error preflop_scenarios(std::uint32_t max_raises,
                          v1::ListPreflopScenariosResponse* out) const override;
  Error preflop_ranges(std::string_view path, v1::GetPreflopRangesResponse* out) const override;

  bool save(std::ostream& out) const override;
  bool load(std::istream& in, std::string* error) override;
  std::uint64_t memory_bytes() const noexcept override;

  const mc::Solver& solver() const noexcept { return *solver_; }

  /// A path with "POS:action" segments written out in full (folds added).
  std::optional<std::string> expand_path(std::string_view path, std::string* error) const;

 private:
  McEngine() = default;

  std::unique_ptr<mc::Solver> solver_;
  v1::Spot spot_;
  std::vector<std::string> positions_;
  /// Chips each player posted before the action (blinds, antes): preflop
  /// EVs are reported relative to the stack before posting.
  std::vector<double> posted_;
  bool preflop_ = false;

  std::uint64_t chunk_ = 2000;
  mutable std::chrono::steady_clock::time_point last_measured_ = std::chrono::steady_clock::now();
  mutable double last_evaluation_seconds_ = 0;
};

}  // namespace gto::server
