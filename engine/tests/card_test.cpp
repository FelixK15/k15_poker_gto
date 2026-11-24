#include "gto/card.h"

#include <gtest/gtest.h>

namespace gto {

TEST(Card, RoundTripsAllFiftyTwoCards) {
  for (int i = 0; i < Card::kDeckSize; ++i) {
    const Card card(static_cast<std::uint8_t>(i));
    const auto parsed = Card::parse(card.to_string());
    ASSERT_TRUE(parsed.has_value()) << card.to_string();
    EXPECT_EQ(*parsed, card);
  }
}

TEST(Card, RankAndSuit) {
  const auto ace_spades = Card::parse("As");
  ASSERT_TRUE(ace_spades.has_value());
  EXPECT_EQ(ace_spades->rank(), 12);
  EXPECT_EQ(ace_spades->suit(), 3);
  EXPECT_EQ(ace_spades->index(), 51);

  const auto deuce_clubs = Card::parse("2c");
  ASSERT_TRUE(deuce_clubs.has_value());
  EXPECT_EQ(deuce_clubs->index(), 0);
}

TEST(Card, ParseIsCaseInsensitive) {
  EXPECT_EQ(Card::parse("tD"), Card::parse("Td"));
  EXPECT_EQ(Card::parse("aS"), Card::parse("As"));
}

TEST(Card, ParseRejectsGarbage) {
  EXPECT_FALSE(Card::parse("").has_value());
  EXPECT_FALSE(Card::parse("A").has_value());
  EXPECT_FALSE(Card::parse("1s").has_value());
  EXPECT_FALSE(Card::parse("Ax").has_value());
  EXPECT_FALSE(Card::parse("Asd").has_value());
}

TEST(Card, ParseCardsHandlesBoards) {
  const auto flop = parse_cards("AsKd7c");
  ASSERT_TRUE(flop.has_value());
  ASSERT_EQ(flop->size(), 3u);
  EXPECT_EQ((*flop)[0].to_string(), "As");
  EXPECT_EQ((*flop)[2].to_string(), "7c");

  const auto spaced = parse_cards("As Kd, 7c  2h");
  ASSERT_TRUE(spaced.has_value());
  EXPECT_EQ(spaced->size(), 4u);

  EXPECT_TRUE(parse_cards("")->empty());
}

TEST(Card, ParseCardsRejectsDuplicatesAndFragments) {
  EXPECT_FALSE(parse_cards("AsAs").has_value());
  EXPECT_FALSE(parse_cards("AsK").has_value());
  EXPECT_FALSE(parse_cards("AsZz").has_value());
}

}  // namespace gto
