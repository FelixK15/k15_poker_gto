#pragma once

// N-player game description for the Monte Carlo CFR engine (gto::mc).
//
// The mc engine solves poker games with 2 to 9 players, No-Limit Hold'em or
// Pot-Limit Omaha (PLO4), starting on any street, by external-sampling
// Monte Carlo CFR over a hand abstraction (buckets). It lives next to the
// exact heads-up vector solver (gto::Solver) and shares only cards, the hand
// evaluator, ranges and the thread pool with it.
//
// Units: chips are doubles in any unit (usually big blinds). Nothing is
// rounded: odd chips do not exist.
//
// Seats:
//  * Preflop start (start_street == Preflop): players 0..n-1 are, in order,
//    UTG ... BTN, SB, BB for n >= 3, i.e. player n-2 is the small blind and
//    player n-1 the big blind. Heads-up, player 0 is SB/BTN and player 1 the
//    BB. Preflop the action starts with player 0 (heads-up: the SB) and goes
//    up; postflop it starts with the SB (heads-up: the BB) and goes around.
//  * Postflop start: player 0 acts first on every street, then 1, 2, ...
//
// Payoffs: a player's value is what they take out of the pot minus what they
// put in *during the spot*, i.e. minus (committed at the end - `committed`
// at the start). Chips posted before the spot (blinds in `committed`, the
// `dead_pot`) are part of the starting pot, so without rake the values of
// all players add up to starting_pot().

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "gto/card.h"
#include "gto/game_tree.h"  // gto::BetSizing, gto::Action, gto::ActionType, action_label
#include "gto/variant.h"

namespace gto::mc {

inline constexpr int kMaxPlayers = 9;
/// Most actions any decision node may have.
inline constexpr int kMaxActions = 16;

enum class Street : std::uint8_t { Preflop = 0, Flop = 1, Turn = 2, River = 3 };
inline constexpr int kNumStreets = 4;

/// Board cards on the table during a street: 0, 3, 4, 5.
constexpr int board_size(Street street) noexcept {
  constexpr int sizes[] = {0, 3, 4, 5};
  return sizes[static_cast<int>(street)];
}
/// "preflop", "flop", "turn", "river".
const char* street_name(Street street) noexcept;

/// Postflop bet sizes for one street: one gto::BetSizing (pot fractions for
/// bets, fractions of the pot after calling for raises, optional all-in) for
/// everybody, optionally overridden per player.
struct StreetSizing {
  BetSizing sizing;
  /// Empty, or one entry per player; an empty optional uses `sizing`.
  std::vector<std::optional<BetSizing>> per_player;

