#include "gto/hand_evaluator.h"

#include <bit>

namespace gto {

namespace {

constexpr int highest_rank(unsigned mask) noexcept { return std::bit_width(mask) - 1; }

/// Highest card of the best straight in a 13-bit rank mask, or -1.
/// The wheel (A-2-3-4-5) reports the five (rank 3) as its high card.
constexpr int straight_high(unsigned ranks) noexcept {
  // Shift up by one and put the ace below the deuce as well.
  const unsigned m = (ranks << 1) | ((ranks >> 12) & 1u);
  const unsigned runs = m & (m >> 1) & (m >> 2) & (m >> 3) & (m >> 4);
  if (runs == 0) return -1;
  // A run starting at bit i covers ranks i-1 … i+3.
  return highest_rank(runs) + 3;
}

constexpr HandStrength pack(HandCategory category, int r0 = 0, int r1 = 0, int r2 = 0, int r3 = 0,
                            int r4 = 0) noexcept {
  return (static_cast<HandStrength>(category) << 20) | (static_cast<HandStrength>(r0) << 16) |
         (static_cast<HandStrength>(r1) << 12) | (static_cast<HandStrength>(r2) << 8) |
         (static_cast<HandStrength>(r3) << 4) | static_cast<HandStrength>(r4);
}

/// Packs the `count` highest ranks of `mask` after `lead` already-chosen ranks.
HandStrength pack_kickers(HandCategory category, unsigned mask, int count, int lead0 = -1,
                          int lead1 = -1) noexcept {
  int r[5] = {0, 0, 0, 0, 0};
  int n = 0;
  if (lead0 >= 0) r[n++] = lead0;
  if (lead1 >= 0) r[n++] = lead1;
  for (int i = 0; i < count && mask != 0; ++i) {
    const int top = highest_rank(mask);
    r[n++] = top;
    mask &= ~(1u << top);
  }
  return pack(category, r[0], r[1], r[2], r[3], r[4]);
}

}  // namespace

HandStrength evaluate_hand(std::span<const Card> cards) noexcept {
  unsigned suit_ranks[4] = {0, 0, 0, 0};
  int count[13] = {};
  unsigned ranks = 0;
  for (const Card card : cards) {
    const unsigned bit = 1u << card.rank();
    suit_ranks[card.suit()] |= bit;
    ranks |= bit;
    ++count[card.rank()];
  }

  // With at most seven cards a flush rules out quads and full houses, so it
  // can be decided first.
  for (const unsigned suited : suit_ranks) {
    if (std::popcount(suited) < 5) continue;
    if (const int high = straight_high(suited); high >= 0) {
      return pack(HandCategory::StraightFlush, high);
    }
    return pack_kickers(HandCategory::Flush, suited, 5);
  }

  unsigned quads = 0, trips = 0, pairs = 0;
  for (int rank = 0; rank < 13; ++rank) {
    const unsigned bit = 1u << rank;
    if (count[rank] == 4) quads |= bit;
    else if (count[rank] == 3) trips |= bit;
    else if (count[rank] == 2) pairs |= bit;
  }

  if (quads != 0) {
    const int quad = highest_rank(quads);
    return pack_kickers(HandCategory::Quads, ranks & ~(1u << quad), 1, quad);
  }

  if (trips != 0) {
    const int trip = highest_rank(trips);
    // A second set of trips also fills the house.
    const unsigned fillers = (trips & ~(1u << trip)) | pairs;
    if (fillers != 0) return pack(HandCategory::FullHouse, trip, highest_rank(fillers));
  }

  if (const int high = straight_high(ranks); high >= 0) {
    return pack(HandCategory::Straight, high);
  }

  if (trips != 0) {
    const int trip = highest_rank(trips);
    return pack_kickers(HandCategory::Trips, ranks & ~(1u << trip), 2, trip);
  }

  if (std::popcount(pairs) >= 2) {
    const int high = highest_rank(pairs);
    const int low = highest_rank(pairs & ~(1u << high));
    return pack_kickers(HandCategory::TwoPair, ranks & ~(1u << high) & ~(1u << low), 1, high, low);
  }

  if (pairs != 0) {
    const int pair = highest_rank(pairs);
    return pack_kickers(HandCategory::Pair, ranks & ~(1u << pair), 3, pair);
  }

  return pack_kickers(HandCategory::HighCard, ranks, 5);
}

}  // namespace gto
