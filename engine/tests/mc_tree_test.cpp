// Betting tree shapes of the mc engine.

#include <gtest/gtest.h>

#include "gto/mc/solver.h"
#include "gto/mc/tree.h"

namespace gto::mc {
namespace {

std::vector<std::string> labels(const BettingTree& tree, std::uint32_t node) {
  std::vector<std::string> out;
  for (const Action& a : tree.actions(node)) out.push_back(action_label(a));
  return out;
}

std::uint32_t at(const BettingTree& tree, std::string_view path, std::span<const Card> board = {}) {
  std::string error;
  const auto found = tree.find(path, board, &error);
  EXPECT_TRUE(found.has_value()) << path << ": " << error;
  return found ? found->node : 0;
}

GameConfig river(int players, double pot, double stack) {
  GameConfig c;
  c.num_players = players;
  c.start_street = Street::River;
  c.last_street = Street::River;
  c.board = *parse_cards("AsKsQs7d2c");
  c.stacks.assign(static_cast<std::size_t>(players), stack);
  c.dead_pot = pot;
  return c;
}

void expect_counts_match(const GameConfig& config) {
  const auto tree = build_tree(config);
  ASSERT_TRUE(tree);
  const auto counts = count_tree(config);
  ASSERT_TRUE(counts);
  EXPECT_EQ(counts->nodes, tree->size());
  EXPECT_EQ(counts->action_nodes, tree->num_action_nodes());
  EXPECT_EQ(counts->terminals, tree->num_terminals());
  std::uint64_t decisions = 0;
  for (const auto& street : counts->decisions) {
    for (const auto d : street) decisions += d;
  }
  EXPECT_EQ(decisions, tree->num_action_nodes());
}

TEST(McTree, HeadsUpRiverOneBetSize) {
  GameConfig c = river(2, 10, 20);
  c.sizing[3].sizing.bet = {0.5};
  const auto tree = build_tree(c);
  ASSERT_TRUE(tree);
  EXPECT_EQ(tree->size(), 9u);  // 4 decisions, 5 terminals
  EXPECT_EQ(tree->num_action_nodes(), 4u);
  EXPECT_EQ(labels(*tree, 0), (std::vector<std::string>{"check", "bet5"}));
  EXPECT_EQ(labels(*tree, at(*tree, "bet5")), (std::vector<std::string>{"fold", "call"}));
  EXPECT_EQ(labels(*tree, at(*tree, "check")), (std::vector<std::string>{"check", "bet5"}));
  const std::uint32_t call = at(*tree, "check/bet5/call");
  EXPECT_EQ(tree->node(call).kind, NodeKind::Showdown);
  EXPECT_DOUBLE_EQ(tree->pot(call), 20);
  EXPECT_EQ(tree->node(at(*tree, "bet5/fold")).kind, NodeKind::FoldOut);
  EXPECT_EQ(tree->path(call), "check/bet5/call");
  expect_counts_match(c);
}

TEST(McTree, ThreeWayRiverOneBetNoRaises) {
  GameConfig c = river(3, 30, 100);
  c.sizing[3].sizing.bet = {1.0};
  c.allin_threshold = 0;
  const auto tree = build_tree(c);
  ASSERT_TRUE(tree);
  EXPECT_EQ(tree->size(), 25u);
  EXPECT_EQ(tree->num_action_nodes(), 12u);
  EXPECT_EQ(tree->num_terminals(), 13u);
  // Second player facing a bet, third player behind: fold or call.
  EXPECT_EQ(labels(*tree, at(*tree, "bet30")), (std::vector<std::string>{"fold", "call"}));
  // Bet, fold, fold: everybody but the bettor folded.
  const std::uint32_t out = at(*tree, "bet30/fold/fold");
  EXPECT_EQ(tree->node(out).kind, NodeKind::FoldOut);
  EXPECT_EQ(tree->node(out).folded, 0b110);
  // Bet, call, fold: showdown of two with the folder's chips... none in.
  const std::uint32_t sd = at(*tree, "bet30/call/fold");
  EXPECT_EQ(tree->node(sd).kind, NodeKind::Showdown);
  EXPECT_DOUBLE_EQ(tree->pot(sd), 90);
  expect_counts_match(c);
}

TEST(McTree, RaisesAndAllInThreshold) {
  GameConfig c = river(2, 10, 30);
  c.sizing[3].sizing.bet = {0.5};
  c.sizing[3].sizing.raise = {1.0};
  c.raise_cap = 2;
  const auto tree = build_tree(c);
  ASSERT_TRUE(tree);
  // Facing bet5 (pot 15): pot-size raise = 5 + 1.0 * 20 = 25, which puts in
  // 25 of 30 remaining (> 67%) and becomes all-in.
  EXPECT_EQ(labels(*tree, at(*tree, "bet5")), (std::vector<std::string>{"fold", "call", "allin"}));
  EXPECT_EQ(labels(*tree, at(*tree, "bet5/allin")), (std::vector<std::string>{"fold", "call"}));
  expect_counts_match(c);
}

TEST(McTree, PotLimitCapsBetsAndRaises) {
  GameConfig c = river(2, 10, 100);
  c.pot_limit = true;
  c.sizing[3].sizing.bet = {2.0};
  c.sizing[3].sizing.raise = {3.0};
  c.sizing[3].sizing.add_allin = true;
  const auto tree = build_tree(c);
  ASSERT_TRUE(tree);
  EXPECT_EQ(labels(*tree, 0), (std::vector<std::string>{"check", "bet10"}));
  // Facing 10 into 20: the pot-size raise is to 10 + (20 + 10) = 40.
  EXPECT_EQ(labels(*tree, at(*tree, "bet10")), (std::vector<std::string>{"fold", "call", "raise40"}));
}

TEST(McTree, PerPlayerSizing) {
  GameConfig c = river(3, 30, 100);
  c.sizing[3].sizing.bet = {1.0};
  c.sizing[3].per_player.resize(3);
  BetSizing small;
  small.bet = {0.5};
  c.sizing[3].per_player[1] = small;
  const auto tree = build_tree(c);
  ASSERT_TRUE(tree);
  EXPECT_EQ(labels(*tree, 0), (std::vector<std::string>{"check", "bet30"}));
  EXPECT_EQ(labels(*tree, at(*tree, "check")), (std::vector<std::string>{"check", "bet15"}));
}

TEST(McTree, HeadsUpPreflopBlindsLimpAndRaiseLevels) {
  GameConfig c = GameConfig::preflop_game(2, 100);
  c.preflop.sb_open_bb = {3};
  c.preflop.open_bb = {2.5};
  c.preflop.threebet_oop = {4};
  c.preflop.threebet_ip = {3};
  c.preflop.fourbet = {2.3};
  const auto tree = build_tree(c);
  ASSERT_TRUE(tree);
  EXPECT_EQ(tree->node(0).player, 0);  // the SB acts first preflop
  EXPECT_EQ(labels(*tree, 0), (std::vector<std::string>{"fold", "call", "raise3"}));
  // The BB gets the option after a limp: check or raise (2.5 + 1 per limper).
  const std::uint32_t option = at(*tree, "call");
  EXPECT_EQ(tree->node(option).player, 1);
  EXPECT_EQ(labels(*tree, option), (std::vector<std::string>{"check", "raise3.5"}));
  // Last street preflop: check ends in an all-in equity showdown.
  EXPECT_EQ(tree->node(at(*tree, "call/check")).kind, NodeKind::Showdown);
  // The BB is out of position against the SB: 4x 3-bet.
  EXPECT_EQ(labels(*tree, at(*tree, "raise3")), (std::vector<std::string>{"fold", "call", "raise12"}));
  EXPECT_EQ(labels(*tree, at(*tree, "raise3/raise12")),
            (std::vector<std::string>{"fold", "call", "raise27.6"}));
  // 5-bet: all-in only.
  EXPECT_EQ(labels(*tree, at(*tree, "raise3/raise12/raise27.6")),
            (std::vector<std::string>{"fold", "call", "allin"}));
  EXPECT_EQ(labels(*tree, at(*tree, "raise3/raise12/raise27.6/allin")),
            (std::vector<std::string>{"fold", "call"}));
  EXPECT_EQ(tree->node(at(*tree, "fold")).kind, NodeKind::FoldOut);
  expect_counts_match(c);

  // Raise cap 2: no 4-bet.
  c.preflop.raise_cap = 2;
  const auto capped = build_tree(c);
  ASSERT_TRUE(capped);
  EXPECT_EQ(labels(*capped, at(*capped, "raise3/raise12")), (std::vector<std::string>{"fold", "call"}));
}

TEST(McTree, ThreeHandedPreflopOrderAndColdCalls) {
  GameConfig c = GameConfig::preflop_game(3, 100);  // BTN, SB, BB
  c.preflop.open_bb = {2.5};
  c.preflop.sb_open_bb = {3};
  c.preflop.threebet_ip = {3};
  c.preflop.threebet_oop = {4};
  const auto tree = build_tree(c);
  ASSERT_TRUE(tree);
  EXPECT_EQ(tree->node(0).player, 0);
  EXPECT_EQ(labels(*tree, 0), (std::vector<std::string>{"fold", "raise2.5"}));  // no limping
  EXPECT_EQ(tree->node(at(*tree, "fold")).player, 1);
  EXPECT_EQ(labels(*tree, at(*tree, "fold")), (std::vector<std::string>{"fold", "call", "raise3"}));
  EXPECT_EQ(tree->node(at(*tree, "fold/fold")).kind, NodeKind::FoldOut);
  // SB facing the BTN open: blinds may call; a 3-bet OOP is 4x.
  EXPECT_EQ(labels(*tree, at(*tree, "raise2.5")), (std::vector<std::string>{"fold", "call", "raise10"}));
  // BB after SB 3-bet: a cold call of the 3-bet is not allowed by default.
  EXPECT_EQ(labels(*tree, at(*tree, "raise2.5/raise10")), (std::vector<std::string>{"fold", "raise23"}));
  // BB squeeze over open + SB call: (4 + 1 per caller) x 2.5.
  EXPECT_EQ(labels(*tree, at(*tree, "raise2.5/call")), (std::vector<std::string>{"fold", "call", "raise12.5"}));
  // The opener may call a 3-bet.
  EXPECT_EQ(labels(*tree, at(*tree, "raise2.5/raise10/fold")), (std::vector<std::string>{"fold", "call", "raise23"}));
  expect_counts_match(c);
}

TEST(McTree, SixMaxPreflopCountsMatch) {
  GameConfig c = GameConfig::preflop_game(6, 100);
  c.preflop.open_bb = {2.5};
  c.preflop.sb_open_bb = {3};
  c.preflop.threebet_ip = {3};
  c.preflop.threebet_oop = {4};
  c.preflop.fourbet = {2.3};
  expect_counts_match(c);
}

TEST(McTree, StreetsChanceNodesAndAllInRunouts) {
  GameConfig c;
  c.num_players = 2;
  c.start_street = Street::Flop;
  c.last_street = Street::River;
  c.board = *parse_cards("Ks7d2c");
  c.stacks = {15, 15};
  c.dead_pot = 10;
  for (int s = 1; s < 4; ++s) c.sizing[static_cast<std::size_t>(s)].sizing.bet = {0.5};
  c.sizing[1].sizing.add_allin = true;
  const auto tree = build_tree(c);
  ASSERT_TRUE(tree);
  const std::uint32_t chance = at(*tree, "check/check");
  EXPECT_EQ(tree->node(chance).kind, NodeKind::Chance);
  std::string error;
  const auto turn = tree->find("check/check/Th", c.board, &error);
  ASSERT_TRUE(turn) << error;
  EXPECT_EQ(tree->node(turn->node).street, Street::Turn);
  ASSERT_EQ(turn->dealt.size(), 1u);
  EXPECT_EQ(tree->path(turn->node, turn->dealt), "check/check/Th");
  EXPECT_FALSE(tree->find("check/check/Ks", c.board));    // on the board
  EXPECT_FALSE(tree->find("check/check/ThJh", c.board));  // one card for the turn
  // All-in and call on the flop: straight to showdown, no chance nodes.
  const std::uint32_t runout = at(*tree, "allin/call");
  EXPECT_EQ(tree->node(runout).kind, NodeKind::Showdown);
  EXPECT_EQ(tree->node(runout).street, Street::Flop);
  EXPECT_EQ(tree->node(runout).allin, 0b11);
  expect_counts_match(c);

  // A preflop game with a flop: the flop is one path entry of three cards.
  GameConfig p = GameConfig::preflop_game(2, 20);
  p.last_street = Street::Flop;
  p.sizing[1].sizing.bet = {0.5};
  const auto pt = build_tree(p);
  ASSERT_TRUE(pt);
  const auto flop = pt->find("call/check/Ks7d2c", {}, &error);
  ASSERT_TRUE(flop) << error;
  EXPECT_EQ(pt->node(flop->node).street, Street::Flop);
  EXPECT_EQ(pt->node(flop->node).player, 1);  // the BB acts first postflop heads-up
  EXPECT_EQ(labels(*pt, flop->node), (std::vector<std::string>{"check", "bet1"}));
}

TEST(McTree, NodeLimitAndEstimate) {
  GameConfig c = GameConfig::preflop_game(6, 100);
  c.max_nodes = 100;
  EXPECT_FALSE(build_tree(c));
  const auto counts = count_tree(c);
  ASSERT_TRUE(counts);
  EXPECT_TRUE(counts->too_large);

  GameConfig r = river(3, 30, 100);
  r.sizing[3].sizing.bet = {1.0};
  r.allin_threshold = 0;
  const auto e = estimate_game(r);
  ASSERT_TRUE(e);
  // Lossless river buckets: every live hand (1326 - C(5)... combos off the board).
  EXPECT_EQ(e->live_hands[0], 1081u);
  EXPECT_EQ(e->abstraction[3].kind, AbstractionKind::Lossless);
  EXPECT_EQ(e->info_sets, 12u * 1081u);
  EXPECT_EQ(e->table_entries, 2u * 12u * 1081u);
  std::string error;
  const auto solver = Solver::create(r, {.threads = 1}, &error);
  ASSERT_TRUE(solver) << error;
  EXPECT_EQ(solver->layout().info_sets(), e->info_sets);
  EXPECT_EQ(solver->layout().size(), e->table_entries);
}

}  // namespace
}  // namespace gto::mc
