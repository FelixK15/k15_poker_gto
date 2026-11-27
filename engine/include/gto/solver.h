#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "gto/card.h"
#include "gto/game_tree.h"
#include "gto/hands.h"
#include "gto/omaha.h"
#include "gto/range.h"
#include "gto/terminal.h"
#include "gto/thread_pool.h"
#include "gto/variant.h"

namespace gto {

/// A fully specified two-player spot: betting tree, private hands, payoffs.
///
/// Payoff convention: a player's value is what they take out of the pot minus
/// what they put in since the start of the spot. Without rake the two players'
/// values add up to the starting pot.
struct Game {
  GameTree tree;
  Hands hands;
  /// One table per entry of tree.runouts.
  std::vector<ShowdownTable> showdowns;
  /// Fraction of the final pot taken as rake (0.05 = 5%).
  double rake_rate = 0;
  /// Largest rake in chips; 0 means no cap.
  double rake_cap = 0;
};

/// A heads-up Hold'em or PLO4 spot as a user describes it.
struct SpotConfig {
  Variant variant = Variant::Holdem;
  /// Three cards for a flop spot, four for a turn spot, five for a river spot.
  std::vector<Card> board;
  /// Hold'em ranges, indexed by player: [kOop], [kIp].
  std::array<Range, 2> ranges;
  /// PLO4 ranges (Variant::Omaha4), indexed by player. Optional so that a
  /// Hold'em spot does not carry two 270,725-weight vectors; a missing range
  /// counts as empty. PLO spots normally also set tree.pot_limit.
  std::array<std::optional<OmahaRange>, 2> omaha_ranges;
  TreeConfig tree;
  double rake_rate = 0;
  double rake_cap = 0;
  /// Solve interchangeable turn and river cards only once. Exact, and only
  /// ever worth turning off to test it.
  bool use_isomorphism = true;
};

/// Turns a spot into a solvable game. PLO hands have four cards.
std::optional<Game> make_game(const SpotConfig& config, std::string* error = nullptr);

/// What solving a spot would take, measured without building it.
struct SpotEstimate {
  TreeStats tree;
  /// Hands in each player's range that are possible on the board.
  std::array<std::size_t, 2> hands{0, 0};
  /// Memory for regrets and strategies, as floats and in 16 bits
  /// (SolverConfig::compress).
  std::uint64_t strategy_bytes = 0;
  std::uint64_t compressed_strategy_bytes = 0;

  std::uint64_t strategy_bytes_for(bool compressed) const noexcept {
    return compressed ? compressed_strategy_bytes : strategy_bytes;
  }
  std::uint64_t total_bytes(bool compressed = false) const noexcept {
    return strategy_bytes_for(compressed) + tree.tree_bytes();
  }
};

/// Sizes up a spot. Counting stops at config.tree.max_nodes, in which case
/// `tree.too_large` is set and the numbers are lower bounds.
std::optional<SpotEstimate> estimate_spot(const SpotConfig& config, std::string* error = nullptr);

/// Float strategies from this size on are kept in 16 bits instead: there,
/// moving less memory more than pays for packing and unpacking it. Below it
/// the trees are small enough that the packing costs a few percent.
constexpr std::uint64_t kCompressFromBytes = std::uint64_t{256} << 20;

/// Whether to solve a spot with SolverConfig::compress: when it is large
/// (see kCompressFromBytes), or when only the 16-bit version fits within
/// `memory_budget` bytes (0 for no limit).
bool should_compress(const SpotEstimate& estimate, std::uint64_t memory_budget) noexcept;

/// How good the current average strategies are.
struct Evaluation {
  /// Expected value of each player, in chips.
  std::array<double, 2> ev{0, 0};
  /// Value each player would get by best-responding to the other.
  std::array<double, 2> best_response{0, 0};
  /// Average gain available to a best-responding player, in chips.
  double exploitability = 0;
  /// The same as a percentage of the starting pot.
  double exploitability_pct = 0;
};

struct Progress {
  int iteration = 0;
  Evaluation evaluation;
};

struct SolverConfig {
  /// Threads to use; 0 means one per hardware thread.
  unsigned threads = 0;
  /// Keep regrets and average strategies in 16 bits instead of 32, each
  /// node's values scaled to its largest. Halves the solver's main memory
  /// at a small cost in precision.
  bool compress = false;
};

struct SolveOptions {
  int max_iterations = 1000;
  /// Stop once exploitability is at or below this percentage of the pot.
  double target_exploitability_pct = 0.3;
  /// Measure exploitability every this many iterations.
  int check_interval = 10;
  /// Called after each measurement. Return false to stop the solve.
  std::function<bool(const Progress&)> on_progress;
};

struct SolveResult {
  int iterations = 0;
  Evaluation evaluation;
};

/// Everything about one node of a solved tree.
struct NodeReport {
  NodeType type = NodeType::Action;
  /// Action node: player to act. Fold node: player who folded.
  int player = 0;
  std::vector<Action> actions;
  /// How much of each hand reaches this node: starting weight times the
  /// probability that the player takes the actions leading here. Zero for
  /// hands that the cards dealt so far make impossible.
  std::array<std::vector<float>, 2> reach;
  /// Acting player's strategy, [action * num_hands + hand]. Empty unless this
  /// is an action node.
  std::vector<float> strategy;
  /// Expected value of each hand of each player, in chips.
  std::array<std::vector<float>, 2> ev;
  /// Expected value of each player's whole range at this node, in chips.
  std::array<double, 2> range_ev{0, 0};
  /// Equity of each hand against the other player's range here: its share of
  /// the pot if no more money went in, from 0 to 1. Zero where the hand ended
  /// with a fold.
  std::array<std::vector<float>, 2> equity;
  /// Equity of each player's whole range.
  std::array<double, 2> range_equity{0, 0};
  /// For each hand of the acting player, whether its strategy here is locked.
  /// Empty unless this is an action node.
  std::vector<std::uint8_t> locked;

