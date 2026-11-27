// Preflop presets, positional paths, scenarios and the range library (see
// gto/mc/preflop.h).

#include "gto/mc/preflop.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <istream>
#include <ostream>
#include <sstream>

#include "gto/mc/abstraction.h"

namespace gto::mc {

namespace {

constexpr double kEps = 1e-9;

std::string lower(std::string_view s) {
  std::string out(s);
  for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

std::string upper(std::string_view s) {
  std::string out(s);
  for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return out;
}

std::string_view trim(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
  return s;
}

std::vector<std::string_view> split(std::string_view s, char sep) {
  std::vector<std::string_view> parts;
  std::size_t start = 0;
  while (true) {
    const std::size_t at = s.find(sep, start);
    parts.push_back(s.substr(start, at == std::string_view::npos ? std::string_view::npos : at - start));
    if (at == std::string_view::npos) break;
    start = at + 1;
  }
  return parts;
}

/// "2.5", "100", "0.0612": short decimal form.
std::string fmt(double value, int digits = 6) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%.*g", digits, value);
  return buffer;
}

std::string amount(double value) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%.2f", value);
  std::string text = buffer;
  while (text.back() == '0') text.pop_back();
  if (text.back() == '.') text.pop_back();
  return text;
}

bool ends_preflop(const BettingTree& tree, std::uint32_t id) {
  const TreeNode& node = tree.node(id);
  if (node.street != Street::Preflop) return true;
  return node.kind == NodeKind::Chance || node.kind == NodeKind::Showdown;
}

int count_bits(std::uint16_t mask, int n) {
  int c = 0;
  for (int p = 0; p < n; ++p) c += (mask >> p) & 1;
  return c;
}

/// The line from the root to a node: (parent, action index).
std::vector<std::pair<std::uint32_t, int>> line_to(const BettingTree& tree, std::uint32_t id) {
  std::vector<std::pair<std::uint32_t, int>> line;
  for (std::uint32_t at = id; at != BettingTree::kRoot;) {
    const TreeNode& child = tree.node(at);
    line.push_back({child.parent, child.parent_action});
    at = child.parent;
  }
  std::reverse(line.begin(), line.end());
  return line;
}

/// A representative combo per class (the first in combo order).
const std::array<int, 169>& class_representatives() {
  static const std::array<int, 169> reps = [] {
    std::array<int, 169> r;
    r.fill(-1);
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      const auto [a, b] = Range::combo_cards(combo);
      const int cls = holdem_class(a, b);
      if (r[static_cast<std::size_t>(cls)] < 0) r[static_cast<std::size_t>(cls)] = combo;
    }
    return r;
  }();
  return reps;
}

const std::array<int, Range::kNumCombos>& combo_classes() {
  static const std::array<int, Range::kNumCombos> classes = [] {
    std::array<int, Range::kNumCombos> c{};
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      const auto [a, b] = Range::combo_cards(combo);
      c[static_cast<std::size_t>(combo)] = holdem_class(a, b);
    }
    return c;
  }();
  return classes;
}

/// Average strategy of `player` at a preflop node for every class
/// ([class * na + a]); uniform for classes with no live hand.
std::vector<float> class_strategy(const Solver& solver, std::uint32_t node, int player) {
  const int na = solver.tree().node(node).num_children;
  std::vector<float> out(169 * static_cast<std::size_t>(na), 1.0f / static_cast<float>(na));
  const PlayerHands& hands = solver.hands(player);
  const auto& classes = combo_classes();
  std::array<int, 169> live{};
  live.fill(-1);
  for (std::size_t h = 0; h < hands.hands.size(); ++h) {
    const auto cls = static_cast<std::size_t>(classes[static_cast<std::size_t>(hands.hands[h].combo)]);
    if (live[cls] < 0) live[cls] = static_cast<int>(h);
  }
  float sigma[kMaxActions];
  for (std::size_t c = 0; c < 169; ++c) {
    if (live[c] < 0) continue;
    const int bucket = solver.bucket(Street::Preflop, player, live[c], {});
    solver.average_strategy(node, bucket, sigma);
    for (int a = 0; a < na; ++a) out[c * static_cast<std::size_t>(na) + static_cast<std::size_t>(a)] = sigma[a];
  }
  return out;
}

bool is_raise(const Action& a) {
  return a.type == ActionType::Raise || a.type == ActionType::AllIn || a.type == ActionType::Bet;
}

