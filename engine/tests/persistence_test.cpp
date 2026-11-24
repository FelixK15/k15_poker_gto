// Saving and restoring solver state, and sizing up spots without building them.

#include <sstream>

#include <gtest/gtest.h>

#include "gto/solver.h"

namespace gto {
namespace {

constexpr std::size_t kFlop = static_cast<std::size_t>(Street::Flop);
constexpr std::size_t kTurn = static_cast<std::size_t>(Street::Turn);
constexpr std::size_t kRiver = static_cast<std::size_t>(Street::River);

SpotConfig spot_on(std::string_view board) {
  SpotConfig spot;
  spot.board = *parse_cards(board);
  spot.ranges[kOop] = *Range::parse("TT,77,AKo,KQs,QJs,98s,A5s");
  spot.ranges[kIp] = *Range::parse("AA,KK,KTs,AQs,JTs,87s,99");
  spot.tree.pot = 100;
  spot.tree.effective_stack = 250;
  for (const std::size_t street : {kFlop, kTurn, kRiver}) {
    spot.tree.sizing[street][kOop].bet = {0.5};
    spot.tree.sizing[street][kIp].bet = {0.75};
    spot.tree.sizing[street][kIp].raise = {0.6};
  }
  spot.tree.raise_cap = 1;
  return spot;
}

std::string saved_state(const Solver& solver) {
  std::ostringstream out(std::ios::binary);
  solver.save(out);
  return out.str();
}

TEST(SolverState, RoundTripsExactly) {
  Solver original(*make_game(spot_on("KsTs7h4h")));
  for (int i = 0; i < 25; ++i) original.iterate();
  const std::string state = saved_state(original);

  Solver restored(*make_game(spot_on("KsTs7h4h")));
  std::istringstream in(state, std::ios::binary);
  std::string error;
  ASSERT_TRUE(restored.load(in, &error)) << error;

  EXPECT_EQ(restored.iterations(), 25);
  const Evaluation a = original.evaluate();
  const Evaluation b = restored.evaluate();
  EXPECT_EQ(a.ev[0], b.ev[0]);
  EXPECT_EQ(a.exploitability, b.exploitability);
  EXPECT_EQ(original.report(GameTree::kRoot).strategy, restored.report(GameTree::kRoot).strategy);

  // Both continue identically: nothing that matters was left out of the file.
  original.iterate();
  restored.iterate();
  EXPECT_EQ(saved_state(original), saved_state(restored));
}

TEST(SolverState, RejectsForeignAndDamagedData) {
  Solver source(*make_game(spot_on("KsTs7h4h")));
  for (int i = 0; i < 5; ++i) source.iterate();
  const std::string state = saved_state(source);

  Solver target(*make_game(spot_on("KsTs7h4h")));
  for (int i = 0; i < 3; ++i) target.iterate();
  const std::string before = saved_state(target);
  std::string error;

  // Not a state file at all.
  std::istringstream garbage(std::string(4096, 'x'), std::ios::binary);
  EXPECT_FALSE(target.load(garbage, &error));
  EXPECT_FALSE(error.empty());

  // Empty.
  std::istringstream empty(std::string(), std::ios::binary);
  EXPECT_FALSE(target.load(empty, &error));

  // Cut short.
  std::istringstream truncated(state.substr(0, state.size() / 2), std::ios::binary);
  EXPECT_FALSE(target.load(truncated, &error));
  EXPECT_NE(error.find("incomplete"), std::string::npos) << error;

  // From a different spot.
  Solver other(*make_game(spot_on("KsTs7h4h2c")));
  other.iterate();
  std::istringstream foreign(saved_state(other), std::ios::binary);
  EXPECT_FALSE(target.load(foreign, &error));
  EXPECT_NE(error.find("different spot"), std::string::npos) << error;

  // None of the failed attempts changed anything.
  EXPECT_EQ(saved_state(target), before);
  EXPECT_EQ(target.iterations(), 3);
}

class Estimate : public ::testing::TestWithParam<const char*> {};

TEST_P(Estimate, MatchesTheBuiltGame) {
  for (const bool isomorphism : {true, false}) {
    SCOPED_TRACE(isomorphism ? "with isomorphism" : "without isomorphism");
    SpotConfig spot = spot_on(GetParam());
    spot.use_isomorphism = isomorphism;

    std::string error;
    const auto estimate = estimate_spot(spot, &error);
    ASSERT_TRUE(estimate.has_value()) << error;
    const auto game = make_game(spot, &error);
    ASSERT_TRUE(game.has_value()) << error;

    EXPECT_FALSE(estimate->tree.too_large);
    EXPECT_EQ(estimate->tree.nodes, game->tree.nodes.size());
    EXPECT_EQ(estimate->tree.action_nodes, game->tree.num_action_nodes());
    EXPECT_EQ(estimate->hands[kOop], game->hands[kOop].size());
    EXPECT_EQ(estimate->hands[kIp], game->hands[kIp].size());
    EXPECT_EQ(estimate->strategy_bytes, Solver::estimate_memory_bytes(*game));
    // The tree estimate covers its three large arrays.
    EXPECT_EQ(estimate->tree.tree_bytes(),
              game->tree.nodes.size() * sizeof(Node) +
                  game->tree.children.size() * sizeof(std::uint32_t) +
                  game->tree.actions.size() * sizeof(Action) + sizeof(std::uint32_t));
  }
}

INSTANTIATE_TEST_SUITE_P(Boards, Estimate,
                         ::testing::Values("KsTs7h4h2c", "KsTs7h4h", "Ks8d5h3c", "Ks9s4s", "Ks9s4d"));

TEST(Estimate, StopsCountingAtTheLimit) {
  SpotConfig spot = spot_on("Ks9s4d");
  spot.tree.max_nodes = 500;
  const auto estimate = estimate_spot(spot);
  ASSERT_TRUE(estimate.has_value());
  EXPECT_TRUE(estimate->tree.too_large);
  EXPECT_EQ(estimate->tree.nodes, 500u);
}

TEST(Estimate, ReportsUnsolvableSpots) {
  std::string error;
  SpotConfig bad_board = spot_on("Ks9s");
  EXPECT_FALSE(estimate_spot(bad_board, &error).has_value());

  SpotConfig no_hands = spot_on("Ks9s4d");
  no_hands.ranges[kIp] = Range{};
  EXPECT_FALSE(estimate_spot(no_hands, &error).has_value());
  EXPECT_NE(error.find("IP"), std::string::npos) << error;

  SpotConfig no_pot = spot_on("Ks9s4d");
  no_pot.tree.pot = 0;
  EXPECT_FALSE(estimate_spot(no_pot, &error).has_value());
}

}  // namespace
}  // namespace gto
