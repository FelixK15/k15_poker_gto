#include "mc_engine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <istream>
#include <limits>
#include <numeric>
#include <ostream>
#include <set>
#include <type_traits>

#include "convert.h"
#include "gto/hands.h"
#include "gto/mc/abstraction.h"
#include "gto/mc/preflop.h"
#include "gto/mc/spot.h"
#include "gto/omaha.h"
#include "gto/range.h"

namespace gto::server {

namespace {

Error invalid(std::string message) {
  return make_error(ErrorKind::InvalidArgument, std::move(message));
}

bool positive(double value) { return std::isfinite(value) && value > 0; }

// --- Reading the spot ----------------------------------------------------------

/// One player of a postflop spot, from Spot.players or the heads-up fields.
struct PlayerView {
  std::string field;  // for errors
  std::string range;
  double stack = 0;
  std::string position;
};

std::vector<PlayerView> players_of(const v1::Spot& spot) {
  std::vector<PlayerView> players;
  if (spot.players_size() == 0) {
    players.push_back({"oop_range", spot.oop_range(), spot.effective_stack(), ""});
    players.push_back({"ip_range", spot.ip_range(), spot.effective_stack(), ""});
    return players;
  }
  for (int i = 0; i < spot.players_size(); ++i) {
    const v1::PlayerSpot& player = spot.players(i);
    players.push_back({"players[" + std::to_string(i) + "].range", player.range(),
                       player.stack() > 0 ? player.stack() : spot.effective_stack(),
                       player.position()});
  }
  return players;
}

/// Weights of a range in the variant's combo numbering.
std::optional<std::vector<float>> parse_weights(const std::string& text, v1::Variant variant,
                                                std::string* error) {
  if (variant == v1::VARIANT_OMAHA4) {
    auto range = OmahaRange::parse(text, error);
    if (!range) return std::nullopt;
    return range->weights();
  }
  const auto range = Range::parse(text, error);
  if (!range) return std::nullopt;
  return std::vector<float>(range->weights().begin(), range->weights().end());
}

Error check_range(const std::string& field, const std::string& text, v1::Variant variant) {
  std::string error;
  const auto weights = parse_weights(text, variant, &error);
  if (!weights) return invalid(field + ": " + error);
  if (std::none_of(weights->begin(), weights->end(), [](float w) { return w > 0; })) {
    return invalid(field + ": the range is empty");
  }
  return {};
}

Error check_abstraction(const v1::Abstraction& abstraction) {
  const struct {
    const char* name;
    bool set;
    std::uint32_t value;
  } streets[] = {
      {"preflop_buckets", abstraction.has_preflop_buckets(), abstraction.preflop_buckets()},
      {"flop_buckets", abstraction.has_flop_buckets(), abstraction.flop_buckets()},
      {"turn_buckets", abstraction.has_turn_buckets(), abstraction.turn_buckets()},
      {"river_buckets", abstraction.has_river_buckets(), abstraction.river_buckets()},
  };
  for (const auto& street : streets) {
    if (street.set && street.value > 1'000'000) {
      return invalid(std::string("abstraction.") + street.name + ": at most 1,000,000");
    }
  }
  return {};
}

Error check_preflop(const v1::Spot& spot) {
  const v1::PreflopSettings& preflop = spot.preflop();
  if (!preflop.preset().empty()) {
    if (preset_players(preflop.preset()) == 0) {
      return invalid("preflop.preset: '" + preflop.preset() + "' is not hu, 6max or 9max");
    }
    if (!(preflop.stack_bb() > 2) || !std::isfinite(preflop.stack_bb())) {
      return invalid("preflop.stack_bb: a preset needs more than 2 big blinds");
    }
    return {};
  }
  if (preflop.players() < 2 || preflop.players() > 9) {
    return invalid("preflop.players: a table has 2 to 9 players");
  }
  if (!positive(preflop.stack_bb())) return invalid("preflop.stack_bb: must be positive");
  const double small_blind = preflop.has_small_blind() ? preflop.small_blind() : 0.5;
  const double big_blind = preflop.has_big_blind() ? preflop.big_blind() : 1.0;
  if (!positive(small_blind) || !positive(big_blind) || small_blind > big_blind) {
    return invalid("preflop: the blinds must be positive, the small blind at most the big one");
  }
  if (!(preflop.ante() >= 0) || !std::isfinite(preflop.ante())) {
    return invalid("preflop.ante: cannot be negative");
  }
  for (const double size : preflop.open_sizes_bb()) {
    if (!(size > big_blind) || !std::isfinite(size)) {
      return invalid("preflop.open_sizes_bb: an open must be larger than the big blind");
    }
  }
  for (const double size : preflop.sb_open_sizes_bb()) {
    if (!(size > big_blind) || !std::isfinite(size)) {
      return invalid("preflop.sb_open_sizes_bb: an open must be larger than the big blind");
    }
  }
  const struct {
    const char* name;
    bool set;
    double value;
  } multipliers[] = {
      {"three_bet_ip_multiplier", preflop.has_three_bet_ip_multiplier(),
       preflop.three_bet_ip_multiplier()},
      {"three_bet_oop_multiplier", preflop.has_three_bet_oop_multiplier(),
       preflop.three_bet_oop_multiplier()},
      {"four_bet_multiplier", preflop.has_four_bet_multiplier(), preflop.four_bet_multiplier()},
  };
  for (const auto& multiplier : multipliers) {
    if (multiplier.set && (!(multiplier.value > 1) || !std::isfinite(multiplier.value))) {
      return invalid(std::string("preflop.") + multiplier.name + ": must be larger than 1");
    }
  }
  if (preflop.has_raise_cap() && (preflop.raise_cap() < 1 || preflop.raise_cap() > 10)) {
    return invalid("preflop.raise_cap: 1 to 10");
  }
  return {};
}

Error check_postflop(const v1::Spot& spot) {
  const std::vector<PlayerView> players = players_of(spot);
  const int count = static_cast<int>(players.size());
  if (count < 2 || count > 9) return invalid("players: a spot has 2 to 9 players");

  const auto board = parse_cards(spot.board());
  if (!board) return invalid("board: cannot read the cards");
  if (board->size() < 3 || board->size() > 5) {
    return invalid("board: a postflop spot needs three to five cards");
  }
  std::set<int> distinct;
  for (const Card card : *board) distinct.insert(card.index());
  if (distinct.size() != board->size()) return invalid("board: a card appears twice");

  if (!positive(spot.pot())) return invalid("pot: must be positive");

  std::vector<std::string> names;
  bool named = false;
  for (int i = 0; i < count; ++i) {
    const PlayerView& player = players[static_cast<std::size_t>(i)];
    if (Error error = check_range(player.field, player.range, spot.variant())) return error;
    if (!positive(player.stack) ||
        (spot.players_size() > 0 && spot.players(i).stack() < 0)) {
      return invalid("players[" + std::to_string(i) +
                     "].stack: needs a positive stack (or Spot.effective_stack)");
    }
    names.push_back(player.position);
    named = named || !player.position.empty();
  }
  if (named) {
    std::string error;
    if (!check_positions(names, &error)) return invalid(error);
  }

  const v1::TreeSettings& tree = spot.tree();
  for (const v1::StreetSizing* street : {&tree.flop(), &tree.turn(), &tree.river()}) {
    if (street->players_size() > count) {
      return invalid("tree: sizes for more players than the spot has");
    }
  }
  return {};
}

// --- Building the mc game ----------------------------------------------------------

BetSizing to_sizing(const v1::BetSizing& sizing) {
  BetSizing result;
  for (const double pct : sizing.bet_pct()) result.bet.push_back(pct / 100.0);
  for (const double pct : sizing.raise_pct()) result.raise.push_back(pct / 100.0);
  result.add_allin = sizing.allin();
  return result;
}

/// Same rule as the exact engine heads-up: a player's own sizes (oop / ip
/// heads-up, players[p]), else `all`.
mc::StreetSizing to_street(const v1::StreetSizing& sizing, int players) {
  mc::StreetSizing result;
  result.sizing = to_sizing(sizing.all());
  result.per_player.resize(static_cast<std::size_t>(players));
  for (int p = 0; p < players; ++p) {
    const v1::BetSizing* own = nullptr;
    if (players == 2 && p == 0 && sizing.has_oop()) own = &sizing.oop();
    if (players == 2 && p == 1 && sizing.has_ip()) own = &sizing.ip();
    if (own == nullptr && p < sizing.players_size()) own = &sizing.players(p);
    if (own != nullptr) result.per_player[static_cast<std::size_t>(p)] = to_sizing(*own);
  }
  return result;
}

void apply_tree(const v1::TreeSettings& tree, int players,
                std::array<mc::StreetSizing, mc::kNumStreets>* sizing, int* raise_cap,
                double* allin_threshold) {
  (*sizing)[1] = to_street(tree.flop(), players);
  (*sizing)[2] = to_street(tree.turn(), players);
  (*sizing)[3] = to_street(tree.river(), players);
  if (tree.has_raise_cap()) *raise_cap = static_cast<int>(std::min<std::uint32_t>(tree.raise_cap(), 100));
  if (tree.has_allin_threshold_pct()) *allin_threshold = tree.allin_threshold_pct() / 100.0;
}

void apply_abstraction(const v1::Abstraction& abstraction, mc::AbstractionConfig* out) {
  if (abstraction.lossless()) {
    for (int s = 1; s < mc::kNumStreets; ++s) {
      out->streets[static_cast<std::size_t>(s)].kind = mc::AbstractionKind::Lossless;
    }
  }
  const std::pair<bool, std::uint32_t> buckets[] = {
      {false, 0},
      {abstraction.has_flop_buckets(), abstraction.flop_buckets()},
      {abstraction.has_turn_buckets(), abstraction.turn_buckets()},
      {abstraction.has_river_buckets(), abstraction.river_buckets()},
  };
  for (int s = 1; s < mc::kNumStreets; ++s) {
    const auto [set, count] = buckets[s];
    if (!set || count == 0) continue;
    mc::StreetAbstraction& street = out->streets[static_cast<std::size_t>(s)];
    street.kind = mc::AbstractionKind::Strength;
    const int n = static_cast<int>(count);
    if (s != 3 && n >= 8 && n % 4 == 0) {
      street.strength_bins = n / 4;
      street.potential_bins = 4;
    } else {
      street.strength_bins = n;
      street.potential_bins = 1;
    }
  }
  if (abstraction.has_preflop_buckets() && abstraction.preflop_buckets() > 0) {
    out->omaha_preflop_buckets = static_cast<int>(abstraction.preflop_buckets());
  }
}

/// The engine's preflop settings for a spot.
std::optional<mc::PreflopSettings> preflop_settings(const v1::Spot& spot, std::string* error) {
  const v1::PreflopSettings& pf = spot.preflop();
  mc::PreflopSettings settings;
  if (!pf.preset().empty()) {
    auto preset = mc::preflop_preset(pf.preset(), pf.stack_bb(), error);
    if (!preset) return std::nullopt;
    settings = std::move(preset->settings);
  } else {
    settings.players = static_cast<int>(pf.players());
    settings.small_blind = pf.has_small_blind() ? pf.small_blind() : 0.5;
    settings.big_blind = pf.has_big_blind() ? pf.big_blind() : 1.0;
    settings.ante = pf.ante();
    settings.bb_ante = pf.bb_ante();
    settings.open_sizes_bb.assign(pf.open_sizes_bb().begin(), pf.open_sizes_bb().end());
    for (const v1::PositionSizes& sizes : pf.open_sizes_by_position()) {
      settings.open_sizes_by_position[sizes.position()] =
          std::vector<double>(sizes.sizes_bb().begin(), sizes.sizes_bb().end());
    }
    settings.sb_open_sizes_bb.assign(pf.sb_open_sizes_bb().begin(), pf.sb_open_sizes_bb().end());
    settings.allow_limp = pf.allow_limp();
    settings.allow_sb_complete = pf.allow_sb_complete();
    if (pf.has_three_bet_ip_multiplier()) {
      settings.three_bet_ip_multiplier = pf.three_bet_ip_multiplier();
    }
    if (pf.has_three_bet_oop_multiplier()) {
      settings.three_bet_oop_multiplier = pf.three_bet_oop_multiplier();
    }
    if (pf.has_four_bet_multiplier()) settings.four_bet_multiplier = pf.four_bet_multiplier();
    settings.allow_cold_calls = pf.allow_cold_calls();
    settings.allow_cold_call_3bet = pf.allow_cold_call_3bet();
    settings.allow_cold_call_4bet = pf.allow_cold_call_4bet();
    if (pf.has_squeeze_per_caller()) settings.squeeze_per_caller = pf.squeeze_per_caller();
    if (pf.has_raise_cap()) settings.raise_cap = static_cast<int>(pf.raise_cap());
    settings.solve_postflop = pf.solve_postflop();
    if (settings.solve_postflop) {
      settings.flop_sizing = to_street(spot.tree().flop(), settings.players);
      if (spot.tree().has_raise_cap()) {
        settings.flop_raise_cap = static_cast<int>(std::min<std::uint32_t>(spot.tree().raise_cap(), 100));
      }
    }
  }
  settings.stack_bb = pf.stack_bb();
  settings.rake_rate = spot.rake_pct() / 100.0;
  settings.rake_cap = spot.rake_cap();
  return settings;
}

std::optional<mc::GameConfig> preflop_config(const v1::Spot& spot, const EngineLimits& limits,
                                             std::string* error) {
  const auto settings = preflop_settings(spot, error);
  if (!settings) return std::nullopt;
  auto config = mc::make_preflop_config(*settings, error);
  if (!config) return std::nullopt;
  if (spot.variant() == v1::VARIANT_OMAHA4) {
    config->variant = Variant::Omaha4;
    config->ranges.clear();
  }
  if (spot.has_pot_limit() || spot.variant() == v1::VARIANT_OMAHA4) {
    config->pot_limit = spot.has_pot_limit() ? spot.pot_limit() : true;
  }
  apply_abstraction(spot.abstraction(), &config->abstraction);
  config->max_nodes = static_cast<std::size_t>(limits.max_nodes);
  if (!config->validate(error)) return std::nullopt;
  return config;
}

std::optional<mc::GameConfig> postflop_config(const v1::Spot& spot, const EngineLimits& limits,
                                              std::string* error) {
  const std::vector<PlayerView> players = players_of(spot);
  mc::MultiwaySpot mw;
  mw.variant = spot.variant() == v1::VARIANT_OMAHA4 ? Variant::Omaha4 : Variant::Holdem;
  mw.board = *parse_cards(spot.board());
  for (const PlayerView& player : players) {
    std::string range_error;
    if (mw.variant == Variant::Omaha4) {
      mw.omaha_ranges.push_back(*OmahaRange::parse(player.range, &range_error));
    } else {
      mw.ranges.push_back(*Range::parse(player.range, &range_error));
    }
    mw.stacks.push_back(player.stack);
  }
  mw.pot = spot.pot();
  apply_tree(spot.tree(), static_cast<int>(players.size()), &mw.sizing, &mw.raise_cap,
             &mw.allin_threshold);
  mw.pot_limit = spot.has_pot_limit() ? spot.pot_limit() : mw.variant == Variant::Omaha4;
  mw.rake.rate = spot.rake_pct() / 100.0;
  mw.rake.cap = spot.rake_cap();
  apply_abstraction(spot.abstraction(), &mw.abstraction);
  mw.max_nodes = static_cast<std::size_t>(limits.max_nodes);
  return mc::make_game_config(mw, error);
}

std::vector<std::string> positions_of(const v1::Spot& spot, const mc::GameConfig& config) {
  if (spot.has_preflop()) return preflop_positions(static_cast<std::size_t>(config.num_players));
  return spot_positions(spot);
}

// --- Keys ------------------------------------------------------------------------------

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

void append(std::string& key, const BetSizing& sizing) {
  append(key, sizing.bet);
  append(key, sizing.raise);
  append(key, static_cast<std::uint8_t>(sizing.add_allin));
}

/// 128-bit hash of a range's weights, blocked combos left out.
void append_range(std::string& key, const mc::GameConfig& config, int player, std::uint64_t dead) {
  std::uint64_t fnv = 0xcbf29ce484222325ull;
  std::uint64_t mix = 0x9e3779b97f4a7c15ull;
  for (int combo = 0; combo < config.num_combos(); ++combo) {
    float weight = config.weight(player, combo);
    if (weight == 0.0f) continue;
    if ((mc::decode_combo(config, combo).mask & dead) != 0) continue;
    std::uint32_t bits = 0;
    std::memcpy(&bits, &weight, sizeof(bits));
    const std::uint64_t item = (static_cast<std::uint64_t>(combo) << 32) | bits;
    for (int byte = 0; byte < 8; ++byte) {
      fnv ^= (item >> (8 * byte)) & 0xff;
      fnv *= 0x100000001b3ull;
    }
    std::uint64_t z = item + 0x9e3779b97f4a7c15ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    z ^= z >> 31;
    mix = (mix ^ z) * 0xff51afd7ed558ccdull + 0x2545f4914f6cdd1dull;
  }
  append(key, fnv);
  append(key, mix);
}

std::string game_key(const mc::GameConfig& config) {
  std::string key;
  append(key, static_cast<std::uint8_t>(config.variant));
  append(key, static_cast<std::int32_t>(config.num_players));
  append(key, static_cast<std::uint8_t>(config.start_street));
  append(key, static_cast<std::uint8_t>(config.last_street));
  std::vector<Card> board = config.board;
  const std::size_t flop = std::min<std::size_t>(board.size(), 3);
  std::sort(board.begin(), board.begin() + static_cast<std::ptrdiff_t>(flop));
  std::uint64_t dead = 0;
  append(key, static_cast<std::uint8_t>(board.size()));
  for (const Card card : board) {
    append(key, card.index());
    dead |= card.mask();
  }
  append(key, config.stacks);
  std::vector<double> committed;
  for (int p = 0; p < config.num_players; ++p) committed.push_back(config.committed_of(p));
  append(key, committed);
  append(key, config.dead_pot);
  append(key, config.big_blind);
  for (int p = 0; p < config.num_players; ++p) append_range(key, config, p, dead);
  for (int s = 1; s < mc::kNumStreets; ++s) {
    const mc::StreetSizing& sizing = config.sizing[static_cast<std::size_t>(s)];
    for (int p = 0; p < config.num_players; ++p) append(key, sizing.for_player(p));
  }
  const mc::PreflopSizing& pf = config.preflop;
  append(key, pf.open_bb);
  append(key, pf.sb_open_bb);
  append(key, pf.iso_per_limper_bb);
  append(key, pf.threebet_ip);
  append(key, pf.threebet_oop);
  append(key, pf.fourbet);
  append(key, pf.per_caller);
  for (const bool flag : {pf.allow_limp, pf.allow_sb_complete, pf.allin_every_level,
                          pf.cold_call_open, pf.cold_call_3bet, pf.cold_call_4bet}) {
    append(key, static_cast<std::uint8_t>(flag));
  }
  append(key, static_cast<std::int32_t>(pf.raise_cap));
  append(key, static_cast<std::int32_t>(config.raise_cap));
  append(key, config.allin_threshold);
  append(key, static_cast<std::uint8_t>(config.pot_limit));
  append(key, config.rake.rate);
  append(key, config.rake.cap);
  append(key, static_cast<std::uint8_t>(config.rake.no_flop_no_drop));
  const mc::AbstractionConfig& abstraction = config.abstraction;
  for (const mc::StreetAbstraction& street : abstraction.streets) {
    append(key, static_cast<std::uint8_t>(street.kind));
    append(key, static_cast<std::int32_t>(street.strength_bins));
    append(key, static_cast<std::int32_t>(street.potential_bins));
  }
  append(key, static_cast<std::int32_t>(abstraction.omaha_preflop_buckets));
  append(key, abstraction.lossless_limit);
  append(key, static_cast<std::int32_t>(abstraction.omaha_strength_samples));
  append(key, static_cast<std::int32_t>(abstraction.omaha_potential_cards));
  return key;
}

// --- Reports ---------------------------------------------------------------------------

v1::ActionType to_proto(ActionType type) {
  switch (type) {
    case ActionType::Fold: return v1::ACTION_TYPE_FOLD;
    case ActionType::Check: return v1::ACTION_TYPE_CHECK;
    case ActionType::Call: return v1::ACTION_TYPE_CALL;
    case ActionType::Bet: return v1::ACTION_TYPE_BET;
    case ActionType::Raise: return v1::ACTION_TYPE_RAISE;
    case ActionType::AllIn: return v1::ACTION_TYPE_ALLIN;
  }
  return v1::ACTION_TYPE_UNSPECIFIED;
}

v1::NodeType to_proto(mc::NodeKind kind) {
  switch (kind) {
    case mc::NodeKind::Action: return v1::NODE_TYPE_ACTION;
    case mc::NodeKind::Chance: return v1::NODE_TYPE_CHANCE;
    case mc::NodeKind::FoldOut: return v1::NODE_TYPE_FOLD;
    case mc::NodeKind::Showdown: return v1::NODE_TYPE_SHOWDOWN;
  }
  return v1::NODE_TYPE_UNSPECIFIED;
}

/// The node's actions with how often the actor's whole range takes each.
template <typename AddAction>
void fill_actions(const mc::NodeReport& report, AddAction add_action) {
  if (report.kind != mc::NodeKind::Action) return;
  const auto& reach = report.reach[static_cast<std::size_t>(report.player)];
  double total = 0;
  for (const float value : reach) total += value;
  for (std::size_t a = 0; a < report.actions.size(); ++a) {
    v1::NodeAction* proto = add_action();
    proto->set_type(to_proto(report.actions[a].type));
    proto->set_amount(report.actions[a].amount);
    proto->set_label(report.labels[a]);
    double weighted = 0;
    for (int combo = 0; combo < report.num_combos; ++combo) {
      const float r = reach[static_cast<std::size_t>(combo)];
      if (r > 0) weighted += static_cast<double>(r) * report.frequency(a, combo);
    }
    proto->set_frequency(total > 0 ? weighted / total : 0.0);
  }
}

double finite_or_zero(double value) { return std::isfinite(value) ? value : 0.0; }

std::string combo_name(Variant variant, int combo) {
  if (variant == Variant::Omaha4) return omaha_hand_name(omaha_combo_cards(combo));
  const auto [a, b] = Range::combo_cards(combo);
  return holdem_hand_name(make_hand({a.index(), b.index()}, 1.0f));
}

/// The 169 classes of one player at a Hold'em node.
void fill_classes(const mc::NodeReport& report, std::size_t player, bool actor,
                  std::uint64_t dead, v1::PlayerRange* out) {
  constexpr int kClasses = 169;
  const std::size_t actions = actor ? report.actions.size() : 0;
  std::array<int, kClasses> possible{};
  std::vector<double> reach(kClasses, 0.0), ev(kClasses, 0.0), ev_reach(kClasses, 0.0);
  std::vector<double> strategy(kClasses * actions, 0.0);
  const auto& player_reach = report.reach[player];
  const bool have_ev = player < report.ev.size() && !report.ev[player].empty();
  for (int combo = 0; combo < Range::kNumCombos; ++combo) {
    const auto [a, b] = Range::combo_cards(combo);
    if (((a.mask() | b.mask()) & dead) != 0) continue;
    const auto cls = static_cast<std::size_t>(mc::holdem_class(a, b));
    ++possible[cls];
    const double r = player_reach[static_cast<std::size_t>(combo)];
    if (r <= 0) continue;
    reach[cls] += r;
    if (have_ev) {
      const double value = report.ev[player][static_cast<std::size_t>(combo)];
      if (std::isfinite(value)) {
        ev[cls] += r * value;
        ev_reach[cls] += r;
      }
    }
    for (std::size_t act = 0; act < actions; ++act) {
      strategy[cls * actions + act] += r * report.frequency(act, combo);
    }
  }
  for (std::size_t cls = 0; cls < kClasses; ++cls) {
    v1::HandClass* proto = out->add_classes();
    proto->set_name(mc::holdem_class_name(static_cast<int>(cls)));
    proto->set_weight(possible[cls] > 0 ? static_cast<float>(reach[cls] / possible[cls]) : 0.0f);
    proto->set_combos(static_cast<float>(reach[cls]));
    if (ev_reach[cls] > 0) proto->set_ev(static_cast<float>(ev[cls] / ev_reach[cls]));
    for (std::size_t act = 0; act < actions; ++act) {
      proto->add_strategy(reach[cls] > 0 ? static_cast<float>(strategy[cls * actions + act] / reach[cls])
                                         : 0.0f);
    }
  }
}

std::string board_text(const std::vector<Card>& board) {
  std::string text;
  for (const Card card : board) {
    if (!text.empty()) text += ' ';
    text += card.to_string();
  }
  return text;
}

/// Splits a path into its '/' segments.
std::vector<std::string_view> segments(std::string_view path) {
  std::vector<std::string_view> parts;
  while (!path.empty()) {
    const std::size_t slash = path.find('/');
    parts.push_back(path.substr(0, slash));
    if (slash == std::string_view::npos) break;
    path.remove_prefix(slash + 1);
  }
  return parts;
}

}  // namespace

// --- Validation, estimate, key ----------------------------------------------------------

Error McEngine::validate(const v1::Spot& spot) {
  if (spot.variant() != v1::VARIANT_HOLDEM && spot.variant() != v1::VARIANT_OMAHA4) {
    return invalid("variant: unknown value " + std::to_string(static_cast<int>(spot.variant())));
  }
  if (spot.rake_pct() < 0 || spot.rake_pct() > 100 || spot.rake_cap() < 0) {
    return invalid("rake: percent 0 to 100, cap not negative");
  }
  if (Error error = check_abstraction(spot.abstraction())) return error;
  if (spot.has_preflop()) return check_preflop(spot);
  return check_postflop(spot);
}

std::optional<mc::GameConfig> McEngine::game_config(const v1::Spot& spot,
                                                    const EngineLimits& limits,
                                                    std::string* error) {
  if (Error invalid_spot = validate(spot)) {
    *error = invalid_spot.message;
    return std::nullopt;
  }
  return spot.has_preflop() ? preflop_config(spot, limits, error)
                            : postflop_config(spot, limits, error);
}

Error McEngine::estimate(const v1::Spot& spot, const EngineLimits& limits,
                         v1::EstimateTreeResponse* out) {
  std::string error;
  const auto config = game_config(spot, limits, &error);
  if (!config) return invalid(error);
  const auto estimate = mc::estimate_game(*config, &error);
  if (!estimate) return invalid(error);

  out->set_nodes(estimate->tree.nodes);
  out->set_decision_nodes(estimate->tree.action_nodes);
  for (const std::size_t hands : estimate->live_hands) {
    out->add_player_hands(static_cast<std::uint32_t>(hands));
  }
  if (estimate->live_hands.size() == 2) {
    out->set_oop_hands(static_cast<std::uint32_t>(estimate->live_hands[0]));
    out->set_ip_hands(static_cast<std::uint32_t>(estimate->live_hands[1]));
  }
  out->set_info_sets(estimate->info_sets);
  out->set_strategy_entries(estimate->table_entries);
  out->set_strategy_bytes(estimate->storage_bytes);
  out->set_tree_bytes(estimate->tree_bytes + estimate->bucket_bytes + estimate->hands_bytes);
  out->set_too_large(estimate->tree.too_large);
  out->set_memory_budget_bytes(limits.memory_budget_bytes);
  out->set_can_solve(!estimate->tree.too_large &&
                     (limits.memory_budget_bytes == 0 ||
                      estimate->total_bytes() <= limits.memory_budget_bytes));
  out->set_engine(v1::ENGINE_MONTE_CARLO);
  out->set_exploitability_kind(config->num_players > 2
                                   ? v1::EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY
                                   : v1::EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER);
  out->set_player_count(static_cast<std::uint32_t>(config->num_players));
  for (const std::string& position : positions_of(spot, *config)) out->add_positions(position);
  for (int s = 0; s < mc::kNumStreets; ++s) {
    const mc::StreetPlan& plan = estimate->abstraction[static_cast<std::size_t>(s)];
    if (plan.description.empty()) continue;
    out->add_abstraction(std::string(mc::street_name(static_cast<mc::Street>(s))) + ": " +
                         plan.description);
  }
  return {};
}

std::string McEngine::key(const v1::Spot& spot, std::string* error) {
  EngineLimits limits;
  const auto config = game_config(spot, limits, error);
  if (!config) return {};
  return game_key(*config);
}

std::unique_ptr<McEngine> McEngine::create(const v1::Spot& spot, const EngineLimits& limits,
                                           std::string* error) {
  const auto config = game_config(spot, limits, error);
  if (!config) return nullptr;
  mc::SolverConfig solver_config;
  solver_config.threads = limits.threads;
  solver_config.seed = 1;
  auto solver = mc::Solver::create(*config, solver_config, error);
  if (!solver) return nullptr;

  std::unique_ptr<McEngine> engine(new McEngine());
  engine->solver_ = std::move(solver);
  engine->spot_ = spot;
  engine->positions_ = positions_of(spot, *config);
  engine->preflop_ = spot.has_preflop();
  engine->posted_.assign(static_cast<std::size_t>(config->num_players), 0.0);
  if (engine->preflop_) {
    // Blinds are in `committed`; antes came off the stacks into the dead pot.
    const auto settings = preflop_settings(spot, error);
    const double stack = settings ? settings->stack_bb : 0.0;
    for (int p = 0; p < config->num_players; ++p) {
      const auto seat = static_cast<std::size_t>(p);
      const double ante = seat < config->stacks.size() ? stack - config->stacks[seat] : 0.0;
      engine->posted_[seat] = config->committed_of(p) + std::max(0.0, ante);
    }
  }
  return engine;
}

// --- Solving ---------------------------------------------------------------------------

void McEngine::run_chunk(std::uint64_t budget) {
  const std::uint64_t done = solver_->iterations();
  if (done >= budget) return;
  const std::uint64_t count = std::min(chunk_, budget - done);
  const auto started = std::chrono::steady_clock::now();
  solver_->run(count);
  const double seconds =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
  if (count == chunk_ && seconds > 0) {
    const double next = static_cast<double>(count) * 0.25 / seconds;
    chunk_ = static_cast<std::uint64_t>(std::clamp(next, 1000.0, 100'000'000.0));
  }
}

bool McEngine::measurement_due(int) const {
  const double since =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - last_measured_).count();
  return since >= std::max(3.0, 4.0 * last_evaluation_seconds_);
}