/// The preset sizes for a format and stack (100/200/300 bands).
PreflopSettings preset_settings(int players, double stack_bb) {
  PreflopSettings s;
  s.players = players;
  s.stack_bb = stack_bb;
  const int band = stack_bb < 150 ? 0 : stack_bb < 250 ? 1 : 2;
  static constexpr double kThreeIp[] = {3.0, 3.25, 3.5};
  static constexpr double kThreeOop[] = {4.0, 4.5, 5.0};
  static constexpr double kFourBet[] = {2.3, 2.4, 2.5};
  s.three_bet_ip_multiplier = kThreeIp[band];
  s.three_bet_oop_multiplier = kThreeOop[band];
  s.four_bet_multiplier = kFourBet[band];
  s.squeeze_per_caller = 1.0;
  s.raise_cap = 4;
  if (players == 2) {
    s.open_sizes_by_position["BTN"] = {2.5};
    s.allow_sb_complete = true;
  } else {
    s.open_sizes_bb = {2.5};
    s.open_sizes_by_position["BTN"] = {2.3};
    s.sb_open_sizes_bb = {3.0};
    s.allow_cold_calls = true;
    if (players >= 7) s.max_cold_callers = 1;
  }
  return s;
}

}  // namespace

// --- Positions --------------------------------------------------------------

std::string preflop_position_name(int num_players, int seat) {
  if (num_players == 2) return seat == 0 ? "BTN" : "BB";
  return seat_name(num_players, Street::Preflop, seat);
}

int preflop_seat(int num_players, std::string_view position) {
  std::string name = upper(trim(position));
  if (name == "UTG+1") name = "UTG1";
  if (name == "UTG+2") name = "UTG2";
  if (num_players == 2 && (name == "SB" || name == "BU")) name = "BTN";
  if (name == "BU") name = "BTN";
  for (int p = 0; p < num_players; ++p) {
    if (preflop_position_name(num_players, p) == name) return p;
  }
  return -1;
}

// --- Settings ---------------------------------------------------------------

std::optional<GameConfig> make_preflop_config(const PreflopSettings& s, std::string* error) {
  const auto fail = [&](std::string message) -> std::optional<GameConfig> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };
  const int n = s.players;
  if (n < 2 || n > kMaxPlayers) return fail("preflop games need 2 to 9 players");
  if (!(s.big_blind > 0) || !(s.small_blind >= 0) || s.small_blind > s.big_blind) {
    return fail("bad blinds");
  }
  if (!(s.ante >= 0)) return fail("the ante cannot be negative");
  const double bb_ante = s.bb_ante ? s.ante : 0.0;
  const double each_ante = s.bb_ante ? 0.0 : s.ante;
  if (!std::isfinite(s.stack_bb) || s.stack_bb <= s.big_blind + bb_ante + each_ante + kEps) {
    return fail("the stack must be larger than the big blind and ante");
  }
  const int cap = s.raise_cap.value_or(4);
  if (cap < 1 || cap > 10) return fail("the preflop raise cap must be 1 to 10");

  GameConfig c = GameConfig::preflop_game(n, s.stack_bb, s.small_blind, each_ante);
  c.big_blind = s.big_blind;
  c.committed[static_cast<std::size_t>(n - 1)] = s.big_blind;
  if (s.bb_ante) {
    c.stacks[static_cast<std::size_t>(n - 1)] -= bb_ante;
    c.dead_pot += bb_ante;
  }

  PreflopSizing& pf = c.preflop;
  pf.open_bb = s.open_sizes_bb.empty() ? std::vector<double>{2.5} : s.open_sizes_bb;
  pf.sb_open_bb = s.sb_open_sizes_bb.empty() ? std::vector<double>{3.0} : s.sb_open_sizes_bb;
  if (n == 2) {
    // The button posts the small blind and opens first; the engine treats it
    // as the SB. Its sizes: BTN override, else SB sizes, else open sizes.
    const auto btn = s.open_sizes_by_position.find("BTN");
    if (btn != s.open_sizes_by_position.end() && !btn->second.empty()) {
      pf.sb_open_bb = btn->second;
    } else if (s.sb_open_sizes_bb.empty()) {
      pf.sb_open_bb = pf.open_bb;
    }
  }
  for (const auto& [position, sizes] : s.open_sizes_by_position) {
    const int seat = preflop_seat(n, position);
    if (seat < 0) return fail("unknown position '" + position + "' in the open sizes");
    if (n == 2) {
      if (seat != 0) return fail("the big blind cannot open");
      continue;  // handled above
    }
    if (seat == n - 1) return fail("the big blind cannot open");
    if (seat == n - 2) {
      if (!sizes.empty()) pf.sb_open_bb = sizes;
      continue;
    }
    if (pf.open_bb_by_seat.empty()) pf.open_bb_by_seat.assign(static_cast<std::size_t>(n), {});
    pf.open_bb_by_seat[static_cast<std::size_t>(seat)] = sizes;
  }
  pf.allow_limp = s.allow_limp;
  pf.allow_sb_complete = s.allow_limp || s.allow_sb_complete;
  pf.threebet_ip = {s.three_bet_ip_multiplier.value_or(3.0)};
  pf.threebet_oop = {s.three_bet_oop_multiplier.value_or(4.0)};
  pf.fourbet = {s.four_bet_multiplier.value_or(2.3)};
  pf.per_caller = s.squeeze_per_caller.value_or(1.0);
  pf.allin_every_level = false;
  pf.cold_call_open = s.allow_cold_calls;
  pf.max_cold_callers = s.max_cold_callers.value_or(-1);
  pf.cold_call_3bet = s.allow_cold_call_3bet;
  pf.cold_call_4bet = s.allow_cold_call_4bet;
  pf.raise_cap = cap;

  if (s.solve_postflop) {
    c.last_street = Street::Flop;
    c.sizing[static_cast<std::size_t>(Street::Flop)] = s.flop_sizing;
    c.raise_cap = s.flop_raise_cap;
  }
  c.rake.rate = s.rake_rate;
  c.rake.cap = s.rake_cap;
  c.rake.no_flop_no_drop = true;

  std::string message;
  if (!c.validate(&message)) return fail(message);
  return c;
}

