#include "spot_config.h"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace gto::cli {

namespace {

using nlohmann::json;

/// Thrown for anything wrong with the content of the file.
struct ConfigError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

std::vector<double> percentages(const json& object, const char* key) {
  std::vector<double> fractions;
  if (!object.contains(key)) return fractions;
  for (const json& value : object.at(key)) fractions.push_back(value.get<double>() / 100.0);
  return fractions;
}

BetSizing read_sizing(const json& street, const char* player) {
  BetSizing sizing;
  if (!street.contains(player)) return sizing;
  const json& object = street.at(player);
  sizing.bet = percentages(object, "bet_pct");
  sizing.raise = percentages(object, "raise_pct");
  sizing.add_allin = object.value("allin", false);
  return sizing;
}

Range read_range(const json& ranges, const char* player) {
  std::string error;
  const auto range = Range::parse(ranges.at(player).get<std::string>(), &error);
  if (!range) throw ConfigError(std::string("ranges.") + player + ": " + error);
  return *range;
}

OmahaRange read_omaha_range(const json& ranges, const char* player) {
  std::string error;
  const auto range = OmahaRange::parse(ranges.at(player).get<std::string>(), &error);
  if (!range) throw ConfigError(std::string("ranges.") + player + ": " + error);
  return *range;
}

LoadedSpot read_spot(const json& root) {
  LoadedSpot loaded;
  SpotConfig& spot = loaded.spot;

  const auto board = parse_cards(root.at("board").get<std::string>());
  if (!board) throw ConfigError("board: cannot read the cards");
  spot.board = *board;

  const std::string variant = root.value("variant", std::string("holdem"));
  if (variant == "omaha4" || variant == "plo" || variant == "plo4") {
    spot.variant = Variant::Omaha4;
  } else if (variant != "holdem") {
    throw ConfigError("variant: expected \"holdem\" or \"omaha4\", not \"" + variant + "\"");
  }

  const json& ranges = root.at("ranges");
  if (spot.variant == Variant::Omaha4) {
    spot.omaha_ranges[kOop] = read_omaha_range(ranges, "oop");
    spot.omaha_ranges[kIp] = read_omaha_range(ranges, "ip");
    // PLO is pot-limit unless the tree says otherwise.
    spot.tree.pot_limit = true;
  } else {
    spot.ranges[kOop] = read_range(ranges, "oop");
    spot.ranges[kIp] = read_range(ranges, "ip");
  }

  spot.tree.pot = root.at("pot").get<double>();
  spot.tree.effective_stack = root.at("effective_stack").get<double>();

  if (root.contains("rake")) {
    const json& rake = root.at("rake");
    spot.rake_rate = rake.value("percent", 0.0) / 100.0;
    spot.rake_cap = rake.value("cap", 0.0);
  }

  if (root.contains("tree")) {
    const json& tree = root.at("tree");
    static constexpr const char* kStreets[] = {"flop", "turn", "river"};
    for (std::size_t street = 0; street < 3; ++street) {
      if (!tree.contains(kStreets[street])) continue;
      const json& object = tree.at(kStreets[street]);
      spot.tree.sizing[street][kOop] = read_sizing(object, "oop");
      spot.tree.sizing[street][kIp] = read_sizing(object, "ip");
    }
    spot.tree.raise_cap = tree.value("raise_cap", spot.tree.raise_cap);
    spot.tree.pot_limit = tree.value("pot_limit", spot.tree.pot_limit);
    if (tree.contains("allin_threshold_pct")) {
      spot.tree.allin_threshold = tree.at("allin_threshold_pct").get<double>() / 100.0;
    }
  }

  if (root.contains("solve")) {
    const json& solve = root.at("solve");
    loaded.solve.max_iterations = solve.value("max_iterations", loaded.solve.max_iterations);
    loaded.solve.target_exploitability_pct =
        solve.value("target_exploitability_pct", loaded.solve.target_exploitability_pct);
    loaded.solve.check_interval = solve.value("check_interval", loaded.solve.check_interval);
  }
  return loaded;
}

}  // namespace

std::optional<LoadedSpot> load_spot(const std::string& path, std::string* error) {
  std::ifstream file(path);
  if (!file) {
    *error = "cannot open '" + path + "'";
    return std::nullopt;
  }
  try {
    // Comments are allowed so that example files can explain themselves.
    const json root = json::parse(file, nullptr, true, true);
    return read_spot(root);
  } catch (const ConfigError& e) {
    *error = e.what();
  } catch (const json::exception& e) {
    // Covers syntax errors, missing keys and values of the wrong type.
    *error = e.what();
  }
  return std::nullopt;
}

}  // namespace gto::cli