EngineEvaluation McEngine::evaluate() const {
  const auto started = std::chrono::steady_clock::now();
  mc::EvalOptions options;
  options.ev_samples = 200'000;
  options.best_response = true;
  options.br_samples_cap = 1'000'000;
  const mc::Evaluation evaluation = solver_->evaluate(options);
  last_measured_ = std::chrono::steady_clock::now();
  last_evaluation_seconds_ = std::chrono::duration<double>(last_measured_ - started).count();

  EngineEvaluation result;
  result.exploitability_pct = evaluation.exploitability_pct;
  result.exploitability_kind = positions_.size() > 2
                                   ? v1::EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY
                                   : v1::EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER;
  result.estimated = true;
  for (std::size_t p = 0; p < evaluation.ev.size(); ++p) {
    result.evs.push_back(evaluation.ev[p] - (p < posted_.size() ? posted_[p] : 0.0));
  }
  return result;
}

// --- Browsing --------------------------------------------------------------------------

std::optional<std::string> McEngine::expand_path(std::string_view path,
                                                 std::string* error) const {
  if (path.find(':') == std::string_view::npos) return std::string(path);
  if (preflop_) {
    const auto resolved = mc::resolve_preflop_path(solver_->tree(), path, error);
    if (!resolved) return std::nullopt;
    return resolved->engine_path;
  }
  const mc::BettingTree& tree = solver_->tree();
  std::uint32_t node = mc::BettingTree::kRoot;
  std::string out;
  const auto add = [&](std::string_view segment) {
    if (!out.empty()) out += '/';
    out += segment;
  };
  for (std::string_view segment : segments(path)) {
    const std::size_t colon = segment.find(':');
    if (colon != std::string_view::npos) {
      const std::string_view position = segment.substr(0, colon);
      const auto it = std::find(positions_.begin(), positions_.end(), position);
      if (it == positions_.end()) {
        *error = "'" + std::string(position) + "' is not a position of this spot";
        return std::nullopt;
      }
      const int seat = static_cast<int>(it - positions_.begin());
      while (tree.node(node).kind == mc::NodeKind::Action && tree.node(node).player != seat) {
        const auto fold = tree.child(node, "fold");
        if (!fold) {
          *error = std::string(positions_[tree.node(node).player]) + " cannot fold before " +
                   std::string(position) + " acts";
          return std::nullopt;
        }
        add("fold");
        node = *fold;
      }
      if (tree.node(node).kind != mc::NodeKind::Action) {
        *error = std::string(position) + " does not get to act here";
        return std::nullopt;
      }
      segment = segment.substr(colon + 1);
    }
    const mc::TreeNode& current = tree.node(node);
    if (current.kind == mc::NodeKind::Chance) {
      add(segment);  // cards; checked by the report
      node = tree.children(node)[0];
    } else if (current.kind == mc::NodeKind::Action) {
      const auto next = tree.child(node, segment);
      if (!next) {
        *error = "no action '" + std::string(segment) + "' here";
        return std::nullopt;
      }
      add(segment);
      node = *next;
    } else {
      *error = "the hand is over before '" + std::string(segment) + "'";
      return std::nullopt;
    }
  }
  return out;
}

