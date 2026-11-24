#include "gto/hand_evaluator.h"

#include <algorithm>
#include <array>
#include <random>
#include <vector>

#include <gtest/gtest.h>

namespace gto {
namespace {

HandStrength eval(std::string_view text) {
  const auto cards = parse_cards(text);
  EXPECT_TRUE(cards.has_value()) << text;
  return evaluate_hand(*cards);
}

TEST(HandEvaluator, CountsEveryFiveCardHandByCategory) {
  std::array<int, 9> counts{};
  std::array<Card, 5> hand;
  for (int a = 0; a < 52; ++a)
    for (int b = a + 1; b < 52; ++b)
      for (int c = b + 1; c < 52; ++c)
        for (int d = c + 1; d < 52; ++d)
          for (int e = d + 1; e < 52; ++e) {
            hand = {Card(static_cast<std::uint8_t>(a)), Card(static_cast<std::uint8_t>(b)),
                    Card(static_cast<std::uint8_t>(c)), Card(static_cast<std::uint8_t>(d)),
                    Card(static_cast<std::uint8_t>(e))};
            ++counts[static_cast<std::size_t>(hand_category(evaluate_hand(hand)))];
          }

  // The well-known frequencies of five-card poker hands.
  EXPECT_EQ(counts[static_cast<int>(HandCategory::HighCard)], 1302540);
  EXPECT_EQ(counts[static_cast<int>(HandCategory::Pair)], 1098240);
  EXPECT_EQ(counts[static_cast<int>(HandCategory::TwoPair)], 123552);
  EXPECT_EQ(counts[static_cast<int>(HandCategory::Trips)], 54912);
  EXPECT_EQ(counts[static_cast<int>(HandCategory::Straight)], 10200);
  EXPECT_EQ(counts[static_cast<int>(HandCategory::Flush)], 5108);
  EXPECT_EQ(counts[static_cast<int>(HandCategory::FullHouse)], 3744);
  EXPECT_EQ(counts[static_cast<int>(HandCategory::Quads)], 624);
  EXPECT_EQ(counts[static_cast<int>(HandCategory::StraightFlush)], 40);
}

TEST(HandEvaluator, OrdersCategories) {
  const std::vector<std::string_view> ascending = {
      "7c5d4h3s2c",  // seven high
      "AcKdQhJs9c",  // ace high
      "2c2d5h4s3c",  // pair of deuces
      "AcAdKhQsJc",  // pair of aces
      "3c3d2h2sAc",  // threes and deuces
      "AcAdKhKsQc",  // aces and kings
      "2c2d2h4s3c",  // trip deuces
      "5c4d3h2sAc",  // wheel
      "6c5d4h3s2c",  // six-high straight
      "AcKdQhJsTc",  // broadway
      "7c5c4c3c2c",  // seven-high flush
      "AcKcQcJc9c",  // ace-high flush
      "2c2d2h3s3c",  // deuces full
      "AcAdAhKsKc",  // aces full
      "2c2d2h2s3c",  // quad deuces
      "5c4c3c2cAc",  // steel wheel
      "AcKcQcJcTc",  // royal flush
  };
  for (std::size_t i = 1; i < ascending.size(); ++i) {
    EXPECT_LT(eval(ascending[i - 1]), eval(ascending[i]))
        << ascending[i - 1] << " should lose to " << ascending[i];
  }
}

TEST(HandEvaluator, KickersDecide) {
  EXPECT_GT(eval("AcAdKhQsJc"), eval("AhAsKdQcTc"));
  EXPECT_GT(eval("AcAdKhKs3c"), eval("AhAsKdKc2c"));
  EXPECT_GT(eval("KcKdKh3s2c"), eval("QcQdQhAsKc"));
  EXPECT_GT(eval("AcQc9c5c3c"), eval("AdQd9d5d2d"));
}

TEST(HandEvaluator, SuitsDoNotMatter) {
  EXPECT_EQ(eval("AcKdQhJs9c"), eval("AsKhQdJc9d"));
  EXPECT_EQ(eval("AcKcQcJc9c"), eval("AsKsQsJs9s"));
}

TEST(HandEvaluator, SevenCardSpecialCases) {
  // Board plays: both hands make the same straight.
  EXPECT_EQ(eval("AcKdQhJsTc2d3h"), eval("AcKdQhJsTc4d4h"));
  // Three pairs: the third pair is only a kicker.
  EXPECT_EQ(eval("AcAdKhKsQcQd2h"), eval("AcAdKhKsQc7d2h"));
  // Two sets of trips make a full house.
  EXPECT_EQ(hand_category(eval("AcAdAhKsKcKd2h")), HandCategory::FullHouse);
  EXPECT_EQ(eval("AcAdAhKsKcKd2h"), eval("AcAdAhKsKc7d2h"));
  // A flush beats the straight that is also there.
  EXPECT_EQ(hand_category(eval("9c8c7c6d5h2cKc")), HandCategory::Flush);
  // Six-card straight uses the top five; the wheel ace does not count as high.
  EXPECT_EQ(eval("6c5d4h3s2cAd9h"), eval("6c5d4h3s2cKdQh"));
}

TEST(HandEvaluator, SevenCardsEqualBestFiveCardSubset) {
  std::mt19937 rng(20261124);
  std::array<int, 52> deck;
  for (int i = 0; i < 52; ++i) deck[static_cast<std::size_t>(i)] = i;

  for (int trial = 0; trial < 20000; ++trial) {
    std::shuffle(deck.begin(), deck.end(), rng);
    std::array<Card, 7> seven;
    for (std::size_t i = 0; i < 7; ++i) seven[i] = Card(static_cast<std::uint8_t>(deck[i]));

    HandStrength best = 0;
    for (std::size_t skip1 = 0; skip1 < 7; ++skip1) {
      for (std::size_t skip2 = skip1 + 1; skip2 < 7; ++skip2) {
        std::array<Card, 5> five;
        std::size_t n = 0;
        for (std::size_t i = 0; i < 7; ++i) {
          if (i != skip1 && i != skip2) five[n++] = seven[i];
        }
        best = std::max(best, evaluate_hand(five));
      }
    }
    ASSERT_EQ(evaluate_hand(seven), best) << "trial " << trial;
  }
}

}  // namespace
}  // namespace gto
