#include "solve_engine.h"

#include <algorithm>

#include "convert.h"
#include "exact_engine.h"
#include "mc_engine.h"

namespace gto::server {

namespace {

Error plan_monte_carlo(const v1::Spot& spot, const EngineLimits& limits, SpotPlan* out) {
  out->estimate.Clear();
  if (Error error = McEngine::estimate(spot, limits, &out->estimate)) return error;
  out->engine = v1::ENGINE_MONTE_CARLO;
  std::string error;
  out->key = reuse_key(spot, v1::ENGINE_MONTE_CARLO, &error);
  return {};
}

Error preflop_only(const char* what) {
  return make_error(ErrorKind::FailedPrecondition,
                    std::string(what) + " exists for preflop solves only");
}

}  // namespace

std::size_t hand_limit(std::uint32_t requested, v1::Variant variant) noexcept {
  if (requested == 0) {
    return variant == v1::VARIANT_OMAHA4 ? kDefaultOmahaHands : kMaxHandsPerPlayer;
  }
  return std::min<std::size_t>(requested, kMaxHandsPerPlayer);
}

std::vector<std::string> spot_positions(const v1::Spot& spot) {
  const SpotShape shape = spot_shape(spot);
  if (shape.preflop) return preflop_positions(shape.players);
  std::vector<std::string> names;
  bool named = false;
  for (const v1::PlayerSpot& player : spot.players()) {
    names.push_back(player.position());
    named = named || !player.position().empty();
  }
  if (!named) {
    names.clear();
    if (spot.players_size() == 0 || spot.players_size() == 2) {
      names = {"OOP", "IP"};
    } else {
      for (int i = 0; i < spot.players_size(); ++i) names.push_back("P" + std::to_string(i + 1));
    }
  }
  return names;
}

std::string reuse_key(const v1::Spot& spot, v1::Engine engine, std::string* error) {
  if (engine == v1::ENGINE_MONTE_CARLO) {
    const std::string key = McEngine::key(spot, error);
    return key.empty() ? key : "M" + key;
  }
  const auto config = to_spot_config(spot, error);
  if (!config) return {};
  return "E" + spot_key(*config);
}

Error plan_spot(const v1::Spot& spot, const EngineLimits& limits, SpotPlan* out) {
  const v1::Engine requested = spot.engine();
  if (requested != v1::ENGINE_AUTO && requested != v1::ENGINE_EXACT &&
      requested != v1::ENGINE_MONTE_CARLO) {
    return make_error(ErrorKind::InvalidArgument,
                      "engine: unknown value " + std::to_string(static_cast<int>(requested)));
  }

  const SpotShape shape = spot_shape(spot);
  if (shape.preflop || (spot.players_size() != 0 && spot.players_size() != 2)) {
    if (requested == v1::ENGINE_EXACT) {
      return make_error(ErrorKind::InvalidArgument,
                        "the exact engine solves heads-up postflop spots only; use "
                        "ENGINE_AUTO or ENGINE_MONTE_CARLO");
    }
    return plan_monte_carlo(spot, limits, out);
  }
  if (requested == v1::ENGINE_MONTE_CARLO) return plan_monte_carlo(spot, limits, out);

  v1::EstimateTreeResponse estimate;
  if (Error error = ExactEngine::estimate(spot, limits, &estimate)) return error;
  if (requested == v1::ENGINE_AUTO && !estimate.can_solve() && kMonteCarloAvailable) {
    // Too large to solve exactly: Monte Carlo if that fits, else the exact
    // estimate stands (and a submit is refused with its numbers).
    SpotPlan monte_carlo;
    if (!plan_monte_carlo(spot, limits, &monte_carlo) && monte_carlo.estimate.can_solve()) {
      *out = std::move(monte_carlo);
      return {};
    }
  }
  out->engine = v1::ENGINE_EXACT;
  out->estimate = std::move(estimate);
  std::string error;
  out->key = reuse_key(spot, v1::ENGINE_EXACT, &error);
  return {};
}

std::unique_ptr<SolveEngine> make_engine(const v1::Spot& spot, v1::Engine engine,
                                         const EngineLimits& limits, std::string* error) {
  if (engine == v1::ENGINE_MONTE_CARLO) return McEngine::create(spot, limits, error);
  return ExactEngine::create(spot, limits, error);
}

// --- Defaults of the optional parts of SolveEngine ------------------------------

Error SolveEngine::preflop_scenarios(std::uint32_t, v1::ListPreflopScenariosResponse*) const {
  return preflop_only("ListPreflopScenarios");
}

Error SolveEngine::preflop_ranges(std::string_view, v1::GetPreflopRangesResponse*) const {
  return preflop_only("GetPreflopRanges");
}

Error SolveEngine::check_locks(
    const google::protobuf::RepeatedPtrField<v1::NodeLock>& locks) const {
  if (locks.empty()) return {};
  return make_error(ErrorKind::FailedPrecondition, "this job's engine does not support locks");
}

Error SolveEngine::apply_locks(const google::protobuf::RepeatedPtrField<v1::NodeLock>& locks) {
  return check_locks(locks);
}

}  // namespace gto::server