  const BetSizing& for_player(int player) const noexcept {
    const auto p = static_cast<std::size_t>(player);
    if (p < per_player.size() && per_player[p]) return *per_player[p];
    return sizing;
  }
};

/// Preflop bet sizes. Raise levels count raises in the preflop round: the
/// first raise (open) is level 1, a 3-bet level 2, a 4-bet level 3, a 5-bet
/// level 4. Amounts are "raise to" totals for the round.
struct PreflopSizing {
  /// Open-raise sizes in big blinds (raise to), first in from any seat but the SB.
  std::vector<double> open_bb{2.5};
  /// Per player overrides of open_bb: empty, or one entry per player where an
  /// empty entry uses open_bb (preflop presets open smaller from the BTN).
  std::vector<std::vector<double>> open_bb_by_seat;
  /// Open-raise sizes of the SB when everybody folded to it.
  std::vector<double> sb_open_bb{3.0};
  /// Added to an open (iso-raise) per limper, in big blinds.
  double iso_per_limper_bb = 1.0;
  /// Players other than the SB may call the big blind when nobody raised.
  bool allow_limp = false;
  /// The SB may complete when everybody folded to it.
  bool allow_sb_complete = true;
  /// 3-bet sizes as multiples of the open's raise-to amount, when the 3-bettor
  /// will be in position against the opener postflop, and out of position.
  std::vector<double> threebet_ip{3.0};
  std::vector<double> threebet_oop{4.0};
  /// 4-bet sizes as multiples of the 3-bet's raise-to amount.
  std::vector<double> fourbet{2.3};
  /// Added to a 3-bet or 4-bet per caller of the raise being re-raised, in
  /// multiples of that raise (a squeeze over an open and one call at 3x with
  /// 1.0 per caller goes to 4x).
  double per_caller = 1.0;
  /// 5-bets and beyond are all-in only. Also offer all-in at every level
  /// (open, 3-bet, 4-bet) in addition to the sizes above.
  bool allin_every_level = false;
  /// Players who have not yet put chips in voluntarily may call an open
  /// (the blinds always may).
  bool cold_call_open = true;
  /// Most players who may cold call an open (call it without having put
  /// chips in voluntarily), counted over everyone who called it so far;
  /// the big blind may always call. -1 = no limit. Keeps 9-max trees
  /// small: with 1, a second player behind a cold caller folds or 3-bets.
  int max_cold_callers = -1;
  /// Players who have not yet raised or called a raise may call a 3-bet
  /// (cold call) / a 4-bet or more. The open-raiser may always call a 3-bet,
  /// a 3-bettor always a 4-bet, and so on: whoever has put chips in
  /// voluntarily may call.
  bool cold_call_3bet = false;
  bool cold_call_4bet = false;
  /// Most raises preflop (levels as above). 4 allows up to the 5-bet.
  int raise_cap = 4;
};

/// How private hands are grouped into information sets on one street.
enum class AbstractionKind : std::uint8_t {
  /// Lossless when it fits (see AbstractionConfig::lossless_limit), else
  /// strength buckets. Preflop: 169 classes (Hold'em) or rank percentiles
  /// (PLO).
  Auto,
  /// Every hand is its own information set. Preflop Hold'em: the 169 suit
  /// classes (lossless for suit-symmetric starting ranges). On a later
  /// street of a postflop spot: hand × cards dealt since the start.
  /// Preflop PLO: not available (percentile buckets are used).
  Lossless,
  /// Strength buckets: postflop hand strength (and potential), preflop PLO
  /// rank percentiles.
  Strength,
};

struct StreetAbstraction {
  AbstractionKind kind = AbstractionKind::Auto;
  /// Strength buckets: bins of expected hand strength.
  int strength_bins = 32;
  /// Strength buckets before the river: bins of hand potential (EHS²) within
  /// each strength bin. On the river the hand has no potential and the
  /// street gets strength_bins × potential_bins strength bins instead.
  int potential_bins = 4;
};

struct AbstractionConfig {
  /// Indexed by Street.
  std::array<StreetAbstraction, kNumStreets> streets;
  /// PLO preflop: buckets by percentile of omaha_preflop_rank().
  int omaha_preflop_buckets = 1000;
  /// The Auto rule on a postflop street: lossless if and only if no player
  /// then has more than this many buckets, i.e. live hands (weight > 0, not
  /// on the starting board) × the number of ways the cards dealt since the
  /// start of the spot can fall (1 on the start street, 49 for a turn after
  /// a flop start, 49 × 48 for the river after a flop start, C(52,3) for a
  /// flop after a preflop start, ...). Otherwise strength buckets. Memory
  /// grows with buckets × decision nodes of that street.
  std::uint64_t lossless_limit = 16384;
  /// PLO strength buckets: opponent hands sampled per board to estimate
  /// strength percentiles, and next cards looked at for the potential.
  int omaha_strength_samples = 4096;
  int omaha_potential_cards = 8;
};

struct RakeConfig {
  /// Fraction of the pot (0.05 = 5%).
  double rate = 0;
  /// Largest rake in chips; 0 = no cap.
  double cap = 0;
  /// No rake when the hand ends before a flop is dealt.
  bool no_flop_no_drop = true;
};

/// A complete game description. The solver and the tree builder validate it.
struct GameConfig {
  Variant variant = Variant::Holdem;
  int num_players = 2;
  /// Street the spot starts on, with board_size(start_street) cards in `board`.
  Street start_street = Street::River;
  std::vector<Card> board;
  /// Last street with betting. After its betting the rest of the board is
  /// dealt and the hand goes to showdown. Last street = Preflop gives a
  /// preflop game whose showdowns are all-in equity at the end of preflop.
  Street last_street = Street::River;

