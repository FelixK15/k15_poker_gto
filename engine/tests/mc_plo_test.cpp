// PLO in the mc engine: strength buckets, pot limit, preflop percentiles.

#include <gtest/gtest.h>

#include <numeric>

#include "gto/mc/solver.h"
#include "gto/mc/spot.h"

namespace gto::mc {
namespace {

int combo(const char* text) {
  const auto c = *parse_cards(text);
  return omaha_combo_index({c[0], c[1], c[2], c[3]});
}

TEST(McPlo, ThreeWayRiverWithStrengthBuckets) {
  MultiwaySpot mw;
  mw.variant = Variant::Omaha4;
  mw.board = *parse_cards("AsKd7h4c2s");
  const OmahaRange wide = *OmahaRange::parse("top40%");
  OmahaRange with_nuts = wide;
  with_nuts.set_weight(combo("5c3d9h9d"), 1.0f);  // the wheel: the nuts here
  mw.omaha_ranges = {with_nuts, wide, wide};
  mw.pot = 30;
  mw.stacks = {100, 100, 100};
  mw.sizing[3].sizing.bet = {0.5, 1.0};
  mw.sizing[3].sizing.raise = {1.0};
  mw.raise_cap = 1;
  mw.abstraction.streets[3].strength_bins = 25;
  mw.abstraction.streets[3].potential_bins = 2;
  std::string error;
  const auto estimate = estimate_spot(mw, &error);
  ASSERT_TRUE(estimate) << error;
  EXPECT_EQ(estimate->abstraction[3].kind, AbstractionKind::Strength);
  EXPECT_EQ(estimate->abstraction[3].buckets[0], 50u);
  auto solver = make_solver(mw, {.seed = 21}, &error);
  ASSERT_TRUE(solver) << error;
  EXPECT_TRUE(solver->config().pot_limit);
  EXPECT_EQ(solver->layout().info_sets(), estimate->info_sets);
  solver->run(400'000);

  const Evaluation e = solver->evaluate({.ev_samples = 100'000, .br_samples = 100'000});
  EXPECT_NEAR(std::accumulate(e.ev.begin(), e.ev.end(), 0.0), 30.0, 1e-6);
  EXPECT_LT(e.exploitability_pct, 10.0);

  // Pot limit: a pot-size bet is the largest.
  const auto root = solver->report("", {}, &error);
  ASSERT_TRUE(root) << error;
  EXPECT_EQ(root->labels, (std::vector<std::string>{"check", "bet15", "bet30"}));
  // The nuts never folds.
  const int nuts = combo("5c3d9h9d");
  EXPECT_GT(root->reach[0][static_cast<std::size_t>(nuts)], 0.0f);
  const auto facing = solver->report("check/bet30/fold", {}, &error);
  ASSERT_TRUE(facing) << error;
  EXPECT_EQ(facing->player, 0);
  ASSERT_EQ(facing->labels[0], "fold");
  EXPECT_LT(facing->frequency(0, nuts), 0.05f);
}

TEST(McPlo, PreflopPercentileBuckets) {
  GameConfig c = GameConfig::preflop_game(2, 30);
  c.variant = Variant::Omaha4;
  c.pot_limit = true;
  c.preflop.sb_open_bb = {3};  // pot-size open
  c.preflop.raise_cap = 2;
  c.abstraction.omaha_preflop_buckets = 50;
  std::string error;
  auto solver = Solver::create(c, {.seed = 3}, &error);
  ASSERT_TRUE(solver) << error;
  EXPECT_EQ(solver->abstraction().num_buckets(Street::Preflop, 0), 50);
  solver->run(50'000);
  const Evaluation e = solver->evaluate({.ev_samples = 20'000, .best_response = false});
  EXPECT_NEAR(e.ev[0] + e.ev[1], 1.5, 1e-6);
  // Pot limit preflop: the SB can raise to at most 3 (pot after calling).
  const auto root = solver->report("", {}, &error);
  ASSERT_TRUE(root) << error;
  EXPECT_EQ(root->labels, (std::vector<std::string>{"fold", "call", "raise3"}));
  // AAKK double-suited is in the top bucket.
  const int top = combo("AsAhKsKh");
  EXPECT_EQ(solver->bucket(Street::Preflop, 0, solver->hands(0).index_of_combo[static_cast<std::size_t>(top)], {}), 0);
}

}  // namespace
}  // namespace gto::mc
