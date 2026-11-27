// Hand abstraction of the mc engine, and solving across streets and preflop.

#include <gtest/gtest.h>

#include <cmath>
#include <numeric>

#include "gto/mc/abstraction.h"
#include "gto/mc/solver.h"
#include "gto/mc/spot.h"

namespace gto::mc {
namespace {

Card card(const char* text) { return *Card::parse(text); }

TEST(McAbstraction, HoldemClasses) {
  EXPECT_EQ(holdem_class(card("As"), card("Ah")), 0);
  EXPECT_EQ(holdem_class(card("As"), card("Ks")), 1);
  EXPECT_EQ(holdem_class(card("Kd"), card("Ac")), 13);
  EXPECT_EQ(holdem_class(card("2s"), card("2h")), 168);
  EXPECT_EQ(holdem_class_name(1), "AKs");
  EXPECT_EQ(holdem_class_name(13), "AKo");
  EXPECT_EQ(holdem_class_name(168), "22");
  int total = 0;
  for (int c = 0; c < 169; ++c) total += holdem_class_combos(c);
  EXPECT_EQ(total, 1326);
}

TEST(McAbstraction, HandStrengthWithCardRemoval) {
  const auto board = *parse_cards("AsKsQs7d2c");
  std::vector<float> hs(1326);
  holdem_hand_strength(board, hs.data());
  EXPECT_FLOAT_EQ(hs[static_cast<std::size_t>(Range::combo_index(card("Js"), card("Ts")))], 1.0f);
  EXPECT_FLOAT_EQ(hs[static_cast<std::size_t>(Range::combo_index(card("As"), card("Ah")))], -1.0f);
  const float trash = hs[static_cast<std::size_t>(Range::combo_index(card("3h"), card("4d")))];
  EXPECT_LT(trash, 0.05f);
  // A board that plays: everybody ties.
  const auto royal = *parse_cards("AsKsQsJsTs");
  holdem_hand_strength(royal, hs.data());
  EXPECT_FLOAT_EQ(hs[static_cast<std::size_t>(Range::combo_index(card("2c"), card("3d")))], 0.5f);

  const auto table = holdem_strength_table(board, 8, 2);
  EXPECT_EQ(table[static_cast<std::size_t>(Range::combo_index(card("Js"), card("Ts")))], 15);
  EXPECT_LT(table[static_cast<std::size_t>(Range::combo_index(card("3h"), card("4d")))], 2);

  // Flop: the nuts sits in the top strength bin, air in the bottom one.
  const auto flop = *parse_cards("Kh7s2d");
  const auto ft = holdem_strength_table(flop, 8, 4);
  EXPECT_GE(ft[static_cast<std::size_t>(Range::combo_index(card("Kd"), card("Kc")))] / 4, 7);
  EXPECT_EQ(ft[static_cast<std::size_t>(Range::combo_index(card("3c"), card("4d")))] / 4, 0);
}

TEST(McAbstraction, AutoPicksLosslessWhenItFits) {
  GameConfig c;
  c.num_players = 2;
  c.start_street = Street::Flop;
  c.board = *parse_cards("Kh7s2d");
  c.stacks = {50, 50};
  c.dead_pot = 10;
  auto plan = plan_abstraction(c, live_counts(c));
  EXPECT_EQ(plan[1].kind, AbstractionKind::Lossless);
  EXPECT_EQ(plan[1].buckets[0], 1176u);
  EXPECT_EQ(plan[2].kind, AbstractionKind::Strength);  // 1176 x 49 > 16384
  EXPECT_EQ(plan[3].kind, AbstractionKind::Strength);
  EXPECT_EQ(plan[3].buckets[0], 128u);

  const Range narrow = *Range::parse("AA,KK,QQ,AK,KQs,76s,T9s");
  c.ranges = {std::vector<float>(narrow.weights().begin(), narrow.weights().end()),
              std::vector<float>(narrow.weights().begin(), narrow.weights().end())};
  const auto live = live_counts(c);
  plan = plan_abstraction(c, live);
  EXPECT_EQ(plan[2].kind, AbstractionKind::Lossless);
  EXPECT_EQ(plan[2].buckets[0], live[0] * 49);
  EXPECT_EQ(plan[3].kind, AbstractionKind::Strength);  // x 49 x 49 does not fit
}

TEST(McStreets, FlopSpotWithTurnAndRiver) {
  MultiwaySpot mw;
  mw.board = *parse_cards("Kh7s2d");
  const Range r = *Range::parse("AA,KK,77,AK,KQs,QJs,JTs,T9s,98s,A5s,66");
  mw.ranges = {r, r};
  mw.pot = 10;
  mw.stacks = {40, 40};
  for (int s = 1; s < 4; ++s) mw.sizing[static_cast<std::size_t>(s)].sizing.bet = {0.66};
  mw.sizing[1].sizing.raise = {1.0};
  mw.raise_cap = 1;
  mw.abstraction.streets[3].strength_bins = 16;
  mw.abstraction.streets[3].potential_bins = 1;
  std::string error;
  const auto estimate = estimate_spot(mw, &error);
  ASSERT_TRUE(estimate) << error;
  auto solver = make_solver(mw, {.seed = 8}, &error);
  ASSERT_TRUE(solver) << error;
  EXPECT_EQ(solver->layout().info_sets(), estimate->info_sets);
  EXPECT_EQ(solver->abstraction().plan()[2].kind, AbstractionKind::Lossless);
  solver->run(300'000);

  const Evaluation e = solver->evaluate({.ev_samples = 100'000, .best_response = false});
  EXPECT_NEAR(e.ev[0] + e.ev[1], 10.0, 1e-6);

  const auto turn = solver->report("check/check/Th", {.ev_samples = 50}, &error);
  ASSERT_TRUE(turn) << error;
  EXPECT_EQ(turn->street, Street::Turn);
  ASSERT_EQ(turn->board.size(), 4u);
  // Strategies add up to one for every hand that can be here.
  const int aa = Range::combo_index(card("Ac"), card("Ad"));
  double total = 0;
  for (std::size_t a = 0; a < turn->actions.size(); ++a) total += turn->frequency(a, aa);
  EXPECT_NEAR(total, 1.0, 1e-5);
  // Hands holding the turn card cannot be here.
  const int blocked = Range::combo_index(card("Th"), card("9h"));
  EXPECT_EQ(turn->reach[0][static_cast<std::size_t>(blocked)], 0.0f);
  EXPECT_FALSE(std::isnan(turn->range_ev[0]));
  // Unnamed cards cannot be reported below a chance node.
  EXPECT_FALSE(solver->report("check/check/*", {}, &error));
}

TEST(McStreets, HeadsUpPreflopPushOrFold) {
  GameConfig c = GameConfig::preflop_game(2, 15);
  c.preflop.sb_open_bb = {2};
  c.preflop.open_bb = {2};
  c.preflop.allin_every_level = true;
  c.preflop.raise_cap = 2;
  std::string error;
  auto solver = Solver::create(c, {.seed = 12}, &error);
  ASSERT_TRUE(solver) << error;
  solver->run(3'000'000);
  const Evaluation e = solver->evaluate({.ev_samples = 200'000, .best_response = false});
  EXPECT_NEAR(e.ev[0] + e.ev[1], 1.5, 1e-6);

  const auto root = solver->report("", {}, &error);
  ASSERT_TRUE(root) << error;
  const ClassStrategy classes = preflop_class_strategy(*root);
  ASSERT_EQ(classes.names.size(), 169u);
  ASSERT_EQ(classes.actions[0], "fold");
  const std::size_t na = classes.actions.size();
  for (std::size_t cls = 0; cls < 169; ++cls) {
    float sum = 0;
    for (std::size_t a = 0; a < na; ++a) sum += classes.strategy[cls * na + a];
    EXPECT_NEAR(sum, 1.0f, 1e-4) << classes.names[cls];
  }
  EXPECT_LT(classes.strategy[0 * na + 0], 0.02f);  // AA never folds

  // The BB facing a shove calls aces and folds 32o.
  const auto shove = solver->report("allin", {}, &error);
  ASSERT_TRUE(shove) << error;
  const ClassStrategy facing = preflop_class_strategy(*shove);
  ASSERT_EQ(facing.actions, (std::vector<std::string>{"fold", "call"}));
  EXPECT_GT(facing.strategy[0 * 2 + 1], 0.95f);
  const std::size_t worst = 12 * 13 + 11;  // 32o
  EXPECT_EQ(facing.names[worst], "32o");
  EXPECT_GT(facing.strategy[worst * 2 + 0], 0.8f);
}

}  // namespace
}  // namespace gto::mc