  float frequency(std::size_t action, std::size_t hand) const {
    return strategy[action * reach[static_cast<std::size_t>(player)].size() + hand];
  }
};

/// Discounted CFR (Brown & Sandholm 2019) with alternating updates, computed
/// over whole ranges at once. The subtrees below the first dealt card run in
/// parallel.
///
/// A Solver may be used from one thread at a time.
class Solver {
 public:
  explicit Solver(Game game, const SolverConfig& config = {});
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;

  const Game& game() const noexcept { return game_; }
  int iterations() const noexcept { return iteration_; }
  unsigned threads() const noexcept { return pool_.size(); }

  /// Runs one iteration: one update pass for each player.
  void iterate();

  /// Iterates until the target exploitability, the iteration limit, or until
  /// the progress callback asks to stop.
  SolveResult solve(const SolveOptions& options);

  /// Measures the current average strategies.
  Evaluation evaluate() const;

  /// Strategy, reach and values at a node under the average strategies.
  /// Hands are reported for the cards the user named, also where the tree
  /// stores an isomorphic line.
  NodeReport report(const NodeRef& ref) const;
  NodeReport report(std::uint32_t node) const { return report(NodeRef{node}); }

  /// Fixes the acting player's strategy at an action node for some hands.
  /// `locked[h]` selects the hands and `strategy[action * hands + h]` gives
  /// their frequencies, which are scaled to add up to one. Everything else
  /// keeps adapting: the opponent to exploit the lock, and the locked player
  /// elsewhere in the tree.
  ///
  /// Hands are in the suits `ref` was found with. Where the tree stores
  /// interchangeable cards only once below this node, a lock must not tell
  /// those suits apart; it is made symmetric by giving every hand of such a
  /// group the group's average.
  ///
  /// Replaces an earlier lock on the same node.
  bool set_lock(const NodeRef& ref, std::span<const std::uint8_t> locked,
                std::span<const float> strategy, std::string* error = nullptr);
  void clear_locks();
  bool has_locks() const noexcept { return !locks_.empty(); }

  /// Forgets the average strategies but keeps the regrets, so that solving
  /// continues from the current strategies instead of from scratch. For
  /// re-solving after locks changed: the old average belongs to another game.
  void warm_restart();

  /// Memory held for regrets and strategies.
  std::size_t memory_bytes() const noexcept;
  bool compressed() const noexcept { return compressed_; }

  /// Writes everything learned so far. The game itself is not written: load()
  /// must be called on a solver built from the same spot.
  void save(std::ostream& out) const;

  /// Restores a state written by save(). Fails, leaving the solver untouched,
  /// if the data is damaged or belongs to a differently shaped game.
  bool load(std::istream& in, std::string* error = nullptr);

  /// The same number before constructing a solver.
  static std::size_t estimate_memory_bytes(const Game& game, bool compressed = false) noexcept;

 private:
  /// Bump allocator for the scratch vectors of one tree traversal, so that the
  /// recursion does not touch the heap.
  class Arena {
   public:
    void resize(std::size_t size) { buffer_.assign(size, 0.0f); }
    float* alloc(std::size_t count) {
      assert(top_ + count <= buffer_.size());
      float* result = buffer_.data() + top_;
      top_ += count;
      return result;
    }
    std::size_t mark() const noexcept { return top_; }
    void release(std::size_t mark) noexcept { top_ = mark; }

   private:
    std::vector<float> buffer_;
    std::size_t top_ = 0;
  };

  /// Equity plays no strategy at all: both players check or call to the
  /// end, and a win counts 1, a tie one half.
  enum class Mode { Train, Average, BestResponse, Equity };

