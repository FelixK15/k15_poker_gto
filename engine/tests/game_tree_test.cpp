#include "gto/game_tree.h"

#include <gtest/gtest.h>

namespace gto {
namespace {

constexpr std::size_t kRiver = static_cast<std::size_t>(Street::River);

GameTree build_ok(const TreeConfig& config) {
  std::string error;
  auto tree = build_tree(config, &error);
  EXPECT_TRUE(tree.has_value()) << error;
  return tree.value_or(GameTree{});
}

std::vector<std::string> labels(const GameTree& tree, std::string_view path) {
  const auto node = tree.find(path);
  EXPECT_TRUE(node.has_value()) << path;
  std::vector<std::string> result;
  if (node) {
    for (const Action& action : tree.actions_of(node->node)) {
      result.push_back(action_label(action));
    }
  }
  return result;
}

using Labels = std::vector<std::string>;

TEST(GameTree, KuhnShape) {
  TreeConfig config;
  config.pot = 2;
  config.effective_stack = 1;
  config.sizing[kRiver][kOop].add_allin = true;
  config.sizing[kRiver][kIp].add_allin = true;
  const GameTree tree = build_ok(config);

  EXPECT_EQ(tree.nodes.size(), 9u);
  EXPECT_EQ(tree.num_action_nodes(), 4u);
  EXPECT_EQ(labels(tree, ""), (Labels{"check", "allin"}));
  EXPECT_EQ(labels(tree, "check"), (Labels{"check", "allin"}));
  EXPECT_EQ(labels(tree, "allin"), (Labels{"fold", "call"}));
  EXPECT_EQ(labels(tree, "check/allin"), (Labels{"fold", "call"}));

  const Node& showdown = tree.nodes[tree.find("check/allin/call")->node];
  EXPECT_EQ(showdown.type, NodeType::Showdown);
  EXPECT_DOUBLE_EQ(showdown.committed[0], 1);
  EXPECT_DOUBLE_EQ(showdown.committed[1], 1);

  const Node& fold = tree.nodes[tree.find("allin/fold")->node];
  EXPECT_EQ(fold.type, NodeType::Fold);
  EXPECT_EQ(fold.player, kIp);
  EXPECT_DOUBLE_EQ(fold.committed[0], 1);
  EXPECT_DOUBLE_EQ(fold.committed[1], 0);

  EXPECT_EQ(tree.nodes[tree.find("check/check")->node].type, NodeType::Showdown);
}

TEST(GameTree, BetAndRaiseSizes) {
  TreeConfig config;
  config.pot = 100;
  config.effective_stack = 10000;
  config.sizing[kRiver][kOop].bet = {0.5};
  config.sizing[kRiver][kOop].raise = {1.0};
  config.sizing[kRiver][kIp].bet = {0.33, 0.75};
  config.sizing[kRiver][kIp].raise = {1.0};
  const GameTree tree = build_ok(config);

  EXPECT_EQ(labels(tree, ""), (Labels{"check", "bet50"}));
  EXPECT_EQ(labels(tree, "check"), (Labels{"check", "bet33", "bet75"}));
  // Pot-size raise over a bet of 50 into 100: call 50, then raise by 200.
  EXPECT_EQ(labels(tree, "bet50"), (Labels{"fold", "call", "raise250"}));
  const Node& raised = tree.nodes[tree.find("bet50/raise250")->node];
  EXPECT_DOUBLE_EQ(raised.committed[kOop], 50);
  EXPECT_DOUBLE_EQ(raised.committed[kIp], 250);
  // Re-raise: call 200 more (pot 600), raise by 600 to 850.
  EXPECT_EQ(labels(tree, "bet50/raise250"), (Labels{"fold", "call", "raise850"}));
}

TEST(GameTree, PathRoundTrips) {
  TreeConfig config;
  config.pot = 100;
  config.effective_stack = 1000;
  config.sizing[kRiver][kOop].bet = {0.5};
  config.sizing[kRiver][kIp].raise = {1.0};
  const GameTree tree = build_ok(config);

  for (std::uint32_t node = 0; node < tree.nodes.size(); ++node) {
    EXPECT_EQ(tree.find(tree.path(node))->node, node);
  }
  EXPECT_EQ(tree.find("BET50/Call")->node, tree.find("bet50/call")->node);

  std::string error;
  EXPECT_FALSE(tree.find("bet60", &error).has_value());
  EXPECT_NE(error.find("bet50"), std::string::npos) << error;
}

TEST(GameTree, RaiseCapStopsTheRaising) {
  TreeConfig config;
  config.pot = 100;
  config.effective_stack = 100000;
  config.raise_cap = 1;
  for (int player = 0; player < 2; ++player) {
    config.sizing[kRiver][static_cast<std::size_t>(player)].bet = {0.5};
    config.sizing[kRiver][static_cast<std::size_t>(player)].raise = {1.0};
  }
  const GameTree tree = build_ok(config);
  EXPECT_EQ(labels(tree, "bet50").size(), 3u);
  EXPECT_EQ(labels(tree, "bet50/raise250"), (Labels{"fold", "call"}));

  config.raise_cap = 0;
  EXPECT_EQ(labels(build_ok(config), "bet50"), (Labels{"fold", "call"}));
}

TEST(GameTree, LargeBetsBecomeAllIn) {
  TreeConfig config;
  config.pot = 100;
  config.effective_stack = 100;
  config.sizing[kRiver][kOop].bet = {0.25, 0.75, 2.0};
  config.sizing[kRiver][kOop].add_allin = true;
  // 75 is at least 67% of the stack and 200 exceeds it: all three collapse.
  EXPECT_EQ(labels(build_ok(config), ""), (Labels{"check", "bet25", "allin"}));

  config.allin_threshold = 0;
  EXPECT_EQ(labels(build_ok(config), ""), (Labels{"check", "bet25", "bet75", "allin"}));
}

TEST(GameTree, RaisesAreAtLeastTheLastBet) {
  TreeConfig config;
  config.pot = 100;
  config.effective_stack = 10000;
  config.sizing[kRiver][kOop].bet = {1.0};
  config.sizing[kRiver][kIp].raise = {0.1};
  // 10% of the pot would be a raise of 30, less than the bet of 100.
  EXPECT_EQ(labels(build_ok(config), "bet100"), (Labels{"fold", "call", "raise200"}));
}

TEST(GameTree, NoRaiseAfterAnAllIn) {
  TreeConfig config;
  config.pot = 100;
  config.effective_stack = 50;
  config.sizing[kRiver][kOop].add_allin = true;
  config.sizing[kRiver][kIp].add_allin = true;
  config.sizing[kRiver][kIp].raise = {1.0};
  EXPECT_EQ(labels(build_ok(config), "allin"), (Labels{"fold", "call"}));
}

TEST(GameTree, NoStackMeansCheckDown) {
  TreeConfig config;
  config.pot = 100;
  config.effective_stack = 0;
  config.sizing[kRiver][kOop].bet = {0.5};
  config.sizing[kRiver][kOop].add_allin = true;
  const GameTree tree = build_ok(config);
  EXPECT_EQ(labels(tree, ""), (Labels{"check"}));
  EXPECT_EQ(tree.nodes.size(), 3u);
}

TEST(GameTree, RejectsBadConfigs) {
  TreeConfig config;
  config.pot = 100;
  config.effective_stack = 100;
  std::string error;

  TreeConfig no_pot = config;
  no_pot.pot = 0;
  EXPECT_FALSE(build_tree(no_pot, &error).has_value());
  EXPECT_FALSE(error.empty());

  TreeConfig negative_size = config;
  negative_size.sizing[kRiver][kOop].bet = {-0.5};
  EXPECT_FALSE(build_tree(negative_size, &error).has_value());

  TreeConfig bad_threshold = config;
  bad_threshold.allin_threshold = 1.5;
  EXPECT_FALSE(build_tree(bad_threshold, &error).has_value());
}

}  // namespace
}  // namespace gto

