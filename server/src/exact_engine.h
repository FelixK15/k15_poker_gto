#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gto/solver.h"
#include "solve_engine.h"

namespace gto::server {

/// Heads-up postflop spots, Hold'em and PLO4, solved over every hand by the
/// vector-CFR gto::Solver: exact exploitability and node locks.
class ExactEngine final : public SolveEngine {
 public:
  /// Sizes up a heads-up postflop spot and fills everything EstimateTree
  /// reports. InvalidArgument if the spot cannot be solved at all.
  static Error estimate(const v1::Spot& spot, const EngineLimits& limits,
                        v1::EstimateTreeResponse* out);

  /// Builds the solver; restores nothing yet (see load()).
  static std::unique_ptr<ExactEngine> create(const v1::Spot& spot, const EngineLimits& limits,
                                             std::string* error);

  v1::Engine kind() const noexcept override { return v1::ENGINE_EXACT; }
  std::size_t player_count() const noexcept override { return 2; }

  std::uint64_t iterations() const noexcept override {
    return static_cast<std::uint64_t>(solver_->iterations());
  }
  void run_chunk(std::uint64_t budget) override;
  bool measurement_due(int check_interval) const override;
  EngineEvaluation evaluate() const override;

  Error node(std::string_view path, const NodeOptions& options,
             v1::GetNodeResponse* out) const override;
  Error runouts(std::string_view path, v1::GetRunoutReportResponse* out) const override;

  bool supports_locks() const noexcept override { return true; }
  Error check_locks(const google::protobuf::RepeatedPtrField<v1::NodeLock>& locks) const override;
  Error apply_locks(const google::protobuf::RepeatedPtrField<v1::NodeLock>& locks) override;
  void warm_restart() override { solver_->warm_restart(); }

  bool save(std::ostream& out) const override;
  bool load(std::istream& in, std::string* error) override;
  std::uint64_t memory_bytes() const noexcept override;

  const Solver& solver() const noexcept { return *solver_; }

 private:
  ExactEngine(std::unique_ptr<Solver> solver, std::vector<Card> board, Variant variant,
              std::vector<std::string> positions);

  std::unique_ptr<Solver> solver_;
  /// The spot's board as the client gave it.
  std::vector<Card> board_;
  Variant variant_;
  std::vector<std::string> positions_;
};

}  // namespace gto::server
