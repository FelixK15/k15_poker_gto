// Heads-up PLO4 through the vector-CFR Solver.

#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "gto/omaha.h"
#include "gto/solver.h"

namespace gto {
namespace {

constexpr std::size_t kRiver = static_cast<std::size_t>(Street::River);

/// OOP's share of the pot if both players check to showdown, from every pair
/// of hands and every completion of the board, with the reference evaluator.
double brute_force_share(const std::vector<Card>& board, const OmahaRange& oop,
                         const OmahaRange& ip) {
  std::uint64_t board_mask = 0;
  for (const Card card : board) board_mask |= card.mask();
  const auto mask_of = [](const std::array<Card, 4>& cards) {
    std::uint64_t mask = 0;
    for (const Card card : cards) mask |= card.mask();
    return mask;
  };

  std::vector<int> ip_combos;
  for (int j = 0; j < kOmahaCombos; ++j) {
    if (ip.weight(j) > 0) ip_combos.push_back(j);
  }

  double weighted_share = 0, total_weight = 0;
  std::vector<Card> cards;
  for (int i = 0; i < kOmahaCombos; ++i) {
    if (!(oop.weight(i) > 0)) continue;
    const auto mine = omaha_combo_cards(i);
    const std::uint64_t my_mask = mask_of(mine);
    if ((my_mask & board_mask) != 0) continue;
    for (const int j : ip_combos) {
      const auto theirs = omaha_combo_cards(j);
      const std::uint64_t their_mask = mask_of(theirs);
      if ((their_mask & (board_mask | my_mask)) != 0) continue;
      const std::uint64_t used = board_mask | my_mask | their_mask;

      double share = 0;
      int runouts = 0;
      const auto showdown = [&](const std::vector<Card>& full) {
        const HandStrength s1 = evaluate_omaha(mine, full);
        const HandStrength s2 = evaluate_omaha(theirs, full);
        share += s1 > s2 ? 1.0 : s1 == s2 ? 0.5 : 0.0;
        ++runouts;
      };
      if (board.size() == 5) {
        showdown(board);
      } else {
        for (int x = 0; x < 52; ++x) {
          const Card card(static_cast<std::uint8_t>(x));
          if ((used & card.mask()) != 0) continue;
          cards = board;
          cards.push_back(card);
          showdown(cards);
        }
      }
      const double weight = static_cast<double>(oop.weight(i)) * ip.weight(j);
      weighted_share += weight * share / runouts;
      total_weight += weight;
    }
  }
  return weighted_share / total_weight;
}

SpotConfig plo_spot(std::string_view board, std::string_view oop, std::string_view ip) {
  SpotConfig spot;
  spot.variant = Variant::Omaha4;
  spot.board = *parse_cards(board);
  spot.omaha_ranges[kOop] = OmahaRange::parse(oop);
  spot.omaha_ranges[kIp] = OmahaRange::parse(ip);
  EXPECT_TRUE(spot.omaha_ranges[kOop].has_value()) << oop;
  EXPECT_TRUE(spot.omaha_ranges[kIp].has_value()) << ip;
  spot.tree.pot = 100;
  spot.tree.pot_limit = true;
  return spot;
}

struct EquityCase {
  const char* board;
  const char* oop;
  const char* ip;
};

class PloCheckDown : public ::testing::TestWithParam<EquityCase> {};

TEST_P(PloCheckDown, EvEqualsEquity) {
  const EquityCase& c = GetParam();
  for (const bool isomorphism : {true, false}) {
    SCOPED_TRACE(isomorphism ? "with isomorphism" : "without isomorphism");
    SpotConfig spot = plo_spot(c.board, c.oop, c.ip);
    spot.tree.effective_stack = 0;  // nothing to bet: the hand is checked down
    spot.use_isomorphism = isomorphism;
    std::string error;
    auto game = make_game(spot, &error);
    ASSERT_TRUE(game.has_value()) << error;
    ASSERT_EQ(game->hands[0].front().num_cards, 4);

    Solver solver(std::move(*game));
    const Evaluation evaluation = solver.evaluate();
    const double expected =
        100.0 * brute_force_share(spot.board, *spot.omaha_ranges[kOop], *spot.omaha_ranges[kIp]);
    EXPECT_NEAR(evaluation.ev[kOop], expected, 0.01);
    EXPECT_NEAR(evaluation.ev[kIp], 100.0 - expected, 0.01);
    EXPECT_NEAR(evaluation.exploitability, 0.0, 1e-3);

    // The equity report agrees.
    const NodeReport root = solver.report(0);
    EXPECT_NEAR(root.range_equity[kOop], expected / 100.0, 1e-4);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Plo, PloCheckDown,
    ::testing::Values(
        // River: no cards to come.
        EquityCase{"Ks8d5h3s2d", "AAKK, QQJJds", "KQJT, 9876ds"},
        // River with a four-flush on the board: one heart in hand is no flush.
        EquityCase{"Ah9h6h3h2c", "KhQsJsTd, KKxxds:0.3", "QJT9ss:0.5, 5544"},
        // Turn, two-tone: clubs and diamonds are interchangeable.
        EquityCase{"KsTs7h4h", "AAQQ", "JT98ds"},
        // Turn, ranges that tell suits apart.
        EquityCase{"Kh9h4c2s", "AsAhKsQh, AcAdKcKd:0.5", "JhTh9d8d, QQ77"}));

TEST(PloRiver, SolvesToLowExploitability) {
  SpotConfig spot = plo_spot("Kd9s6h4c2s", "AAxxds, KKxxds, QJT9, JT98ss",
                             "KQJx:0.3, 9966, 7653, AAKK, 5432ds");
  spot.tree.effective_stack = 300;
  for (const int player : {kOop, kIp}) {
    BetSizing& sizing = spot.tree.sizing[kRiver][static_cast<std::size_t>(player)];
    sizing.bet = {0.5, 1.0};
    sizing.raise = {1.0};
    sizing.add_allin = true;
  }
  std::string error;
  auto game = make_game(spot, &error);
  ASSERT_TRUE(game.has_value()) << error;
  // Pot limit: nothing larger than the pot at the root.
  for (const Action& action : game->tree.actions_of(0)) EXPECT_LE(action.amount, 100.0 + 1e-9);

  Solver solver(std::move(*game));
  SolveOptions options;
  options.max_iterations = 600;
  options.target_exploitability_pct = 0.2;
  const SolveResult result = solver.solve(options);
  EXPECT_LT(result.evaluation.exploitability_pct, 0.3);
  EXPECT_NEAR(result.evaluation.ev[0] + result.evaluation.ev[1], 100.0, 0.05);

  // Reports name four-card hands.
  const NodeReport root = solver.report(0);
  ASSERT_FALSE(root.strategy.empty());
  EXPECT_EQ(hand_name(solver.game().hands[0].front()).size(), 8u);
}

TEST(PloTurn, IsomorphismDoesNotChangeTheSolution) {
  SpotConfig spot = plo_spot("KsTs7h4h", "AAQQ, JJ99", "KQJ8:0.2, 6655");
  spot.tree.effective_stack = 200;
  for (const std::size_t street : {std::size_t{1}, kRiver}) {
    for (const int player : {kOop, kIp}) {
      spot.tree.sizing[street][static_cast<std::size_t>(player)].bet = {1.0};
    }
  }
  SpotConfig full_spot = spot;
  full_spot.use_isomorphism = false;
  auto merged_game = make_game(spot);
  auto full_game = make_game(full_spot);
  ASSERT_TRUE(merged_game && full_game);
  EXPECT_LT(merged_game->tree.nodes.size(), full_game->tree.nodes.size());
  Solver merged(std::move(*merged_game));
  Solver full(std::move(*full_game));

  // After one iteration the two agree to rounding (later they drift apart
  // on the way to the same equilibrium, see HoldemTurn).
  merged.iterate();
  full.iterate();
  EXPECT_NEAR(merged.evaluate().ev[0], full.evaluate().ev[0], 1e-3);
  // Including on a river card only the full tree stores (7d is merged into 7c).
  for (const char* path : {"check/check/7d", "check/check/7d/check", "bet100/call/2d"}) {
    SCOPED_TRACE(path);
    const NodeReport x = merged.report(*merged.game().tree.find(path));
    const NodeReport y = full.report(*full.game().tree.find(path));
    ASSERT_EQ(x.strategy.size(), y.strategy.size());
    for (std::size_t player = 0; player < 2; ++player) {
      for (std::size_t h = 0; h < x.reach[player].size(); ++h) {
        EXPECT_NEAR(x.reach[player][h], y.reach[player][h], 1e-3);
        EXPECT_NEAR(x.ev[player][h], y.ev[player][h], 0.02);
      }
    }
    for (std::size_t i = 0; i < x.strategy.size(); ++i) {
      EXPECT_NEAR(x.strategy[i], y.strategy[i], 1e-3);
    }
  }

  for (int i = 0; i < 150; ++i) {
    merged.iterate();
    full.iterate();
  }
  const Evaluation a = merged.evaluate();
  const Evaluation b = full.evaluate();
  EXPECT_LT(a.exploitability_pct, 0.5);
  EXPECT_LT(b.exploitability_pct, 0.5);
  EXPECT_NEAR(a.ev[0], b.ev[0], 0.1);
}

}  // namespace
}  // namespace gto