Error McEngine::node(std::string_view path, const NodeOptions& options,
                     v1::GetNodeResponse* out) const {
  const Variant variant = solver_->config().variant;
  const bool holdem = variant == Variant::Holdem;
  if (!holdem && options.detail != v1::HAND_DETAIL_HANDS) {
    return invalid("hand classes exist for Hold'em only");
  }
  std::string error;
  const auto full = expand_path(path, &error);
  if (!full) return make_error(ErrorKind::NotFound, "path '" + std::string(path) + "': " + error);
  mc::ReportOptions report_options;
  report_options.ev_samples = 100;
  report_options.max_ev_combos = 5000;
  const auto report = solver_->report(*full, report_options, &error);
  if (!report) return make_error(ErrorKind::NotFound, "path '" + std::string(path) + "': " + error);

  const bool acting = report->kind == mc::NodeKind::Action;
  const std::size_t players = positions_.size();
  out->set_type(to_proto(report->kind));
  out->set_acting_player(static_cast<std::uint32_t>(report->player));
  out->set_player(report->player == 0 ? v1::PLAYER_OOP : v1::PLAYER_IP);
  out->set_path(std::string(path));
  out->set_iteration(static_cast<std::uint32_t>(
      std::min<std::uint64_t>(solver_->iterations(), std::numeric_limits<std::uint32_t>::max())));
  out->set_pot(report->pot);
  out->set_board(board_text(report->board));
  out->set_ev_estimated(true);

  std::uint64_t dead = 0;
  for (const Card card : report->board) dead |= card.mask();
  const bool list_hands = options.detail != v1::HAND_DETAIL_CLASSES;
  const bool list_classes = options.detail != v1::HAND_DETAIL_HANDS;
  const std::size_t limit =
      hand_limit(options.max_hands, holdem ? v1::VARIANT_HOLDEM : v1::VARIANT_OMAHA4);
  std::vector<int> actor_combos;

  for (std::size_t p = 0; p < players; ++p) {
    out->add_committed(report->committed[p]);
    out->add_folded(report->folded[p]);
    out->add_all_in(report->allin[p]);

    v1::PlayerRange* range = out->add_players();
    range->set_position(positions_[p]);
    const auto& live = solver_->hands(static_cast<int>(p)).hands;
    const auto& reach = report->reach[p];
    range->set_total_hands(static_cast<std::uint32_t>(live.size()));
    range->set_range_ev(p < report->range_ev.size() ? finite_or_zero(report->range_ev[p]) : 0.0);
    const bool actor = acting && static_cast<std::size_t>(report->player) == p;

    if (list_hands) {
      // The live hands, or those with the most reach here, in combo order.
      std::vector<int> picked;
      picked.reserve(live.size());
      for (const mc::HoleHand& hand : live) picked.push_back(hand.combo);
      double total = 0;
      for (const int combo : picked) total += reach[static_cast<std::size_t>(combo)];
      if (picked.size() > limit) {
        const auto more_reach = [&](int x, int y) {
          const float rx = reach[static_cast<std::size_t>(x)];
          const float ry = reach[static_cast<std::size_t>(y)];
          return rx != ry ? rx > ry : x < y;
        };
        std::nth_element(picked.begin(), picked.begin() + static_cast<std::ptrdiff_t>(limit),
                         picked.end(), more_reach);
        picked.resize(limit);
      }
      std::sort(picked.begin(), picked.end());
      const bool have_ev = p < report->ev.size() && !report->ev[p].empty();
      double shown = 0;
      for (const int combo : picked) {
        const auto c = static_cast<std::size_t>(combo);
        range->add_hands(combo_name(variant, combo));
        range->add_reach(reach[c]);
        if (have_ev) range->add_ev(static_cast<float>(finite_or_zero(report->ev[p][c])));
        shown += reach[c];
      }
      range->set_shown_reach_fraction(total > 0 ? shown / total : 1.0);
      if (actor) actor_combos = std::move(picked);
    }
    if (list_classes) fill_classes(*report, p, actor, dead, range);
  }
  if (players == 2) {
    *out->mutable_oop() = out->players(0);
    *out->mutable_ip() = out->players(1);
    out->set_oop_committed(report->committed[0]);
    out->set_ip_committed(report->committed[1]);
  }

  if (acting) {
    fill_actions(*report, [&] { return out->add_actions(); });
    for (std::size_t a = 0; a < report->actions.size(); ++a) {
      for (const int combo : actor_combos) out->add_strategy(report->frequency(a, combo));
    }
  }
  if (report->kind == mc::NodeKind::Chance) {
    for (int card = 0; card < Card::kDeckSize; ++card) {
      const Card next(static_cast<std::uint8_t>(card));
      if ((next.mask() & dead) == 0) out->add_next_cards(next.to_string());
    }
  }
  return {};
}

