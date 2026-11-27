#pragma once

// External-sampling Monte Carlo CFR for N-player poker (gto::mc::Solver).
//
// Algorithm (Lanctot et al. 2009, with the Linear CFR discount of Brown &
// Sandholm 2019 as used by Pluribus):
//  * One iteration = one traversal per player. For each traversal a deal is
//    sampled: every player's hand jointly from the starting ranges with card
//    removal (rejection sampling of the whole tuple, so the joint
//    distribution is exact), then the rest of the board uniformly.
//  * The traverser explores all of its actions and updates its regrets with
//    sampled counterfactual values; every other player samples one action
//    from its current (regret-matching) strategy.
//  * Average strategy: at a non-traverser's node its current strategy is
//    added to its strategy sums, weighted by 1 / (sampling probability of
//    the actions of the players other than the traverser and the acting
//    player on the way). Heads-up this weight is always 1 (the classic
//    "opponent node" update); with 3+ players it keeps the average weighted
//    by the player's own reach only, as CFR's average is defined.
//  * Linear CFR: every `discount_interval` iterations, interval k ends with
//    all regrets and strategy sums multiplied by k / (k + 1), while
//    iterations <= discount_until.
//  * Threads share one table without locks (see storage.h). A fixed seed
//    with one thread is fully deterministic: iteration i always draws from
//    a generator seeded by (seed, i).
//
// With 3+ players CFR has no convergence guarantee to a Nash equilibrium,
// and the "exploitability" reported by evaluate() is the sum (and mean) of
// what each player could gain by deviating alone, estimated by sampling
// (see Evaluation).

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "gto/mc/abstraction.h"
#include "gto/mc/game.h"
#include "gto/mc/storage.h"
#include "gto/mc/tree.h"
#include "gto/thread_pool.h"

