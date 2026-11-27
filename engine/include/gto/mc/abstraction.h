#pragma once

// Hand abstraction (bucketing) for the mc engine.
//
// An information set of the mc engine is (decision node, bucket of the
// acting player's hand on that node's street). Each street has one
// BucketFunction, chosen by AbstractionConfig:
//
//  * Preflop Hold'em: the 169 suit classes (AA, AKs, AKo, ...). Lossless when
//    the starting ranges treat suits alike (the usual case).
//  * Preflop PLO: buckets by percentile of omaha_preflop_rank()
//    (AbstractionConfig::omaha_preflop_buckets).
//  * Postflop lossless: every live hand of the player is its own bucket on
//    the start street; on a later street, hand × cards dealt since the start
//    (flop dealt from preflop as an unordered set, turn and river cards one
//    by one). Used under Auto only when it fits, see
//    AbstractionConfig::lossless_limit.
//  * Postflop strength buckets:
//      Hold'em: per board, each hand's strength HS (share of the opponent
//      holdings it beats, ties half, with card removal) and, before the
//      river, over every next card the mean EHS and mean square EHS² of the
//      next street's HS. Buckets: EHS quantile (strength_bins) × EHS²
//      quantile within it (potential_bins); on the river HS quantiles
//      (strength_bins × potential_bins). Quantiles are over all hands
//      possible on the board, equally weighted (not range weighted). Tables
//      are computed once per board on first use and cached (2 bytes per
//      combo per board, thread-safe).
//      PLO: HS against a fixed sample of opponent holdings
//      (omaha_strength_samples) using OmahaBoardEvaluator, potential from
//      omaha_potential_cards next cards; bins are fixed intervals of EHS and
//      of the standard deviation sqrt(EHS² - EHS²); per-board data is
//      cached, the bucket itself computed per query (~1-3 µs).
//  * Toy deck (tests): the card.

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "gto/card.h"
#include "gto/mc/game.h"

namespace gto::mc {

/// A player's private cards.
struct HoleHand {
  std::array<std::uint8_t, 4> cards{};
  std::uint8_t count = 0;
  std::uint64_t mask = 0;
  /// Combo index in the variant's combo numbering (toy deck: the card).
  int combo = 0;
  /// Weight in the starting range.
  float weight = 0;
};

/// The cards of a combo of `config` (Hold'em, PLO or toy deck).
HoleHand decode_combo(const GameConfig& config, int combo);

/// A player's live hands: weight > 0 and not blocked by the starting board.
struct PlayerHands {
  std::vector<HoleHand> hands;
  /// For every combo, its index in `hands`, or -1.
  std::vector<std::int32_t> index_of_combo;
};
std::vector<PlayerHands> live_hands(const GameConfig& config);
/// Live hand counts per player, cheaper than live_hands().
std::vector<std::size_t> live_counts(const GameConfig& config);

/// Hold'em 169-class index of two cards on the 13×13 grid: row-major with
/// AA at 0, suited hands above the diagonal (AKs = 1), offsuit below
/// (AKo = 13), 22 = 168.
int holdem_class(Card a, Card b) noexcept;
/// "AA", "AKs", "AKo" ... for a class index.
std::string holdem_class_name(int cls);
/// Number of combos in a class (6 pairs, 4 suited, 12 offsuit).
int holdem_class_combos(int cls) noexcept;

/// Maps a hand on one street to its bucket.
class BucketFunction {
 public:
  virtual ~BucketFunction() = default;
  virtual int num_buckets(int player) const = 0;
  /// `board` holds board_size(street) cards (everything on the table on this
  /// street; starting board first). `live_index` is the hand's index in the
  /// player's PlayerHands. Thread-safe.
  virtual int bucket(int player, const HoleHand& hand, int live_index,
                     const std::uint8_t* board) const = 0;
  virtual std::string describe() const = 0;
  /// Memory held now (caches grow while solving).
  virtual std::size_t memory_bytes() const = 0;
};

/// How a street is abstracted, resolved from the config before building.
struct StreetPlan {
  /// Lossless or Strength (preflop Hold'em: Lossless = 169 classes; preflop
  /// PLO: Strength = rank percentiles; toy deck: Lossless).
  AbstractionKind kind = AbstractionKind::Lossless;
  /// Buckets per player.
  std::array<std::uint64_t, kMaxPlayers> buckets{};
  /// Rough bytes of bucket tables and caches once a solve has seen every board.
  std::uint64_t cache_bytes = 0;
  std::string description;
};

/// Resolves the abstraction of every street with betting (others stay
/// empty) without building any tables. `live` = live hands per player.
std::array<StreetPlan, kNumStreets> plan_abstraction(const GameConfig& config,
                                                     std::span<const std::size_t> live);

/// The bucket functions of a game, one per street with betting.
class Abstraction {
 public:
  static std::optional<Abstraction> create(const GameConfig& config,
                                           const std::vector<PlayerHands>& hands,
                                           std::string* error = nullptr);

  const BucketFunction& street(Street s) const { return *streets_[static_cast<std::size_t>(s)]; }
  bool has_street(Street s) const noexcept {
    return streets_[static_cast<std::size_t>(s)] != nullptr;
  }
  int num_buckets(Street s, int player) const { return street(s).num_buckets(player); }
  const std::array<StreetPlan, kNumStreets>& plan() const noexcept { return plan_; }
  std::size_t memory_bytes() const;

 private:
  std::array<std::shared_ptr<const BucketFunction>, kNumStreets> streets_;
  std::array<StreetPlan, kNumStreets> plan_;
};

// --- Lower-level helpers (exposed for tests and tools) ---------------------

/// Hold'em hand strength on a board of 3 to 5 cards for all 1326 combos:
/// share of the other possible combos it beats (ties count half), with card
/// removal. Combos that touch the board get -1.
void holdem_hand_strength(std::span<const Card> board, float* out1326);

/// Hold'em strength bucket table of one board (one entry per combo, 0 for
/// combos touching the board), as described above.
std::vector<std::uint16_t> holdem_strength_table(std::span<const Card> board, int strength_bins,
                                                 int potential_bins);

}  // namespace gto::mc