  /// Per player: total chips at the start of the spot, including the chips
  /// already in `committed`. A player is all-in once committed == stack.
  std::vector<double> stacks;
  /// Per player: chips already put in during the current betting round
  /// (blinds, straddles). They are the player's own chips in the pot (for
  /// side pots) and set the bet to call. Empty = all zero.
  std::vector<double> committed;
  /// Chips in the pot that belong to nobody in particular: antes, dead
  /// money, the pot of earlier streets of a postflop spot. Main pot.
  double dead_pot = 0;
  /// Size of the big blind in chips (preflop sizes are in big blinds; it is
  /// also the smallest raise increment preflop).
  double big_blind = 1;

  /// Per player: one weight per combo, num_combos() entries (Hold'em combo
  /// index as in gto::Range, PLO as omaha_combo_index). An empty vector, or
  /// no entry for a player, means every combo with weight 1.
  std::vector<std::vector<float>> ranges;

  /// Postflop bet sizes, indexed by Street (the Preflop entry is unused).
  std::array<StreetSizing, kNumStreets> sizing;
  /// Preflop bet sizes (used when start_street == Preflop).
  PreflopSizing preflop;
  /// Most raises allowed on a postflop street after the opening bet.
  int raise_cap = 3;
  /// A bet or raise that puts in at least this fraction of the player's
  /// remaining stack becomes all-in. 0 disables the rule.
  double allin_threshold = 0.67;
  /// Pot limit (PLO): no bet or raise above pot size; larger sizes are cut
  /// to the pot-size raise and all-in is only offered if it is within it.
  bool pot_limit = false;
  RakeConfig rake;
  AbstractionConfig abstraction;
  /// Building stops with an error beyond this many nodes.
  std::size_t max_nodes = 20'000'000;

  /// Test hook: a toy deck of this many cards (card i has strength i, the
  /// highest card wins), one private card per player, no board. Used for
  /// Kuhn poker. 0 = real cards. Ranges then have toy_deck_size entries,
  /// start and last street must be the river and `board` empty.
  int toy_deck_size = 0;

  /// dead_pot plus all chips in `committed`.
  double starting_pot() const noexcept;
  /// Entries of a range: combo_count(variant), or toy_deck_size.
  int num_combos() const noexcept;
  /// Private cards per player: 2, 4, or 1 for the toy deck.
  int hole_cards() const noexcept;
  double committed_of(int player) const noexcept {
    const auto p = static_cast<std::size_t>(player);
    return p < committed.size() ? committed[p] : 0.0;
  }
  /// Weight of a combo in a player's range (1 for an empty range).
  float weight(int player, int combo) const noexcept {
    const auto p = static_cast<std::size_t>(player);
    if (p >= ranges.size() || ranges[p].empty()) return 1.0f;
    return ranges[p][static_cast<std::size_t>(combo)];
  }

  /// Checks everything; on failure returns false with a message.
  bool validate(std::string* error = nullptr) const;

  /// A preflop game with standard blinds: `num_players` players with
  /// `stack_bb` big blinds each (big_blind = 1, stacks include the blinds),
  /// SB and BB posted, `ante_bb` per player as dead money, last street
  /// preflop (all-in equity at the end of preflop), all ranges full.
  static GameConfig preflop_game(int num_players, double stack_bb, double sb_bb = 0.5,
                                 double ante_bb = 0);
};

/// Seats in the order they act on `street` of a game that starts on `start`
/// (see the seat conventions above).
std::vector<int> action_order(int num_players, Street start, Street street);

/// Seat name. Preflop games: "UTG", "UTG1", ..., "LJ", "HJ", "CO", "BTN",
/// "SB", "BB" (heads-up "SB", "BB"). Postflop starts: "P0", "P1", ...
std::string seat_name(int num_players, Street start, int player);

}  // namespace gto::mc
