#include "mc_cli.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "gto/mc/preflop.h"
#include "gto/mc/spot.h"
#include "gto/omaha.h"

namespace gto::cli {

namespace {

using nlohmann::json;
using namespace gto::mc;

struct ConfigError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

constexpr double kMB = 1024.0 * 1024.0;

std::optional<json> read_json(const std::string& path, std::string* error) {
  std::ifstream file(path);
  if (!file) {
    *error = "cannot open '" + path + "'";
    return std::nullopt;
  }
  try {
    return json::parse(file, nullptr, true, true);
  } catch (const json::exception& e) {
    *error = e.what();
    return std::nullopt;
  }
}

std::string fmt_count(std::uint64_t value) {
  std::string digits = std::to_string(value);
  std::string out;
  for (std::size_t i = 0; i < digits.size(); ++i) {
    if (i > 0 && (digits.size() - i) % 3 == 0) out += ',';
    out += digits[i];
  }
  return out;
}

void print_estimate(const GameEstimate& e) {
  std::printf("Tree: %s nodes (%s decisions, %s terminals)%s\n", fmt_count(e.tree.nodes).c_str(),
              fmt_count(e.tree.action_nodes).c_str(), fmt_count(e.tree.terminals).c_str(),
              e.tree.too_large ? " -- TOO LARGE (counting stopped)" : "");
  std::printf("Information sets: %s, table entries: %s\n", fmt_count(e.info_sets).c_str(),
              fmt_count(e.table_entries).c_str());
  std::printf("Memory: %.1f MB table, %.1f MB tree, up to %.1f MB bucket caches (%.1f MB total)\n",
              static_cast<double>(e.storage_bytes) / kMB, static_cast<double>(e.tree_bytes) / kMB,
              static_cast<double>(e.bucket_bytes) / kMB, static_cast<double>(e.total_bytes()) / kMB);
  for (int s = 0; s < kNumStreets; ++s) {
    const StreetPlan& plan = e.abstraction[static_cast<std::size_t>(s)];
    if (!plan.description.empty()) {
      std::printf("  %s: %s\n", street_name(static_cast<mc::Street>(s)), plan.description.c_str());
    }
  }
}

// --- 13x13 grids ----------------------------------------------------------

/// Prints one value (0..1) per class as a 13x13 grid of percentages; classes
/// with no reach (mask false) print as blanks.
void print_grid(const std::string& title, const std::array<float, 169>& value,
                const std::array<bool, 169>* shown = nullptr) {
  static const char kRanks[] = "AKQJT98765432";
  std::printf("%s\n     ", title.c_str());
  for (int c = 0; c < 13; ++c) std::printf("  %c ", kRanks[c]);
  std::printf("\n");
  for (int r = 0; r < 13; ++r) {
    std::printf("  %c  ", kRanks[r]);
    for (int c = 0; c < 13; ++c) {
      const auto cls = static_cast<std::size_t>(r * 13 + c);
      if (shown != nullptr && !(*shown)[cls]) {
        std::printf("   -");
        continue;
      }
      const int pct = static_cast<int>(std::lround(100.0 * value[cls]));
      if (pct == 0) std::printf("   .");
      else std::printf(" %3d", pct);
    }
    std::printf("\n");
  }
}

void print_node_strategy(const PreflopNodeStrategy& s, bool all_actions) {
  std::printf("\n=== %s ===\n", s.path.short_path.empty() ? "root" : s.path.short_path.c_str());
  std::printf("%s to act, pot %.2f; reaches with %.1f%% of hands; actions:", s.position.c_str(),
              s.pot, 100.0 * s.reach_fraction);
  for (std::size_t a = 0; a < s.actions.size(); ++a) {
    std::printf(" %s %.1f%%", s.actions[a].c_str(), 100.0 * s.action_frequency[a]);
  }
  std::printf("\n");
  std::array<bool, 169> shown{};
  for (std::size_t c = 0; c < 169; ++c) shown[c] = s.reach[c] > 1e-4f;
  const std::size_t na = s.actions.size();
  if (!all_actions) {
    // Everything but fold, summed: "plays".
    std::array<float, 169> play{};
    for (std::size_t c = 0; c < 169; ++c) {
      for (std::size_t a = 0; a < na; ++a) {
        if (s.actions[a] != "fold") play[c] += s.strategy[c * na + a];
      }
    }
    print_grid("Continue (% of each class, everything but fold):", play, &shown);
    return;
  }
  for (std::size_t a = 0; a < na; ++a) {
    if (s.actions[a] == "fold") continue;
    std::array<float, 169> v{};
    for (std::size_t c = 0; c < 169; ++c) v[c] = s.strategy[c * na + a];
    print_grid(s.actions[a] + " (% of each class):", v, &shown);
  }
}

// --- Preflop ----------------------------------------------------------------

double num(const json& j, const char* key, double fallback) {
  return j.contains(key) ? j.at(key).get<double>() : fallback;
}

std::vector<double> doubles(const json& j) {
  std::vector<double> out;
  for (const json& v : j) out.push_back(v.get<double>());
  return out;
}

/// Reads a "preflop" object: a preset ("format", "stack_bb") with overrides,
/// or everything spelled out ("players", ...).
PreflopSettings read_preflop_settings(const json& j, std::string* format_out) {
  PreflopSettings s;
  const double stack = num(j, "stack_bb", 100);
  if (j.contains("format")) {
    std::string error;
    auto preset = preflop_preset(j.at("format").get<std::string>(), stack, &error);
    if (!preset) throw ConfigError("preflop.format: " + error);
    s = preset->settings;
    *format_out = preset->format;
  } else {
    s.players = j.value("players", 6);
    s.stack_bb = stack;
    *format_out = std::to_string(s.players) + "max";
  }
  s.players = j.value("players", s.players);
  s.small_blind = num(j, "small_blind", s.small_blind);
  s.big_blind = num(j, "big_blind", s.big_blind);
  s.ante = num(j, "ante", s.ante);
  s.bb_ante = j.value("bb_ante", s.bb_ante);
  if (j.contains("open_sizes_bb")) s.open_sizes_bb = doubles(j.at("open_sizes_bb"));
  if (j.contains("open_sizes_by_position")) {
    s.open_sizes_by_position.clear();
    for (const auto& [k, v] : j.at("open_sizes_by_position").items()) {
      s.open_sizes_by_position[k] = doubles(v);
    }
  }
  if (j.contains("sb_open_sizes_bb")) s.sb_open_sizes_bb = doubles(j.at("sb_open_sizes_bb"));
  s.allow_limp = j.value("allow_limp", s.allow_limp);
  s.allow_sb_complete = j.value("allow_sb_complete", s.allow_sb_complete);
  if (j.contains("three_bet_ip_multiplier")) s.three_bet_ip_multiplier = j.at("three_bet_ip_multiplier").get<double>();
  if (j.contains("three_bet_oop_multiplier")) s.three_bet_oop_multiplier = j.at("three_bet_oop_multiplier").get<double>();
  if (j.contains("four_bet_multiplier")) s.four_bet_multiplier = j.at("four_bet_multiplier").get<double>();
  s.allow_cold_calls = j.value("allow_cold_calls", s.allow_cold_calls);
  if (j.contains("max_cold_callers")) s.max_cold_callers = j.at("max_cold_callers").get<int>();
  s.allow_cold_call_3bet = j.value("allow_cold_call_3bet", s.allow_cold_call_3bet);
  s.allow_cold_call_4bet = j.value("allow_cold_call_4bet", s.allow_cold_call_4bet);
  if (j.contains("squeeze_per_caller")) s.squeeze_per_caller = j.at("squeeze_per_caller").get<double>();
  if (j.contains("raise_cap")) s.raise_cap = j.at("raise_cap").get<int>();
  if (j.contains("rake")) {
    s.rake_rate = num(j.at("rake"), "percent", 0) / 100.0;
    s.rake_cap = num(j.at("rake"), "cap", 0);
  }
  return s;
}

/// First-in raise frequency per class for each opener (all seats but the BB).
std::vector<std::array<float, 169>> opening_tables(const Solver& solver) {
  std::vector<std::array<float, 169>> tables;
  const int n = solver.config().num_players;
  std::string path;
  for (int p = 0; p < n - 1; ++p) {
    auto s = preflop_node_strategy(solver, path);
    if (!s || s->seat != p) break;
    std::array<float, 169> raise{};
    const std::size_t na = s->actions.size();
    for (std::size_t c = 0; c < 169; ++c) {
      for (std::size_t a = 0; a < na; ++a) {
        const std::string& l = s->actions[a];
        if (l.rfind("raise", 0) == 0 || l == "allin") raise[c] += s->strategy[c * na + a];
      }
    }
    tables.push_back(raise);
    path += (path.empty() ? "" : "/") + std::string("fold");
  }
  return tables;
}

double table_frequency(const std::array<float, 169>& t) {
  double total = 0;
  for (int c = 0; c < 169; ++c) total += t[static_cast<std::size_t>(c)] * holdem_class_combos(c);
  return total / 1326.0;
}

struct PreflopArgs {
  std::string format = "6max";
  double stack = 100;
  std::string json_path;
  double seconds = 0;
  std::uint64_t iterations = 0;
  unsigned threads = 0;
  std::string out;
  std::vector<std::string> nodes;
  bool estimate_only = false;
  bool evaluate = true;
  int max_raises = 3;
  double min_frequency = 0.0005;
  int show_scenarios = 40;
  std::uint64_t seed = 1;
};

int preflop_usage_error(const std::string& message) {
  std::fprintf(stderr, "error: %s\n", message.c_str());
  return EXIT_FAILURE;
}

}  // namespace

int run_preflop(const std::vector<std::string>& args) {
  PreflopArgs a;
  for (std::size_t i = 0; i < args.size(); ++i) {
    const std::string& arg = args[i];
    const auto value = [&]() -> std::string {
      if (i + 1 >= args.size()) throw ConfigError(arg + " needs a value");
      return args[++i];
    };
    try {
      if (arg == "--format") a.format = value();
      else if (arg == "--stack") a.stack = std::stod(value());
      else if (arg == "--seconds") a.seconds = std::stod(value());
      else if (arg == "--iterations") a.iterations = static_cast<std::uint64_t>(std::stod(value()));
      else if (arg == "--threads") a.threads = static_cast<unsigned>(std::stoi(value()));
      else if (arg == "--out") a.out = value();
      else if (arg == "--node") a.nodes.push_back(value());
      else if (arg == "--estimate") a.estimate_only = true;
      else if (arg == "--no-eval") a.evaluate = false;
      else if (arg == "--max-raises") a.max_raises = std::stoi(value());
      else if (arg == "--min-frequency") a.min_frequency = std::stod(value());
      else if (arg == "--scenarios") a.show_scenarios = std::stoi(value());
      else if (arg == "--seed") a.seed = static_cast<std::uint64_t>(std::stoll(value()));
      else if (!arg.empty() && arg[0] != '-' && a.json_path.empty()) a.json_path = arg;
      else return preflop_usage_error("unexpected argument '" + arg + "'");
    } catch (const std::exception& e) {
      return preflop_usage_error(std::string(arg) + ": " + e.what());
    }
  }

  PreflopSettings settings;
  std::string format;
  if (!a.json_path.empty()) {
    std::string error;
    const auto root = read_json(a.json_path, &error);
    if (!root) return preflop_usage_error(error);
    try {
      if (!root->contains("preflop")) throw ConfigError("the file has no \"preflop\" object");
      settings = read_preflop_settings(root->at("preflop"), &format);
      if (root->contains("solve")) {
        const json& s = root->at("solve");
        if (a.seconds == 0) a.seconds = num(s, "max_seconds", 0);
        if (a.iterations == 0) a.iterations = static_cast<std::uint64_t>(num(s, "iterations", 0));
        if (a.threads == 0) a.threads = s.value("threads", 0u);
      }
      if (root->contains("output")) {
        const json& o = root->at("output");
        if (a.out.empty()) a.out = o.value("library", std::string());
        a.max_raises = o.value("max_raises", a.max_raises);
        a.min_frequency = num(o, "min_frequency", a.min_frequency);
      }
    } catch (const std::exception& e) {
      return preflop_usage_error(e.what());
    }
  } else {
    std::string error;
    auto preset = preflop_preset(a.format, a.stack, &error);
    if (!preset) return preflop_usage_error(error);
    settings = preset->settings;
    format = preset->format;
  }
  if (a.seconds == 0 && a.iterations == 0) a.seconds = 30;

  std::string error;
  const auto config = make_preflop_config(settings, &error);
  if (!config) return preflop_usage_error(error);
  const int n = config->num_players;
  std::printf("Preflop %s, %g BB: %s\n", format.c_str(), settings.stack_bb,
              describe_preflop_settings(settings).c_str());
  std::printf("Approximation: no postflop betting; the board is run out (100%% equity realisation).\n");
  const auto estimate = estimate_game(*config, &error);
  if (!estimate) return preflop_usage_error(error);
  print_estimate(*estimate);
  if (a.estimate_only) return EXIT_SUCCESS;

  SolverConfig sc;
  sc.threads = a.threads;
  sc.seed = a.seed;
  auto solver = Solver::create(*config, sc, &error);
  if (!solver) return preflop_usage_error(error);
  std::printf("Threads: %u\n", solver->threads());

  // Resolve the requested nodes before solving.
  for (const std::string& path : a.nodes) {
    if (!resolve_preflop_path(solver->tree(), path, &error)) {
      return preflop_usage_error("--node " + path + ": " + error);
    }
  }

  const auto started = std::chrono::steady_clock::now();
  const auto run_half = [&](double seconds, std::uint64_t iterations, double offset) {
    SolveOptions o;
    o.max_seconds = seconds;
    o.max_iterations = iterations;
    o.progress_interval_seconds = std::max(2.0, std::min(30.0, (seconds > 0 ? seconds : 60) / 5));
    o.on_progress = [offset](const Progress& p) {
      std::printf("  %7.0f s  %s iterations  (%.2f M/s)\n", offset + p.seconds,
                  fmt_count(p.iterations).c_str(), p.iterations_per_second / 1e6);
      std::fflush(stdout);
      return true;
    };
    return solver->solve(o);
  };
  const SolveResult first = run_half(a.seconds / 2, a.iterations / 2, 0);
  const auto half_tables = opening_tables(*solver);
  run_half(a.seconds / 2, a.iterations - a.iterations / 2, first.seconds);
  const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started;
  std::printf("Done: %s iterations in %.1f s, memory %.1f MB\n",
              fmt_count(solver->iterations()).c_str(), elapsed.count(),
              static_cast<double>(solver->memory_bytes()) / kMB);

  // Opening ranges and their stability between half-time and the end.
  const auto tables = opening_tables(*solver);
  std::printf("\nFirst-in raise frequency (stability: change since half-time)\n");
  std::printf("  %-5s %7s %9s %11s\n", "pos", "open", "change", "max class");
  for (std::size_t p = 0; p < tables.size(); ++p) {
    double max_diff = 0;
    for (std::size_t c = 0; c < 169; ++c) {
      if (p < half_tables.size()) {
        max_diff = std::max(max_diff, static_cast<double>(std::fabs(tables[p][c] - half_tables[p][c])));
      }
    }
    const double now = table_frequency(tables[p]);
    const double before = p < half_tables.size() ? table_frequency(half_tables[p]) : now;
    std::printf("  %-5s %6.1f%% %+8.2f%% %10.1f%%\n",
                preflop_position_name(n, static_cast<int>(p)).c_str(), 100 * now,
                100 * (now - before), 100 * max_diff);
  }

  double exploitability_pct = 0;
  if (a.evaluate) {
    const auto t0 = std::chrono::steady_clock::now();
    EvalOptions eo;
    const Evaluation ev = solver->evaluate(eo);
    const std::chrono::duration<double> et = std::chrono::steady_clock::now() - t0;
    exploitability_pct = ev.exploitability_pct;
    std::printf("\nEvaluation (%.1f s): EV per seat in BB (relative to the stack before posting):\n",
                et.count());
    for (int p = 0; p < n; ++p) {
      const auto pi = static_cast<std::size_t>(p);
      std::printf("  %-5s %+7.3f +- %.3f   best-response gain %.4f (holdout %.4f)\n",
                  preflop_position_name(n, p).c_str(), ev.ev[pi] - config->committed_of(p),
                  ev.ev_std_error[pi], ev.br_gain[pi], ev.br_gain_holdout[pi]);
    }
    std::printf("Exploitability (ESTIMATE%s): %.4f BB per player = %.3f%% of the starting pot "
                "(%.0f samples per information set)\n",
                ev.nash_guarantee ? "" : "; no Nash guarantee for 3+ players", ev.exploitability,
                ev.exploitability_pct, ev.br_samples_per_info_set);
  }

  // Opening charts.
  std::printf("\n");
  std::string path;
  for (int p = 0; p < n - 1; ++p) {
    auto s = preflop_node_strategy(*solver, path, &error);
    if (!s || s->seat != p) break;
    print_node_strategy(*s, s->actions.size() > 2);
    path += (path.empty() ? "" : "/") + std::string("fold");
  }

  // Scenarios.
  const auto scenarios = list_preflop_scenarios(*solver, a.max_raises, a.min_frequency);
  std::vector<const PreflopScenario*> sorted;
  for (const auto& s : scenarios) sorted.push_back(&s);
  std::sort(sorted.begin(), sorted.end(),
            [](const PreflopScenario* x, const PreflopScenario* y) { return x->frequency > y->frequency; });
  std::printf("\n%zu heads-up flop scenarios (up to %d raises, frequency >= %g); the %d most frequent:\n",
              scenarios.size(), a.max_raises, a.min_frequency, a.show_scenarios);
  std::printf("  %-6s %-5s %-5s %7s %7s %8s  %s\n", "kind", "OOP", "IP", "pot", "eff", "freq",
              "line");
  for (std::size_t i = 0; i < sorted.size() && static_cast<int>(i) < a.show_scenarios; ++i) {
    const PreflopScenario& s = *sorted[i];
    std::printf("  %-6s %-5s %-5s %7.2f %7.2f %7.3f%%  %s\n", preflop_scenario_kind_name(s.kind),
                s.positions[0].c_str(), s.positions[1].c_str(), s.pot, s.effective_stack,
                100 * s.frequency, s.path.short_path.c_str());
  }

  for (const std::string& node_path : a.nodes) {
    auto s = preflop_node_strategy(*solver, node_path, &error);
    if (s) {
      print_node_strategy(*s, true);
      continue;
    }
    auto r = preflop_ranges(*solver, node_path, &error);
    if (!r) {
      std::fprintf(stderr, "error: --node %s: %s\n", node_path.c_str(), error.c_str());
      continue;
    }
    std::printf("\n=== %s === pot %.2f, effective stack %.2f%s\n", r->path.short_path.c_str(), r->pot,
                r->effective_stack, r->complete ? " (preflop over)" : "");
    for (const auto& pr : r->players) {
      std::printf("%s (%.1f combos): %s\n", pr.position.c_str(), pr.range.total_weight(),
                  pr.range.to_string().c_str());
    }
  }

  if (!a.out.empty()) {
    const PreflopLibrary lib =
        make_preflop_library(*solver, settings, format, exploitability_pct, a.max_raises, a.min_frequency);
    const std::filesystem::path out_path(a.out);
    if (out_path.has_parent_path()) std::filesystem::create_directories(out_path.parent_path());
    std::ofstream file(a.out, std::ios::binary);
    write_preflop_library(file, lib);
    if (!file) return preflop_usage_error("cannot write '" + a.out + "'");
    std::printf("\nWrote %zu scenarios to %s\n", lib.entries.size(), a.out.c_str());
  }
  return EXIT_SUCCESS;
}

// --- Monte Carlo spots ---------------------------------------------------------

bool is_mc_spot(const std::string& path) {
  std::string error;
  const auto root = read_json(path, &error);
  if (!root || !root->is_object()) return false;
  return root->contains("players") || root->contains("preflop") ||
         root->value("engine", std::string()) == "mc";
}

namespace {

BetSizing read_bet_sizing(const json& o) {
  BetSizing s;
  if (o.contains("bet_pct")) {
    for (const json& v : o.at("bet_pct")) s.bet.push_back(v.get<double>() / 100.0);
  }
  if (o.contains("raise_pct")) {
    for (const json& v : o.at("raise_pct")) s.raise.push_back(v.get<double>() / 100.0);
  }
  s.add_allin = o.value("allin", false);
  return s;
}

StreetAbstraction buckets(int count) {
  StreetAbstraction a;
  a.kind = AbstractionKind::Strength;
  if (count >= 8 && count % 4 == 0) {
    a.strength_bins = count / 4;
    a.potential_bins = 4;
  } else {
    a.strength_bins = std::max(1, count);
    a.potential_bins = 1;
  }
  return a;
}

struct McSpotFile {
  MultiwaySpot spot;
  std::vector<std::string> positions;
  double seconds = 0;
  std::uint64_t iterations = 0;
  bool evaluate = true;
  std::uint64_t br_samples = 0;
};

McSpotFile read_mc_spot(const json& root) {
  McSpotFile f;
  MultiwaySpot& s = f.spot;
  const std::string variant = root.value("variant", std::string("holdem"));
  if (variant == "omaha4" || variant == "plo" || variant == "plo4") s.variant = Variant::Omaha4;
  else if (variant != "holdem") throw ConfigError("variant: expected \"holdem\" or \"omaha4\"");

  const auto board = parse_cards(root.at("board").get<std::string>());
  if (!board) throw ConfigError("board: cannot read the cards");
  s.board = *board;
  s.pot = root.at("pot").get<double>();

  std::vector<std::pair<std::string, std::string>> players;  // position, range
  std::vector<double> stacks;
  const double effective = num(root, "effective_stack", 0);
  if (root.contains("players")) {
    int i = 0;
    for (const json& p : root.at("players")) {
      players.emplace_back(p.value("position", "P" + std::to_string(i + 1)), p.at("range").get<std::string>());
      const double stack = num(p, "stack", 0);
      stacks.push_back(stack > 0 ? stack : effective);
      ++i;
    }
  } else if (root.contains("ranges")) {
    players.emplace_back("OOP", root.at("ranges").at("oop").get<std::string>());
    players.emplace_back("IP", root.at("ranges").at("ip").get<std::string>());
    stacks = {effective, effective};
  } else {
    throw ConfigError("need \"players\" (or \"ranges\" for two players)");
  }
  if (players.size() < 2) throw ConfigError("need at least two players");
  for (std::size_t i = 0; i < players.size(); ++i) {
    std::string error;
    if (!(stacks[i] > 0)) throw ConfigError("players[" + std::to_string(i) + "]: stack missing");
    if (s.variant == Variant::Omaha4) {
      auto r = OmahaRange::parse(players[i].second, &error);
      if (!r) throw ConfigError("players[" + std::to_string(i) + "].range: " + error);
      s.omaha_ranges.push_back(std::move(*r));
    } else {
      auto r = Range::parse(players[i].second, &error);
      if (!r) throw ConfigError("players[" + std::to_string(i) + "].range: " + error);
      s.ranges.push_back(*r);
    }
    f.positions.push_back(players[i].first);
  }
  s.stacks = stacks;

  if (root.contains("tree")) {
    const json& tree = root.at("tree");
    static constexpr const char* kStreets[] = {"flop", "turn", "river"};
    for (int st = 0; st < 3; ++st) {
      if (!tree.contains(kStreets[st])) continue;
      const json& o = tree.at(kStreets[st]);
      StreetSizing& sizing = s.sizing[static_cast<std::size_t>(st + 1)];
      if (o.contains("all")) sizing.sizing = read_bet_sizing(o.at("all"));
      else if (o.contains("bet_pct") || o.contains("raise_pct")) sizing.sizing = read_bet_sizing(o);
      if (o.contains("players")) {
        for (const json& p : o.at("players")) {
          sizing.per_player.push_back(p.is_null() ? std::nullopt : std::optional<BetSizing>(read_bet_sizing(p)));
        }
        sizing.per_player.resize(players.size());
      } else if (players.size() == 2 && (o.contains("oop") || o.contains("ip"))) {
        sizing.per_player.resize(2);
        if (o.contains("oop")) sizing.per_player[0] = read_bet_sizing(o.at("oop"));
        if (o.contains("ip")) sizing.per_player[1] = read_bet_sizing(o.at("ip"));
      }
    }
    s.raise_cap = tree.value("raise_cap", s.raise_cap);
    s.pot_limit = tree.value("pot_limit", s.pot_limit);
    if (tree.contains("allin_threshold_pct")) s.allin_threshold = tree.at("allin_threshold_pct").get<double>() / 100.0;
    if (tree.contains("last_street")) {
      const std::string last = tree.at("last_street").get<std::string>();
      if (last == "flop") s.last_street = mc::Street::Flop;
      else if (last == "turn") s.last_street = mc::Street::Turn;
      else if (last == "river") s.last_street = mc::Street::River;
      else throw ConfigError("tree.last_street: flop, turn or river");
    }
  }
  if (root.contains("rake")) {
    s.rake.rate = num(root.at("rake"), "percent", 0) / 100.0;
    s.rake.cap = num(root.at("rake"), "cap", 0);
    s.rake.no_flop_no_drop = false;
  }
  if (root.contains("abstraction")) {
    const json& a = root.at("abstraction");
    if (a.value("lossless", false)) {
      for (auto& st : s.abstraction.streets) st.kind = AbstractionKind::Lossless;
    }
    static constexpr const char* kKeys[] = {"preflop_buckets", "flop_buckets", "turn_buckets", "river_buckets"};
    for (int st = 1; st < 4; ++st) {
      if (a.contains(kKeys[st])) s.abstraction.streets[static_cast<std::size_t>(st)] = buckets(a.at(kKeys[st]).get<int>());
    }
    if (a.contains("lossless_limit")) s.abstraction.lossless_limit = a.at("lossless_limit").get<std::uint64_t>();
    if (a.contains("omaha_strength_samples")) s.abstraction.omaha_strength_samples = a.at("omaha_strength_samples").get<int>();
  }
  if (root.contains("solve")) {
    const json& o = root.at("solve");
    f.seconds = num(o, "max_seconds", 0);
    f.iterations = static_cast<std::uint64_t>(num(o, "mc_iterations", num(o, "iterations", 0)));
    f.evaluate = o.value("evaluate", true);
    f.br_samples = static_cast<std::uint64_t>(num(o, "br_samples", 0));
  }
  if (f.seconds == 0 && f.iterations == 0) f.seconds = 20;
  return f;
}

void print_mc_node(const Solver& solver, const NodeReport& r, const std::vector<std::string>& positions,
                   const std::string& path) {
  std::printf("\n=== %s ===\n", path.empty() ? "root" : path.c_str());
  switch (r.kind) {
    case NodeKind::FoldOut: std::puts("The hand ends with a fold."); return;
    case NodeKind::Showdown: std::puts("The hand goes to showdown."); return;
    case NodeKind::Chance: std::puts("Cards are dealt here; add them to the line, e.g. .../Ts"); return;
    case NodeKind::Action: break;
  }
  const auto p = static_cast<std::size_t>(r.player);
  std::printf("%s to act, pot %.2f\n", positions[p].c_str(), r.pot);
  const auto& reach = r.reach[p];
  const std::size_t na = r.actions.size();
  const bool holdem = solver.config().variant == Variant::Holdem;
  std::map<int, std::pair<std::string, std::vector<double>>> rows;  // class -> name, [reach, actions...]
  std::vector<double> total(na, 0.0);
  double total_reach = 0;
  std::vector<int> reached;
  for (int combo = 0; combo < r.num_combos; ++combo) {
    const double w = reach[static_cast<std::size_t>(combo)];
    if (!(w > 0)) continue;
    reached.push_back(combo);
    total_reach += w;
    for (std::size_t a = 0; a < na; ++a) total[a] += w * r.frequency(a, combo);
    if (holdem) {
      const auto [x, y] = Range::combo_cards(combo);
      const int cls = holdem_class(x, y);
      auto& row = rows[cls];
      if (row.first.empty()) {
        row.first = holdem_class_name(cls);
        row.second.assign(na + 1, 0.0);
      }
      row.second[0] += w;
      for (std::size_t a = 0; a < na; ++a) row.second[a + 1] += w * r.frequency(a, combo);
    }
  }
  if (total_reach <= 0) {
    std::puts("No hands reach this node.");
    return;
  }
  std::printf("%-9s %8s", "hand", "combos");
  for (const std::string& l : r.labels) std::printf(" %9s", l.c_str());
  std::printf("\n");
  if (holdem) {
    for (const auto& [cls, row] : rows) {
      std::printf("%-9s %8.2f", row.first.c_str(), row.second[0]);
      for (std::size_t a = 0; a < na; ++a) std::printf(" %8.1f%%", 100 * row.second[a + 1] / row.second[0]);
      std::printf("\n");
    }
  } else {
    // PLO: too many hands to list. Show the 10 hands that bet or raise most
    // and the 10 that do least, among those with at least half the top reach.
    double top = 0;
    for (const int c : reached) top = std::max(top, static_cast<double>(reach[static_cast<std::size_t>(c)]));
    std::vector<std::pair<double, int>> scored;
    for (const int c : reached) {
      if (reach[static_cast<std::size_t>(c)] < 0.5 * top) continue;
      double aggression = 0;
      for (std::size_t a = 0; a < na; ++a) {
        const ActionType t = r.actions[a].type;
        if (t == ActionType::Bet || t == ActionType::Raise || t == ActionType::AllIn) aggression += r.frequency(a, c);
      }
      scored.emplace_back(-aggression, c);
    }
    std::stable_sort(scored.begin(), scored.end());
    const auto show = [&](std::size_t i) {
      const int combo = scored[i].second;
      std::string name;
      for (const Card c : omaha_combo_cards(combo)) name += c.to_string();
      std::printf("%-9s %8.3f", name.c_str(), reach[static_cast<std::size_t>(combo)]);
      for (std::size_t a = 0; a < na; ++a) std::printf(" %8.1f%%", 100 * r.frequency(a, combo));
      std::printf("\n");
    };
    const std::size_t k = std::min<std::size_t>(10, scored.size());
    std::printf("(%zu hands reach this node; the %zu most and least aggressive shown)\n", reached.size(), k);
    for (std::size_t i = 0; i < k; ++i) show(i);
    if (scored.size() > k) std::printf("...\n");
    for (std::size_t i = std::max(k, scored.size() - k); i < scored.size(); ++i) show(i);
  }
  std::printf("%-9s %8.2f", "all", total_reach);
  for (std::size_t a = 0; a < na; ++a) std::printf(" %8.1f%%", 100 * total[a] / total_reach);
  std::printf("\n");
}

}  // namespace

int run_mc_solve(const std::string& path, const std::vector<std::string>& node_paths, unsigned threads) {
  std::string error;
  const auto root = read_json(path, &error);
  if (!root) return preflop_usage_error(error);
  if (root->contains("preflop")) {
    std::vector<std::string> args{path};
    if (threads > 0) {
      args.push_back("--threads");
      args.push_back(std::to_string(threads));
    }
    for (const auto& n : node_paths) {
      args.push_back("--node");
      args.push_back(n);
    }
    return run_preflop(args);
  }
  McSpotFile f;
  try {
    f = read_mc_spot(*root);
  } catch (const std::exception& e) {
    return preflop_usage_error(e.what());
  }
  const int n = static_cast<int>(f.positions.size());
  std::printf("Monte Carlo engine: %s, %d players, board", f.spot.variant == Variant::Omaha4 ? "PLO4" : "NLH", n);
  for (const Card c : f.spot.board) std::printf(" %s", c.to_string().c_str());
  std::printf(", pot %.2f\n", f.spot.pot);
  const auto estimate = estimate_spot(f.spot, &error);
  if (!estimate) return preflop_usage_error(error);
  print_estimate(*estimate);
  for (int p = 0; p < n; ++p) {
    std::printf("  %s: %zu live hands, stack %.2f\n", f.positions[static_cast<std::size_t>(p)].c_str(),
                estimate->live_hands[static_cast<std::size_t>(p)], f.spot.stacks[static_cast<std::size_t>(p)]);
  }

  SolverConfig sc;
  sc.threads = threads;
  auto solver = make_solver(f.spot, sc, &error);
  if (!solver) return preflop_usage_error(error);
  for (const std::string& node : node_paths) {
    if (!solver->tree().find(node, solver->config().board, &error)) {
      return preflop_usage_error("--node " + node + ": " + error);
    }
  }
  std::printf("Threads: %u\n", solver->threads());
  SolveOptions o;
  o.max_seconds = f.seconds;
  o.max_iterations = f.iterations;
  o.progress_interval_seconds = std::max(1.0, std::min(10.0, (f.seconds > 0 ? f.seconds : 20) / 5));
  o.on_progress = [](const Progress& p) {
    std::printf("  %6.1f s  %s iterations  (%.2f M/s)\n", p.seconds, fmt_count(p.iterations).c_str(),
                p.iterations_per_second / 1e6);
    std::fflush(stdout);
    return true;
  };
  const SolveResult result = solver->solve(o);
  std::printf("Done: %s iterations in %.1f s, memory %.1f MB\n", fmt_count(result.total_iterations).c_str(),
              result.seconds, static_cast<double>(solver->memory_bytes()) / kMB);

  EvalOptions eo;
  eo.best_response = f.evaluate;
  eo.br_samples = f.br_samples;
  const auto t0 = std::chrono::steady_clock::now();
  const Evaluation ev = solver->evaluate(eo);
  const std::chrono::duration<double> et = std::chrono::steady_clock::now() - t0;
  std::printf("\nEV (chips won from the pot minus chips put in, %.1f s to evaluate):\n", et.count());
  for (int p = 0; p < n; ++p) {
    const auto pi = static_cast<std::size_t>(p);
    std::printf("  %-6s %9.3f +- %.3f", f.positions[pi].c_str(), ev.ev[pi], ev.ev_std_error[pi]);
    if (f.evaluate) std::printf("   best-response gain %.4f (holdout %.4f)", ev.br_gain[pi], ev.br_gain_holdout[pi]);
    std::printf("\n");
  }
  if (f.evaluate) {
    if (ev.nash_guarantee) {
      std::printf("Exploitability (estimate, sampled): %.4f chips = %.3f%% of the pot\n", ev.exploitability,
                  ev.exploitability_pct);
    } else {
      std::printf("Exploitability: ESTIMATE; no Nash guarantee for 3+ players (or with rake). Mean gain of a\n"
                  "single player deviating alone: %.4f chips = %.3f%% of the pot (sum %.4f)\n",
                  ev.exploitability, ev.exploitability_pct, ev.total_gain);
    }
    std::printf("  (%.0f best-response samples per information set; below ~50 the in-sample gain is noisy)\n",
                ev.br_samples_per_info_set);
  }

  std::vector<std::string> paths{""};
  paths.insert(paths.end(), node_paths.begin(), node_paths.end());
  for (const std::string& node : paths) {
    const auto report = solver->report(node, {}, &error);
    if (!report) {
      std::fprintf(stderr, "error: --node %s: %s\n", node.c_str(), error.c_str());
      continue;
    }
    print_mc_node(*solver, *report, f.positions, node);
  }
  return EXIT_SUCCESS;
}

}  // namespace gto::cli
