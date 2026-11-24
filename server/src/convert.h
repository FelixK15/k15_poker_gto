#pragma once

#include <optional>
#include <string>

#include "gto/solver.h"
#include "gto/v1/common.pb.h"

namespace gto::server {

/// Reads a spot from the wire. Checks what can be checked without building
/// the tree: the board and both ranges must be readable.
std::optional<SpotConfig> to_spot_config(const v1::Spot& spot, std::string* error);

/// Solve settings with the documented defaults filled in.
struct ResolvedSettings {
  int max_iterations = 1000;
  double target_exploitability_pct = 0.3;
};
ResolvedSettings resolve(const v1::SolveSettings& settings);

}  // namespace gto::server