namespace gto::mc {

struct SolverConfig {
  /// Threads to use; 0 = one per hardware thread.
  unsigned threads = 0;
  std::uint64_t seed = 1;
  /// Linear CFR discount interval in iterations; 0 = automatic (the table
  /// size / 4, between 1,000 and 1,000,000 iterations).
  std::uint64_t discount_interval = 0;
  /// Discounting stops after this many iterations (Pluribus discounted only
  /// early on); the default discounts forever, i.e. Linear CFR throughout.
  std::uint64_t discount_until = UINT64_MAX;
  /// Strategy sums ignore the first this many iterations.
  std::uint64_t averaging_delay = 0;
};

/// What a game would take, measured without building it.
struct GameEstimate {
  TreeCounts tree;
  std::array<StreetPlan, kNumStreets> abstraction;
  std::vector<std::size_t> live_hands;
  std::uint64_t info_sets = 0;
  /// Regret/strategy entries (information sets × actions).
  std::uint64_t table_entries = 0;
  std::uint64_t storage_bytes = 0;
  std::uint64_t tree_bytes = 0;
  /// Bucket tables and caches once every board has been seen (upper bound;
  /// they fill while solving).
  std::uint64_t bucket_bytes = 0;
  /// Live hand lists.
  std::uint64_t hands_bytes = 0;
  std::uint64_t total_bytes() const noexcept {
    return storage_bytes + tree_bytes + bucket_bytes + hands_bytes;
  }
};
std::optional<GameEstimate> estimate_game(const GameConfig& config, std::string* error = nullptr);

struct Progress {
  std::uint64_t iterations = 0;
  double seconds = 0;
  /// Iterations per second since the start of this solve() call.
  double iterations_per_second = 0;
};

struct SolveOptions {
  /// Iterations to run in this call; 0 = no limit (then set max_seconds,
  /// a callback or `cancel`).
  std::uint64_t max_iterations = 0;
  /// Wall-clock budget in seconds; 0 = none.
  double max_seconds = 0;
  /// How often on_progress is called.
  double progress_interval_seconds = 1.0;
  /// Return false to stop.
  std::function<bool(const Progress&)> on_progress;
  /// Set from any thread to stop soon.
  const std::atomic<bool>* cancel = nullptr;
};

struct SolveResult {
  /// Iterations run by this call, and in total.
  std::uint64_t iterations = 0;
  std::uint64_t total_iterations = 0;
  double seconds = 0;
  bool stopped = false;  // by the callback or `cancel`
};

/// How a number was obtained.
enum class Accuracy : std::uint8_t {
  /// Computed over every deal; exact within the abstraction.
  Exact,
  /// Monte Carlo estimate (see the standard errors).
  Estimated,
};

/// Quality of the current average strategies.
///
/// Best response gain of player p: the value p would get by switching alone
/// to the best strategy within the abstraction (deciding per information
/// set = node × bucket, the others keeping their average strategies) minus
/// p's value now. Estimated version: a fixed set of sampled scenarios
/// (deal + the others' sampled actions) is solved for p's best choice per
/// information set by policy iteration. Choosing the best action on the same
/// samples it is scored on makes `br_gain` biased UPWARDS, the more so the
/// fewer samples each information set gets (lossless buckets on wide ranges,
/// big trees): a large gain at a low br_samples_per_info_set may be noise
/// rather than a real leak. To damp this, the response only deviates from
/// the average strategy where the advantage is statistically significant
/// (EvalOptions::br_significance standard errors); that in turn can miss
/// small real gains. `br_gain_holdout` scores the chosen response on fresh
/// samples and is therefore an unbiased estimate of a gain that is truly
/// available, i.e. it tends to UNDERstate the best gain (it can be negative
/// when the chosen deviations were noise). The true best-response gain
/// within the abstraction lies, up to noise, roughly between the two; both
/// approach it as the sample count grows. Treat the numbers as a trend
/// indicator (do they shrink as the solve goes on?) rather than as a bound.
///
/// Imperfect recall: when buckets merge hands on later streets, the "best
/// response" is the one computed bottom-up per information set; it is what
/// the abstraction allows, not a best response in the real game.
struct Evaluation {
  Accuracy accuracy = Accuracy::Estimated;
  /// Only heads-up without rake is the game zero-sum, so that exploitability
  /// near zero means a near-Nash equilibrium (within the abstraction). With
  /// 3+ players (or rake) there is no such guarantee: the numbers only say
  /// how much a single player could gain by deviating.
  bool nash_guarantee = false;
  /// Value of each player under the average strategies, in chips (payoff
  /// convention of game.h), and its standard error (0 when exact).
  std::vector<double> ev;
  std::vector<double> ev_std_error;
  /// Per player best-response gain (in-sample, upward biased), its standard
  /// error, and the holdout estimate (see above). All equal when exact.
  std::vector<double> br_gain;
  std::vector<double> br_gain_std_error;
  std::vector<double> br_gain_holdout;
  /// Sum of br_gain over players.
  double total_gain = 0;
  /// Mean br_gain per player, in chips and in % of the starting pot. For
  /// two players this is the usual exploitability (as gto::Solver reports).
  double exploitability = 0;
  double exploitability_pct = 0;
  /// Whether policy iteration settled for every player.
  bool br_converged = true;
  std::uint64_t ev_samples = 0;
  /// Best-response scenarios used per player (the largest).
  std::uint64_t br_samples = 0;
  /// Scenarios per information set of the player with the most information
  /// sets. The in-sample gain is only meaningful when this is well above 1
  /// (say 50+); below that it mostly measures noise. 0 when exact or skipped.
  double br_samples_per_info_set = 0;
};

struct EvalOptions {
  /// Sampled hands (whole paths under the average strategies) for the EVs.
  std::uint64_t ev_samples = 200'000;
  /// Estimate best-response gains (the slow part).
  bool best_response = true;
  /// Scenarios per player for the best response; 0 = automatic: 64 per
  /// information set of that player, at least 100,000 and at most
  /// br_samples_cap.
  std::uint64_t br_samples = 0;
  std::uint64_t br_samples_cap = 4'000'000;
  /// Deviate from the average strategy only where the advantage exceeds
  /// this many standard errors (0 = always take the sample-best action).
  double br_significance = 2.0;
  std::uint64_t seed = 7;
};

/// Everything about one node under the average strategies.
struct NodeReport {
  std::uint32_t node = 0;
  NodeKind kind = NodeKind::Action;
  Street street = Street::Preflop;
  /// Action node: player to act; FoldOut: the player who folded last.
  int player = 0;
  std::vector<Action> actions;
  std::vector<std::string> labels;
  double pot = 0;
  std::vector<double> committed;
  std::vector<bool> folded;
  std::vector<bool> allin;
  /// Starting board plus the cards named in the path.
  std::vector<Card> board;
  bool cards_named = true;
  /// Entries per range (combo_count(variant) or the toy deck size).
  int num_combos = 0;
  /// [player][combo]: starting weight × the player's own average action
  /// probabilities along the path (via its bucket at each node). Zero for
  /// combos not in the range or blocked by the board.
  std::vector<std::vector<float>> reach;
  /// Acting player's average strategy, [action * num_combos + combo];
  /// zero for combos blocked or not in the range.
  std::vector<float> strategy;
  /// [player][combo] EV estimates in chips (payoff convention of game.h)
  /// from ReportOptions::ev_samples per combo; empty for a player whose EVs
  /// were not computed. NaN for combos with zero reach.
  std::vector<std::vector<float>> ev;
  /// Reach-weighted EV of each player's range here; NaN if not computed.
  std::vector<double> range_ev;

