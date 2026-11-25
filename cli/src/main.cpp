// gto-cli: runs the solver engine without a server.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gto/cpu.h"
#include "gto/solver.h"
#include "gto/version.h"
#include "spot_config.h"

namespace {

void print_usage() {
  std::puts(
      "Usage:\n"
      "  gto-cli solve <spot.json> [--node <path>]... [--threads <n>] [--compress]\n"
      "  gto-cli --version\n\n"
      "solve  Solves the spot described in the JSON file (see examples/) and\n"
      "       prints the strategy at the root.\n"
      "  --node <path>  Also print the node reached by this line of actions and\n"
      "                 dealt cards, e.g. --node check/bet50/call/Ts. May be\n"
      "                 given several times.\n"
      "  --threads <n>  Threads to use (default: all).\n"
      "  --compress     Keep regrets and strategies in 16 bits: half the\n"
      "                 memory, slightly less precise.");
}

/// Sort key that lists hand classes from AA down: pairs and high cards first.
int class_order(gto::Card a, gto::Card b) {
  const int high = std::max(a.rank(), b.rank());
  const int low = std::min(a.rank(), b.rank());
  const int kind = high == low ? 0 : a.suit() == b.suit() ? 1 : 2;
  return ((12 - high) * 13 + (12 - low)) * 3 + kind;
}

struct ClassRow {
  std::string name;
  double combos = 0;
  std::vector<double> action_weight;
  double ev_weight = 0;
};

void print_node(const gto::Solver& solver, const gto::NodeRef& ref, const std::string& path) {
  const gto::NodeReport report = solver.report(ref);

  std::printf("\n=== %s ===\n", path.empty() ? "root" : path.c_str());
  switch (report.type) {
    case gto::NodeType::Fold:
      std::puts("The hand ends with a fold.");
      return;
    case gto::NodeType::Showdown:
      std::puts("The hand goes to showdown.");
      return;
    case gto::NodeType::Chance:
      std::puts("The next card is dealt here; add it to the line, e.g. .../Ts");
      return;
    case gto::NodeType::Action:
      break;
  }

  const auto player = static_cast<std::size_t>(report.player);
  const auto& hands = solver.game().hands[player];
  const auto& reach = report.reach[player];
  const std::size_t actions = report.actions.size();
  std::printf("%s to act\n", report.player == gto::kOop ? "OOP" : "IP");

  std::map<int, ClassRow> rows;
  std::vector<double> total_action(actions, 0.0);
  double total_reach = 0;
  for (std::size_t h = 0; h < hands.size(); ++h) {
    if (!(reach[h] > 0)) continue;
    const gto::Card a(hands[h].cards[0]);
    const gto::Card b(hands[h].cards[1]);
    ClassRow& row = rows[class_order(a, b)];
    if (row.name.empty()) {
      row.name = gto::hand_class_name(a, b);
      row.action_weight.assign(actions, 0.0);
    }
    row.combos += reach[h];
    row.ev_weight += static_cast<double>(reach[h]) * report.ev[player][h];
    total_reach += reach[h];
    for (std::size_t action = 0; action < actions; ++action) {
      const double weight = static_cast<double>(reach[h]) * report.frequency(action, h);
      row.action_weight[action] += weight;
      total_action[action] += weight;
    }
  }

  if (total_reach <= 0) {
    std::puts("No hands reach this node.");
    return;
  }

  std::printf("%-5s %7s", "hand", "combos");
  for (const gto::Action& action : report.actions) {
    std::printf(" %9s", gto::action_label(action).c_str());
  }
  std::printf(" %9s\n", "EV");

  for (const auto& [order, row] : rows) {
    std::printf("%-5s %7.2f", row.name.c_str(), row.combos);
    for (const double weight : row.action_weight) {
      std::printf(" %8.1f%%", 100.0 * weight / row.combos);
    }
    std::printf(" %9.2f\n", row.ev_weight / row.combos);
  }

  std::printf("%-5s %7.2f", "all", total_reach);
  for (const double weight : total_action) std::printf(" %8.1f%%", 100.0 * weight / total_reach);
  std::printf("\n");
}

int run_solve(const std::string& config_path, const std::vector<std::string>& node_paths,
              unsigned threads, bool compress) {
  std::string error;
  auto loaded = gto::cli::load_spot(config_path, &error);
  if (!loaded) {
    std::fprintf(stderr, "error: %s\n", error.c_str());
    return EXIT_FAILURE;
  }
  auto game = gto::make_game(loaded->spot, &error);
  if (!game) {
    std::fprintf(stderr, "error: %s\n", error.c_str());
    return EXIT_FAILURE;
  }

  // Resolve the requested nodes before spending time on the solve.
  std::vector<std::pair<gto::NodeRef, std::string>> nodes = {{gto::NodeRef{}, ""}};
  for (const std::string& path : node_paths) {
    const auto node = game->tree.find(path, &error);
    if (!node) {
      std::fprintf(stderr, "error: --node %s: %s\n", path.c_str(), error.c_str());
      return EXIT_FAILURE;
    }
    nodes.emplace_back(*node, path);
  }

  const double megabyte = 1024.0 * 1024.0;
  std::printf("Tree: %zu nodes (%zu decisions), hands: OOP %zu, IP %zu\n",
              game->tree.nodes.size(), game->tree.num_action_nodes(), game->hands[0].size(),
              game->hands[1].size());
  std::printf("Memory: %.1f MB for strategies, %.1f MB for the tree\n",
              static_cast<double>(gto::Solver::estimate_memory_bytes(*game, compress)) / megabyte,
              static_cast<double>(game->tree.memory_bytes()) / megabyte);

  gto::SolverConfig solver_config;
  solver_config.threads = threads;
  solver_config.compress = compress;
  gto::Solver solver(std::move(*game), solver_config);
  std::printf("Threads: %u\n", solver.threads());
  loaded->solve.on_progress = [](const gto::Progress& progress) {
    std::printf("  iteration %5d   exploitability %.4f%% of pot\n", progress.iteration,
                progress.evaluation.exploitability_pct);
    std::fflush(stdout);
    return true;
  };

  const auto started = std::chrono::steady_clock::now();
  const gto::SolveResult result = solver.solve(loaded->solve);
  const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started;

  std::printf("Done: %d iterations in %.2f s, exploitability %.4f%% of pot\n", result.iterations,
              elapsed.count(), result.evaluation.exploitability_pct);
  std::printf("EV: OOP %.3f, IP %.3f (pot %.3f)\n", result.evaluation.ev[0],
              result.evaluation.ev[1], solver.game().tree.pot);

  for (const auto& [ref, path] : nodes) print_node(solver, ref, path);
  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char** argv) {
  if (!gto::cpu_is_supported()) {
    std::fprintf(stderr, "error: %s\n", gto::cpu_requirement_message());
    return EXIT_FAILURE;
  }

  const std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty() || args[0] == "--help" || args[0] == "-h") {
    print_usage();
    return EXIT_SUCCESS;
  }
  if (args[0] == "--version") {
    const std::string_view version = gto::engine_version();
    std::printf("gto-cli (engine %.*s)\n", static_cast<int>(version.size()), version.data());
    return EXIT_SUCCESS;
  }
  if (args[0] == "solve") {
    std::string config_path;
    std::vector<std::string> node_paths;
    unsigned threads = 0;
    bool compress = false;
    for (std::size_t i = 1; i < args.size(); ++i) {
      if (args[i] == "--compress") {
        compress = true;
        continue;
      }
      if (args[i] == "--threads") {
        const int value = i + 1 < args.size() ? std::atoi(args[++i].c_str()) : 0;
        if (value < 1 || value > 1024) {
          std::fputs("error: --threads needs a number from 1 to 1024\n", stderr);
          return EXIT_FAILURE;
        }
        threads = static_cast<unsigned>(value);
        continue;
      }
      if (args[i] == "--node") {
        if (i + 1 >= args.size()) {
          std::fputs("error: --node needs an action line\n", stderr);
          return EXIT_FAILURE;
        }
        node_paths.push_back(args[++i]);
      } else if (config_path.empty()) {
        config_path = args[i];
      } else {
        std::fprintf(stderr, "error: unexpected argument '%s'\n", args[i].c_str());
        return EXIT_FAILURE;
      }
    }
    if (config_path.empty()) {
      std::fputs("error: solve needs a spot file\n", stderr);
      return EXIT_FAILURE;
    }
    return run_solve(config_path, node_paths, threads, compress);
  }

  std::fprintf(stderr, "error: unknown command '%s'\n", args[0].c_str());
  return EXIT_FAILURE;
}
