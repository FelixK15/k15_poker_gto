#include "gto/omaha.h"

#include <algorithm>
#include <bit>
#include <random>
#include <set>
#include <string_view>

#include <gtest/gtest.h>

namespace gto {
namespace {

std::array<Card, 4> hole(std::string_view text) {
  const auto cards = *parse_cards(text);
  return {cards[0], cards[1], cards[2], cards[3]};
}

/// Every five-card subset of hole + board that uses exactly two hole cards,
/// found by enumerating bit masks over all the cards.
HandStrength brute_force(const std::array<Card, 4>& hand, const std::vector<Card>& board) {
  std::vector<Card> all(hand.begin(), hand.end());
  all.insert(all.end(), board.begin(), board.end());
  const int n = static_cast<int>(all.size());
  HandStrength best = 0;
  for (unsigned mask = 0; mask < (1u << n); ++mask) {
    if (std::popcount(mask) != 5 || std::popcount(mask & 0xFu) != 2) continue;
    std::vector<Card> five;
    for (int i = 0; i < n; ++i) {
      if (mask & (1u << i)) five.push_back(all[static_cast<std::size_t>(i)]);
    }
    best = std::max(best, evaluate_hand(five));
  }
  return best;
}

/// Deals four hole cards and `board_size` board cards.
std::pair<std::array<Card, 4>, std::vector<Card>> deal(std::mt19937& rng, int board_size) {
  std::vector<int> deck(52);
  for (int i = 0; i < 52; ++i) deck[static_cast<std::size_t>(i)] = i;
  std::shuffle(deck.begin(), deck.end(), rng);
  std::array<Card, 4> hand;
  for (std::size_t i = 0; i < 4; ++i) hand[i] = Card(static_cast<std::uint8_t>(deck[i]));
  std::vector<Card> board;
  for (int i = 0; i < board_size; ++i) {
    board.emplace_back(static_cast<std::uint8_t>(deck[4 + static_cast<std::size_t>(i)]));
  }
  return {hand, board};
}

TEST(Omaha, ComboIndexRoundTripsForEveryCombo) {
  std::vector<bool> seen(kOmahaCombos, false);
  for (int index = 0; index < kOmahaCombos; ++index) {
    const auto cards = omaha_combo_cards(index);
    ASSERT_TRUE(cards[0] < cards[1] && cards[1] < cards[2] && cards[2] < cards[3]) << index;
    ASSERT_EQ(omaha_combo_index(cards), index);
    // Order does not matter.
    ASSERT_EQ(omaha_combo_index({cards[3], cards[1], cards[0], cards[2]}), index);
    seen[static_cast<std::size_t>(index)] = true;
  }
  EXPECT_EQ(omaha_combo_index(hole("2c2d2h2s")), 0);
  EXPECT_EQ(omaha_combo_index(hole("AsAhAdKs")), kOmahaCombos - 2);
  EXPECT_EQ(omaha_combo_index(hole("AsAhAdAc")), kOmahaCombos - 1);
}

TEST(Omaha, HandNameOrdersByRank) {
  EXPECT_EQ(omaha_hand_name(hole("JhQhKsAs")), "AsKsQhJh");
  EXPECT_EQ(omaha_hand_name(hole("2c9d9s3h")), "9s9d3h2c");
}

TEST(Omaha, MustUseExactlyTwoHoleCards) {
  const auto board = *parse_cards("AhKhQh2c3d");
  // Four hearts would be a flush in Hold'em; with one heart in hand it is not.
  const auto one_heart = *parse_cards("AhKhQh5h2c");
  EXPECT_NE(hand_category(evaluate_omaha(hole("Jh9s8s7c"), one_heart)), HandCategory::Flush);
  EXPECT_EQ(hand_category(evaluate_omaha(hole("Jh9h8s7c"), one_heart)), HandCategory::Flush);

  // A straight on the board does not play by itself.
  const auto straight_board = *parse_cards("9c8d7h6s5c");
  EXPECT_NE(hand_category(evaluate_omaha(hole("AhAdKsKc"), straight_board)),
            HandCategory::Straight);
  EXPECT_EQ(hand_category(evaluate_omaha(hole("AhAdKsKc"), straight_board)), HandCategory::Pair);

  // Quads on the board: the best hand is trips (three of them) with two kickers.
  const auto quads_board = *parse_cards("7c7d7h7sKd");
  const HandStrength quads = evaluate_omaha(hole("AhAs2c3d"), quads_board);
  EXPECT_EQ(hand_category(quads), HandCategory::FullHouse);  // 777 + AA
  EXPECT_EQ(hand_category(evaluate_omaha(hole("AhQs2c3d"), quads_board)), HandCategory::Trips);

  // Trips on the board: two unpaired hole cards only add kickers (a single
  // king cannot pair the board as in Hold'em), a pair in hand fills up, and
  // the fourth nine with any card makes quads.
  const auto trips_board = *parse_cards("9c9d9h4s2d");
  EXPECT_EQ(hand_category(evaluate_omaha(hole("KsQd8c3h"), trips_board)), HandCategory::Trips);
  EXPECT_EQ(hand_category(evaluate_omaha(hole("9sKdQc3h"), trips_board)), HandCategory::Quads);
  EXPECT_EQ(hand_category(evaluate_omaha(hole("KsKdQc3h"), trips_board)), HandCategory::FullHouse);

  // Two pair on the board is only one pair: three board cards can hold one
  // of them.
  EXPECT_EQ(hand_category(evaluate_omaha(hole("AsQd8c3h"), *parse_cards("KcKd7h7s2c"))),
            HandCategory::Pair);

  // Two hole cards of a straight are needed.
  EXPECT_EQ(hand_category(evaluate_omaha(hole("JsTd2c2h"), board)), HandCategory::Straight);
  EXPECT_NE(hand_category(evaluate_omaha(hole("JsAc2c2h"), board)), HandCategory::Straight);
}

TEST(Omaha, EvaluatorMatchesBruteForce) {
  std::mt19937 rng(42);
  for (int trial = 0; trial < 20000; ++trial) {
    const int board_size = 3 + trial % 3;
    const auto [hand, board] = deal(rng, board_size);
    ASSERT_EQ(evaluate_omaha(hand, board), brute_force(hand, board)) << omaha_hand_name(hand);
  }
}

TEST(Omaha, BoardEvaluatorMatchesReference) {
  std::mt19937 rng(5);
  for (int board_size = 3; board_size <= 5; ++board_size) {
    for (int b = 0; b < 30; ++b) {
      const auto [first, board] = deal(rng, board_size);
      (void)first;
      const OmahaBoardEvaluator evaluator(board);
      std::uint64_t board_mask = 0;
      for (const Card card : board) board_mask |= card.mask();
      for (int trial = 0; trial < 400; ++trial) {
        const int combo = static_cast<int>(rng() % kOmahaCombos);
        const auto hand = omaha_combo_cards(combo);
        std::uint64_t mask = 0;
        for (const Card card : hand) mask |= card.mask();
        if ((mask & board_mask) != 0) continue;
        ASSERT_EQ(evaluator.evaluate(hand), evaluate_omaha(hand, board));
      }
    }
  }
}

TEST(OmahaRange, ParsesTheDocumentedSyntax) {
  std::string error;
  const auto empty = OmahaRange::parse("", &error);
  ASSERT_TRUE(empty.has_value());
  EXPECT_EQ(empty->total_weight(), 0.0);

  EXPECT_EQ(OmahaRange::parse("*")->total_weight(), kOmahaCombos);
  EXPECT_EQ(OmahaRange::parse("all:0.5")->total_weight(), kOmahaCombos * 0.5);

  const auto exact = OmahaRange::parse("AsKsQhJh:0.25");
  ASSERT_TRUE(exact.has_value());
  EXPECT_DOUBLE_EQ(exact->total_weight(), 0.25);
  EXPECT_FLOAT_EQ(exact->weight(omaha_combo_index(hole("JhQhKsAs"))), 0.25f);

  // AAKK: 6 · 6 combos.
  EXPECT_EQ(OmahaRange::parse("AAKK")->total_weight(), 36.0);
  // AKQJ: 4^4.
  EXPECT_EQ(OmahaRange::parse("AKQJ")->total_weight(), 256.0);
  // AAKKds: the aces and the kings in the same two suits.
  EXPECT_EQ(OmahaRange::parse("AAKKds")->total_weight(), 6.0);
  // AKQJ by suits; with the rest (three or four of a suit) they add up.
  const double ds = OmahaRange::parse("AKQJds")->total_weight();
  const double ss = OmahaRange::parse("AKQJss")->total_weight();
  const double r = OmahaRange::parse("AKQJr")->total_weight();
  EXPECT_EQ(r, 24.0);
  EXPECT_EQ(ds, 36.0);    // 3 ways to pair up four cards, 4·3 suit choices
  EXPECT_EQ(ss, 144.0);   // 6 pairs · 4 suits · 3 · 2
  // 4 monotone + 48 with three of one suit.
  EXPECT_EQ(ds + ss + r + 4 + 48, 256.0);

  // "AAxx": at least two aces = C(4,2)·C(48,2) + C(4,3)·48 + 1.
  EXPECT_EQ(OmahaRange::parse("AAxx")->total_weight(), 6.0 * 1128 + 4 * 48 + 1);

  // Later items override earlier ones.
  const auto mixed = OmahaRange::parse("AAxx, AAKK:0.5, AsAhKsKh:0");
  ASSERT_TRUE(mixed.has_value());
  EXPECT_EQ(mixed->weight(omaha_combo_index(hole("AsAhKsKh"))), 0.0f);
  EXPECT_EQ(mixed->weight(omaha_combo_index(hole("AsAhKsKd"))), 0.5f);
  EXPECT_EQ(mixed->weight(omaha_combo_index(hole("AsAh7s2d"))), 1.0f);

  const auto top = OmahaRange::parse("top10%");
  ASSERT_TRUE(top.has_value());
  EXPECT_NEAR(top->total_weight(), 0.1 * kOmahaCombos, 1.0);

  for (const char* bad : {"AAK", "AAKKq", "AsKsQh", "top%", "top120%", "AAKK:2", "AAKK,,QQJJ",
                          "ZZxx"}) {
    EXPECT_FALSE(OmahaRange::parse(bad, &error).has_value()) << bad;
  }
}

TEST(OmahaRange, PreflopRankIsAPermutationWithSensibleOrder) {
  const auto& rank = omaha_preflop_rank();
  ASSERT_EQ(rank.size(), static_cast<std::size_t>(kOmahaCombos));
  std::vector<int> position(kOmahaCombos, -1);
  for (std::size_t i = 0; i < rank.size(); ++i) {
    ASSERT_EQ(position[static_cast<std::size_t>(rank[i])], -1);
    position[static_cast<std::size_t>(rank[i])] = static_cast<int>(i);
  }
  const auto at = [&](std::string_view text) {
    return position[static_cast<std::size_t>(omaha_combo_index(hole(text)))];
  };
  // Double-suited aces and kings is about the best hand there is.
  EXPECT_LT(at("AsAhKsKh"), 50);
  EXPECT_LT(at("AsAhKsKh"), at("AsAhKsKd"));
  EXPECT_LT(at("AsAhKsKd"), at("AsAhKcKd"));
  EXPECT_LT(at("JsTs9h8h"), at("Js7s4h2d"));
  EXPECT_LT(at("AsAhJsTh"), at("7s6h4d2c"));
  EXPECT_LT(at("AsAhKsKh"), at("2s2h2d2c"));
  EXPECT_GT(at("2s2h2d2c"), kOmahaCombos / 2);
}

}  // namespace
}  // namespace gto