std::optional<PreflopPreset> preflop_preset(std::string_view format, double stack_bb,
                                            std::string* error) {
  const std::string f = lower(trim(format));
  int players = 0;
  std::string name;
  if (f == "hu" || f == "headsup" || f == "heads-up" || f == "2") {
    players = 2;
    name = "hu";
  } else if (f == "6max" || f == "6-max" || f == "6") {
    players = 6;
    name = "6max";
  } else if (f == "9max" || f == "9-max" || f == "9") {
    players = 9;
    name = "9max";
  } else {
    if (error) *error = "unknown format '" + std::string(format) + "' (hu, 6max, 9max)";
    return std::nullopt;
  }
  if (!(stack_bb > 2) || !std::isfinite(stack_bb)) {
    if (error) *error = "the stack must be more than 2 BB";
    return std::nullopt;
  }
  PreflopPreset preset;
  preset.format = name;
  preset.name = name + "_" + amount(stack_bb) + "bb";
  preset.settings = preset_settings(players, stack_bb);
  return preset;
}

std::vector<PreflopPreset> library_presets() {
  std::vector<PreflopPreset> presets;
  for (const char* format : {"6max", "9max"}) {
    for (const double stack : {100.0, 200.0, 300.0}) presets.push_back(*preflop_preset(format, stack));
  }
  return presets;
}

std::string describe_preflop_settings(const PreflopSettings& s) {
  std::ostringstream out;
  const int n = s.players;
  out << "blinds " << fmt(s.small_blind) << "/" << fmt(s.big_blind);
  if (s.ante > 0) out << ", " << (s.bb_ante ? "BB ante " : "ante ") << fmt(s.ante);
  auto config = make_preflop_config(s);
  if (!config) return out.str();
  const PreflopSizing& pf = config->preflop;
  const auto sizes = [](const std::vector<double>& v) {
    std::string t;
    for (const double x : v) t += (t.empty() ? "" : "|") + amount(x);
    return t;
  };
  out << "; opens";
  for (int p = 0; p < n - 1; ++p) {
    const bool sb = n == 2 ? p == 0 : p == n - 2;
    const auto seat = static_cast<std::size_t>(p);
    const std::vector<double>& v =
        sb ? pf.sb_open_bb
           : (seat < pf.open_bb_by_seat.size() && !pf.open_bb_by_seat[seat].empty())
                 ? pf.open_bb_by_seat[seat]
                 : pf.open_bb;
    out << " " << preflop_position_name(n, p) << " " << sizes(v);
  }
  out << "; 3-bet IP " << amount(pf.threebet_ip[0]) << "x OOP " << amount(pf.threebet_oop[0])
      << "x; 4-bet " << amount(pf.fourbet[0]) << "x; squeeze +" << amount(pf.per_caller)
      << "x/caller; cap " << pf.raise_cap << " (then all-in)";
  out << "; limps " << (pf.allow_limp ? "yes" : pf.allow_sb_complete ? "SB only" : "no");
  out << "; cold calls " << (pf.cold_call_open ? "opens" : "no");
  if (pf.cold_call_open && pf.max_cold_callers >= 0) out << " (max " << pf.max_cold_callers << ")";
  if (pf.cold_call_3bet) out << "+3bets";
  if (pf.cold_call_4bet) out << "+4bets";
  out << (s.solve_postflop ? "; flop played" : "; equity at the flop (100% realisation)");
  if (s.rake_rate > 0) out << "; rake " << fmt(100 * s.rake_rate) << "% cap " << fmt(s.rake_cap);
  return out.str();
}

// --- Paths --------------------------------------------------------------------

