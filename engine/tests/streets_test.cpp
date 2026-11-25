// Spots with cards still to come: chance nodes, isomorphism, threads.

#include <cmath>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "gto/solver.h"

namespace gto {
namespace {

constexpr std::size_t kFlop = static_cast<std::size_t>(Street::Flop);
constexpr std::size_t kTurn = static_cast<std::size_t>(Street::Turn);
constexpr std::size_t kRiver = static_cast<std::size_t>(Street::River);

// ---------------------------------------------------------------------------
// Leduc poker: six cards (J, Q, K in two suits), one private card each, ante 1.
// A betting round with bets of 2, one public card, a round with bets of 4.
// At most a bet and one raise per round. A pair with the public card wins.
// ---------------------------------------------------------------------------

class LeducBuilder {
 public:
  GameTree build() {
    tree_.pot = 2;
    betting(0, {0, 0}, {0, 0}, kOop, 0, kNoCard);
    for (std::uint8_t card = 0; card < 6; ++card) tree_.runouts.push_back({card, kNoCard});
    return std::move(tree_);
  }

 private:
  using Chips = std::array<double, 2>;
  static constexpr double kBet[2] = {2, 4};

  std::uint32_t betting(int round, Chips committed, Chips street_bet, int player, int bets,
                        std::uint8_t public_card) {
    Node node;
    node.type = NodeType::Action;
    node.player = static_cast<std::uint8_t>(player);
    node.committed = committed;
    const std::uint32_t index = tree_.add_node(node);

    const auto p = static_cast<std::size_t>(player);
    const double to_call = street_bet[1 - p] - street_bet[p];
    std::vector<Action> actions;
    std::vector<std::uint32_t> children;

    if (to_call == 0) {
      actions.push_back({ActionType::Check, 0});
      children.push_back(player == kOop
                             ? betting(round, committed, street_bet, kIp, bets, public_card)
                             : end_round(round, committed, public_card));

      Chips after = committed, bet = street_bet;
      after[p] += kBet[round];
      bet[p] = kBet[round];
      actions.push_back({ActionType::Bet, bet[p]});
      children.push_back(betting(round, after, bet, 1 - player, 1, public_card));
    } else {
      Node fold;
      fold.type = NodeType::Fold;
      fold.player = static_cast<std::uint8_t>(player);
      fold.committed = committed;
      actions.push_back({ActionType::Fold, 0});
      children.push_back(tree_.add_node(fold));

      Chips called = committed;
      called[p] += to_call;
      actions.push_back({ActionType::Call, street_bet[1 - p]});
      children.push_back(end_round(round, called, public_card));

      if (bets < 2) {
        Chips after = committed, bet = street_bet;
        bet[p] = street_bet[1 - p] + kBet[round];
        after[p] += bet[p] - street_bet[p];
        actions.push_back({ActionType::Raise, bet[p]});
        children.push_back(betting(round, after, bet, 1 - player, 2, public_card));
      }
    }
    tree_.set_actions(index, actions, children);
    return index;
  }

  std::uint32_t end_round(int round, Chips committed, std::uint8_t public_card) {
    Node node;
    node.committed = committed;
    if (round == 1) {
      node.type = NodeType::Showdown;
      node.aux = public_card;  // runout index
      return tree_.add_node(node);
    }
    node.type = NodeType::Chance;
    const std::uint32_t index = tree_.add_node(node);
    std::vector<std::uint32_t> children;
    for (std::uint8_t card = 0; card < 6; ++card) {
      const std::uint32_t child = betting(1, committed, {0, 0}, kOop, 0, card);
      tree_.nodes[child].card = card;
      children.push_back(child);
    }
    tree_.set_deals(index, children);
    return index;
  }

