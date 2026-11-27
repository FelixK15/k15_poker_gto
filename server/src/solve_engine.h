#pragma once

// The server's view of a solver. JobManager queues, runs, saves and browses
// jobs through this interface only; each engine turns a v1::Spot into its own
// game and answers in proto terms.
//
//   ExactEngine (exact_engine.h)  heads-up postflop, NLH and PLO4, through
//                                 the vector-CFR gto::Solver. Exact
//                                 exploitability, node locks.
//   McEngine    (mc_engine.h)     multiway postflop, preflop and spots too
//                                 large for the exact engine, through the
//                                 gto::mc Monte Carlo CFR engine.

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "error.h"
#include "gto/v1/common.pb.h"
#include "gto/v1/solutions.pb.h"
#include "gto/v1/trees.pb.h"

namespace gto::server {

/// The server's resource limits, as far as engines need them.
struct EngineLimits {
  /// Threads for a solve; 0 means one per hardware thread.
  unsigned threads = 0;
  /// Largest solve to accept, in bytes; 0 for no limit.
  std::uint64_t memory_budget_bytes = 0;
  /// Largest exact tree to accept.
  std::uint64_t max_nodes = 100'000'000;
};

/// A measurement of the current strategies.
struct EngineEvaluation {
  double exploitability_pct = 0;
  v1::ExploitabilityKind exploitability_kind = v1::EXPLOITABILITY_KIND_EXACT;
  /// EV of each player in seat order, in chips.
  std::vector<double> evs;
  /// EVs and exploitability are sampled estimates.
  bool estimated = false;
};

/// How much of the ranges a node report lists (GetNodeRequest).
struct NodeOptions {
  std::uint32_t max_hands = 0;
  v1::HandDetail detail = v1::HAND_DETAIL_HANDS;
};

/// Per-player hand limit of a node report: `requested` (0 for the default)
/// resolved for the variant, see GetNodeRequest.max_hands.
inline constexpr std::size_t kDefaultOmahaHands = 2000;
inline constexpr std::size_t kMaxHandsPerPlayer = 20000;
std::size_t hand_limit(std::uint32_t requested, v1::Variant variant) noexcept;

/// What a submit of a spot runs on, decided from the spot alone and the same
/// way every time: at estimate, at submit and when a job is loaded again.
struct SpotPlan {
  /// Never ENGINE_AUTO.
  v1::Engine engine = v1::ENGINE_EXACT;
  /// Complete answer to EstimateTree, including engine, exploitability kind,
  /// players and positions.
  v1::EstimateTreeResponse estimate;
  /// reuse_key() of the spot on that engine.
  std::string key;
};

/// Checks a spot and decides which engine solves it (Spot.engine, AUTO: exact
/// when a heads-up postflop tree fits the limits). Errors: InvalidArgument
/// for a spot no engine can solve, FailedPrecondition when the engine it
/// needs is not available in this build. A spot too large for the limits is
/// no error: estimate.can_solve() is false.
Error plan_spot(const v1::Spot& spot, const EngineLimits& limits, SpotPlan* out);

/// A key two spots share exactly when the engine would solve the same game
/// (see spot_key() in convert.h). Without the tree estimate, so cheap enough
/// for every job at start-up. Empty with an error for a spot that cannot be
/// read.
std::string reuse_key(const v1::Spot& spot, v1::Engine engine, std::string* error);

/// Position names in seat order for any spot (Job.positions).
std::vector<std::string> spot_positions(const v1::Spot& spot);

class SolveEngine {
 public:
  virtual ~SolveEngine() = default;

  /// ENGINE_EXACT or ENGINE_MONTE_CARLO.
  virtual v1::Engine kind() const noexcept = 0;
  virtual std::size_t player_count() const noexcept = 0;

  // --- Solving ---

  /// Work done so far in the unit a job's budget counts: CFR iterations for
  /// the exact engine (SolveSettings.max_iterations), sampled iterations for
  /// Monte Carlo (SolveSettings.mc_iterations).
  virtual std::uint64_t iterations() const noexcept = 0;
  /// One unit of work, short enough that pausing and browsing (which wait
  /// for it) stay responsive: one iteration for the exact engine, a batch of
  /// samples (a fraction of a second) for Monte Carlo. Never runs past
  /// `budget` iterations.
  virtual void run_chunk(std::uint64_t budget) = 0;
  /// Whether to measure after the latest chunk. JobManager measures in any
  /// case when the job stops on its budget.
  virtual bool measurement_due(int check_interval) const = 0;
  virtual EngineEvaluation evaluate() const = 0;

  // --- Browsing; `path` as in GetNodeRequest ---

  virtual Error node(std::string_view path, const NodeOptions& options,
                     v1::GetNodeResponse* out) const = 0;
  virtual Error runouts(std::string_view path, v1::GetRunoutReportResponse* out) const = 0;
  /// Preflop solves only; the default refuses.
  virtual Error preflop_scenarios(std::uint32_t max_raises,
                                  v1::ListPreflopScenariosResponse* out) const;
  virtual Error preflop_ranges(std::string_view path, v1::GetPreflopRangesResponse* out) const;

  // --- Node locks (locks.proto) ---

  virtual bool supports_locks() const noexcept { return false; }
  /// Checks locks against the solution without applying them.
  virtual Error check_locks(const google::protobuf::RepeatedPtrField<v1::NodeLock>& locks) const;
  virtual Error apply_locks(const google::protobuf::RepeatedPtrField<v1::NodeLock>& locks);
  /// Forgets the average strategy but keeps what was learned, for a re-solve
  /// under locks.
  virtual void warm_restart() {}

  // --- State ---

  /// Writes everything learned so far, without the game: load() needs an
  /// engine built from the same spot. JobManager wraps these streams in zstd.
  virtual bool save(std::ostream& out) const = 0;
  /// Restores a state written by save(), or fails leaving the engine as it
  /// was.
  virtual bool load(std::istream& in, std::string* error) = 0;
  /// Memory held for the solve.
  virtual std::uint64_t memory_bytes() const noexcept = 0;
};

/// Builds the engine a plan chose (Job.engine). `engine` ENGINE_AUTO, as
/// stored by servers before engines existed, means exact.
std::unique_ptr<SolveEngine> make_engine(const v1::Spot& spot, v1::Engine engine,
                                         const EngineLimits& limits, std::string* error);

}  // namespace gto::server