PreflopPath preflop_path_of(const BettingTree& tree, std::uint32_t node) {
  PreflopPath result;
  result.node = node;
  const int n = tree.num_players();
  const auto line = line_to(tree, node);
  // Index of the last non-fold action (later folds are kept in the short form).
  int last_kept = -1;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const TreeNode& at = tree.node(line[i].first);
    if (at.kind != NodeKind::Action) {
      last_kept = static_cast<int>(i);
      continue;
    }
    if (tree.actions(line[i].first)[static_cast<std::size_t>(line[i].second)].type != ActionType::Fold) {
      last_kept = static_cast<int>(i);
    }
  }
  const auto append = [](std::string& path, const std::string& segment) {
    if (!path.empty()) path += '/';
    path += segment;
  };
  for (std::size_t i = 0; i < line.size(); ++i) {
    const TreeNode& at = tree.node(line[i].first);
    if (at.kind != NodeKind::Action) {
      append(result.short_path, "*");
      append(result.full_path, "*");
      append(result.engine_path, "*");
      continue;
    }
    const Action& action = tree.actions(line[i].first)[static_cast<std::size_t>(line[i].second)];
    const std::string label = action_label(action);
    const std::string positional = preflop_position_name(n, at.player) + ":" + label;
    append(result.full_path, positional);
    append(result.engine_path, label);
    if (action.type != ActionType::Fold || static_cast<int>(i) > last_kept) {
      append(result.short_path, positional);
    }
  }
  return result;
}

std::optional<PreflopPath> resolve_preflop_path(const BettingTree& tree, std::string_view path,
                                                std::string* error) {
  const auto fail = [&](std::string message) -> std::optional<PreflopPath> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };
  if (tree.start_street() != Street::Preflop) return fail("not a preflop game");
  const int n = tree.num_players();
  std::uint32_t node = BettingTree::kRoot;
  std::vector<Card> dealt;
  std::uint64_t used = 0;
  const auto labels_at = [&](std::uint32_t id) {
    std::string t;
    for (const Action& a : tree.actions(id)) t += (t.empty() ? "" : ", ") + action_label(a);
    return t;
  };
  const auto apply = [&](std::string_view label) -> bool {
    const auto next = tree.child(node, label);
    if (!next) return false;
    node = *next;
    return true;
  };

  for (std::string_view raw : split(path, '/')) {
    const std::string_view segment = trim(raw);
    if (segment.empty()) continue;
    const TreeNode& at = tree.node(node);
    if (at.terminal()) return fail("the hand is over before '" + std::string(segment) + "'");
    if (at.kind == NodeKind::Chance) {
      if (segment == "*") return fail("name the flop cards (e.g. Ks7d2c)");
      const auto cards = parse_cards(segment);
      if (!cards || cards->size() != 3) {
        return fail("expected the three flop cards, not '" + std::string(segment) + "'");
      }
      for (const Card c : *cards) {
        if (used & c.mask()) return fail("card " + c.to_string() + " named twice");
        used |= c.mask();
        dealt.push_back(c);
      }
      node = tree.children(node)[0];
      continue;
    }
    const std::size_t colon = segment.find(':');
    std::string_view label = segment;
    if (colon != std::string_view::npos) {
      const std::string_view position = trim(segment.substr(0, colon));
      label = trim(segment.substr(colon + 1));
      const int seat = preflop_seat(n, position);
      if (seat < 0) return fail("unknown position '" + std::string(position) + "'");
      // Fold everyone whose turn comes first.
      while (tree.node(node).kind == NodeKind::Action && tree.node(node).player != seat) {
        const std::uint32_t before = node;
        if (!apply("fold")) {
          return fail(preflop_position_name(n, tree.node(before).player) +
                      " cannot fold before " + preflop_position_name(n, seat) + " acts (" +
                      labels_at(before) + ")");
        }
        if ((tree.node(node).folded >> seat) & 1) break;
      }
      const TreeNode& now = tree.node(node);
      if (now.kind != NodeKind::Action || now.player != seat) {
        return fail(preflop_position_name(n, seat) + " does not act in '" + std::string(segment) +
                    "' (the betting round ended or they are out)");
      }
    }
    const std::uint32_t before = node;
    if (!apply(label)) {
      return fail("no action '" + std::string(label) + "' for " +
                  preflop_position_name(n, tree.node(before).player) + " (available: " +
                  labels_at(before) + ")");
    }
  }
  PreflopPath result = preflop_path_of(tree, node);
  result.dealt = dealt;
  if (dealt.size() == 3) {
    std::string flop;
    for (const Card c : dealt) flop += c.to_string();
    for (std::string* p : {&result.short_path, &result.full_path, &result.engine_path}) {
      const std::size_t star = p->find('*');
      if (star != std::string::npos) p->replace(star, 1, flop);
    }
  }
  return result;
}

// --- Browsing -------------------------------------------------------------------