Error McEngine::runouts(std::string_view path, v1::GetRunoutReportResponse* out) const {
  std::string error;
  const auto full = expand_path(path, &error);
  if (!full) return make_error(ErrorKind::NotFound, "path '" + std::string(path) + "': " + error);
  const mc::BettingTree& tree = solver_->tree();
  const auto found = tree.find(*full, solver_->config().board, &error);
  if (!found) return make_error(ErrorKind::NotFound, "path '" + std::string(path) + "': " + error);
  if (tree.node(found->node).kind != mc::NodeKind::Chance) {
    return invalid("no card is dealt at '" + std::string(path) + "'");
  }
  const std::uint32_t next = tree.children(found->node)[0];
  if (tree.node(next).street == mc::Street::Flop) {
    return invalid("runout reports cover single turn and river cards; a flop is dealt as three "
                   "cards in one path entry");
  }
  std::uint64_t dead = 0;
  for (const Card card : solver_->config().board) dead |= card.mask();
  for (const Card card : found->dealt) dead |= card.mask();

  mc::ReportOptions options;
  options.ev_samples = 30;
  options.max_ev_combos = 2000;
  const std::size_t players = positions_.size();
  for (int index = 0; index < Card::kDeckSize; ++index) {
    const Card card(static_cast<std::uint8_t>(index));
    if ((card.mask() & dead) != 0) continue;
    const std::string child = full->empty() ? card.to_string() : *full + "/" + card.to_string();
    const auto report = solver_->report(child, options, &error);
    if (!report) return make_error(ErrorKind::Internal, error);
    v1::Runout* runout = out->add_runouts();
    runout->set_card(card.to_string());
    for (std::size_t p = 0; p < players; ++p) {
      runout->add_evs(p < report->range_ev.size() ? finite_or_zero(report->range_ev[p]) : 0.0);
      runout->add_equities(0.0);
    }
    if (players == 2) {
      runout->set_oop_ev(runout->evs(0));
      runout->set_ip_ev(runout->evs(1));
    }
    runout->set_type(to_proto(report->kind));
    runout->set_player(report->player == 0 ? v1::PLAYER_OOP : v1::PLAYER_IP);
    runout->set_acting_player(static_cast<std::uint32_t>(report->player));
    fill_actions(*report, [&] { return runout->add_actions(); });
  }
  return {};
}

