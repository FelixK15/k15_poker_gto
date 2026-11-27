#pragma once

// Preflop solving on the mc engine: presets, settings, positional paths,
// heads-up scenario extraction and the plain-text range library.
//
// A preflop game is an mc::GameConfig that starts preflop with blinds posted
// and every player holding every hand. By default its last street with
// betting is the preflop itself:
//
//   MAIN APPROXIMATION. When the preflop betting ends with two or more
//   players in the hand, the board is run out and the pot goes to the best
//   hand: every player realises 100% of their equity, there is no postflop
//   play. Real play under-realises equity out of position and with weak,
//   uncoordinated holdings (offsuit trash, dominated hands) and over-realises
//   it in position. The solved ranges therefore defend somewhat wide with
//   weak offsuit hands from the blinds and cold call more often than a
//   full-game solution would. `solve_postflop` adds flop betting (strength
//   buckets) for experiments; the shipped library does not use it.
//
// Seats are the engine's: players 0..n-1 = UTG ... BTN, SB, BB (heads-up:
// 0 = BTN posting the small blind, 1 = BB). Position names follow the API
// (proto PreflopSettings): "UTG", "UTG1", "UTG2", "LJ", "HJ", "CO", "BTN",
// "SB", "BB"; heads-up "BTN" and "BB" ("SB" is accepted for the heads-up
// button when parsing, "UTG+1"/"UTG+2" for "UTG1"/"UTG2").
//
// Positional paths: '/'-separated segments. "POS:action" first folds every
// player whose turn comes before POS, then applies the action for POS; a
// plain "action" segment applies to whoever is to act ("fold", "check",
// "call", "raise2.5" = raise to 2.5 BB, "allin"). With solve_postflop the
// flop is one segment of three cards after the preflop betting. Examples:
//   "BTN:raise2.3/BB:call"   UTG, HJ, CO fold, BTN opens, SB folds, BB calls
//   "UTG:raise2.5/CO:raise7.5/UTG:call"
// Each resolved path is given back in three forms: short (positional, folds
// left out), full (every action with its position) and engine (plain labels
// as BettingTree::find takes them).

#include <array>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "gto/mc/game.h"
#include "gto/mc/solver.h"
#include "gto/range.h"

