// Side pots, ties, folded contributors and rake in the mc engine.

#include <gtest/gtest.h>

#include <numeric>

#include "gto/mc/tree.h"

namespace gto::mc {
namespace {

constexpr std::uint16_t bit(int p) { return static_cast<std::uint16_t>(1u << p); }

TEST(McPot, ThreeWayAllInSidePot) {
  // B all-in for 50, A and C in for 100. B best, A second.
  const std::vector<double> committed{100, 50, 100};
  const std::vector<std::uint32_t> strength{5, 9, 1};
  const auto layers = pot_layers(committed, 0, 0);
  ASSERT_EQ(layers.size(), 2u);
  EXPECT_DOUBLE_EQ(layers[0].amount, 150);
  EXPECT_EQ(layers[0].eligible, bit(0) | bit(1) | bit(2));
  EXPECT_DOUBLE_EQ(layers[1].amount, 100);
  EXPECT_EQ(layers[1].eligible, bit(0) | bit(2));

  const auto pay = settle_pot(committed, 0, 0, strength, {}, false);
  EXPECT_DOUBLE_EQ(pay[0], 100);
  EXPECT_DOUBLE_EQ(pay[1], 150);
  EXPECT_DOUBLE_EQ(pay[2], 0);
}

TEST(McPot, TieInTheMainPot) {
  const std::vector<double> committed{100, 50, 100};
  const std::vector<std::uint32_t> strength{9, 9, 1};
  const auto pay = settle_pot(committed, 0, 0, strength, {}, false);
  EXPECT_DOUBLE_EQ(pay[0], 75 + 100);
  EXPECT_DOUBLE_EQ(pay[1], 75);
  EXPECT_DOUBLE_EQ(pay[2], 0);
}

TEST(McPot, FourWayWithFoldedContributorAndDeadMoney) {
  // C put in 120 and folded; B is all-in for 50; 10 dead (antes).
  const std::vector<double> committed{200, 50, 120, 200};
  const std::vector<std::uint32_t> strength{1, 9, 100, 5};  // C's strength is ignored
  const auto layers = pot_layers(committed, 10, bit(2));
  ASSERT_EQ(layers.size(), 3u);
  EXPECT_DOUBLE_EQ(layers[0].amount, 210);
  EXPECT_EQ(layers[0].eligible, bit(0) | bit(1) | bit(3));
  EXPECT_DOUBLE_EQ(layers[1].amount, 210);
  EXPECT_EQ(layers[1].eligible, bit(0) | bit(3));
  EXPECT_EQ(layers[1].contributors, bit(0) | bit(2) | bit(3));
  EXPECT_DOUBLE_EQ(layers[2].amount, 160);

  const auto pay = settle_pot(committed, 10, bit(2), strength, {}, false);
  EXPECT_DOUBLE_EQ(pay[0], 0);
  EXPECT_DOUBLE_EQ(pay[1], 210);
  EXPECT_DOUBLE_EQ(pay[2], 0);
  EXPECT_DOUBLE_EQ(pay[3], 370);
  EXPECT_DOUBLE_EQ(std::accumulate(pay.begin(), pay.end(), 0.0), 580);
}

TEST(McPot, FourWayTieInASidePot) {
  const std::vector<double> committed{30, 60, 100, 100};
  const std::vector<std::uint32_t> strength{9, 5, 5, 1};
  const auto pay = settle_pot(committed, 0, 0, strength, {}, false);
  EXPECT_DOUBLE_EQ(pay[0], 120);
  EXPECT_DOUBLE_EQ(pay[1], 45);
  EXPECT_DOUBLE_EQ(pay[2], 45 + 80);
  EXPECT_DOUBLE_EQ(pay[3], 0);
}

TEST(McPot, RakeIsCappedAndTakenFromContestedLayers) {
  const std::vector<double> committed{100, 50, 100};
  const std::vector<std::uint32_t> strength{5, 9, 1};
  RakeConfig rake;
  rake.rate = 0.05;
  rake.cap = 3;
  const auto pay = settle_pot(committed, 0, 0, strength, rake, false);
  EXPECT_NEAR(pay[1], 150 * 247.0 / 250.0, 1e-9);
  EXPECT_NEAR(pay[0], 100 * 247.0 / 250.0, 1e-9);
  EXPECT_NEAR(pay[0] + pay[1] + pay[2], 247, 1e-9);
}

TEST(McPot, UncalledBetIsNotRakedAndNoFlopNoDrop) {
  // A bet 100, B had 40 in and folded.
  const std::vector<double> committed{100, 40};
  RakeConfig rake;
  rake.rate = 0.1;
  auto pay = settle_pot(committed, 0, bit(1), {}, rake, false);
  EXPECT_NEAR(pay[0], 72 + 60, 1e-9);
  EXPECT_DOUBLE_EQ(pay[1], 0);
  pay = settle_pot(committed, 0, bit(1), {}, rake, /*ended_preflop=*/true);
  EXPECT_DOUBLE_EQ(pay[0], 140);
}

}  // namespace
}  // namespace gto::mc
