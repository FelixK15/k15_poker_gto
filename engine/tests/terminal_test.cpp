#include "gto/terminal.h"

#include <cmath>
#include <random>
#include <set>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "gto/omaha.h"
#include "gto/range.h"

namespace gto {
namespace {

struct Sets {
  Hands hands;
  ShowdownTable table;
};

/// The obvious O(hands²) computation the fast evaluator must agree with.
std::vector<double> naive_showdown(const Sets& sets, int player, double win, double tie,
                                   double lose, const std::vector<float>& opponent_reach) {
  const auto p = static_cast<std::size_t>(player);
  const auto& hands = sets.hands[p];
  const auto& opponent_hands = sets.hands[1 - p];
  const auto& strength = sets.table.strength[p];
  const auto& opponent_strength = sets.table.strength[1 - p];

  std::vector<double> values(hands.size(), 0.0);
  for (std::size_t i = 0; i < hands.size(); ++i) {
    for (std::size_t j = 0; j < opponent_hands.size(); ++j) {
      if ((hands[i].mask & opponent_hands[j].mask) != 0) continue;
      const double payoff =
          strength[i] > opponent_strength[j] ? win : strength[i] < opponent_strength[j] ? lose : tie;
      values[i] += payoff * opponent_reach[j];
    }
  }
  return values;
}

Sets random_holdem_sets(std::mt19937& rng, std::string_view board_text) {
  const auto board = *parse_cards(board_text);
  std::uint64_t board_mask = 0;
  for (const Card card : board) board_mask |= card.mask();

  std::bernoulli_distribution keep(0.6);
  std::uniform_real_distribution<float> weight(0.05f, 1.0f);
  Sets sets;
  std::array<std::vector<HandStrength>, 2> strength;
  for (std::size_t player = 0; player < 2; ++player) {
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      const auto [a, b] = Range::combo_cards(combo);
      if (((a.mask() | b.mask()) & board_mask) != 0 || !keep(rng)) continue;
      std::vector<Card> cards = board;
      cards.push_back(a);
      cards.push_back(b);
      sets.hands[player].push_back(make_hand({a.index(), b.index()}, weight(rng)));
      strength[player].push_back(evaluate_hand(cards));
    }
  }
  sets.table = make_showdown_table(std::move(strength));
  return sets;
}

std::vector<float> random_reach(std::mt19937& rng, std::size_t size) {
  std::uniform_real_distribution<float> reach(0.0f, 1.0f);
  std::bernoulli_distribution zero(0.2);
  std::vector<float> result(size);
  for (float& value : result) value = zero(rng) ? 0.0f : reach(rng);
  return result;
}

void expect_matches_naive(const Sets& sets, std::mt19937& rng, bool prepared);

/// Checks both ways of evaluating a showdown: worked out on the spot, and
/// from the steps prepared in advance.
void expect_matches_naive(const Sets& sets, std::mt19937& rng) {
  expect_matches_naive(sets, rng, false);
  expect_matches_naive(sets, rng, true);
}

void expect_matches_naive(const Sets& given, std::mt19937& rng, bool prepared) {
  SCOPED_TRACE(prepared ? "prepared" : "general");
  Sets sets = given;
  const TerminalEvaluator evaluator(sets.hands);
  if (prepared) evaluator.prepare(sets.table);
  for (int player = 0; player < 2; ++player) {
    const auto p = static_cast<std::size_t>(player);
    const auto reach = random_reach(rng, sets.hands[1 - p].size());
    std::vector<float> fast(sets.hands[p].size());

    evaluator.showdown(sets.table, player, 130.0, 40.0, -50.0, reach.data(), fast.data());
    const auto slow = naive_showdown(sets, player, 130.0, 40.0, -50.0, reach);
    for (std::size_t i = 0; i < fast.size(); ++i) {
      ASSERT_NEAR(fast[i], slow[i], 1e-2 + 1e-4 * std::abs(slow[i])) << "showdown hand " << i;
    }

    // Nothing lost on a loss: the pass that adds it is skipped.
    evaluator.showdown(sets.table, player, 60.0, 30.0, 0.0, reach.data(), fast.data());
    const auto slow_free = naive_showdown(sets, player, 60.0, 30.0, 0.0, reach);
    for (std::size_t i = 0; i < fast.size(); ++i) {
      ASSERT_NEAR(fast[i], slow_free[i], 1e-2 + 1e-4 * std::abs(slow_free[i]))
          << "free showdown hand " << i;
    }

    evaluator.fold(player, 75.0, reach.data(), fast.data());
    const auto slow_fold = naive_showdown(sets, player, 75.0, 75.0, 75.0, reach);
    for (std::size_t i = 0; i < fast.size(); ++i) {
      ASSERT_NEAR(fast[i], slow_fold[i], 1e-2 + 1e-4 * std::abs(slow_fold[i]))
          << "fold hand " << i;
    }

    evaluator.compatible_reach(player, reach.data(), fast.data());
    const auto slow_reach = naive_showdown(sets, player, 1.0, 1.0, 1.0, reach);
    for (std::size_t i = 0; i < fast.size(); ++i) {
      ASSERT_NEAR(fast[i], slow_reach[i], 1e-3 + 1e-4 * std::abs(slow_reach[i]))
          << "reach hand " << i;
    }
  }
}

TEST(TerminalEvaluator, MatchesBruteForceOnHoldemBoards) {
  std::mt19937 rng(7);
  // A dry board, a paired board with many ties, and a board that plays for everyone.
  for (const char* board : {"Ks8d5h3s2d", "QcQd7h7s2c", "AcKcQcJcTc", "9h8h7h6d2s"}) {
    SCOPED_TRACE(board);
    for (int trial = 0; trial < 3; ++trial) {
      expect_matches_naive(random_holdem_sets(rng, board), rng);
    }
  }
}

TEST(TerminalEvaluator, MatchesBruteForceWithOneHoleCard) {
  std::mt19937 rng(11);
  Sets sets;
  std::array<std::vector<HandStrength>, 2> strength;
  for (std::size_t player = 0; player < 2; ++player) {
    for (int card = 0; card < 13; ++card) {
      if (player == 1 && card == 4) continue;  // ranges need not be identical
      sets.hands[player].push_back(make_hand({card}, 1.0f));
      strength[player].push_back(static_cast<HandStrength>(card / 2));  // with ties
    }
  }
  sets.table = make_showdown_table(std::move(strength));
  for (int trial = 0; trial < 5; ++trial) expect_matches_naive(sets, rng);
}

/// `count` random PLO hands per player off the board, a few of them held by
/// both players so that identical hands occur.
Sets random_omaha_sets(std::mt19937& rng, std::string_view board_text, std::size_t count) {
  const auto board = *parse_cards(board_text);
  std::uint64_t board_mask = 0;
  for (const Card card : board) board_mask |= card.mask();
  const OmahaBoardEvaluator evaluator(board);

  std::uniform_int_distribution<int> combo(0, kOmahaCombos - 1);
  std::uniform_real_distribution<float> weight(0.05f, 1.0f);
  std::bernoulli_distribution share(0.1);
  Sets sets;
  std::array<std::vector<HandStrength>, 2> strength;
  std::array<std::set<int>, 2> taken;
  const auto add = [&](std::size_t player, int index) {
    const auto cards = omaha_combo_cards(index);
    std::uint64_t mask = 0;
    for (const Card card : cards) mask |= card.mask();
    if ((mask & board_mask) != 0 || !taken[player].insert(index).second) return;
    sets.hands[player].push_back(make_hand(
        {cards[0].index(), cards[1].index(), cards[2].index(), cards[3].index()}, weight(rng)));
    strength[player].push_back(evaluator.evaluate(cards));
  };
  while (sets.hands[0].size() < count) add(0, combo(rng));
  for (const int index : taken[0]) {
    if (share(rng)) add(1, index);
  }
  while (sets.hands[1].size() < count) add(1, combo(rng));
  sets.table = make_showdown_table(std::move(strength));
  return sets;
}

TEST(TerminalEvaluator, MatchesBruteForceWithFourHoleCards) {
  std::mt19937 rng(19);
  for (const char* board : {"Ks8d5h3s2d", "QcQd7h7s2c", "AcKcQcJcTc", "9h8h7h6d2s"}) {
    SCOPED_TRACE(board);
    for (const std::size_t count : {1u, 5u, 40u, 300u}) {
      SCOPED_TRACE(count);
      expect_matches_naive(random_omaha_sets(rng, board, count), rng);
    }
  }
  // Large sets: many hands share pairs and triples of cards.
  expect_matches_naive(random_omaha_sets(rng, "Ks8d5h3s2d", 4000), rng);
}

TEST(TerminalEvaluator, FourCardsAllHandsOfFewCards) {
  // Every four-card hand out of a ten-card deck for both players: nearly all
  // of them overlap, which exercises every inclusion-exclusion term.
  std::mt19937 rng(23);
  const auto board = *parse_cards("2c3d4h5s7c");
  const OmahaBoardEvaluator evaluator(board);
  const auto deck = *parse_cards("AsAhKsKhQsQhJsJhTsTh");
  Sets sets;
  std::array<std::vector<HandStrength>, 2> strength;
  for (std::size_t player = 0; player < 2; ++player) {
    for (std::size_t a = 0; a < deck.size(); ++a)
      for (std::size_t b = a + 1; b < deck.size(); ++b)
        for (std::size_t c = b + 1; c < deck.size(); ++c)
          for (std::size_t d = c + 1; d < deck.size(); ++d) {
            sets.hands[player].push_back(make_hand(
                {deck[a].index(), deck[b].index(), deck[c].index(), deck[d].index()}, 1.0f));
            strength[player].push_back(evaluator.evaluate({deck[a], deck[b], deck[c], deck[d]}));
          }
  }
  sets.table = make_showdown_table(std::move(strength));
  for (int trial = 0; trial < 3; ++trial) expect_matches_naive(sets, rng);
}

TEST(TerminalEvaluator, BoardThatPlaysTiesEverything) {
  std::mt19937 rng(3);
  const Sets sets = random_holdem_sets(rng, "AcKcQcJcTc");
  const TerminalEvaluator evaluator(sets.hands);
  const std::vector<float> reach(sets.hands[1].size(), 1.0f);
  std::vector<float> values(sets.hands[0].size());
  std::vector<float> compatible(sets.hands[0].size());

  evaluator.showdown(sets.table, 0, 100.0, 7.0, -100.0, reach.data(), values.data());
  evaluator.compatible_reach(0, reach.data(), compatible.data());
  for (std::size_t i = 0; i < values.size(); ++i) {
    EXPECT_NEAR(values[i], 7.0 * compatible[i], 1e-2);
  }
}

TEST(ShowdownTable, OrdersHandsByStrength) {
  const ShowdownTable table = make_showdown_table({{{5, 1, 9, 1}, {2}}});
  ASSERT_EQ(table.order[0].size(), 4u);
  for (std::size_t i = 1; i < 4; ++i) {
    EXPECT_LE(table.strength[0][table.order[0][i - 1]], table.strength[0][table.order[0][i]]);
  }
  EXPECT_EQ(table.order[0].back(), 2u);
  EXPECT_EQ(table.order[1], (std::vector<std::uint32_t>{0}));
}

}  // namespace
}  // namespace gto
