// MCCFR correctness on Kuhn poker (two and three players), through the toy
// deck hook of the mc engine: the same tree, sampling and traversal code as
// real poker.

#include <gtest/gtest.h>

#include <sstream>

#include "gto/mc/solver.h"

namespace gto::mc {
namespace {

/// Kuhn poker with n players: a deck of n + 1 cards, one card each, ante 1,
/// one bet of 1 chip and no raises.
GameConfig kuhn(int players) {
  GameConfig c;
  c.num_players = players;
  c.toy_deck_size = players + 1;
  c.start_street = Street::River;
  c.last_street = Street::River;
  c.stacks.assign(static_cast<std::size_t>(players), 10.0);
  c.dead_pot = players;  // the antes
  c.sizing[3].sizing.bet = {1.0 / players};  // 1 chip
  c.raise_cap = 0;
  c.allin_threshold = 0;
  return c;
}

TEST(McKuhn, TreeIsKuhn) {
  const auto tree = build_tree(kuhn(2));
  ASSERT_TRUE(tree);
  EXPECT_EQ(tree->size(), 9u);  // 4 decisions, 5 terminals
  const auto tree3 = build_tree(kuhn(3));
  ASSERT_TRUE(tree3);
  EXPECT_EQ(tree3->num_action_nodes(), 12u);
}

TEST(McKuhn, TwoPlayerConvergesToTheGameValue) {
  std::string error;
  auto solver = Solver::create(kuhn(2), {.threads = 1, .seed = 3}, &error);
  ASSERT_TRUE(solver) << error;

  solver->run(2000);
  const auto early = solver->evaluate_exact();
  ASSERT_TRUE(early);
  solver->run(298'000);
  const auto late = solver->evaluate_exact();
  ASSERT_TRUE(late);
  EXPECT_EQ(late->accuracy, Accuracy::Exact);
  EXPECT_TRUE(late->nash_guarantee);
  EXPECT_LT(late->exploitability, early->exploitability);
  EXPECT_LT(late->exploitability, 0.01);
  // Values net of the ante: the first player loses 1/18 per hand.
  EXPECT_NEAR(late->ev[0] - 1.0, -1.0 / 18.0, 0.01);
  EXPECT_NEAR(late->ev[1] - 1.0, 1.0 / 18.0, 0.01);

  // The second player after a check: bluffs the jack a third, always bets the king.
  const auto check = solver->report("check", {}, &error);
  ASSERT_TRUE(check) << error;
  EXPECT_NEAR(check->frequency(1, 0), 1.0 / 3.0, 0.05);
  EXPECT_NEAR(check->frequency(1, 2), 1.0, 0.03);
  // Facing a bet: calls the queen a third, never the jack.
  const auto bet = solver->report("bet1", {}, &error);
  ASSERT_TRUE(bet) << error;
  EXPECT_NEAR(bet->frequency(1, 0), 0.0, 0.03);
  EXPECT_NEAR(bet->frequency(1, 1), 1.0 / 3.0, 0.05);

  // The sampled estimate agrees with the exact numbers.
  const Evaluation estimate = solver->evaluate({.ev_samples = 400'000, .br_samples = 40'000});
  EXPECT_EQ(estimate.accuracy, Accuracy::Estimated);
  EXPECT_NEAR(estimate.ev[0], late->ev[0], 4 * estimate.ev_std_error[0] + 1e-3);
  EXPECT_LT(estimate.exploitability, 0.03);
  EXPECT_GE(estimate.br_gain[0] + 4 * estimate.br_gain_std_error[0], late->br_gain[0] - 0.01);
}

TEST(McKuhn, ThreePlayerReachesTheEquilibriumFamily) {
  std::string error;
  auto solver = Solver::create(kuhn(3), {.threads = 1, .seed = 5}, &error);
  ASSERT_TRUE(solver) << error;
  solver->run(5000);
  const auto early = solver->evaluate_exact();
  ASSERT_TRUE(early);
  solver->run(995'000);
  const auto late = solver->evaluate_exact();
  ASSERT_TRUE(late);
  EXPECT_FALSE(late->nash_guarantee);
  EXPECT_LT(late->total_gain, early->total_gain);
  // An approximate Nash equilibrium: nobody gains much by deviating.
  for (int p = 0; p < 3; ++p) EXPECT_LT(late->br_gain[static_cast<std::size_t>(p)], 0.01) << p;
  // Szafron, Gibson & Sturtevant (2013) describe a family of equilibria in
  // which one player's value is fixed at -1/48 per hand while the other two
  // trade utility (one gets -1/48 - d, the other 1/24 + d, d >= 0). Our
  // exact evaluation of the solved profile (an epsilon-equilibrium by the
  // gains above) shows exactly this structure, with the fixed -1/48 for the
  // second player to act; the first player's value varies with the seed
  // between about -0.026 and -0.030, the third player gets 1/24 + d.
  EXPECT_NEAR(late->ev[1] - 1.0, -1.0 / 48.0, 0.002);
  EXPECT_LE(late->ev[0] - 1.0, -1.0 / 48.0 + 0.002);
  EXPECT_GE(late->ev[2] - 1.0, 1.0 / 24.0 - 0.002);
  // Zero-sum: values net of antes add up to zero.
  EXPECT_NEAR(late->ev[0] + late->ev[1] + late->ev[2] - 3.0, 0.0, 1e-6);
}

TEST(McKuhn, OneThreadIsDeterministicAndStateRoundTrips) {
  auto a = Solver::create(kuhn(3), {.threads = 1, .seed = 9});
  auto b = Solver::create(kuhn(3), {.threads = 1, .seed = 9});
  ASSERT_TRUE(a && b);
  a->run(20'000);
  b->run(20'000);
  std::stringstream sa, sb;
  a->save(sa);
  b->save(sb);
  EXPECT_EQ(sa.str(), sb.str());

  auto c = Solver::create(kuhn(3), {.threads = 1, .seed = 1});
  ASSERT_TRUE(c);
  std::string error;
  ASSERT_TRUE(c->load(sa, &error)) << error;
  EXPECT_EQ(c->iterations(), 20'000u);
  std::stringstream sc;
  c->save(sc);
  EXPECT_EQ(sc.str(), sb.str());

  // A differently shaped game refuses the state.
  auto d = Solver::create(kuhn(2), {.threads = 1});
  ASSERT_TRUE(d);
  std::stringstream again(sb.str());
  EXPECT_FALSE(d->load(again, &error));
  // So does damaged data.
  std::stringstream broken(sb.str().substr(0, 30));
  EXPECT_FALSE(c->load(broken, &error));
  EXPECT_EQ(c->iterations(), 20'000u);
}

}  // namespace
}  // namespace gto::mc
