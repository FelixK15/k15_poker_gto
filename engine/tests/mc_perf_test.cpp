// Throughput measurements of the mc engine. Disabled by default; run with
//   solver_tests --gtest_also_run_disabled_tests --gtest_filter=McPerf.*
// Set GTO_MC_PERF_SECONDS to change the 10 s per spot.

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS  // std::getenv
#endif

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>

#include "gto/mc/solver.h"
#include "gto/mc/spot.h"

namespace gto::mc {
namespace {

double seconds() {
  const char* env = std::getenv("GTO_MC_PERF_SECONDS");
  return env ? std::atof(env) : 10.0;
}

void measure(const char* name, const GameConfig& config) {
  std::string error;
  const auto estimate = estimate_game(config, &error);
  ASSERT_TRUE(estimate) << error;
  auto solver = Solver::create(config, {}, &error);
  ASSERT_TRUE(solver) << error;
  SolveOptions options;
  options.max_seconds = seconds();
  const SolveResult result = solver->solve(options);
  std::printf(
      "[%s] threads %u: %llu iterations in %.1f s = %.0f it/s (%.0f traversals/s); nodes %llu, "
      "decision nodes %llu, info sets %llu, table %.1f MB, solver memory %.1f MB (estimate %.1f MB)\n",
      name, solver->threads(), static_cast<unsigned long long>(result.iterations), result.seconds,
      static_cast<double>(result.iterations) / result.seconds,
      static_cast<double>(result.iterations) * config.num_players / result.seconds,
      static_cast<unsigned long long>(solver->tree().size()),
      static_cast<unsigned long long>(solver->tree().num_action_nodes()),
      static_cast<unsigned long long>(solver->layout().info_sets()),
      static_cast<double>(DenseFloatStorage::bytes_for(solver->layout().size())) / 1048576.0,
      static_cast<double>(solver->memory_bytes()) / 1048576.0,
      static_cast<double>(estimate->total_bytes()) / 1048576.0);
  const Evaluation e = solver->evaluate();
  std::printf("[%s] estimated exploitability %.3f%% of the pot (%.0f scenarios per info set; mean gain %.4f, holdout",
              name, e.exploitability_pct, e.br_samples_per_info_set, e.exploitability);
  for (const double g : e.br_gain_holdout) std::printf(" %.4f", g);
  std::printf(")\n");
}

TEST(McPerf, DISABLED_HeadsUpRiver) {
  MultiwaySpot mw;
  mw.board = *parse_cards("Ks9h5c3d2s");
  mw.ranges = {*Range::parse("22+,A2s+,K2s+,Q8s+,J8s+,T8s+,98s,A9o+,KTo+,QTo+"),
               *Range::parse("22+,A2s+,K9s+,Q9s+,J9s+,T9s,ATo+,KJo+")};
  mw.pot = 10;
  mw.stacks = {50, 50};
  mw.sizing[3].sizing.bet = {0.33, 0.75};
  mw.sizing[3].sizing.raise = {1.0};
  mw.sizing[3].sizing.add_allin = true;
  mw.raise_cap = 2;
  measure("HU NLH river", *make_game_config(mw));
}

TEST(McPerf, DISABLED_ThreeWayFlop) {
  MultiwaySpot mw;
  mw.board = *parse_cards("Kh8d4c");
  const Range r = *Range::parse("22+,A2s+,K8s+,Q9s+,J9s+,T8s+,97s+,86s+,75s+,ATo+,KJo+");
  mw.ranges = {r, r, r};
  mw.pot = 15;
  mw.stacks = {90, 90, 90};
  for (int s = 1; s < 4; ++s) {
    mw.sizing[static_cast<std::size_t>(s)].sizing.bet = {0.33, 0.75};
    mw.sizing[static_cast<std::size_t>(s)].sizing.raise = {1.0};
    mw.sizing[static_cast<std::size_t>(s)].sizing.add_allin = true;
  }
  mw.raise_cap = 1;
  measure("3-way NLH flop", *make_game_config(mw));
}

TEST(McPerf, DISABLED_ThreeWayPloRiver) {
  MultiwaySpot mw;
  mw.variant = Variant::Omaha4;
  mw.board = *parse_cards("AsKd7h4c2s");
  const OmahaRange r = *OmahaRange::parse("top40%");
  mw.omaha_ranges = {r, r, r};
  mw.pot = 30;
  mw.stacks = {100, 100, 100};
  mw.sizing[3].sizing.bet = {0.5, 1.0};
  mw.sizing[3].sizing.raise = {1.0};
  mw.raise_cap = 1;
  measure("3-way PLO river", *make_game_config(mw));
}

TEST(McPerf, DISABLED_SixMaxPreflop) {
  GameConfig c = GameConfig::preflop_game(6, 100);
  c.preflop.open_bb = {2.5};
  c.preflop.sb_open_bb = {3};
  c.preflop.threebet_ip = {3};
  c.preflop.threebet_oop = {4};
  c.preflop.fourbet = {2.3};
  c.preflop.allin_every_level = false;
  c.preflop.raise_cap = 4;
  measure("6-max preflop", c);
}

}  // namespace
}  // namespace gto::mc
