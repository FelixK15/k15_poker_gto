#pragma once

#include <memory>
#include <string>

#include "solve_engine.h"

namespace gto::server {

/// Whether this build has the Monte Carlo engine. While it is false, AUTO
/// keeps refusing heads-up spots too large for the exact engine with
/// RESOURCE_EXHAUSTED, and Monte Carlo spots fail with FAILED_PRECONDITION
/// after validation.
inline constexpr bool kMonteCarloAvailable = false;
inline constexpr const char* kMonteCarloMissing = "Monte Carlo engine not built yet";

/// Multiway postflop (2 to 9 players), preflop solves and heads-up spots too
/// large for the exact engine, through the gto::mc external-sampling MCCFR
/// engine.
///
/// STUB: validates spots and otherwise refuses with kMonteCarloMissing.
/// Filling it in means: estimate() from the mc tree/bucket estimate, create()
/// building the mc game from the spot (see validate() for what is read),
/// the SolveEngine overrides mapping to the mc API, and kMonteCarloAvailable
/// set to true.
class McEngine final : public SolveEngine {
 public:
  /// Checks everything a Monte Carlo spot needs that can be checked without
  /// building it: player count, positions, ranges in the variant's syntax,
  /// stacks, board, preflop settings, abstraction. InvalidArgument if bad.
  static Error validate(const v1::Spot& spot);

  /// Fills EstimateTree (engine MONTE_CARLO, the exploitability kind for the
  /// player count, players and positions). Until the engine exists:
  /// validate(), then FailedPrecondition.
  static Error estimate(const v1::Spot& spot, const EngineLimits& limits,
                        v1::EstimateTreeResponse* out);

  /// Until the engine exists: nullptr with kMonteCarloMissing.
  static std::unique_ptr<McEngine> create(const v1::Spot& spot, const EngineLimits& limits,
                                          std::string* error);

  v1::Engine kind() const noexcept override { return v1::ENGINE_MONTE_CARLO; }
  std::size_t player_count() const noexcept override { return players_; }

  std::uint64_t iterations() const noexcept override { return 0; }
  void run_chunk(std::uint64_t budget) override;
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
  std::uint64_t memory_bytes() const noexcept override { return 0; }

 private:
  McEngine() = default;
  std::size_t players_ = 0;
};

}  // namespace gto::server
