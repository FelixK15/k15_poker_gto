#include "gto/range.h"

#include <set>

#include <gtest/gtest.h>

namespace gto {
namespace {

Range parse_ok(std::string_view text) {
  std::string error;
  const auto range = Range::parse(text, &error);
  EXPECT_TRUE(range.has_value()) << text << ": " << error;
  return range.value_or(Range{});
}

Card card(std::string_view text) { return *Card::parse(text); }

TEST(Range, ComboIndexIsABijection) {
  std::set<int> seen;
  for (int a = 0; a < 52; ++a) {
    for (int b = a + 1; b < 52; ++b) {
      const Card first(static_cast<std::uint8_t>(a));
      const Card second(static_cast<std::uint8_t>(b));
      const int index = Range::combo_index(first, second);
      ASSERT_GE(index, 0);
      ASSERT_LT(index, Range::kNumCombos);
      EXPECT_EQ(index, Range::combo_index(second, first));
      EXPECT_TRUE(seen.insert(index).second);
      const auto [low, high] = Range::combo_cards(index);
      EXPECT_EQ(low, first);
      EXPECT_EQ(high, second);
    }
  }
  EXPECT_EQ(seen.size(), 1326u);
}

TEST(Range, CountsCombosPerItem) {
  EXPECT_DOUBLE_EQ(parse_ok("").total_weight(), 0);
  EXPECT_DOUBLE_EQ(parse_ok("AA").total_weight(), 6);
  EXPECT_DOUBLE_EQ(parse_ok("AKs").total_weight(), 4);
  EXPECT_DOUBLE_EQ(parse_ok("AKo").total_weight(), 12);
  EXPECT_DOUBLE_EQ(parse_ok("AK").total_weight(), 16);
  EXPECT_DOUBLE_EQ(parse_ok("KA").total_weight(), 16);
  EXPECT_DOUBLE_EQ(parse_ok("AhKh").total_weight(), 1);
  EXPECT_DOUBLE_EQ(parse_ok("QQ+").total_weight(), 18);
  EXPECT_DOUBLE_EQ(parse_ok("22+").total_weight(), 78);
  EXPECT_DOUBLE_EQ(parse_ok("ATs+").total_weight(), 16);
  EXPECT_DOUBLE_EQ(parse_ok("KTo+").total_weight(), 36);
  EXPECT_DOUBLE_EQ(parse_ok("QQ-TT").total_weight(), 18);
  EXPECT_DOUBLE_EQ(parse_ok("TT-QQ").total_weight(), 18);
  EXPECT_DOUBLE_EQ(parse_ok("A5s-A2s").total_weight(), 16);
  EXPECT_DOUBLE_EQ(parse_ok("22+, A2s+, K2s+, A2o+").total_weight(), 78 + 48 + 44 + 144);
}

TEST(Range, SetsTheRightCombos) {
  const Range range = parse_ok("AKs, 77, T9o");
  EXPECT_EQ(range.weight(card("Ah"), card("Kh")), 1.0f);
  EXPECT_EQ(range.weight(card("Ah"), card("Kd")), 0.0f);
  EXPECT_EQ(range.weight(card("7c"), card("7s")), 1.0f);
  EXPECT_EQ(range.weight(card("Tc"), card("9d")), 1.0f);
  EXPECT_EQ(range.weight(card("Tc"), card("9c")), 0.0f);
}

TEST(Range, ReadsWeightsAndLaterItemsOverride) {
  const Range range = parse_ok(" AA:0.5 , AKs:0.25, AhKh ,KQ:0 ");
  EXPECT_FLOAT_EQ(range.weight(card("Ac"), card("Ad")), 0.5f);
  EXPECT_FLOAT_EQ(range.weight(card("As"), card("Ks")), 0.25f);
  EXPECT_FLOAT_EQ(range.weight(card("Ah"), card("Kh")), 1.0f);
  EXPECT_DOUBLE_EQ(range.total_weight(), 3.0 + 0.75 + 1.0);
}

TEST(Range, RejectsMalformedInput) {
  for (const char* text : {"AX", "AAs", "AKx", "AA:1.5", "AA:-0.1", "AA:", "AA:x", "AhAh",
                           "AKs-KQs", "AKs-AQo", "QQ-AKs", "A", "AKQ+", "hello"}) {
    std::string error;
    EXPECT_FALSE(Range::parse(text, &error).has_value()) << text;
    EXPECT_FALSE(error.empty()) << text;
  }
}

TEST(Range, ToStringRoundTrips) {
  for (const char* text :
       {"", "AA", "QQ+, AKs, AQo:0.5, A5s-A2s, 98s", "AhKh:0.25, AsKs, 72o", "22+, A2s+, KTo+"}) {
    const Range original = parse_ok(text);
    const Range again = parse_ok(original.to_string());
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      ASSERT_FLOAT_EQ(original.weight(combo), again.weight(combo))
          << text << " -> " << original.to_string();
    }
  }
  EXPECT_EQ(parse_ok("AKs, AA:0.5").to_string(), "AA:0.5,AKs");
}

TEST(Range, HandClassName) {
  EXPECT_EQ(hand_class_name(card("Ah"), card("Kh")), "AKs");
  EXPECT_EQ(hand_class_name(card("Kd"), card("Ah")), "AKo");
  EXPECT_EQ(hand_class_name(card("7c"), card("7s")), "77");
}

}  // namespace
}  // namespace gto