namespace gto {
namespace {

TreeConfig pot_limit_config(double stack, bool pot_limit) {
  TreeConfig config;
  config.pot = 10;
  config.effective_stack = stack;
  config.pot_limit = pot_limit;
  for (const int player : {kOop, kIp}) {
    BetSizing& sizing = config.sizing[kRiver][static_cast<std::size_t>(player)];
    sizing.bet = {0.5, 2.0};
    sizing.raise = {1.0, 3.0};
    sizing.add_allin = true;
  }
  return config;
}

TEST(GameTree, PotLimitCapsBetsAndRaises) {
  const GameTree tree = build_ok(pot_limit_config(100, true));
  // 2x pot becomes a pot-size bet, and all-in beyond the pot becomes the same.
  EXPECT_EQ(labels(tree, ""), (Labels{"check", "bet5", "bet10"}));
  // Facing 5 into 10: call 5, then raise the 20 in the pot: to 25.
  EXPECT_EQ(labels(tree, "bet5"), (Labels{"fold", "call", "raise25"}));
  // Facing a pot bet of 10 into 10: call, raise 30 more: to 40.
  EXPECT_EQ(labels(tree, "bet10"), (Labels{"fold", "call", "raise40"}));
  // Facing 25 with 40 in the pot: the call makes it 60, so the re-raise
  // goes to 25 + 60 = 85.
  EXPECT_EQ(labels(tree, "bet5/raise25"), (Labels{"fold", "call", "raise85"}));
  // Beyond the stack: 100 is within the pot-size raise, so all-in.
  EXPECT_EQ(labels(tree, "bet10/raise40"), (Labels{"fold", "call", "allin"}));

  // The same sizes without the cap.
  const GameTree no_limit = build_ok(pot_limit_config(100, false));
  EXPECT_EQ(labels(no_limit, ""), (Labels{"check", "bet5", "bet20", "allin"}));
}

TEST(GameTree, PotLimitAllInOnlyWithinTheCap) {
  // 8 behind with 10 in the pot: every bet of at least the threshold may go
  // all-in, since all-in is below a pot-size bet.
  const GameTree shallow = build_ok(pot_limit_config(8, true));
  EXPECT_EQ(labels(shallow, ""), (Labels{"check", "bet5", "allin"}));

  // 14 behind: a pot bet (10) is past the all-in threshold (0.67 * 14) but
  // all-in itself is beyond the pot, so the pot bet stays.
  const GameTree deeper = build_ok(pot_limit_config(14, true));
  EXPECT_EQ(labels(deeper, ""), (Labels{"check", "bet5", "bet10"}));
  const GameTree deeper_nl = build_ok(pot_limit_config(14, false));
  EXPECT_EQ(labels(deeper_nl, ""), (Labels{"check", "bet5", "allin"}));
}

}  // namespace
}  // namespace gto
