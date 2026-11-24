#include "gto/solver.h"

#include <gtest/gtest.h>

namespace gto {
namespace {

constexpr std::size_t kRiver = static_cast<std::size_t>(Street::River);

/// Kuhn poker: three cards (J, Q, K), one each, ante 1, a single bet of 1.
/// It is a river spot with pot 2, stack 1 and all-in as the only bet.
Game kuhn_game() {
  TreeConfig config;
  config.pot = 2;
  config.effective_stack = 1;
  config.sizing[kRiver][kOop].add_allin = true;
  config.sizing[kRiver][kIp].add_allin = true;

  Game game;
  game.tree = *build_tree(config);
  std::array<std::vector<HandStrength>, 2> strength;
  for (std::size_t player = 0; player < 2; ++player) {
    for (int card = 0; card < 3; ++card) {
      game.hands[player].push_back(make_hand({card}, 1.0f));
      strength[player].push_back(static_cast<HandStrength>(card));
    }
  }
  game.showdowns.push_back(make_showdown_table(std::move(strength)));
  return game;
}

constexpr std::size_t kJack = 0, kQueen = 1, kKing = 2;

TEST(Solver, KuhnPokerReachesTheKnownEquilibrium) {
  Solver solver(kuhn_game());
  SolveOptions options;
  options.max_iterations = 5000;
  options.target_exploitability_pct = 0;
  options.check_interval = 5000;
  const SolveResult result = solver.solve(options);

  EXPECT_LT(result.evaluation.exploitability, 0.002);
  // The first player loses 1/18 of an ante per hand. In pot-share terms that
  // is 1 - 1/18 of the 2-chip pot for them and 1 + 1/18 for the second player.
  EXPECT_NEAR(result.evaluation.ev[kOop], 17.0 / 18.0, 0.002);
  EXPECT_NEAR(result.evaluation.ev[kIp], 19.0 / 18.0, 0.002);

  const GameTree& tree = solver.game().tree;

  // Second player after a check: [check, allin].
  const NodeReport after_check = solver.report(*tree.find("check"));
  EXPECT_NEAR(after_check.frequency(1, kJack), 1.0 / 3.0, 0.03);  // bluffs a third
  EXPECT_NEAR(after_check.frequency(1, kQueen), 0.0, 0.03);       // never bets the queen
  EXPECT_NEAR(after_check.frequency(1, kKing), 1.0, 0.03);        // always bets the king

  // Second player facing a bet: [fold, call].
  const NodeReport facing_bet = solver.report(*tree.find("allin"));
  EXPECT_NEAR(facing_bet.frequency(1, kJack), 0.0, 0.03);
  EXPECT_NEAR(facing_bet.frequency(1, kQueen), 1.0 / 3.0, 0.03);
  EXPECT_NEAR(facing_bet.frequency(1, kKing), 1.0, 0.03);

  // First player: any bluffing frequency x in [0, 1/3] with the jack is an
  // equilibrium, as long as the king value-bets three times as often.
  const NodeReport root = solver.report(GameTree::kRoot);
  const float jack_bet = root.frequency(1, kJack);
  EXPECT_GE(jack_bet, -0.01f);
  EXPECT_LE(jack_bet, 1.0f / 3.0f + 0.03f);
  EXPECT_NEAR(root.frequency(1, kKing), 3 * jack_bet, 0.06);
  EXPECT_NEAR(root.frequency(1, kQueen), 0.0, 0.03);
  // …and then calls a bet with the queen with probability x + 1/3.
  const NodeReport check_then_bet = solver.report(*tree.find("check/allin"));
  EXPECT_NEAR(check_then_bet.frequency(1, kQueen), jack_bet + 1.0 / 3.0, 0.05);
}

SpotConfig polarised_river() {
  SpotConfig spot;
  spot.board = *parse_cards("Kc8d5h3s2d");
  spot.ranges[kOop] = *Range::parse("88");
  spot.ranges[kIp] = *Range::parse("KK,QJo");
  spot.tree.pot = 100;
  spot.tree.effective_stack = 100;
  spot.tree.sizing[kRiver][kIp].bet = {1.0};
  return spot;
}

/// Reach-weighted frequency of `action` over the hands that satisfy `filter`.
template <typename Filter>
double frequency(const Solver& solver, const NodeReport& report, std::size_t action,
                 Filter filter) {
  const auto player = static_cast<std::size_t>(report.player);
  const auto& hands = solver.game().hands[player];
  double weighted = 0, total = 0;
  for (std::size_t h = 0; h < hands.size(); ++h) {
    if (!filter(hands[h])) continue;
    weighted += static_cast<double>(report.reach[player][h]) * report.frequency(action, h);
    total += report.reach[player][h];
  }
  return total > 0 ? weighted / total : 0.0;
}

bool is_pocket_kings(const Hand& hand) {
  return Card(hand.cards[0]).rank() == 11 && Card(hand.cards[1]).rank() == 11;
}

TEST(Solver, PolarisedRiverMatchesTheory) {
  std::string error;
  auto game = make_game(polarised_river(), &error);
  ASSERT_TRUE(game.has_value()) << error;
  EXPECT_EQ(game->hands[kOop].size(), 3u);   // 8c8h, 8c8s, 8h8s
  EXPECT_EQ(game->hands[kIp].size(), 15u);   // 3 KK + 12 QJo

  Solver solver(std::move(*game));
  SolveOptions options;
  options.max_iterations = 3000;
  options.target_exploitability_pct = 0;
  options.check_interval = 3000;
  const SolveResult result = solver.solve(options);

  EXPECT_LT(result.evaluation.exploitability_pct, 0.05);
  // A pot-size bet lets IP win the pot 1.5 times as often as they hold the
  // nuts: 3/15 * 1.5 = 30% of the pot.
  EXPECT_NEAR(result.evaluation.ev[kIp], 30.0, 0.2);
  EXPECT_NEAR(result.evaluation.ev[kOop], 70.0, 0.2);

  const GameTree& tree = solver.game().tree;
  const NodeReport ip = solver.report(*tree.find("check"));
  ASSERT_EQ(ip.actions.size(), 2u);  // check, allin
  EXPECT_NEAR(frequency(solver, ip, 1, is_pocket_kings), 1.0, 0.01);
  // One bluff for every two value bets: 1.5 of the 12 QJo combos.
  EXPECT_NEAR(frequency(solver, ip, 1, [](const Hand& h) { return !is_pocket_kings(h); }), 0.125,
              0.01);

  const NodeReport oop = solver.report(*tree.find("check/allin"));
  ASSERT_EQ(oop.actions.size(), 2u);  // fold, call
  EXPECT_NEAR(frequency(solver, oop, 1, [](const Hand&) { return true; }), 0.5, 0.01);

  // Per-hand values at IP's decision: kings win the pot plus half a bet,
  // bluffs are indifferent and worth nothing.
  for (std::size_t h = 0; h < ip.ev[kIp].size(); ++h) {
    const bool kings = is_pocket_kings(solver.game().hands[kIp][h]);
    EXPECT_NEAR(ip.ev[kIp][h], kings ? 150.0 : 0.0, 1.0) << h;
  }
}

SpotConfig realistic_river() {
  SpotConfig spot;
  spot.board = *parse_cards("KsTd7h4c2s");
  spot.ranges[kOop] = *Range::parse(
      "TT-22,AQo-ATo,KQo-KTo,QJo,AJs-A2s,KQs-K2s,QJs-Q8s,JTs-J8s,T9s-T7s,98s-97s,87s,76s,65s");
  spot.ranges[kIp] = *Range::parse(
      "AA:0.25,KK:0.5,QQ-22,AKs-A2s,AKo-ATo,KQs-K8s,KQo-KTo,QJs-Q9s,QJo,JTs-J9s,T9s,98s,87s,76s");
  spot.tree.pot = 20;
  spot.tree.effective_stack = 90;
  for (std::size_t player = 0; player < 2; ++player) {
    spot.tree.sizing[kRiver][player].bet = {0.33, 0.75};
    spot.tree.sizing[kRiver][player].raise = {0.6};
    spot.tree.sizing[kRiver][player].add_allin = true;
  }
  return spot;
}

TEST(Solver, RealisticRiverConverges) {
  auto game = make_game(realistic_river());
  ASSERT_TRUE(game.has_value());
  EXPECT_EQ(Solver::estimate_memory_bytes(*game) % 8, 0u);
  const std::size_t estimate = Solver::estimate_memory_bytes(*game);

  Solver solver(std::move(*game));
  EXPECT_EQ(solver.memory_bytes(), estimate);

  // Before any iteration both players play uniformly at random.
  const double start = solver.evaluate().exploitability_pct;
  EXPECT_GT(start, 5.0);

  SolveOptions options;
  options.max_iterations = 300;
  options.target_exploitability_pct = 0.3;
  int reports = 0;
  options.on_progress = [&](const Progress&) {
    ++reports;
    return true;
  };
  const SolveResult result = solver.solve(options);

  EXPECT_GT(reports, 0);
  EXPECT_LE(result.iterations, 300);
  EXPECT_LT(result.evaluation.exploitability_pct, 0.5);
  EXPECT_GE(result.evaluation.exploitability, -1e-3);
  // Without rake the pot is split between the two players, nothing more or less.
  EXPECT_NEAR(result.evaluation.ev[0] + result.evaluation.ev[1], 20.0, 0.02);
  // Neither player can do worse than their best response says.
  EXPECT_GE(result.evaluation.best_response[0], result.evaluation.ev[0] - 1e-3);
  EXPECT_GE(result.evaluation.best_response[1], result.evaluation.ev[1] - 1e-3);
}

TEST(Solver, ReachesAtChildrenSumToTheParent) {
  auto game = make_game(realistic_river());
  ASSERT_TRUE(game.has_value());
  Solver solver(std::move(*game));
  for (int i = 0; i < 20; ++i) solver.iterate();

  const GameTree& tree = solver.game().tree;
  const NodeReport root = solver.report(GameTree::kRoot);
  const std::size_t hands = root.reach[kOop].size();
  std::vector<double> total(hands, 0.0);
  for (const std::uint32_t child : tree.children_of(GameTree::kRoot)) {
    const NodeReport report = solver.report(child);
    for (std::size_t h = 0; h < hands; ++h) total[h] += report.reach[kOop][h];
    // The other player has not acted yet.
    EXPECT_EQ(report.reach[kIp], root.reach[kIp]);
  }
  for (std::size_t h = 0; h < hands; ++h) EXPECT_NEAR(total[h], root.reach[kOop][h], 1e-5);
}

TEST(Solver, StopsWhenTheCallbackSaysSo) {
  Solver solver(kuhn_game());
  SolveOptions options;
  options.max_iterations = 1000;
  options.target_exploitability_pct = 0;
  options.check_interval = 10;
  options.on_progress = [](const Progress& progress) { return progress.iteration < 30; };
  EXPECT_EQ(solver.solve(options).iterations, 30);
  EXPECT_EQ(solver.iterations(), 30);
}

TEST(Solver, RakeComesOutOfThePot) {
  SpotConfig spot = realistic_river();
  spot.rake_rate = 0.05;
  spot.rake_cap = 3;
  auto game = make_game(spot);
  ASSERT_TRUE(game.has_value());
  Solver solver(std::move(*game));
  SolveOptions options;
  options.max_iterations = 200;
  const SolveResult result = solver.solve(options);

  const double total = result.evaluation.ev[0] + result.evaluation.ev[1];
  // At least 5% of the starting pot is raked, at most the cap.
  EXPECT_LT(total, 20.0 - 0.99);
  EXPECT_GT(total, 20.0 - 3.01);
}

TEST(MakeGame, RejectsImpossibleSpots) {
  std::string error;

  SpotConfig preflop = polarised_river();
  preflop.board = *parse_cards("Kc8d");
  EXPECT_FALSE(make_game(preflop, &error).has_value());
  EXPECT_NE(error.find("three"), std::string::npos) << error;

  SpotConfig duplicate = polarised_river();
  duplicate.board[4] = duplicate.board[0];
  EXPECT_FALSE(make_game(duplicate, &error).has_value());

  SpotConfig empty = polarised_river();
  empty.ranges[kOop] = Range{};
  EXPECT_FALSE(make_game(empty, &error).has_value());
  EXPECT_NE(error.find("OOP"), std::string::npos) << error;

  // Kc is on the board, so "KcKd" is the only hand and it is impossible.
  SpotConfig blocked = polarised_river();
  blocked.ranges[kIp] = *Range::parse("KcKd");
  EXPECT_FALSE(make_game(blocked, &error).has_value());

  SpotConfig clash = polarised_river();
  clash.ranges[kOop] = *Range::parse("AsAh");
  clash.ranges[kIp] = *Range::parse("AsAd");
  EXPECT_FALSE(make_game(clash, &error).has_value());

  SpotConfig bad_rake = polarised_river();
  bad_rake.rake_rate = 1.5;
  EXPECT_FALSE(make_game(bad_rake, &error).has_value());
}

}  // namespace
}  // namespace gto