  /// Values of `node` for every hand of `player` given the opponent's reach.
  /// Train updates regrets and average strategies on the way; the other two
  /// modes only read.
  void traverse(Mode mode, std::uint32_t node, int player, const float* opponent_reach, float* out,
                Arena& arena) const;
  void traverse_chance(Mode mode, std::uint32_t node, int player, const float* opponent_reach,
                       float* out, Arena& arena) const;
  /// A whole pass from the root. With several threads it first finds every
  /// first-dealt card of the tree, values all of them in one parallel batch,
  /// and only then walks the top of the tree, which finds its cards done.
  void traverse_root(Mode mode, int player, float* out) const;
  /// Walks the top of the tree as traverse() would, without changing
  /// anything, and notes each first chance node with the reach it gets.
  void collect_deals(Mode mode, std::uint32_t node, int player, const float* opponent_reach,
                     Arena& arena) const;
  /// Values one card of a chance node: removes the hands it blocks from
  /// both sides and traverses what follows.
  void deal(Mode mode, std::uint32_t child, int player, const float* opponent_reach, float* result,
            Arena& scratch) const;
  bool terminal_values(const Node& node, int player, const float* opponent_reach,
                       float* out) const;
  /// The strategy to play right now, from regrets. The single place where a
  /// node lock will override the solver's own choice.
  /// `scratch` is room for one value per hand of the acting player.
  void current_strategy(std::uint32_t node, float* out, float* scratch) const;
  void average_strategy(std::uint32_t node, float* out, float* scratch) const;
  /// Values of one node as floats, whichever way they are stored.
  void read_regrets(std::uint32_t node, float* out) const;
  void read_strategy_sums(std::uint32_t node, float* out) const;
  /// Stores one node's values; `values` may be overwritten.
  void write_regrets(std::uint32_t node, float* values) const;
  void write_strategy_sums(std::uint32_t node, float* values) const;

  struct NodeLock {
    std::vector<std::uint8_t> locked;  // per hand
    std::vector<float> strategy;       // [action * hands + hand]
  };
  const NodeLock* lock_at(std::uint32_t node) const noexcept {
    if (lock_slot_.empty()) return nullptr;
    const std::uint32_t slot = lock_slot_[node];
    return slot == 0 ? nullptr : &locks_[slot - 1];
  }
  /// For each hand of `player` as the caller names it, the hand the tree
  /// stores it as under `map`.
  std::vector<std::uint32_t> stored_hands(int player, const CardMap& map) const;
  double rake(double final_pot) const noexcept;
  double root_value(int player, const float* values) const;
  std::size_t scratch_need(std::uint32_t node) const;
  std::size_t num_hands(int player) const noexcept {
    return game_.hands[static_cast<std::size_t>(player)].size();
  }

  Game game_;
  TerminalEvaluator terminal_;
  mutable ThreadPool pool_;
  mutable Arena main_arena_;
  mutable std::vector<Arena> worker_arenas_;

  /// A first chance node of the current pass: the reach it was entered with
  /// and, once the batch has run, the values of each of its cards.
  struct DealBatch {
    std::uint32_t node = 0;
    std::vector<float> reach;
    std::vector<float> values;
  };
  struct DealTask {
    std::uint32_t batch = 0;
    std::uint32_t child = 0;
  };
  // Only touched by the thread that runs the pass, outside the parallel part.
  mutable std::vector<DealBatch> batches_;
  mutable std::size_t batch_count_ = 0;
  mutable std::vector<DealTask> deal_tasks_;
  /// Next batch the walk of the top of the tree will meet; SIZE_MAX while
  /// cards are valued where they are met.
  mutable std::size_t batch_cursor_ = SIZE_MAX;

  /// Start of each action node's block in regrets_ and strategy_sum_.
  std::vector<std::uint64_t> offset_;
  // Written during Train traversals, which are logically non-const but share
  // the traversal code with the read-only modes. Either the float vectors are
  // in use or, when compressed, the 16-bit ones with one scale per node: a
  // stored value times its node's scale is the real value.
  bool compressed_ = false;
  mutable std::vector<float> regrets_;
  mutable std::vector<float> strategy_sum_;
  mutable std::vector<std::int16_t> regrets16_;
  mutable std::vector<std::uint16_t> strategy_sum16_;
  mutable std::vector<float> regret_scale_;
  mutable std::vector<float> strategy_sum_scale_;
  /// Chance nodes without a chance node above them: their children run in parallel.
  std::vector<bool> top_level_chance_;
  std::vector<NodeLock> locks_;
  /// Per node: 0, or one more than the node's index in locks_. Empty while
  /// there are no locks, so that solving without them costs nothing.
  std::vector<std::uint32_t> lock_slot_;
  /// For each tree swap and player: the hand each hand turns into.
  std::vector<std::array<std::vector<std::uint32_t>, 2>> swap_hands_;
  std::array<std::unordered_map<std::uint64_t, std::uint32_t>, 2> hand_index_;
  std::array<std::vector<float>, 2> initial_reach_;
  /// Total weight of all compatible hand pairs at the root.
  double pair_weight_ = 0;

  int iteration_ = 0;
  float positive_discount_ = 0;
  float negative_discount_ = 0;
  float strategy_discount_ = 0;
};

}  // namespace gto
