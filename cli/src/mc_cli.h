#pragma once

// gto-cli commands on the Monte Carlo engine (gto::mc): preflop solves and
// multiway / Monte Carlo spots.

#include <string>
#include <vector>

namespace gto::cli {

/// `gto-cli preflop ...` (arguments after the command name).
int run_preflop(const std::vector<std::string>& args);

/// Whether a spot file asks for the mc engine ("engine": "mc" or a
/// "players" list) or is a preflop solve ("preflop" object).
bool is_mc_spot(const std::string& path);

/// Solves an mc spot file and prints EVs, the estimated exploitability and
/// the root strategy (plus the nodes asked for).
int run_mc_solve(const std::string& path, const std::vector<std::string>& node_paths,
                 unsigned threads);

}  // namespace gto::cli