  float frequency(std::size_t action, int combo) const {
    return strategy[action * static_cast<std::size_t>(num_combos) + static_cast<std::size_t>(combo)];
  }
};

struct ReportOptions {
  /// Sampled continuations per combo for EVs; 0 = no EVs.
  std::uint64_t ev_samples = 0;
  /// EVs are skipped for a player with more combos reaching the node than
  /// this (PLO ranges of 100k+ combos would take long).
  std::size_t max_ev_combos = 20'000;
  std::uint64_t seed = 11;
};

/// The actor's average strategy per Hold'em hand class at a preflop node.
struct ClassStrategy {
  std::vector<std::string> names;    // 169, grid order (see holdem_class)
  std::vector<std::string> actions;  // labels
  /// [class * actions + action], weighted by the actor's reach (uniform over
  /// the class's combos where nothing reaches).
  std::vector<float> strategy;
  /// Reach summed over the class's combos.
  std::vector<float> weight;
};
ClassStrategy preflop_class_strategy(const NodeReport& report);

class Solver {
 public:
  static std::unique_ptr<Solver> create(const GameConfig& config, const SolverConfig& solver = {},
                                        std::string* error = nullptr);
  ~Solver();
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;

  const GameConfig& config() const noexcept { return config_; }
  const BettingTree& tree() const noexcept { return tree_; }
  const Abstraction& abstraction() const noexcept { return abstraction_; }
  const TableLayout& layout() const noexcept { return layout_; }
  const PlayerHands& hands(int player) const { return hands_[static_cast<std::size_t>(player)]; }
  unsigned threads() const noexcept { return pool_->size(); }
  std::uint64_t iterations() const noexcept { return iterations_; }

  /// Runs exactly `count` iterations.
  void run(std::uint64_t count);
  /// Runs until a limit, the callback or `cancel` stops it.
  SolveResult solve(const SolveOptions& options);

  /// Monte Carlo evaluation of the average strategies.
  Evaluation evaluate(const EvalOptions& options = {}) const;
  /// Exact evaluation by enumerating every deal; nullopt (with a message)
  /// when deals × nodes exceed `max_work`. For toy games and small spots.
  std::optional<Evaluation> evaluate_exact(std::uint64_t max_work = 20'000'000,
                                           std::string* error = nullptr) const;

  /// Strategies of an information set, `out` gets actions(node) values.
  void average_strategy(std::uint32_t node, int bucket, float* out) const;
  void current_strategy(std::uint32_t node, int bucket, float* out) const;
  /// Bucket of a live hand at a node's street; `board` holds at least
  /// board_size(street) cards (starting board first).
  int bucket(Street street, int player, int live_index, std::span<const Card> board) const;

  /// Report for a path (tree.h) or a found node.
  std::optional<NodeReport> report(std::string_view path, const ReportOptions& options = {},
                                   std::string* error = nullptr) const;
  std::optional<NodeReport> report(const BettingTree::Found& found,
                                   const ReportOptions& options = {},
                                   std::string* error = nullptr) const;

  /// Regrets and strategy sums plus the iteration count, versioned binary.
  /// The game is not written: load() needs a solver of the same game.
  void save(std::ostream& out) const;
  /// Fails, leaving the solver untouched, on damaged data or a different shape.
  bool load(std::istream& in, std::string* error = nullptr);

  /// Memory held now: table, tree, bucket caches, hands.
  std::size_t memory_bytes() const;
  std::uint64_t discount_interval() const noexcept { return discount_interval_; }

  struct Impl;  // traversal internals (solver.cpp)

 private:
  Solver() = default;
  void run_batch(std::uint64_t first, std::uint64_t count);
  void discount_if_due();

  GameConfig config_;
  SolverConfig solver_config_;
  BettingTree tree_;
  std::vector<PlayerHands> hands_;
  Abstraction abstraction_;
  TableLayout layout_;
  std::unique_ptr<DenseFloatStorage> storage_;
  std::unique_ptr<ThreadPool> pool_;
  std::unique_ptr<Impl> impl_;
  std::uint64_t iterations_ = 0;
  std::uint64_t discount_interval_ = 1000;
};

}  // namespace gto::mc
