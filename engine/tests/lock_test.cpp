// Node locking: fixing a player's strategy and solving around it.

#include <string_view>

#include <gtest/gtest.h>

#include "gto/solver.h"

namespace gto {
namespace {

constexpr std::size_t kTurn = static_cast<std::size_t>(Street::Turn);
constexpr std::size_t kRiver = static_cast<std::size_t>(Street::River);

/// IP holds the nuts (KK) or air (QJo), OOP only bluff-catchers (88). OOP
/// checks; IP checks or bets pot, all-in. Unlocked: IP bets all kings and
/// 12.5% of the air, OOP calls half, EV 70 / 30.
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

SolveResult solve(Solver& solver, int iterations) {
  SolveOptions options;
  options.max_iterations = iterations;
  options.target_exploitability_pct = 0;
  options.check_interval = iterations;
  return solver.solve(options);
}

/// Locks every hand of the acting player at `path` to `frequencies`.
void lock_all(Solver& solver, std::string_view path, const std::vector<float>& frequencies) {
  const auto ref = solver.game().tree.find(path);
  ASSERT_TRUE(ref.has_value()) << path;
  const Node& node = solver.game().tree.nodes[ref->node];
  const std::size_t hands = solver.game().hands[node.player].size();
  ASSERT_EQ(frequencies.size(), node.num_children);

  std::vector<std::uint8_t> locked(hands, 1);
  std::vector<float> strategy(frequencies.size() * hands);
  for (std::size_t a = 0; a < frequencies.size(); ++a) {
    for (std::size_t h = 0; h < hands; ++h) strategy[a * hands + h] = frequencies[a];
  }
  std::string error;
  ASSERT_TRUE(solver.set_lock(*ref, locked, strategy, &error)) << error;
}

/// Reach-weighted frequency of `action` over the acting player's hands that
/// satisfy `filter`.
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

bool is_kings(const Hand& hand) {
  return Card(hand.cards[0]).rank() == 11 && Card(hand.cards[1]).rank() == 11;
}
bool is_air(const Hand& hand) { return !is_kings(hand); }

TEST(Lock, AlwaysFoldingMakesTheOpponentBluffEverything) {
  Solver solver(*make_game(polarised_river()));
  // OOP facing the bet: [fold, call].
  lock_all(solver, "check/allin", {1.0f, 0.0f});
  EXPECT_TRUE(solver.has_locks());
  const SolveResult result = solve(solver, 1000);

  const GameTree& tree = solver.game().tree;
  const NodeReport ip = solver.report(*tree.find("check"));
  EXPECT_NEAR(frequency(solver, ip, 1, is_air), 1.0, 0.01);
  // Betting always wins the pot.
  EXPECT_NEAR(result.evaluation.ev[kIp], 100.0, 0.2);
  EXPECT_NEAR(result.evaluation.ev[kOop], 0.0, 0.2);

  // The locked node shows the lock, exactly, for every hand.
  const NodeReport oop = solver.report(*tree.find("check/allin"));
  ASSERT_EQ(oop.locked.size(), 3u);
  for (std::size_t h = 0; h < 3; ++h) {
    EXPECT_EQ(oop.locked[h], 1);
    EXPECT_EQ(oop.frequency(0, h), 1.0f);
    EXPECT_EQ(oop.frequency(1, h), 0.0f);
  }
  // Nothing is locked elsewhere.
  for (const std::uint8_t flag : ip.locked) EXPECT_EQ(flag, 0);

  // OOP cannot fix the leak, so within the locked game nobody can improve.
  EXPECT_NEAR(result.evaluation.exploitability, 0.0, 0.05);
}

TEST(Lock, AlwaysCallingStopsTheBluffs) {
  Solver solver(*make_game(polarised_river()));
  lock_all(solver, "check/allin", {0.0f, 1.0f});
  const SolveResult result = solve(solver, 1000);

  const NodeReport ip = solver.report(*solver.game().tree.find("check"));
  EXPECT_NEAR(frequency(solver, ip, 1, is_air), 0.0, 0.01);
  EXPECT_NEAR(frequency(solver, ip, 1, is_kings), 1.0, 0.01);
  // Kings (3 of 15 hands) win pot plus bet; air gives up the pot.
  EXPECT_NEAR(result.evaluation.ev[kIp], 0.2 * 200.0, 0.2);
  EXPECT_NEAR(result.evaluation.ev[kOop], 0.8 * 100.0 - 0.2 * 100.0, 0.2);
  EXPECT_NEAR(result.evaluation.exploitability, 0.0, 0.05);
}

TEST(Lock, LockingTheSolvedStrategyChangesNothing) {
  Solver solver(*make_game(polarised_river()));
  const SolveResult before = solve(solver, 2000);

  // Lock IP's decision to exactly what the solve found.
  const auto ref = *solver.game().tree.find("check");
  const NodeReport ip = solver.report(ref);
  const std::vector<std::uint8_t> locked(ip.reach[kIp].size(), 1);
  ASSERT_TRUE(solver.set_lock(ref, locked, ip.strategy));

  const SolveResult after = solve(solver, 500);
  EXPECT_NEAR(after.evaluation.ev[kOop], before.evaluation.ev[kOop], 0.1);
  EXPECT_NEAR(after.evaluation.ev[kIp], before.evaluation.ev[kIp], 0.1);
  const NodeReport oop = solver.report(*solver.game().tree.find("check/allin"));
  EXPECT_NEAR(frequency(solver, oop, 1, [](const Hand&) { return true; }), 0.5, 0.02);
  EXPECT_EQ(solver.report(ref).strategy, ip.strategy);
}

TEST(Lock, CanCoverSingleHands) {
  Solver solver(*make_game(polarised_river()));
  const auto ref = *solver.game().tree.find("check/allin");
  // Only the first of the three 88 combos always folds.
  std::vector<std::uint8_t> locked = {1, 0, 0};
  std::vector<float> strategy = {/* fold */ 1, 0, 0, /* call */ 0, 0, 0};
  ASSERT_TRUE(solver.set_lock(ref, locked, strategy));
  const SolveResult result = solve(solver, 2000);

  const NodeReport oop = solver.report(ref);
  EXPECT_EQ(oop.locked, (std::vector<std::uint8_t>{1, 0, 0}));
  EXPECT_EQ(oop.frequency(1, 0), 0.0f);
  // The other two make up for it: half of all hands still have to call, so
  // each of them calls three quarters of the time on average.
  EXPECT_NEAR((oop.frequency(1, 1) + oop.frequency(1, 2)) / 2, 0.75, 0.03);
  EXPECT_NEAR(result.evaluation.ev[kOop], 70.0, 0.3);
}

TEST(Lock, ScalesFrequenciesAndRejectsNonsense) {
  Solver solver(*make_game(polarised_river()));
  const GameTree& tree = solver.game().tree;
  const auto ref = *tree.find("check/allin");
  std::string error;

  // Frequencies given as 1 : 3 become 25% and 75%.
  ASSERT_TRUE(solver.set_lock(ref, std::vector<std::uint8_t>{1, 1, 1},
                              std::vector<float>{1, 1, 1, 3, 3, 3}, &error))
      << error;
  const NodeReport report = solver.report(ref);
  EXPECT_FLOAT_EQ(report.frequency(0, 0), 0.25f);
  EXPECT_FLOAT_EQ(report.frequency(1, 0), 0.75f);

  EXPECT_FALSE(solver.set_lock(ref, std::vector<std::uint8_t>{1, 1},
                               std::vector<float>{1, 1, 1, 0, 0, 0}, &error));
  EXPECT_FALSE(solver.set_lock(ref, std::vector<std::uint8_t>{1, 1, 1},
                               std::vector<float>{1, 1, 1}, &error));
  EXPECT_FALSE(solver.set_lock(ref, std::vector<std::uint8_t>{1, 1, 1},
                               std::vector<float>{0, 1, 1, 0, 0, 0}, &error));
  EXPECT_NE(error.find("zero"), std::string::npos) << error;
  EXPECT_FALSE(solver.set_lock(ref, std::vector<std::uint8_t>{1, 1, 1},
                               std::vector<float>{-1, 1, 1, 2, 0, 0}, &error));
  EXPECT_FALSE(solver.set_lock(ref, std::vector<std::uint8_t>{0, 0, 0},
                               std::vector<float>{1, 1, 1, 0, 0, 0}, &error));
  // A showdown is not a decision.
  const auto showdown = *tree.find("check/check");
  EXPECT_FALSE(solver.set_lock(showdown, {}, {}, &error));

  // The failed attempts left the good lock in place.
  EXPECT_FLOAT_EQ(solver.report(ref).frequency(1, 0), 0.75f);

  solver.clear_locks();
  EXPECT_FALSE(solver.has_locks());
  for (const std::uint8_t flag : solver.report(ref).locked) EXPECT_EQ(flag, 0);
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

std::size_t hand_index(const Solver& solver, int player, std::string_view name) {
  const auto& hands = solver.game().hands[static_cast<std::size_t>(player)];
  for (std::size_t h = 0; h < hands.size(); ++h) {
    if (holdem_hand_name(hands[h]) == name) return h;
  }
  ADD_FAILURE() << "no hand " << name;
  return 0;
}

TEST(Lock, IsMadeSymmetricWhereSuitsAreMerged) {
  // Clubs and diamonds are interchangeable on this board, and the rivers of
  // those suits are stored once. A lock above them must treat AcKd and AdKc
  // alike, or the merged rivers would no longer be the same situation.
  Solver solver(*make_game(turn_spot()));
  const auto root = NodeRef{};
  const std::size_t hands = solver.game().hands[kOop].size();
  const std::size_t actions = solver.game().tree.nodes[0].num_children;
  ASSERT_EQ(actions, 2u);  // check, bet

  const std::size_t one = hand_index(solver, kOop, "AcKd");
  const std::size_t other = hand_index(solver, kOop, "AdKc");
  const std::size_t unrelated = hand_index(solver, kOop, "AsKh");
  std::vector<std::uint8_t> locked(hands, 0);
  std::vector<float> strategy(actions * hands, 0.0f);
  locked[one] = 1;
  strategy[1 * hands + one] = 1;  // always bet
  ASSERT_TRUE(solver.set_lock(root, locked, strategy));

  const NodeReport report = solver.report(root);
  EXPECT_EQ(report.locked[one], 1);
  EXPECT_EQ(report.locked[other], 1);
  EXPECT_EQ(report.locked[unrelated], 0);
  EXPECT_EQ(report.frequency(1, other), 1.0f);

  // The solve still works and stays a solve of one consistent game.
  const SolveResult result = solve(solver, 150);
  EXPECT_LT(result.evaluation.exploitability_pct, 1.0);
  EXPECT_NEAR(result.evaluation.ev[0] + result.evaluation.ev[1], 100.0, 0.05);
}

TEST(Lock, MergedAndFullTreesAgreeUnderASymmetricLock) {
  SpotConfig full_spot = turn_spot();
  full_spot.use_isomorphism = false;
  Solver merged(*make_game(turn_spot()));
  Solver full(*make_game(full_spot));

  // Lock OOP's first decision to always check, in both.
  lock_all(merged, "", {1.0f, 0.0f});
  lock_all(full, "", {1.0f, 0.0f});
  const SolveResult a = solve(merged, 1);
  const SolveResult b = solve(full, 1);
  EXPECT_NEAR(a.evaluation.ev[0], b.evaluation.ev[0], 1e-3);
  EXPECT_NEAR(a.evaluation.exploitability, b.evaluation.exploitability, 1e-3);
}

TEST(Lock, OnAMergedRiverCardUsesTheSuitsAskedFor) {
  Solver solver(*make_game(turn_spot()));
  const GameTree& tree = solver.game().tree;
  // 7d is stored as 7c with clubs and diamonds swapped.
  const auto diamonds = *tree.find("check/check/7d");
  const auto clubs = *tree.find("check/check/7c");
  ASSERT_TRUE(diamonds.remapped);
  ASSERT_EQ(diamonds.node, clubs.node);

  const std::size_t hands = solver.game().hands[kOop].size();
  const std::size_t actions = tree.nodes[diamonds.node].num_children;
  const std::size_t asked = hand_index(solver, kOop, "AdKh");
  const std::size_t mirror = hand_index(solver, kOop, "AcKh");
  std::vector<std::uint8_t> locked(hands, 0);
  std::vector<float> strategy(actions * hands, 0.0f);
  locked[asked] = 1;
  strategy[0 * hands + asked] = 1;  // always check
  ASSERT_TRUE(solver.set_lock(diamonds, locked, strategy));

  // On the 7d river it is AdKh that is locked…
  const NodeReport on_diamonds = solver.report(diamonds);
  EXPECT_EQ(on_diamonds.locked[asked], 1);
  EXPECT_EQ(on_diamonds.locked[mirror], 0);
  EXPECT_EQ(on_diamonds.frequency(0, asked), 1.0f);
  // …and on the 7c river, which is the same situation mirrored, AcKh.
  const NodeReport on_clubs = solver.report(clubs);
  EXPECT_EQ(on_clubs.locked[mirror], 1);
  EXPECT_EQ(on_clubs.locked[asked], 0);
}

TEST(Lock, WarmRestartContinuesFromTheCurrentStrategies) {
  Solver warm(*make_game(turn_spot()));
  solve(warm, 150);
  warm.warm_restart();
  const SolveResult continued = solve(warm, 20);

  Solver cold(*make_game(turn_spot()));
  const SolveResult fresh = solve(cold, 20);

  // Twenty iterations from a solved position beat twenty from nothing.
  EXPECT_LT(continued.evaluation.exploitability_pct, 1.0);
  EXPECT_LT(continued.evaluation.exploitability_pct, fresh.evaluation.exploitability_pct / 4);
  EXPECT_EQ(warm.iterations(), 170);
}

}  // namespace
}  // namespace gto