namespace {

v1::PreflopScenarioKind to_proto(mc::PreflopScenarioKind kind) {
  switch (kind) {
    case mc::PreflopScenarioKind::Limped: return v1::PREFLOP_SCENARIO_KIND_LIMPED;
    case mc::PreflopScenarioKind::SingleRaised: return v1::PREFLOP_SCENARIO_KIND_SINGLE_RAISED;
    case mc::PreflopScenarioKind::ThreeBet: return v1::PREFLOP_SCENARIO_KIND_THREE_BET;
    case mc::PreflopScenarioKind::FourBet: return v1::PREFLOP_SCENARIO_KIND_FOUR_BET;
    case mc::PreflopScenarioKind::FiveBetPlus: return v1::PREFLOP_SCENARIO_KIND_FIVE_BET_PLUS;
  }
  return v1::PREFLOP_SCENARIO_KIND_UNSPECIFIED;
}

}  // namespace

Error McEngine::preflop_scenarios(std::uint32_t max_raises,
                                  v1::ListPreflopScenariosResponse* out) const {
  if (!preflop_) return SolveEngine::preflop_scenarios(max_raises, out);
  if (solver_->config().variant != Variant::Holdem) {
    return make_error(ErrorKind::FailedPrecondition, "preflop scenarios exist for Hold'em only");
  }
  // The API counts pot depth (2 = single-raised), the engine raises.
  const int raises =
      static_cast<int>(std::min<std::uint32_t>(std::max(max_raises, 2u), 11u)) - 1;
  for (const mc::PreflopScenario& scenario : mc::list_preflop_scenarios(*solver_, raises)) {
    v1::PreflopScenario* proto = out->add_scenarios();
    proto->set_name(scenario.name);
    proto->set_path(scenario.path.short_path);
    proto->set_kind(to_proto(scenario.kind));
    for (const std::string& position : scenario.positions) proto->add_positions(position);
    proto->set_aggressor(scenario.aggressor_position);
    proto->set_pot(scenario.pot);
    proto->set_effective_stack(scenario.effective_stack);
    proto->set_frequency(scenario.frequency);
  }
  return {};
}