std::optional<PreflopNodeStrategy> preflop_node_strategy(const Solver& solver,
                                                         std::string_view path,
                                                         std::string* error) {
  const auto fail = [&](std::string message) -> std::optional<PreflopNodeStrategy> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };
  if (solver.config().variant != Variant::Holdem) return fail("preflop classes need Hold'em");
  auto resolved = resolve_preflop_path(solver.tree(), path, error);
  if (!resolved) return std::nullopt;
  const TreeNode& node = solver.tree().node(resolved->node);
  if (node.kind != NodeKind::Action || node.street != Street::Preflop) {
    return fail("'" + resolved->short_path + "' is not a preflop decision");
  }
  BettingTree::Found found;
  found.node = resolved->node;
  const auto report = solver.report(found, {}, error);
  if (!report) return std::nullopt;
  const ClassStrategy cs = preflop_class_strategy(*report);

  PreflopNodeStrategy out;
  out.path = std::move(*resolved);
  out.seat = node.player;
  out.position = preflop_position_name(solver.config().num_players, node.player);
  out.actions = cs.actions;
  out.strategy = cs.strategy;
  out.pot = report->pot;
  const std::size_t na = out.actions.size();
  out.action_frequency.assign(na, 0.0);
  double total = 0;
  for (std::size_t c = 0; c < 169; ++c) {
    const double w = cs.weight[c];
    out.reach[c] = static_cast<float>(w / holdem_class_combos(static_cast<int>(c)));
    total += w;
    for (std::size_t a = 0; a < na; ++a) out.action_frequency[a] += w * cs.strategy[c * na + a];
  }
  if (total > 0) {
    for (double& f : out.action_frequency) f /= total;
  }
  out.reach_fraction = total / Range::kNumCombos;
  return out;
}

// --- Scenarios ------------------------------------------------------------------

const char* preflop_scenario_kind_name(PreflopScenarioKind kind) noexcept {
  switch (kind) {
    case PreflopScenarioKind::Limped: return "limped";
    case PreflopScenarioKind::SingleRaised: return "srp";
    case PreflopScenarioKind::ThreeBet: return "3bet";
    case PreflopScenarioKind::FourBet: return "4bet";
    case PreflopScenarioKind::FiveBetPlus: return "5bet+";
  }
  return "?";
}

std::optional<PreflopScenarioKind> parse_preflop_scenario_kind(std::string_view name) noexcept {
  for (const auto k : {PreflopScenarioKind::Limped, PreflopScenarioKind::SingleRaised,
                       PreflopScenarioKind::ThreeBet, PreflopScenarioKind::FourBet,
                       PreflopScenarioKind::FiveBetPlus}) {
    if (name == preflop_scenario_kind_name(k)) return k;
  }
  return std::nullopt;
}

namespace {

PreflopScenarioKind kind_of(int raises) {
  switch (raises) {
    case 0: return PreflopScenarioKind::Limped;
    case 1: return PreflopScenarioKind::SingleRaised;
    case 2: return PreflopScenarioKind::ThreeBet;
    case 3: return PreflopScenarioKind::FourBet;
    default: return PreflopScenarioKind::FiveBetPlus;
  }
}

/// "UTG open 2.5, CO 3-bet 7.5, UTG call".
std::string line_name(const BettingTree& tree, std::uint32_t node) {
  const int n = tree.num_players();
  std::string name;
  int level = 0;
  for (const auto& [id, a] : line_to(tree, node)) {
    const TreeNode& at = tree.node(id);
    if (at.kind != NodeKind::Action || at.street != Street::Preflop) break;
    const Action& action = tree.actions(id)[static_cast<std::size_t>(a)];
    if (action.type == ActionType::Fold) continue;
    std::string what;
    switch (action.type) {
      case ActionType::Check: what = "check"; break;
      case ActionType::Call:
        what = level == 0 ? ((n == 2 ? at.player == 0 : at.player == n - 2) ? "complete" : "limp")
                          : "call";
        break;
      default: {
        ++level;
        static const char* const kLevel[] = {"open", "3-bet", "4-bet", "5-bet", "6-bet"};
        what = level <= 5 ? kLevel[level - 1] : "raise";
        what += action.type == ActionType::AllIn ? " all-in" : " " + amount(action.amount);
        break;
      }
    }
    if (!name.empty()) name += ", ";
    name += preflop_position_name(n, at.player) + " " + what;
  }
  return name;
}

struct ScenarioWalk {
  const Solver& solver;
  int max_raises;
  double min_frequency;
  int n;
  std::vector<int> postflop_order;
  std::vector<PreflopScenario> out;
  /// reach[p][class] in 0..1.
  std::vector<std::array<double, 169>> reach;
  std::vector<double> fraction;  // per player: share of all combos reaching

  static double fraction_of(const std::array<double, 169>& r) {
    double total = 0;
    for (int c = 0; c < 169; ++c) total += r[static_cast<std::size_t>(c)] * holdem_class_combos(c);
    return total / Range::kNumCombos;
  }

  double frequency() const {
    double f = 1;
    for (const double x : fraction) f *= x;
    return f;
  }