namespace gto::mc {

// --- Settings ---------------------------------------------------------------

/// Mirrors the API's gto.v1.PreflopSettings (no protobuf dependency), plus a
/// few knobs the presets use. Amounts are in big blinds.
struct PreflopSettings {
  /// 2..9 (6 = 6-max, 9 = 9-max).
  int players = 6;
  /// Every player's stack before posting blinds and antes.
  double stack_bb = 100;
  double small_blind = 0.5;
  double big_blind = 1;
  /// Ante per player, or (bb_ante) one ante of this size paid by the BB.
  /// Antes are dead money in the main pot.
  double ante = 0;
  bool bb_ante = false;
  /// Open sizes ("raise to") for every position but the SB; empty = 2.5.
  /// Several sizes give several open actions.
  std::vector<double> open_sizes_bb;
  /// Per position overrides of open_sizes_bb (keys are position names, e.g.
  /// {"BTN", {2.3}}). Presets use them; the API has no such field yet.
  std::map<std::string, std::vector<double>> open_sizes_by_position;
  /// SB first-in open sizes; empty = 3.
  std::vector<double> sb_open_sizes_bb;
  /// Limping (calling the BB first in), the SB completing included.
  bool allow_limp = false;
  /// Let the SB complete even when allow_limp is off (presets: heads-up only).
  bool allow_sb_complete = false;
  /// 3-bet sizes as multiples of the open (3-bettor in / out of position
  /// against the opener postflop); defaults 3 and 4.
  std::optional<double> three_bet_ip_multiplier;
  std::optional<double> three_bet_oop_multiplier;
  /// 4-bet size as a multiple of the 3-bet; default 2.3. The 5-bet (raise
  /// level 4) and beyond are always all-in.
  std::optional<double> four_bet_multiplier;
  /// Players who have not put money in yet may call an open (cold call).
  /// The blinds may always call an open.
  bool allow_cold_calls = false;
  /// Cold calls of 3-bets / of 4-bets and more (off keeps trees small; the
  /// opener may always call a 3-bet, the 3-bettor a 4-bet, ...).
  bool allow_cold_call_3bet = false;
  bool allow_cold_call_4bet = false;
  /// Added to a 3-bet/4-bet per caller of the raise being re-raised, in
  /// multiples of that raise (squeeze sizing); default 1.
  std::optional<double> squeeze_per_caller;
  /// Most raises preflop, the open counting as the first; default 4 (open,
  /// 3-bet, 4-bet, 5-bet all-in). 1..10.
  std::optional<int> raise_cap;
  /// Also play the flop (no turn/river betting) with `flop_sizing` and
  /// strength buckets. Off: equity at the end of preflop (see the top).
  bool solve_postflop = false;
  StreetSizing flop_sizing;
  int flop_raise_cap = 1;
  /// Rake (fraction of the pot, cap in big blinds; no flop, no drop).
  double rake_rate = 0;
  double rake_cap = 0;
};

/// The GameConfig of preflop settings (validated). Errors: player count,
/// stack too small for the blinds, sizes not positive, unknown position in
/// open_sizes_by_position, raise cap out of range.
std::optional<GameConfig> make_preflop_config(const PreflopSettings& settings,
                                              std::string* error = nullptr);

// --- Presets ----------------------------------------------------------------

/// Tuned settings for the shipped library: heads-up, 6-max and 9-max at 100,
/// 200 and 300 BB. Sizes at 100 BB (deeper stacks use bigger 3-/4-bets):
///
/// | format | opens (raise to)                     | 3-bet IP/OOP | 4-bet | cold calls |
/// |--------|--------------------------------------|--------------|-------|------------|
/// | HU     | BTN 2.5 (or limp), BB 3-bets OOP     | 3.0 / 4.0    | 2.3   | -          |
/// | 6-max  | UTG-CO 2.5, BTN 2.3, SB 3.0          | 3.0 / 4.0    | 2.3   | opens only |
/// | 9-max  | UTG-CO 2.5, BTN 2.3, SB 3.0          | 3.0 / 4.0    | 2.3   | opens only |
///
/// 200 BB: 3-bets 3.25 / 4.5, 4-bet 2.4; 300 BB: 3.5 / 5.0, 4-bet 2.5.
/// No limps (except the heads-up button), no cold calls of 3-bets or
/// 4-bets, squeezes +1x per caller, 5-bet all-in, no ante, no rake.
/// Tree sizes and solve times: `gto-cli preflop --estimate` and
/// ranges/README.txt.
struct PreflopPreset {
  std::string name;    // "6max_100bb"
  std::string format;  // "hu", "6max", "9max"
  PreflopSettings settings;
};
/// `format`: "hu"/"headsup"/"2", "6max"/"6-max"/"6", "9max"/"9-max"/"9".
/// Any stack > 2 BB is accepted (sizes follow the nearest of 100/200/300).
std::optional<PreflopPreset> preflop_preset(std::string_view format, double stack_bb,
                                            std::string* error = nullptr);
/// The six library presets (6-max and 9-max at 100, 200, 300 BB).
std::vector<PreflopPreset> library_presets();

// --- Positions and paths ---------------------------------------------------

/// Position name of an engine seat (see the top; heads-up "BTN", "BB").
std::string preflop_position_name(int num_players, int seat);
/// Engine seat of a position name (case-insensitive, aliases above), or -1.
int preflop_seat(int num_players, std::string_view position);

struct PreflopPath {
  std::uint32_t node = 0;
  /// Flop cards named after the preflop betting (solve_postflop only).
  std::vector<Card> dealt;
  std::string short_path;   // "BTN:raise2.3/BB:call"
  std::string full_path;    // "UTG:fold/HJ:fold/CO:fold/BTN:raise2.3/SB:fold/BB:call"
  std::string engine_path;  // "fold/fold/fold/raise2.3/fold/call"
};
/// Resolves a positional (or plain, or mixed) path in a preflop tree. The
/// empty path is the root.
std::optional<PreflopPath> resolve_preflop_path(const BettingTree& tree, std::string_view path,
                                                std::string* error = nullptr);
/// The three path forms of a node (no dealt cards).
PreflopPath preflop_path_of(const BettingTree& tree, std::uint32_t node);

// --- Browsing ---------------------------------------------------------------

/// The acting player's average strategy per hand class at a preflop node.
struct PreflopNodeStrategy {
  PreflopPath path;
  int seat = 0;
  std::string position;
  std::vector<std::string> actions;  // labels
  /// [class * actions + action], 169 classes in grid order (holdem_class:
  /// AA = 0, AKs = 1, AKo = 13, 22 = 168).
  std::vector<float> strategy;
  /// The actor's reach per class (0..1: share of the class's combos that
  /// get here), and the frequency of each action over the reaching range.
  std::array<float, 169> reach{};
  std::vector<double> action_frequency;
  /// Share of all starting hands of the actor that reach this node.
  double reach_fraction = 0;
  double pot = 0;
};
/// Fails on a bad path or a node that is not a preflop decision.
std::optional<PreflopNodeStrategy> preflop_node_strategy(const Solver& solver,
                                                         std::string_view path,
                                                         std::string* error = nullptr);

// --- Scenarios --------------------------------------------------------------

enum class PreflopScenarioKind : std::uint8_t {
  Limped,        // nobody raised
  SingleRaised,  // one raise
  ThreeBet,
  FourBet,
  FiveBetPlus,
};
/// "limped", "srp", "3bet", "4bet", "5bet+".
const char* preflop_scenario_kind_name(PreflopScenarioKind kind) noexcept;
/// Inverse of preflop_scenario_kind_name.
std::optional<PreflopScenarioKind> parse_preflop_scenario_kind(std::string_view name) noexcept;

/// A preflop line that ends with exactly two players seeing the flop, both
/// with chips behind.
struct PreflopScenario {
  std::string name;  // "BTN open 2.3, BB call"
  PreflopPath path;
  PreflopScenarioKind kind = PreflopScenarioKind::SingleRaised;
  int raises = 0;
  /// The two players, out of position first (postflop order).
  std::array<int, 2> seats{};
  std::array<std::string, 2> positions;
  /// Seat / position of the last raiser; -1 / "" when limped.
  int aggressor = -1;
  std::string aggressor_position;
  /// At the flop, in big blinds: pot (antes and dead blinds included) and
  /// the smaller of the two stacks behind.
  double pot = 0;
  double effective_stack = 0;
  /// Probability that a hand dealt to the table follows this line, from the
  /// players' reach fractions (card removal between players ignored).
  double frequency = 0;
};

/// Heads-up flop scenarios of a solved preflop game with at most
/// `max_raises` raises (4 = up to 5-bet pots; lines where someone is all-in
/// are left out), in tree order, those with frequency < min_frequency
/// dropped.
std::vector<PreflopScenario> list_preflop_scenarios(const Solver& solver, int max_raises = 4,
                                                    double min_frequency = 0);

struct PreflopPlayerRange {
  int seat = 0;
  std::string position;
  /// 1326 combo weights (gto::Range::combo_index): the player's reach on
  /// the line (class average strategies × the actions along the path).
  Range range;
  /// Per class weight (0..1), grid order.
  std::array<float, 169> class_weight{};
  /// Chips put in during the hand (blinds included, antes not) and behind.
  double committed = 0;
  double stack = 0;
};

/// Ranges of every player still in the hand at a node (postflop order: out
/// of position first).
struct PreflopRanges {
  PreflopPath path;
  /// Preflop betting is over (flop reached, or showdown with 2+ players).
  bool complete = false;
  std::vector<PreflopPlayerRange> players;
  double pot = 0;
  /// Smallest stack behind among the players still in.
  double effective_stack = 0;
};
/// Weights below `min_weight` are dropped and the rest rounded to 3
/// decimals, so range texts stay readable (0 keeps them exact).
std::optional<PreflopRanges> preflop_ranges(const Solver& solver, std::string_view path,
                                            std::string* error = nullptr,
                                            float min_weight = 0.005f);

// --- Library file ------------------------------------------------------------
//
// Plain text, UTF-8, one file per format × stack (ranges/6max_100bb.txt).
// Lines starting with '#' are comments; blank lines separate blocks.
// The first block is the header, then one block per scenario:
//
//   k15-preflop-library 1
//   format: 6max
//   players: 6
//   stack_bb: 100
//   blinds: 0.5 1
//   ante: 0
//   settings: opens UTG 2.5 ... (free text)
//   iterations: 1200000000
//   exploitability_pct: 0.31
//   open: UTG 0.17          (one line per position: first-in raise frequency)
//
//   scenario: BTN:raise2.3/BB:call
//   name: BTN open 2.3, BB call
//   kind: srp
//   oop: BB
//   ip: BTN
//   aggressor: BTN
//   pot: 5.1
//   effective_stack: 97.7
//   frequency: 0.0612
//   oop_range: AA:0.12,KK:0.05,...    (Range::to_string / Range::parse text)
//   ip_range: ...
//
// Unknown keys are ignored when reading (forward compatible).

struct PreflopLibraryEntry {
  std::string path;  // short positional path
  std::string name;
  PreflopScenarioKind kind = PreflopScenarioKind::SingleRaised;
  std::string oop, ip, aggressor;
  double pot = 0, effective_stack = 0, frequency = 0;
  Range oop_range, ip_range;
};

struct PreflopLibrary {
  int version = 1;
  std::string format;
  int players = 0;
  double stack_bb = 0;
  double small_blind = 0.5, big_blind = 1, ante = 0;
  std::string settings;
  std::uint64_t iterations = 0;
  double exploitability_pct = 0;
  /// First-in raise frequency per position (all positions but the BB).
  std::vector<std::pair<std::string, double>> open_frequency;
  std::vector<PreflopLibraryEntry> entries;
};

/// Builds the library of a solved game (scenarios with up to `max_raises`
/// raises and at least `min_frequency`). `format` names the file's format;
/// `exploitability_pct` is stored as given (an estimate, see Evaluation).
PreflopLibrary make_preflop_library(const Solver& solver, const PreflopSettings& settings,
                                    std::string_view format, double exploitability_pct = 0,
                                    int max_raises = 3, double min_frequency = 0);
/// Short human description of the settings ("opens UTG 2.5, ...").
std::string describe_preflop_settings(const PreflopSettings& settings);

void write_preflop_library(std::ostream& out, const PreflopLibrary& library);
std::optional<PreflopLibrary> read_preflop_library(std::istream& in, std::string* error = nullptr);

}  // namespace gto::mc
