#pragma once

#include <optional>
#include <string>

#include "gto/solver.h"

namespace gto::cli {

struct LoadedSpot {
  SpotConfig spot;
  SolveOptions solve;
};

/// Reads a spot description from a JSON file. See examples/ for the format.
std::optional<LoadedSpot> load_spot(const std::string& path, std::string* error);

}  // namespace gto::cli