  GameTree tree_;
};

Game leduc_game() {
  Game game;
  game.tree = LeducBuilder().build();
  for (std::size_t player = 0; player < 2; ++player) {
    for (int card = 0; card < 6; ++card) game.hands[player].push_back(make_hand({card}, 1.0f));
  }
  for (int public_card = 0; public_card < 6; ++public_card) {
    std::array<std::vector<HandStrength>, 2> strength;
    for (std::size_t player = 0; player < 2; ++player) {
      for (int card = 0; card < 6; ++card) {
        const int rank = card / 2;
        strength[player].push_back(
            static_cast<HandStrength>(rank == public_card / 2 ? 100 + rank : rank));
      }
    }
    game.showdowns.push_back(make_showdown_table(std::move(strength)));
  }
  return game;
}

TEST(Leduc, ReachesTheKnownGameValue) {
  Solver solver(leduc_game());
  SolveOptions options;
  options.max_iterations = 3000;
  options.target_exploitability_pct = 0;
  options.check_interval = 3000;
  const SolveResult result = solver.solve(options);

  EXPECT_LT(result.evaluation.exploitability, 0.003);
  // The first player loses 0.0856 antes per hand at equilibrium. In pot-share
  // terms: 1 - 0.0856 of the 2-chip pot.
  EXPECT_NEAR(result.evaluation.ev[kOop], 1.0 - 0.0856, 0.003);
  EXPECT_NEAR(result.evaluation.ev[kIp], 1.0 + 0.0856, 0.003);
}

TEST(Leduc, SameResultOnEveryThreadCount) {
  SolveOptions options;
  options.max_iterations = 200;
  options.target_exploitability_pct = 0;
  options.check_interval = 200;

  Solver single(leduc_game(), SolverConfig{1});
  Solver multi(leduc_game(), SolverConfig{4});
  const SolveResult a = single.solve(options);
  const SolveResult b = multi.solve(options);
  // Cards are summed in a fixed order, so the result does not depend on timing.
  EXPECT_EQ(a.evaluation.ev[0], b.evaluation.ev[0]);
  EXPECT_EQ(a.evaluation.exploitability, b.evaluation.exploitability);
}

// ---------------------------------------------------------------------------
// Hold'em
// ---------------------------------------------------------------------------

/// OOP's share of the pot if both players check to showdown, by enumerating
/// every pair of hands and every way to complete the board.
double brute_force_share(const std::vector<Card>& board, const Range& oop, const Range& ip) {
  std::uint64_t board_mask = 0;
  for (const Card card : board) board_mask |= card.mask();

  double weighted_share = 0, total_weight = 0;
  std::vector<Card> cards;
  for (int i = 0; i < Range::kNumCombos; ++i) {
    if (!(oop.weight(i) > 0)) continue;
    const auto [a1, a2] = Range::combo_cards(i);
    const std::uint64_t mine = a1.mask() | a2.mask();
    if ((mine & board_mask) != 0) continue;
    for (int j = 0; j < Range::kNumCombos; ++j) {
      if (!(ip.weight(j) > 0)) continue;
      const auto [b1, b2] = Range::combo_cards(j);
      const std::uint64_t theirs = b1.mask() | b2.mask();
      if ((theirs & (board_mask | mine)) != 0) continue;

      const std::uint64_t used = board_mask | mine | theirs;
      double share = 0;
      int runouts = 0;
      const auto showdown = [&](std::uint64_t extra_cards, Card x, Card y, int extra) {
        (void)extra_cards;
        cards = board;
        if (extra >= 1) cards.push_back(x);
        if (extra >= 2) cards.push_back(y);
        const std::size_t n = cards.size();
        cards.push_back(a1);
        cards.push_back(a2);
        const HandStrength s1 = evaluate_hand(cards);
        cards[n] = b1;
        cards[n + 1] = b2;
        const HandStrength s2 = evaluate_hand(cards);
        share += s1 > s2 ? 1.0 : s1 == s2 ? 0.5 : 0.0;
        ++runouts;
      };

      const int missing = 5 - static_cast<int>(board.size());
      if (missing == 0) {
        showdown(0, Card(), Card(), 0);
      } else {
        for (int x = 0; x < 52; ++x) {
          const Card cx(static_cast<std::uint8_t>(x));
          if ((used & cx.mask()) != 0) continue;
          if (missing == 1) {
            showdown(0, cx, Card(), 1);
            continue;
          }
          for (int y = x + 1; y < 52; ++y) {
            const Card cy(static_cast<std::uint8_t>(y));
            if ((used & cy.mask()) != 0) continue;
            showdown(0, cx, cy, 2);
          }
        }
      }
      const double weight = static_cast<double>(oop.weight(i)) * ip.weight(j);
      weighted_share += weight * share / runouts;
      total_weight += weight;
    }
  }
  return weighted_share / total_weight;
}

SpotConfig check_down(std::string_view board, std::string_view oop, std::string_view ip) {
  SpotConfig spot;
  spot.board = *parse_cards(board);
  spot.ranges[kOop] = *Range::parse(oop);
  spot.ranges[kIp] = *Range::parse(ip);
  spot.tree.pot = 100;
  spot.tree.effective_stack = 0;  // nothing to bet: the hand is checked down
  return spot;
}

struct EquityCase {
  const char* board;
  const char* oop;
  const char* ip;
};

class CheckDown : public ::testing::TestWithParam<EquityCase> {};

TEST_P(CheckDown, EvEqualsEquity) {
  const EquityCase& c = GetParam();
  for (const bool isomorphism : {true, false}) {
    SCOPED_TRACE(isomorphism ? "with isomorphism" : "without isomorphism");
    SpotConfig spot = check_down(c.board, c.oop, c.ip);
    spot.use_isomorphism = isomorphism;
    std::string error;
    auto game = make_game(spot, &error);
    ASSERT_TRUE(game.has_value()) << error;

    Solver solver(std::move(*game));
    const Evaluation evaluation = solver.evaluate();
    const double expected =
        100.0 * brute_force_share(spot.board, spot.ranges[kOop], spot.ranges[kIp]);
    EXPECT_NEAR(evaluation.ev[kOop], expected, 0.01);
    EXPECT_NEAR(evaluation.ev[kIp], 100.0 - expected, 0.01);
    EXPECT_NEAR(evaluation.exploitability, 0.0, 1e-3);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Streets, CheckDown,
    ::testing::Values(
        // River: no cards to come.
        EquityCase{"Ks8d5h3s2d", "AA,KQs,76s", "KK,AKo,65s"},
        // Turn, one hand each.
        EquityCase{"Qh7d3c2s", "AhAd", "KsKc"},
        // Turn with a flush draw against made hands, blockers both ways.
        EquityCase{"Kh9h4c2s", "AhQh,AA,KQo", "KK,99,JhTh,AKs"},
        // Turn, two-tone: clubs and diamonds are interchangeable.
        EquityCase{"KsTs7h4h", "AA,KK,AKs,QJs,98s", "QQ,JJ,AQs,T9s,76s"},
        // Flop, rainbow: no suits interchangeable.
        EquityCase{"Ks8d5h", "AA,KQs,76s", "KK,AKo,65s"},
        // Flop, monotone: three suits interchangeable.
        EquityCase{"Ks9s4s", "AA,KQs,QJs,87s", "KK,AQs,JTs,99"},
        // Flop, paired and two-tone, uneven weights.
        EquityCase{"8h8d3h", "AA:0.5,A8s,KQs,77", "99,A3s:0.25,QJs,T9s"},
        // Ranges that tell suits apart: nothing may be merged.
        EquityCase{"Ks9s4d", "AhKh,AcKc:0.5,QQ", "JJ,AsQs,7d6d"}));

TEST(HoldemTree, RainbowTurnDealsEveryRiver) {
  SpotConfig spot = check_down("Ks8d5h3c", "AA,KQs", "QQ,JTs");
  const auto game = make_game(spot);
  ASSERT_TRUE(game.has_value());
  const GameTree& tree = game->tree;

  const auto chance = tree.find("check/check");
  ASSERT_TRUE(chance.has_value());
  EXPECT_EQ(tree.nodes[chance->node].type, NodeType::Chance);
  // Each suit holds a different card, so no two suits are interchangeable.
  EXPECT_EQ(tree.children_of(chance->node).size(), 48u);
  EXPECT_TRUE(tree.iso_of(chance->node).empty());
  EXPECT_EQ(tree.runouts.size(), 48u);
}

TEST(HoldemTree, MonotoneFlopMergesThreeSuits) {
  SpotConfig spot = check_down("Ks9s4s", "AA,KQs,QJs", "KK,AQs,JTs");
  const auto game = make_game(spot);
  ASSERT_TRUE(game.has_value());
  const GameTree& tree = game->tree;

  const auto chance = tree.find("check/check");
  ASSERT_TRUE(chance.has_value());
  // Ten spades left, plus one representative suit of thirteen for the rest.
  EXPECT_EQ(tree.children_of(chance->node).size(), 23u);
  EXPECT_EQ(tree.iso_of(chance->node).size(), 26u);

  // A representative card is found as is; an interchangeable one is remapped.
  const auto clubs = tree.find("check/check/7c");
  ASSERT_TRUE(clubs.has_value());
  EXPECT_FALSE(clubs->remapped);
  const auto hearts = tree.find("check/check/7h");
  ASSERT_TRUE(hearts.has_value());
  EXPECT_TRUE(hearts->remapped);
  EXPECT_EQ(hearts->node, clubs->node);

  std::string error;
  EXPECT_FALSE(tree.find("check/check/Ks", &error).has_value());
  EXPECT_NE(error.find("in play"), std::string::npos) << error;
  EXPECT_FALSE(tree.find("check/check/check", &error).has_value());

  SpotConfig plain = spot;
  plain.use_isomorphism = false;
  const auto full = make_game(plain);
  ASSERT_TRUE(full.has_value());
  EXPECT_EQ(full->tree.children_of(full->tree.find("check/check")->node).size(), 49u);
  EXPECT_LT(tree.nodes.size(), full->tree.nodes.size() / 2);
}

TEST(HoldemTree, AllInBeforeTheRiverRunsOutTheBoard) {
  SpotConfig spot = check_down("Ks8d5h3c", "AA", "QQ");
  spot.tree.effective_stack = 50;
  spot.tree.sizing[kTurn][kOop].add_allin = true;
  const auto game = make_game(spot);
  ASSERT_TRUE(game.has_value());
  const GameTree& tree = game->tree;

  const auto chance = tree.find("allin/call");
  ASSERT_TRUE(chance.has_value());
  EXPECT_EQ(tree.nodes[chance->node].type, NodeType::Chance);
  for (const std::uint32_t child : tree.children_of(chance->node)) {
    EXPECT_EQ(tree.nodes[child].type, NodeType::Showdown);
    EXPECT_DOUBLE_EQ(tree.nodes[child].committed[kOop], 50);
    EXPECT_DOUBLE_EQ(tree.nodes[child].committed[kIp], 50);
  }
}

TEST(HoldemTree, RefusesTreesBeyondTheNodeLimit) {
  SpotConfig spot = check_down("Ks8d5h", "AA", "QQ");
  spot.tree.effective_stack = 100;
  spot.tree.max_nodes = 1000;
  for (auto& street : spot.tree.sizing) {
    for (auto& sizing : street) sizing.bet = {0.5};
  }
  std::string error;
  EXPECT_FALSE(make_game(spot, &error).has_value());
  EXPECT_NE(error.find("1000"), std::string::npos) << error;
}

SpotConfig turn_spot() {
  SpotConfig spot;
  spot.board = *parse_cards("KsTs7h4h");
  spot.ranges[kOop] = *Range::parse("TT,77,44,AKo,KQs,KJs,QJs,J9s,98s,A5s,65s");
  spot.ranges[kIp] = *Range::parse("AA,KK,KTs,AQs,QTs,JTs,T9s,87s,A4s,99,88");
  spot.tree.pot = 100;
  spot.tree.effective_stack = 300;
  for (const std::size_t street : {kTurn, kRiver}) {
    for (std::size_t player = 0; player < 2; ++player) {
      spot.tree.sizing[street][player].bet = {0.66};
      spot.tree.sizing[street][player].raise = {0.75};
    }
  }
  spot.tree.raise_cap = 1;
  return spot;
}

SolveResult solve(Solver& solver, int iterations) {
  SolveOptions options;
  options.max_iterations = iterations;
  options.target_exploitability_pct = 0;
  options.check_interval = iterations;
  return solver.solve(options);
}

TEST(HoldemTurn, Converges) {
  auto game = make_game(turn_spot());
  ASSERT_TRUE(game.has_value());
  Solver solver(std::move(*game));

  const double before = solver.evaluate().exploitability_pct;
  const SolveResult result = solve(solver, 200);
  EXPECT_GT(before, 5.0);
  EXPECT_LT(result.evaluation.exploitability_pct, 0.5);
  EXPECT_GE(result.evaluation.exploitability, -1e-2);
  EXPECT_NEAR(result.evaluation.ev[0] + result.evaluation.ev[1], 100.0, 0.05);
}

TEST(HoldemTurn, IsomorphismDoesNotChangeTheSolution) {
  SpotConfig merged_spot = turn_spot();
  SpotConfig full_spot = turn_spot();
  full_spot.use_isomorphism = false;

  auto merged_game = make_game(merged_spot);
  auto full_game = make_game(full_spot);
  ASSERT_TRUE(merged_game && full_game);
  // Clubs and diamonds are interchangeable on this board: 46 rivers, 13 merged.
  EXPECT_LT(merged_game->tree.nodes.size(), full_game->tree.nodes.size());
  EXPECT_LT(Solver::estimate_memory_bytes(*merged_game), Solver::estimate_memory_bytes(*full_game));

  Solver merged(std::move(*merged_game));
  Solver full(std::move(*full_game));

  // After one iteration the two must agree to rounding. Later they drift:
  // regret matching jumps when a regret crosses zero, and hands whose actions
  // are worth exactly the same sit right on that edge, so the different order
  // of additions is enough to send the two solves down different paths to the
  // same equilibrium.
  const SolveResult a = solve(merged, 1);
  const SolveResult b = solve(full, 1);
  EXPECT_NEAR(a.evaluation.ev[0], b.evaluation.ev[0], 1e-3);
  EXPECT_NEAR(a.evaluation.exploitability, b.evaluation.exploitability, 1e-3);

  // Compare whole nodes, including one on a river card that only the full
  // tree stores (7d is merged into 7c).
  for (const char* path : {"", "check", "bet66", "check/check/2c", "check/check/7d",
                           "check/check/7d/check", "bet66/call/Ad", "bet66/call/Ad/bet153.12"}) {
    SCOPED_TRACE(path);
    const auto merged_ref = merged.game().tree.find(path);
    const auto full_ref = full.game().tree.find(path);
    ASSERT_TRUE(merged_ref.has_value());
    ASSERT_TRUE(full_ref.has_value());
    const NodeReport x = merged.report(*merged_ref);
    const NodeReport y = full.report(*full_ref);
    ASSERT_EQ(x.type, y.type);
    ASSERT_EQ(x.actions.size(), y.actions.size());
    ASSERT_EQ(x.strategy.size(), y.strategy.size());
    for (std::size_t player = 0; player < 2; ++player) {
      for (std::size_t h = 0; h < x.reach[player].size(); ++h) {
        EXPECT_NEAR(x.reach[player][h], y.reach[player][h], 1e-3) << "reach " << player << ":" << h;
        EXPECT_NEAR(x.ev[player][h], y.ev[player][h], 0.02) << "ev " << player << ":" << h;
      }
    }
    for (std::size_t i = 0; i < x.strategy.size(); ++i) {
      EXPECT_NEAR(x.strategy[i], y.strategy[i], 1e-3) << "strategy " << i;
    }
  }

  // Later on both must still be solving the same game equally well.
  const SolveResult c = solve(merged, 150);
  const SolveResult d = solve(full, 150);
  EXPECT_LT(c.evaluation.exploitability_pct, 0.5);
  EXPECT_LT(d.evaluation.exploitability_pct, 0.5);
  EXPECT_NEAR(c.evaluation.ev[0], d.evaluation.ev[0], 0.1);
}

TEST(HoldemTurn, ThreadsDoNotChangeTheSolution) {
  auto game_a = make_game(turn_spot());
  auto game_b = make_game(turn_spot());
  ASSERT_TRUE(game_a && game_b);
  Solver single(std::move(*game_a), SolverConfig{1});
  Solver multi(std::move(*game_b), SolverConfig{4});
  EXPECT_EQ(single.threads(), 1u);
  EXPECT_EQ(multi.threads(), 4u);

  const SolveResult a = solve(single, 30);
  const SolveResult b = solve(multi, 30);
  EXPECT_EQ(a.evaluation.ev[0], b.evaluation.ev[0]);
  EXPECT_EQ(a.evaluation.ev[1], b.evaluation.ev[1]);
  EXPECT_EQ(a.evaluation.exploitability, b.evaluation.exploitability);
  EXPECT_EQ(single.report(GameTree::kRoot).strategy, multi.report(GameTree::kRoot).strategy);
}

SolverConfig compressed(unsigned threads = 0) {
  SolverConfig config;
  config.threads = threads;
  config.compress = true;
  return config;
}

TEST(HoldemTurn, CompressedStorageHalvesMemoryAndSolvesAlike) {
  auto float_game = make_game(turn_spot());
  auto packed_game = make_game(turn_spot());
  ASSERT_TRUE(float_game && packed_game);
  const std::size_t estimate = Solver::estimate_memory_bytes(*packed_game, true);
  EXPECT_LT(estimate, Solver::estimate_memory_bytes(*float_game) / 1.9);

  Solver floats(std::move(*float_game));
  Solver packed(std::move(*packed_game), compressed());
  EXPECT_FALSE(floats.compressed());
  EXPECT_TRUE(packed.compressed());
  EXPECT_EQ(packed.memory_bytes(), estimate);

  const SolveResult a = solve(floats, 200);
  const SolveResult b = solve(packed, 200);
  EXPECT_LT(b.evaluation.exploitability_pct, 0.5);
  EXPECT_NEAR(a.evaluation.ev[0], b.evaluation.ev[0], 0.1);
  EXPECT_NEAR(b.evaluation.ev[0] + b.evaluation.ev[1], 100.0, 0.05);
}

TEST(HoldemTurn, CompressedStorageKeepsConverging) {
  // Rounding to the nearest 16-bit step used to stall solves at a few tenths
  // of a percent of the pot; with stochastic rounding they keep pace with
  // floats.
  auto float_game = make_game(turn_spot());
  auto packed_game = make_game(turn_spot());
  ASSERT_TRUE(float_game && packed_game);
  Solver floats(std::move(*float_game));
  Solver packed(std::move(*packed_game), compressed());

  const SolveResult a = solve(floats, 600);
  const SolveResult b = solve(packed, 600);
  EXPECT_LT(b.evaluation.exploitability_pct, 1.5 * a.evaluation.exploitability_pct + 0.01);
}

TEST(HoldemTurn, CompressedStorageIsTheSameOnEveryThreadCount) {
  auto game_a = make_game(turn_spot());
  auto game_b = make_game(turn_spot());
  ASSERT_TRUE(game_a && game_b);
  Solver single(std::move(*game_a), compressed(1));
  Solver multi(std::move(*game_b), compressed(4));

  const SolveResult a = solve(single, 30);
  const SolveResult b = solve(multi, 30);
  EXPECT_EQ(a.evaluation.ev[0], b.evaluation.ev[0]);
  EXPECT_EQ(a.evaluation.exploitability, b.evaluation.exploitability);
  EXPECT_EQ(single.report(GameTree::kRoot).strategy, multi.report(GameTree::kRoot).strategy);
}

TEST(HoldemTurn, EquityIgnoresTheBettingAndMatchesBruteForce) {
  for (const bool isomorphism : {true, false}) {
    SCOPED_TRACE(isomorphism ? "with isomorphism" : "without isomorphism");
    SpotConfig spot = turn_spot();
    spot.use_isomorphism = isomorphism;
    auto game = make_game(spot);
    ASSERT_TRUE(game.has_value());
    Solver solver(std::move(*game));
    solve(solver, 30);

    const double expected = brute_force_share(spot.board, spot.ranges[kOop], spot.ranges[kIp]);
    const NodeReport root = solver.report(GameTree::kRoot);
    EXPECT_NEAR(root.range_equity[kOop], expected, 1e-4);
    EXPECT_NEAR(root.range_equity[kIp], 1.0 - expected, 1e-4);
    // Being behind in equity is not the same as being behind in value.
    EXPECT_NE(root.range_ev[kOop], 100.0 * root.range_equity[kOop]);
    for (std::size_t player = 0; player < 2; ++player) {
      for (const float equity : root.equity[player]) {
        EXPECT_GE(equity, 0.0f);
        EXPECT_LE(equity, 1.0f + 1e-5f);
      }
    }

    // After a bet and a call the ranges are narrower, and the equity is
    // theirs, not the root's.
    const auto called = solver.game().tree.find("bet66/call/2c");
    ASSERT_TRUE(called.has_value());
    const NodeReport river = solver.report(*called);
    EXPECT_NEAR(river.range_equity[kOop] + river.range_equity[kIp], 1.0, 1e-4);

    // A fold has no equity to speak of.
    const NodeReport fold = solver.report(*solver.game().tree.find("bet66/fold"));
    EXPECT_EQ(fold.range_equity[kOop], 0.0);
  }
}

TEST(HoldemTurn, DealtCardRemovesBlockedHands) {
  auto game = make_game(turn_spot());
  ASSERT_TRUE(game.has_value());
  Solver solver(std::move(*game));
  solve(solver, 20);

  const auto ref = solver.game().tree.find("check/check/Ac");
  ASSERT_TRUE(ref.has_value());
  const NodeReport report = solver.report(*ref);
  const std::uint64_t ace_of_clubs = Card::parse("Ac")->mask();
  int blocked = 0;
  for (std::size_t player = 0; player < 2; ++player) {
    const auto& hands = solver.game().hands[player];
    for (std::size_t h = 0; h < hands.size(); ++h) {
      if ((hands[h].mask & ace_of_clubs) == 0) continue;
      ++blocked;
      EXPECT_EQ(report.reach[player][h], 0.0f);
      EXPECT_EQ(report.ev[player][h], 0.0f);
    }
  }
  EXPECT_GT(blocked, 0);
}

TEST(HoldemFlop, SmallSpotConverges) {
  SpotConfig spot;
  spot.board = *parse_cards("Ks9s4d");
  spot.ranges[kOop] = *Range::parse("KK,99,AKs,QJs,JTs");
  spot.ranges[kIp] = *Range::parse("AA,44,KQs,AsQs,T8s");
  spot.tree.pot = 100;
  spot.tree.effective_stack = 200;
  for (const std::size_t street : {kFlop, kTurn, kRiver}) {
    for (std::size_t player = 0; player < 2; ++player) {
      spot.tree.sizing[street][player].bet = {0.75};
    }
  }
  spot.tree.raise_cap = 0;

  std::string error;
  auto game = make_game(spot, &error);
  ASSERT_TRUE(game.has_value()) << error;
  EXPECT_GT(game->tree.chances.size(), 3u);

  Solver solver(std::move(*game));
  const SolveResult result = solve(solver, 100);
  EXPECT_LT(result.evaluation.exploitability_pct, 1.0);
  EXPECT_GE(result.evaluation.exploitability, -1e-2);
  EXPECT_NEAR(result.evaluation.ev[0] + result.evaluation.ev[1], 100.0, 0.05);

  // Walking a full line down to the river works and stays consistent.
  const auto river = solver.game().tree.find("check/check/2c/check/check/7h");
  ASSERT_TRUE(river.has_value());
  const NodeReport report = solver.report(*river);
  EXPECT_EQ(report.type, NodeType::Action);
  EXPECT_EQ(report.player, kOop);
}

}  // namespace
}  // namespace gto