  void walk(std::uint32_t id, int raises, int aggressor) {
    const BettingTree& tree = solver.tree();
    const TreeNode& node = tree.node(id);
    if (min_frequency > 0 && frequency() < min_frequency) return;
    if (node.kind == NodeKind::FoldOut) return;
    if (ends_preflop(tree, id)) {
      const std::uint16_t in = static_cast<std::uint16_t>(~node.folded & ((1u << n) - 1));
      if (count_bits(in, n) != 2 || (node.allin & in) != 0 || raises > max_raises) return;
      PreflopScenario s;
      s.path = preflop_path_of(tree, id);
      // A scenario ends where the flop would be dealt: drop a trailing "*".
      for (std::string* p : {&s.path.short_path, &s.path.full_path, &s.path.engine_path}) {
        if (p->size() >= 2 && p->compare(p->size() - 2, 2, "/*") == 0) p->resize(p->size() - 2);
      }
      s.name = line_name(tree, id);
      s.raises = raises;
      s.kind = kind_of(raises);
      int k = 0;
      for (const int p : postflop_order) {
        if ((in >> p) & 1) {
          s.seats[static_cast<std::size_t>(k)] = p;
          s.positions[static_cast<std::size_t>(k)] = preflop_position_name(n, p);
          ++k;
        }
      }
      s.aggressor = aggressor;
      if (aggressor >= 0) s.aggressor_position = preflop_position_name(n, aggressor);
      s.pot = tree.pot(id);
      const auto committed = tree.committed(id);
      const auto stacks = tree.stacks();
      s.effective_stack = std::min(stacks[static_cast<std::size_t>(s.seats[0])] - committed[static_cast<std::size_t>(s.seats[0])],
                                   stacks[static_cast<std::size_t>(s.seats[1])] - committed[static_cast<std::size_t>(s.seats[1])]);
      s.frequency = frequency();
      if (s.frequency >= min_frequency) out.push_back(std::move(s));
      return;
    }
    if (node.kind != NodeKind::Action) return;
    const int p = node.player;
    const auto pi = static_cast<std::size_t>(p);
    const auto children = tree.children(id);
    const auto actions = tree.actions(id);
    const std::size_t na = children.size();
    const std::vector<float> sigma = class_strategy(solver, id, p);
    const std::array<double, 169> saved = reach[pi];
    const double saved_fraction = fraction[pi];
    for (std::size_t a = 0; a < na; ++a) {
      for (std::size_t c = 0; c < 169; ++c) reach[pi][c] = saved[c] * sigma[c * na + a];
      fraction[pi] = fraction_of(reach[pi]);
      if (fraction[pi] > 0) {
        const bool raise = is_raise(actions[a]);
        walk(children[a], raises + (raise ? 1 : 0), raise ? p : aggressor);
      }
    }
    reach[pi] = saved;
    fraction[pi] = saved_fraction;
  }
};

}  // namespace

std::vector<PreflopScenario> list_preflop_scenarios(const Solver& solver, int max_raises,
                                                    double min_frequency) {
  const GameConfig& config = solver.config();
  if (config.start_street != Street::Preflop || config.variant != Variant::Holdem) return {};
  ScenarioWalk walk{solver, max_raises, min_frequency, config.num_players,
                    action_order(config.num_players, Street::Preflop, Street::Flop), {}, {}, {}};
  // Starting reach per class: average range weight over the class's combos.
  walk.reach.assign(static_cast<std::size_t>(walk.n), {});
  walk.fraction.assign(static_cast<std::size_t>(walk.n), 0.0);
  const auto& classes = combo_classes();
  for (int p = 0; p < walk.n; ++p) {
    auto& r = walk.reach[static_cast<std::size_t>(p)];
    r.fill(0.0);
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      r[static_cast<std::size_t>(classes[static_cast<std::size_t>(combo)])] += config.weight(p, combo);
    }
    for (int c = 0; c < 169; ++c) r[static_cast<std::size_t>(c)] /= holdem_class_combos(c);
    walk.fraction[static_cast<std::size_t>(p)] = ScenarioWalk::fraction_of(r);
  }
  walk.walk(BettingTree::kRoot, 0, -1);
  return std::move(walk.out);
}

