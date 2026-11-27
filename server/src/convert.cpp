#include "convert.h"

#include "gto/mc/game.h"

#include <algorithm>
#include <cstring>
#include <set>
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

/// Two independent 64-bit hashes over the weights of a PLO range, combos the
/// board blocks counted as zero. A 270,725-float key per range would cost a
/// megabyte per job.
void append_omaha(std::string& key, const std::optional<OmahaRange>& range, std::uint64_t dead) {
  std::uint64_t fnv = 0xcbf29ce484222325ull;
  std::uint64_t mix = 0x9e3779b97f4a7c15ull;
  for (int combo = 0; combo < kOmahaCombos; ++combo) {
    float weight = range ? range->weight(combo) : 0.0f;
    if (weight == 0.0f) continue;
    std::uint64_t mask = 0;
    for (const Card card : omaha_combo_cards(combo)) mask |= card.mask();
    if ((mask & dead) != 0) continue;
    std::uint32_t bits = 0;
    std::memcpy(&bits, &weight, sizeof(bits));
    const std::uint64_t item = (static_cast<std::uint64_t>(combo) << 32) | bits;
    for (int byte = 0; byte < 8; ++byte) {
      fnv ^= (item >> (8 * byte)) & 0xff;
      fnv *= 0x100000001b3ull;
    }
    // splitmix64 of the item, folded in order-dependently.
    std::uint64_t z = item + 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    z ^= z >> 31;
    mix = (mix ^ z) * 0xff51afd7ed558ccdull + 0x2545f4914f6cdd1dull;
  }
  append(key, fnv);
  append(key, mix);
}

}  // namespace

SpotShape spot_shape(const v1::Spot& spot) {
  SpotShape shape;
  shape.variant = spot.variant();
  if (spot.has_preflop()) {
    shape.preflop = true;
    shape.players = spot.preflop().players();
  } else if (spot.players_size() > 0) {
    shape.players = static_cast<std::size_t>(spot.players_size());
  }
  return shape;
}

std::optional<SpotConfig> to_spot_config(const v1::Spot& spot, std::string* error) {
  SpotConfig config;
  const SpotShape shape = spot_shape(spot);
  if (!shape.heads_up_postflop()) {
    *error = "not a heads-up postflop spot";
    return std::nullopt;
  }

  switch (spot.variant()) {
    case v1::VARIANT_HOLDEM: config.variant = Variant::Holdem; break;
    case v1::VARIANT_OMAHA4: config.variant = Variant::Omaha4; break;
    default:
      *error = "variant: unknown value " + std::to_string(static_cast<int>(spot.variant()));
      return std::nullopt;
  }

  const auto board = parse_cards(spot.board());
  if (!board) {
    *error = "board: cannot read the cards";
    return std::nullopt;
  }
  config.board = *board;

  // Two players either way: the heads-up fields, or Spot.players.
  const bool listed = spot.players_size() == 2;
  const std::string names[2] = {listed ? "players[0].range" : "oop_range",
                                listed ? "players[1].range" : "ip_range"};
  const std::string texts[2] = {listed ? spot.players(0).range() : spot.oop_range(),
                                listed ? spot.players(1).range() : spot.ip_range()};
  for (std::size_t player = 0; player < 2; ++player) {
    std::string range_error;
    if (config.variant == Variant::Omaha4) {
      auto range = OmahaRange::parse(texts[player], &range_error);
      if (!range) {
        *error = names[player] + ": " + range_error;
        return std::nullopt;
      }
      config.omaha_ranges[player] = std::move(*range);
    } else {
      const auto range = Range::parse(texts[player], &range_error);
      if (!range) {
        *error = names[player] + ": " + range_error;
        return std::nullopt;
      }
      config.ranges[player] = *range;
    }
  }

  config.tree.pot = spot.pot();
  config.tree.effective_stack = spot.effective_stack();
  if (listed) {
    double effective = -1;
    for (const v1::PlayerSpot& player : spot.players()) {
      const double stack = player.stack() > 0 ? player.stack() : spot.effective_stack();
      effective = effective < 0 ? stack : std::min(effective, stack);
    }
    config.tree.effective_stack = effective;
    std::vector<std::string> positions;
    for (const v1::PlayerSpot& player : spot.players()) positions.push_back(player.position());
    if (!positions[0].empty() || !positions[1].empty()) {
      if (!check_positions(positions, error)) return std::nullopt;
    }
  }
  config.tree.pot_limit =
      spot.has_pot_limit() ? spot.pot_limit() : config.variant == Variant::Omaha4;
  config.rake_rate = spot.rake_pct() / 100.0;
  config.rake_cap = spot.rake_cap();

  const v1::TreeSettings& tree = spot.tree();
  const v1::StreetSizing* streets[3] = {&tree.flop(), &tree.turn(), &tree.river()};
  for (std::size_t street = 0; street < 3; ++street) {
    const v1::StreetSizing& sizing = *streets[street];
    // A player without sizes of their own takes `all`, then players[i].
    const v1::BetSizing* per_player[2] = {
        sizing.has_oop() ? &sizing.oop() : sizing.players_size() > 0 ? &sizing.players(0) : &sizing.all(),
        sizing.has_ip() ? &sizing.ip() : sizing.players_size() > 1 ? &sizing.players(1) : &sizing.all()};
    config.tree.sizing[street][kOop] = to_sizing(*per_player[kOop]);
    config.tree.sizing[street][kIp] = to_sizing(*per_player[kIp]);
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

  if (config.variant == Variant::Omaha4) {
    for (const auto& range : config.omaha_ranges) append_omaha(key, range, dead);
  } else {
    for (const Range& range : config.ranges) {
      for (int combo = 0; combo < Range::kNumCombos; ++combo) {
        const auto [a, b] = Range::combo_cards(combo);
        const bool blocked = ((a.mask() | b.mask()) & dead) != 0;
        append(key, blocked ? 0.0f : range.weight(combo));
      }
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
  append(key, static_cast<std::uint8_t>(config.variant));
  append(key, static_cast<std::uint8_t>(tree.pot_limit));
  return key;
}

std::vector<std::string> preflop_positions(std::size_t players) {
  std::vector<std::string> names;
  if (players < 2 || players > 9) return names;
  const int n = static_cast<int>(players);
  for (int p = 0; p < n; ++p) names.push_back(mc::seat_name(n, mc::Street::Preflop, p));
  return names;
}

bool valid_position(std::string_view name) {
  if (name.empty() || name.size() > 12) return false;
  return std::all_of(name.begin(), name.end(), [](char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
           c == '+' || c == '-' || c == '_';
  });
}

bool check_positions(const std::vector<std::string>& names, std::string* error) {
  std::set<std::string> seen;
  for (std::size_t i = 0; i < names.size(); ++i) {
    const std::string where = "players[" + std::to_string(i) + "].position: ";
    if (!valid_position(names[i])) {
      *error = where + "'" + names[i] +
               "' is not a position name (1 to 12 letters, digits, '+', '-' or '_')";
      return false;
    }
    if (!seen.insert(names[i]).second) {
      *error = where + "'" + names[i] + "' appears twice";
      return false;
    }
  }
  return true;
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
  if (settings.max_seconds() > 0) resolved.max_seconds = settings.max_seconds();
  resolved.mc_iterations =
      settings.mc_iterations() > 0 ? settings.mc_iterations() : kDefaultMcIterations;
  return resolved;
}

}  // namespace gto::server
