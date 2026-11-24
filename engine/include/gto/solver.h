#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "gto/card.h"
#include "gto/game_tree.h"
#include "gto/hands.h"
#include "gto/range.h"
#include "gto/terminal.h"

namespace gto {

/// A fully specified two-player spot: betting tree, private hands, payoffs.
///
/// Payoff convention: a player's value is what they take out of the pot minus
/// what they put in since the start of the spot. Without rake the two players'
/// values add up to the starting pot.
struct Game {
  GameTree tree;
  HandSets hands;
  /// Fraction of the final pot taken as rake (0.05 = 5%).
  double rake_rate = 0;
  /// Largest rake in chips; 0 means no cap.
  double rake_cap = 0;
};

/// A Hold'em spot as a user describes it.
struct SpotConfig {
  std::vector<Card> board;
  /// Indexed by player: [kOop], [kIp].
  std::array<Range, 2> ranges;
  TreeConfig tree;
  double rake_rate = 0;
  double rake_cap = 0;
};

/// Turns a Hold'em spot into a solvable game. Only river spots (five board
/// cards) are supported so far.
std::optional<Game> make_game(const SpotConfig& config, std::string* error = nullptr);

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
  /// probability that the player takes the actions leading here.
  std::array<std::vector<float>, 2> reach;
  /// Acting player's strategy, [action * num_hands + hand]. Empty at terminals.
  std::vector<float> strategy;
  /// Expected value of each hand of each player, in chips.
  std::array<std::vector<float>, 2> ev;

  float frequency(std::size_t action, std::size_t hand) const {
    return strategy[action * reach[static_cast<std::size_t>(player)].size() + hand];
  }
};

/// Discounted CFR (Brown & Sandholm 2019) with alternating updates, computed
/// over whole ranges at once.
class Solver {
 public:
  explicit Solver(Game game);
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;

  const Game& game() const noexcept { return game_; }
  int iterations() const noexcept { return iteration_; }

  /// Runs one iteration: one update pass for each player.
  void iterate();

  /// Iterates until the target exploitability, the iteration limit, or until
  /// the progress callback asks to stop.
  SolveResult solve(const SolveOptions& options);

  /// Measures the current average strategies.
  Evaluation evaluate() const;

  /// Strategy, reach and values at a node under the average strategies.
  NodeReport report(std::uint32_t node) const;

  /// Memory held for regrets and strategies.
  std::size_t memory_bytes() const noexcept;

  /// The same number before constructing a solver.
  static std::size_t estimate_memory_bytes(const Game& game) noexcept;

 private:
  class Arena;

  void cfr(std::uint32_t node, int player, const float* opponent_reach, float* out, Arena& arena);
  void evaluate_node(std::uint32_t node, int player, const float* opponent_reach, float* out,
                     bool best_response, Arena& arena) const;
  bool terminal_values(const Node& node, int player, const float* opponent_reach,
                       float* out) const;
  /// The strategy to play right now, from regrets. The single place where a
  /// node lock will override the solver's own choice.
  void current_strategy(std::uint32_t node, float* out) const;
  void average_strategy(std::uint32_t node, float* out) const;
  double rake(double final_pot) const noexcept;
  double root_value(int player, const float* values) const;
  std::size_t num_hands(int player) const noexcept {
    return game_.hands.hands[static_cast<std::size_t>(player)].size();
  }

  Game game_;
  TerminalEvaluator terminal_;
  /// Start of each action node's block in regrets_ and strategy_sum_.
  std::vector<std::size_t> offset_;
  std::vector<float> regrets_;
  std::vector<float> strategy_sum_;
  std::array<std::vector<float>, 2> initial_reach_;
  /// Total weight of all compatible hand pairs at the root.
  double pair_weight_ = 0;
  std::size_t arena_size_ = 0;

  int iteration_ = 0;
  float positive_discount_ = 0;
  float negative_discount_ = 0;
  float strategy_discount_ = 0;
};

}  // namespace gto