Error McEngine::preflop_ranges(std::string_view path, v1::GetPreflopRangesResponse* out) const {
  if (!preflop_) return SolveEngine::preflop_ranges(path, out);
  if (solver_->config().variant != Variant::Holdem) {
    return make_error(ErrorKind::FailedPrecondition, "preflop ranges exist for Hold'em only");
  }
  std::string error;
  const auto ranges = mc::preflop_ranges(*solver_, path, &error);
  if (!ranges) return make_error(ErrorKind::NotFound, "path '" + std::string(path) + "': " + error);

  out->set_path(ranges->path.full_path);
  out->set_complete(ranges->complete);
  out->set_pot(ranges->pot);
  out->set_effective_stack(ranges->effective_stack);
  out->set_iteration(static_cast<std::uint32_t>(
      std::min<std::uint64_t>(solver_->iterations(), std::numeric_limits<std::uint32_t>::max())));

  v1::Spot& spot = *out->mutable_spot();
  spot.set_variant(v1::VARIANT_HOLDEM);
  spot.set_pot(ranges->pot);
  spot.set_effective_stack(ranges->effective_stack);
  spot.set_rake_pct(spot_.rake_pct());
  spot.set_rake_cap(spot_.rake_cap());
  const bool heads_up = ranges->players.size() == 2;
  for (std::size_t i = 0; i < ranges->players.size(); ++i) {
    const mc::PreflopPlayerRange& player = ranges->players[i];
    v1::PreflopRange* proto = out->add_players();
    proto->set_position(player.position);
    const std::string text = player.range.to_string();
    proto->set_range(text);
    proto->set_committed(player.committed);
    proto->set_stack(player.stack);
    for (int cls = 0; cls < 169; ++cls) {
      v1::HandClass* c = proto->add_classes();
      c->set_name(mc::holdem_class_name(cls));
      const float weight = player.class_weight[static_cast<std::size_t>(cls)];
      c->set_weight(weight);
      c->set_combos(weight * static_cast<float>(mc::holdem_class_combos(cls)));
    }
    if (heads_up) {
      (i == 0 ? spot.mutable_oop_range() : spot.mutable_ip_range())->assign(text);
    } else {
      v1::PlayerSpot* seat = spot.add_players();
      seat->set_position(player.position);
      seat->set_range(text);
      seat->set_stack(player.stack);
    }
  }
  return {};
}

// --- State -----------------------------------------------------------------------------

bool McEngine::save(std::ostream& out) const {
  solver_->save(out);
  return static_cast<bool>(out);
}

bool McEngine::load(std::istream& in, std::string* error) { return solver_->load(in, error); }

std::uint64_t McEngine::memory_bytes() const noexcept { return solver_->memory_bytes(); }

}  // namespace gto::server