std::optional<PreflopRanges> preflop_ranges(const Solver& solver, std::string_view path,
                                            std::string* error, float min_weight) {
  const auto fail = [&](std::string message) -> std::optional<PreflopRanges> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };
  const GameConfig& config = solver.config();
  if (config.variant != Variant::Holdem) return fail("preflop ranges need Hold'em");
  auto resolved = resolve_preflop_path(solver.tree(), path, error);
  if (!resolved) return std::nullopt;
  const BettingTree& tree = solver.tree();
  const std::uint32_t id = resolved->node;
  const TreeNode& node = tree.node(id);
  if (node.street != Street::Preflop && resolved->dealt.size() != 3) {
    return fail("name the flop cards to get ranges after the flop");
  }
  BettingTree::Found found;
  found.node = id;
  found.dealt = resolved->dealt;
  const auto report = solver.report(found, {}, error);
  if (!report) return std::nullopt;

  const int n = config.num_players;
  PreflopRanges out;
  out.complete = node.street != Street::Preflop || node.kind == NodeKind::Chance ||
                 node.kind == NodeKind::Showdown;
  out.pot = tree.pot(id);
  out.effective_stack = 1e300;
  const auto& classes = combo_classes();
  for (const int p : action_order(n, Street::Preflop, Street::Flop)) {
    if ((node.folded >> p) & 1) continue;
    const auto pi = static_cast<std::size_t>(p);
    PreflopPlayerRange pr;
    pr.seat = p;
    pr.position = preflop_position_name(n, p);
    std::array<double, 169> sum{};
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      float w = report->reach[pi][static_cast<std::size_t>(combo)];
      sum[static_cast<std::size_t>(classes[static_cast<std::size_t>(combo)])] += w;
      if (min_weight > 0) {
        w = w < min_weight ? 0.0f : std::round(w * 1000.0f) / 1000.0f;
      }
      pr.range.set_weight(combo, std::clamp(w, 0.0f, 1.0f));
    }
    for (int c = 0; c < 169; ++c) {
      pr.class_weight[static_cast<std::size_t>(c)] =
          static_cast<float>(sum[static_cast<std::size_t>(c)] / holdem_class_combos(c));
    }
    pr.committed = tree.committed(id)[pi];
    pr.stack = tree.stacks()[pi] - pr.committed;
    out.effective_stack = std::min(out.effective_stack, pr.stack);
    out.players.push_back(std::move(pr));
  }
  if (out.players.empty()) out.effective_stack = 0;
  out.path = std::move(*resolved);
  return out;
}

// --- Library ----------------------------------------------------------------------

PreflopLibrary make_preflop_library(const Solver& solver, const PreflopSettings& settings,
                                    std::string_view format, double exploitability_pct,
                                    int max_raises, double min_frequency) {
  PreflopLibrary lib;
  const int n = solver.config().num_players;
  lib.format = std::string(format);
  lib.players = n;
  lib.stack_bb = settings.stack_bb;
  lib.small_blind = settings.small_blind;
  lib.big_blind = settings.big_blind;
  lib.ante = settings.ante;
  lib.settings = describe_preflop_settings(settings);
  lib.iterations = solver.iterations();
  lib.exploitability_pct = exploitability_pct;

  // First-in raise frequency per position.
  const BettingTree& tree = solver.tree();
  std::uint32_t node = BettingTree::kRoot;
  for (int p = 0; p < n - 1; ++p) {
    if (tree.node(node).kind != NodeKind::Action || tree.node(node).player != p) break;
    const std::vector<float> sigma = class_strategy(solver, node, p);
    const auto actions = tree.actions(node);
    const std::size_t na = actions.size();
    double raise = 0;
    for (std::size_t c = 0; c < 169; ++c) {
      for (std::size_t a = 0; a < na; ++a) {
        if (is_raise(actions[a])) raise += sigma[c * na + a] * holdem_class_combos(static_cast<int>(c));
      }
    }
    lib.open_frequency.emplace_back(preflop_position_name(n, p), raise / Range::kNumCombos);
    const auto next = tree.child(node, "fold");
    if (!next) break;
    node = *next;
  }

  for (const PreflopScenario& s : list_preflop_scenarios(solver, max_raises, min_frequency)) {
    const auto ranges = preflop_ranges(solver, s.path.engine_path);
    if (!ranges || ranges->players.size() != 2) continue;
    PreflopLibraryEntry e;
    e.path = s.path.short_path;
    e.name = s.name;
    e.kind = s.kind;
    e.oop = s.positions[0];
    e.ip = s.positions[1];
    e.aggressor = s.aggressor_position;
    e.pot = s.pot;
    e.effective_stack = s.effective_stack;
    e.frequency = s.frequency;
    e.oop_range = ranges->players[0].range;
    e.ip_range = ranges->players[1].range;
    lib.entries.push_back(std::move(e));
  }
  return lib;
}

void write_preflop_library(std::ostream& out, const PreflopLibrary& lib) {
  out << "k15-preflop-library " << lib.version << "\n";
  out << "# Preflop ranges solved by K15 GTO (Monte Carlo CFR, 169 hand classes).\n";
  out << "# Approximation: no postflop play; the board is run out when the preflop\n";
  out << "# betting ends (100% equity realisation). Ranges are each player's reach\n";
  out << "# on the line (everyone else folding). Weights below 0.005 are dropped.\n";
  out << "format: " << lib.format << "\n";
  out << "players: " << lib.players << "\n";
  out << "stack_bb: " << fmt(lib.stack_bb) << "\n";
  out << "blinds: " << fmt(lib.small_blind) << " " << fmt(lib.big_blind) << "\n";
  out << "ante: " << fmt(lib.ante) << "\n";
  out << "settings: " << lib.settings << "\n";
  out << "iterations: " << lib.iterations << "\n";
  out << "exploitability_pct: " << fmt(lib.exploitability_pct, 4) << "\n";
  for (const auto& [position, freq] : lib.open_frequency) {
    out << "open: " << position << " " << fmt(freq, 4) << "\n";
  }
  for (const PreflopLibraryEntry& e : lib.entries) {
    out << "\n";
    out << "scenario: " << e.path << "\n";
    out << "name: " << e.name << "\n";
    out << "kind: " << preflop_scenario_kind_name(e.kind) << "\n";
    out << "oop: " << e.oop << "\n";
    out << "ip: " << e.ip << "\n";
    out << "aggressor: " << e.aggressor << "\n";
    out << "pot: " << fmt(e.pot) << "\n";
    out << "effective_stack: " << fmt(e.effective_stack) << "\n";
    out << "frequency: " << fmt(e.frequency, 4) << "\n";
    out << "oop_range: " << e.oop_range.to_string() << "\n";
    out << "ip_range: " << e.ip_range.to_string() << "\n";
  }
}

