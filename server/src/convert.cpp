#include "convert.h"

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace gto::server {

namespace {

BetSizing to_sizing(const v1::BetSizing& sizing) {
  BetSizing result;
  for (const double pct : sizing.bet_pct()) result.bet.push_back(pct / 100.0);
  for (const double pct : sizing.raise_pct()) result.raise.push_back(pct / 100.0);
  result.add_allin = sizing.allin();
  return result;
}

template <typename T>
void append(std::string& key, T value) {
  static_assert(std::is_trivially_copyable_v<T>);
  char bytes[sizeof(T)];
  std::memcpy(bytes, &value, sizeof(T));
  key.append(bytes, sizeof(T));
}

void append(std::string& key, const std::vector<double>& values) {
  append(key, static_cast<std::uint32_t>(values.size()));
  for (const double value : values) append(key, value);
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

std::string spot_key(const SpotConfig& config) {
  std::string key;
  key.reserve(2 * Range::kNumCombos * sizeof(float) + 256);

  std::vector<Card> board = config.board;
  const std::size_t flop = std::min<std::size_t>(board.size(), 3);
  std::sort(board.begin(), board.begin() + static_cast<std::ptrdiff_t>(flop));
  append(key, static_cast<std::uint8_t>(board.size()));
  std::uint64_t dead = 0;
  for (const Card card : board) {
    append(key, card.index());
    dead |= card.mask();
  }

  for (const Range& range : config.ranges) {
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      const auto [a, b] = Range::combo_cards(combo);
      const bool blocked = ((a.mask() | b.mask()) & dead) != 0;
      append(key, blocked ? 0.0f : range.weight(combo));
    }
  }

  const TreeConfig& tree = config.tree;
  append(key, tree.pot);
  append(key, tree.effective_stack);
  for (const auto& street : tree.sizing) {
    for (const BetSizing& sizing : street) {
      append(key, sizing.bet);
      append(key, sizing.raise);
      append(key, static_cast<std::uint8_t>(sizing.add_allin));
    }
  }
  append(key, static_cast<std::int32_t>(tree.raise_cap));
  append(key, tree.allin_threshold);
  append(key, config.rake_rate);
  append(key, config.rake_cap);
  return key;
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
