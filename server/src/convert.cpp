#include "convert.h"

#include <algorithm>

namespace gto::server {

namespace {

BetSizing to_sizing(const v1::BetSizing& sizing) {
  BetSizing result;
  for (const double pct : sizing.bet_pct()) result.bet.push_back(pct / 100.0);
  for (const double pct : sizing.raise_pct()) result.raise.push_back(pct / 100.0);
  result.add_allin = sizing.allin();
  return result;
}

}  // namespace

std::optional<SpotConfig> to_spot_config(const v1::Spot& spot, std::string* error) {
  SpotConfig config;

  const auto board = parse_cards(spot.board());
  if (!board) {
    *error = "board: cannot read the cards";
    return std::nullopt;
  }
  config.board = *board;

  std::string range_error;
  const auto oop = Range::parse(spot.oop_range(), &range_error);
  if (!oop) {
    *error = "oop_range: " + range_error;
    return std::nullopt;
  }
  const auto ip = Range::parse(spot.ip_range(), &range_error);
  if (!ip) {
    *error = "ip_range: " + range_error;
    return std::nullopt;
  }
  config.ranges[kOop] = *oop;
  config.ranges[kIp] = *ip;

  config.tree.pot = spot.pot();
  config.tree.effective_stack = spot.effective_stack();
  config.rake_rate = spot.rake_pct() / 100.0;
  config.rake_cap = spot.rake_cap();

  const v1::TreeSettings& tree = spot.tree();
  const v1::StreetSizing* streets[3] = {&tree.flop(), &tree.turn(), &tree.river()};
  for (std::size_t street = 0; street < 3; ++street) {
    config.tree.sizing[street][kOop] = to_sizing(streets[street]->oop());
    config.tree.sizing[street][kIp] = to_sizing(streets[street]->ip());
  }
  if (tree.has_raise_cap()) {
    config.tree.raise_cap = static_cast<int>(std::min<std::uint32_t>(tree.raise_cap(), 100));
  }
  if (tree.has_allin_threshold_pct()) {
    config.tree.allin_threshold = tree.allin_threshold_pct() / 100.0;
  }
  return config;
}

ResolvedSettings resolve(const v1::SolveSettings& settings) {
  ResolvedSettings resolved;
  if (settings.max_iterations() > 0) {
    resolved.max_iterations =
        static_cast<int>(std::min<std::uint32_t>(settings.max_iterations(), 1'000'000));
  }
  if (settings.has_target_exploitability_pct()) {
    resolved.target_exploitability_pct = std::max(0.0, settings.target_exploitability_pct());
  }
  return resolved;
}

}  // namespace gto::server