std::optional<PreflopLibrary> read_preflop_library(std::istream& in, std::string* error) {
  int line_no = 0;
  const auto fail = [&](std::string message) -> std::optional<PreflopLibrary> {
    if (error) *error = "line " + std::to_string(line_no) + ": " + message;
    return std::nullopt;
  };
  PreflopLibrary lib;
  bool magic = false;
  PreflopLibraryEntry* entry = nullptr;
  std::string raw;
  const auto number = [](std::string_view v, double* outv) {
    const std::string s(trim(v));
    char* end = nullptr;
    *outv = std::strtod(s.c_str(), &end);
    return !s.empty() && end == s.c_str() + s.size();
  };
  while (std::getline(in, raw)) {
    ++line_no;
    const std::string_view line = trim(raw);
    if (line.empty() || line.front() == '#') continue;
    if (!magic) {
      if (line.rfind("k15-preflop-library", 0) != 0) return fail("not a K15 preflop library");
      double v = 0;
      if (!number(line.substr(19), &v) || v < 1) return fail("bad version");
      if (v > 1) return fail("library version " + fmt(v) + " is newer than this program");
      lib.version = static_cast<int>(v);
      magic = true;
      continue;
    }
    const std::size_t colon = line.find(':');
    if (colon == std::string_view::npos) return fail("expected 'key: value'");
    const std::string key = lower(trim(line.substr(0, colon)));
    const std::string_view value = trim(line.substr(colon + 1));
    double v = 0;
    if (key == "scenario") {
      lib.entries.emplace_back();
      entry = &lib.entries.back();
      entry->path = std::string(value);
      continue;
    }
    if (entry == nullptr) {
      if (key == "format") lib.format = std::string(value);
      else if (key == "players") {
        if (!number(value, &v)) return fail("bad number");
        lib.players = static_cast<int>(v);
      } else if (key == "stack_bb") {
        if (!number(value, &lib.stack_bb)) return fail("bad number");
      } else if (key == "blinds") {
        const auto parts = split(value, ' ');
        if (parts.size() != 2 || !number(parts[0], &lib.small_blind) || !number(parts[1], &lib.big_blind)) {
          return fail("expected 'blinds: <sb> <bb>'");
        }
      } else if (key == "ante") {
        if (!number(value, &lib.ante)) return fail("bad number");
      } else if (key == "settings") lib.settings = std::string(value);
      else if (key == "iterations") {
        if (!number(value, &v)) return fail("bad number");
        lib.iterations = static_cast<std::uint64_t>(v);
      } else if (key == "exploitability_pct") {
        if (!number(value, &lib.exploitability_pct)) return fail("bad number");
      } else if (key == "open") {
        const std::size_t space = value.rfind(' ');
        if (space == std::string_view::npos || !number(value.substr(space + 1), &v)) {
          return fail("expected 'open: <position> <frequency>'");
        }
        lib.open_frequency.emplace_back(std::string(trim(value.substr(0, space))), v);
      }
      continue;
    }
    if (key == "name") entry->name = std::string(value);
    else if (key == "kind") {
      const auto k = parse_preflop_scenario_kind(value);
      if (!k) return fail("unknown kind '" + std::string(value) + "'");
      entry->kind = *k;
    } else if (key == "oop") entry->oop = std::string(value);
    else if (key == "ip") entry->ip = std::string(value);
    else if (key == "aggressor") entry->aggressor = std::string(value);
    else if (key == "pot") {
      if (!number(value, &entry->pot)) return fail("bad number");
    } else if (key == "effective_stack") {
      if (!number(value, &entry->effective_stack)) return fail("bad number");
    } else if (key == "frequency") {
      if (!number(value, &entry->frequency)) return fail("bad number");
    } else if (key == "oop_range" || key == "ip_range") {
      std::string message;
      auto range = Range::parse(value, &message);
      if (!range) return fail(key + ": " + message);
      (key == "oop_range" ? entry->oop_range : entry->ip_range) = *range;
    }
  }
  if (!magic) {
    line_no = 0;
    return fail("empty file");
  }
  return lib;
}

}  // namespace gto::mc
