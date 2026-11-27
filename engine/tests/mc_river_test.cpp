// River spots in the mc engine: cross-check against the exact heads-up
// solver, and a 3-way spot.

#include <gtest/gtest.h>

#include <cmath>
#include <numeric>

#include "gto/mc/solver.h"
#include "gto/mc/spot.h"
#include "gto/solver.h"

namespace gto::mc {
namespace {

constexpr const char* kBoard = "Ks9h5c3d2s";

TEST(McRiver, MatchesTheExactHeadsUpSolver) {
  const auto board = *parse_cards(kBoard);
  const Range oop = *Range::parse("KK,99,K9s,AK,KQ,QQ,JJ,T9s,87s,A5s");
  const Range ip = *Range::parse("AA,KQ,KJs,K9s,99,55,QJs,JTs,A4s,64s");

  // Exact heads-up solve.
  gto::SpotConfig spot;
  spot.board = board;
  spot.ranges = {oop, ip};
  spot.tree.pot = 10;
  spot.tree.effective_stack = 30;
  for (auto& player : spot.tree.sizing[2]) {
    player.bet = {0.5, 1.0};
    player.raise = {1.0};
  }
  spot.tree.raise_cap = 1;
  spot.tree.allin_threshold = 0.67;
  auto game = gto::make_game(spot);
  ASSERT_TRUE(game);
  gto::Solver exact(std::move(*game), {.threads = 1});
  gto::SolveOptions options;
  options.max_iterations = 2000;
  options.target_exploitability_pct = 0.01;
  const auto solved = exact.solve(options);
  ASSERT_LT(solved.evaluation.exploitability_pct, 0.1);
  const gto::NodeReport exact_root = exact.report(0);

  // The same spot in the mc engine.
  MultiwaySpot mw;
  mw.board = board;
  mw.ranges = {oop, ip};
  mw.pot = 10;
  mw.stacks = {30, 30};
  mw.sizing[3].sizing.bet = {0.5, 1.0};
  mw.sizing[3].sizing.raise = {1.0};
  mw.raise_cap = 1;
  mw.allin_threshold = 0.67;
  const auto estimate = estimate_spot(mw);
  ASSERT_TRUE(estimate);
  std::string error;
  auto solver = make_solver(mw, {.seed = 2}, &error);
  ASSERT_TRUE(solver) << error;
  EXPECT_EQ(solver->tree().size(), exact.game().tree.nodes.size());
  EXPECT_EQ(solver->layout().info_sets(), estimate->info_sets);
  solver->run(1'500'000);

  const auto value = solver->evaluate_exact();
  ASSERT_TRUE(value) << "exact evaluation should fit";
  EXPECT_NEAR(value->ev[0], solved.evaluation.ev[0], 0.08);
  EXPECT_NEAR(value->ev[1], solved.evaluation.ev[1], 0.08);
  EXPECT_LT(value->exploitability_pct, 0.5);

  // Root frequencies over the OOP range.
  const auto root = solver->report("");
  ASSERT_TRUE(root);
  ASSERT_EQ(root->actions.size(), exact_root.actions.size());
  const auto& hands = exact.game().hands[0];
  for (std::size_t a = 0; a < root->actions.size(); ++a) {
    double exact_freq = 0, mc_freq = 0, total = 0;
    for (std::size_t h = 0; h < hands.size(); ++h) {
      const int combo = Range::combo_index(Card(hands[h].cards[0]), Card(hands[h].cards[1]));
      const double w = hands[h].weight;
      exact_freq += w * exact_root.frequency(a, h);
      mc_freq += w * root->frequency(a, combo);
      total += w;
    }
    EXPECT_NEAR(mc_freq / total, exact_freq / total, 0.06) << root->labels[a];
  }

  // The sampled estimate of exploitability is small too.
  const Evaluation sampled = solver->evaluate({.ev_samples = 200'000, .br_samples = 50'000});
  EXPECT_NEAR(sampled.ev[0], value->ev[0], 4 * sampled.ev_std_error[0] + 0.02);
  EXPECT_LT(sampled.exploitability_pct, 0.6);
  EXPECT_LE(sampled.br_gain_holdout[0], sampled.br_gain[0] + 4 * sampled.br_gain_std_error[0]);
}

MultiwaySpot three_way() {
  MultiwaySpot mw;
  mw.board = *parse_cards("AsKsQs7d2c");
  const Range r1 = *Range::parse("JsTs,AA,KK,AQ,KQ,77,22,Ah5h,Ad9d,JJ,QJs,T9s,98s");
  const Range r2 = *Range::parse("JsTs,QQ,AK,AJ,KJ,Q7s,JTs,T9s,Kh7h,66,55");
  const Range r3 = *Range::parse("JsTs,AA,A7s,K7s,72s,AT,KT,QT,JJ,TT,99,8s9s");
  mw.ranges = {r1, r2, r3};
  mw.pot = 30;
  mw.stacks = {100, 100, 100};
  mw.sizing[3].sizing.bet = {0.5};
  mw.sizing[3].sizing.raise = {1.0};
  mw.raise_cap = 1;
  return mw;
}

TEST(McRiver, ThreeWaySpotSolvesSanely) {
  std::string error;
  auto solver = make_solver(three_way(), {.seed = 4}, &error);
  ASSERT_TRUE(solver) << error;
  solver->run(1'000'000);

  const Evaluation e = solver->evaluate({.ev_samples = 200'000, .br_samples = 20'000});
  EXPECT_FALSE(e.nash_guarantee);
  // No rake: the values add up to the pot (exactly, per sampled hand).
  EXPECT_NEAR(std::accumulate(e.ev.begin(), e.ev.end(), 0.0), 30.0, 1e-6);
  for (int p = 0; p < 3; ++p) {
    EXPECT_GT(e.ev[static_cast<std::size_t>(p)], 0.0);
    EXPECT_LT(e.br_gain_holdout[static_cast<std::size_t>(p)], 0.03 * 30) << p;
  }
  const auto exact = solver->evaluate_exact();
  ASSERT_TRUE(exact);
  EXPECT_NEAR(std::accumulate(exact->ev.begin(), exact->ev.end(), 0.0), 30.0, 1e-6);
  EXPECT_LT(exact->exploitability_pct, 3.0);

  // The nuts (JsTs, a royal flush) never folds and never checks behind.
  const int nuts = Range::combo_index(*Card::parse("Js"), *Card::parse("Ts"));
  for (const char* path : {"bet15", "bet15/call", "check/bet15", "check/check/bet15",
                           "bet15/allin"}) {
    const auto r = solver->report(path, {}, &error);
    ASSERT_TRUE(r) << path << ": " << error;
    ASSERT_EQ(r->labels[0], "fold");
    if (r->reach[static_cast<std::size_t>(r->player)][static_cast<std::size_t>(nuts)] > 0) {
      EXPECT_LT(r->frequency(0, nuts), 0.05) << path;
    }
  }
  // Reach: zero for combos blocked by the board, starting weight at the root.
  const auto root = solver->report("");
  ASSERT_TRUE(root);
  const int blocked = Range::combo_index(*Card::parse("As"), *Card::parse("Ah"));
  EXPECT_EQ(root->reach[0][static_cast<std::size_t>(blocked)], 0.0f);
  EXPECT_EQ(root->reach[0][static_cast<std::size_t>(nuts)], 1.0f);
}

TEST(McRiver, ReportEvsAndRake) {
  MultiwaySpot mw = three_way();
  mw.rake.rate = 0.05;
  mw.rake.cap = 3;
  std::string error;
  auto solver = make_solver(mw, {.seed = 6}, &error);
  ASSERT_TRUE(solver) << error;
  solver->run(200'000);
  const Evaluation e = solver->evaluate({.ev_samples = 100'000, .best_response = false});
  // Rake comes out of the pot: values add up to less than 30.
  const double sum = std::accumulate(e.ev.begin(), e.ev.end(), 0.0);
  EXPECT_LT(sum, 30.0);
  EXPECT_GT(sum, 27.0 - 1e-9);

  const auto r = solver->report("check", {.ev_samples = 200}, &error);
  ASSERT_TRUE(r) << error;
  EXPECT_EQ(r->player, 1);
  ASSERT_EQ(r->ev.size(), 3u);
  const int nuts = Range::combo_index(*Card::parse("Js"), *Card::parse("Ts"));
  ASSERT_FALSE(r->ev[1].empty());
  // The nuts wins at least the pot minus rake (30 - 1.5) on average.
  EXPECT_GT(r->ev[1][static_cast<std::size_t>(nuts)], 28.0);
  EXPECT_FALSE(std::isnan(r->range_ev[0]));
  EXPECT_TRUE(std::isnan(r->ev[1][0]));  // 2c2d-ish combos not in the range
}

}  // namespace
}  // namespace gto::mc
