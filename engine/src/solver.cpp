#include "gto/solver.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace gto {

namespace {

// Discounted CFR parameters recommended by Brown & Sandholm.
constexpr double kAlpha = 1.5;  // positive regrets
constexpr double kGamma = 2.0;  // average strategy
constexpr float kNegativeDiscount = 0.5f;  // beta = 0

std::size_t block_size(const Game& game, const Node& node) {
  return node.actions.size() * game.hands.hands[node.player].size();
}

}  // namespace

/// Bump allocator for the scratch vectors of one tree traversal, so that the
/// recursion does not touch the heap.
class Solver::Arena {
 public:
  explicit Arena(std::size_t size) : buffer_(size) {}

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

std::optional<Game> make_game(const SpotConfig& config, std::string* error) {
  const auto fail = [&](std::string message) -> std::optional<Game> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };

  if (config.board.size() != 5) {
    return fail("only river spots (five board cards) are supported so far");
  }
  std::uint64_t board_mask = 0;
  for (const Card card : config.board) {
    if ((board_mask & card.mask()) != 0) return fail("the board contains a card twice");
    board_mask |= card.mask();
  }

  Game game;
  game.rake_rate = config.rake_rate;
  game.rake_cap = config.rake_cap;
  if (!(game.rake_rate >= 0 && game.rake_rate < 1)) return fail("rake must be between 0 and 1");
  if (!(game.rake_cap >= 0)) return fail("the rake cap cannot be negative");

  auto tree = build_tree(config.tree, Street::River, error);
  if (!tree) return std::nullopt;
  game.tree = std::move(*tree);

  std::array<Card, 7> cards{};
  std::copy(config.board.begin(), config.board.end(), cards.begin());
  for (std::size_t player = 0; player < 2; ++player) {
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      const float weight = config.ranges[player].weight(combo);
      if (!(weight > 0)) continue;
      const auto [a, b] = Range::combo_cards(combo);
      if (((a.mask() | b.mask()) & board_mask) != 0) continue;
      cards[5] = a;
      cards[6] = b;
      game.hands.hands[player].push_back(make_hand({a.index(), b.index()}, weight));
      game.hands.strength[player].push_back(evaluate_hand(cards));
    }
    if (game.hands.hands[player].empty()) {
      return fail(std::string(player == kOop ? "the OOP" : "the IP") +
                  " range has no hands that are possible on this board");
    }
  }

  // Both ranges may be non-empty and still never meet, e.g. AsAh against AsAd.
  bool compatible = false;
  for (const Hand& a : game.hands.hands[0]) {
    for (const Hand& b : game.hands.hands[1]) {
      if ((a.mask & b.mask) == 0) {
        compatible = true;
        break;
      }
    }
    if (compatible) break;
  }
  if (!compatible) return fail("the two ranges cannot be held at the same time");

  return game;
}

Solver::Solver(Game game) : game_(std::move(game)), terminal_(game_.hands) {
  const auto& nodes = game_.tree.nodes;

  offset_.assign(nodes.size(), 0);
  std::size_t total = 0;
  std::size_t max_actions = 1;
  std::size_t max_depth = 1;
  std::vector<std::size_t> depth(nodes.size(), 0);
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const Node& node = nodes[i];
    // Nodes are stored parent first, so the parent's depth is already known.
    const std::size_t parent_depth = i == 0 ? 0 : depth[node.parent];
    if (node.type != NodeType::Action) {
      depth[i] = parent_depth;
      continue;
    }
    depth[i] = parent_depth + 1;
    max_depth = std::max(max_depth, depth[i]);
    max_actions = std::max(max_actions, node.actions.size());
    offset_[i] = total;
    total += block_size(game_, node);
  }
  regrets_.assign(total, 0.0f);
  strategy_sum_.assign(total, 0.0f);

  const std::size_t max_hands = std::max(num_hands(0), num_hands(1));
  arena_size_ = max_depth * (2 * max_actions + 1) * max_hands;

  for (int player = 0; player < 2; ++player) {
    for (const Hand& hand : game_.hands.hands[player]) {
      initial_reach_[player].push_back(hand.weight);
    }
  }

  std::vector<float> compatible(num_hands(0));
  terminal_.compatible_reach(0, initial_reach_[1].data(), compatible.data());
  for (std::size_t h = 0; h < compatible.size(); ++h) {
    pair_weight_ += static_cast<double>(initial_reach_[0][h]) * compatible[h];
  }
  assert(pair_weight_ > 0);
}

std::size_t Solver::estimate_memory_bytes(const Game& game) noexcept {
  std::size_t total = 0;
  for (const Node& node : game.tree.nodes) {
    if (node.type == NodeType::Action) total += block_size(game, node);
  }
  return total * 2 * sizeof(float);
}

std::size_t Solver::memory_bytes() const noexcept {
  return (regrets_.size() + strategy_sum_.size()) * sizeof(float);
}

double Solver::rake(double final_pot) const noexcept {
  const double amount = game_.rake_rate * final_pot;
  return game_.rake_cap > 0 ? std::min(amount, game_.rake_cap) : amount;
}

bool Solver::terminal_values(const Node& node, int player, const float* opponent_reach,
                             float* out) const {
  const double pot = game_.tree.pot;
  switch (node.type) {
    case NodeType::Fold: {
      // The folder loses what they put in; the uncalled part of the last bet
      // goes back to the winner, so the final pot holds the folder's chips twice.
      const double folded = node.committed[node.player];
      const double payoff =
          player == node.player ? -folded : pot + folded - rake(pot + 2 * folded);
      terminal_.fold(player, payoff, opponent_reach, out);
      return true;
    }
    case NodeType::Showdown: {
      const double mine = node.committed[static_cast<std::size_t>(player)];
      const double theirs = node.committed[static_cast<std::size_t>(1 - player)];
      const double taken = rake(pot + mine + theirs);
      terminal_.showdown(player, pot + theirs - taken, (pot - taken) / 2, -mine, opponent_reach,
                         out);
      return true;
    }
    case NodeType::Chance:
      assert(false && "chance nodes are not supported yet");
      return true;
    case NodeType::Action:
      return false;
  }
  return false;
}

void Solver::current_strategy(std::uint32_t node_index, float* out) const {
  const Node& node = game_.tree.nodes[node_index];
  const std::size_t actions = node.actions.size();
  const std::size_t hands = num_hands(node.player);
  const float* regrets = regrets_.data() + offset_[node_index];
  const float uniform = 1.0f / static_cast<float>(actions);

  for (std::size_t h = 0; h < hands; ++h) {
    float sum = 0;
    for (std::size_t a = 0; a < actions; ++a) sum += std::max(regrets[a * hands + h], 0.0f);
    for (std::size_t a = 0; a < actions; ++a) {
      out[a * hands + h] = sum > 0 ? std::max(regrets[a * hands + h], 0.0f) / sum : uniform;
    }
  }
}

void Solver::average_strategy(std::uint32_t node_index, float* out) const {
  const Node& node = game_.tree.nodes[node_index];
  const std::size_t actions = node.actions.size();
  const std::size_t hands = num_hands(node.player);
  const float* sums = strategy_sum_.data() + offset_[node_index];
  const float uniform = 1.0f / static_cast<float>(actions);

  for (std::size_t h = 0; h < hands; ++h) {
    float sum = 0;
    for (std::size_t a = 0; a < actions; ++a) sum += sums[a * hands + h];
    for (std::size_t a = 0; a < actions; ++a) {
      out[a * hands + h] = sum > 0 ? sums[a * hands + h] / sum : uniform;
    }
  }
}

void Solver::cfr(std::uint32_t node_index, int player, const float* opponent_reach, float* out,
                 Arena& arena) {
  const Node& node = game_.tree.nodes[node_index];
  if (terminal_values(node, player, opponent_reach, out)) return;

  const std::size_t mark = arena.mark();
  const std::size_t actions = node.actions.size();
  const std::size_t my_hands = num_hands(player);
  const std::size_t actor_hands = num_hands(node.player);

  float* strategy = arena.alloc(actions * actor_hands);
  current_strategy(node_index, strategy);

  if (node.player == player) {
    // Our decision: value every action, then move regrets towards the better ones.
    float* action_values = arena.alloc(actions * my_hands);
    for (std::size_t a = 0; a < actions; ++a) {
      cfr(node.children[a], player, opponent_reach, action_values + a * my_hands, arena);
    }

    std::fill(out, out + my_hands, 0.0f);
    for (std::size_t a = 0; a < actions; ++a) {
      for (std::size_t h = 0; h < my_hands; ++h) {
        out[h] += strategy[a * my_hands + h] * action_values[a * my_hands + h];
      }
    }

    float* regrets = regrets_.data() + offset_[node_index];
    for (std::size_t a = 0; a < actions; ++a) {
      for (std::size_t h = 0; h < my_hands; ++h) {
        float& regret = regrets[a * my_hands + h];
        regret = regret * (regret > 0 ? positive_discount_ : negative_discount_) +
                 action_values[a * my_hands + h] - out[h];
      }
    }
  } else {
    // Opponent's decision: split their reach by their strategy and record it
    // in their average strategy, weighted by how often each hand gets here.
    float* child_reach = arena.alloc(actor_hands);
    float* child_values = arena.alloc(my_hands);
    float* sums = strategy_sum_.data() + offset_[node_index];

    std::fill(out, out + my_hands, 0.0f);
    for (std::size_t a = 0; a < actions; ++a) {
      for (std::size_t h = 0; h < actor_hands; ++h) {
        child_reach[h] = opponent_reach[h] * strategy[a * actor_hands + h];
        float& sum = sums[a * actor_hands + h];
        sum = sum * strategy_discount_ + child_reach[h];
      }
      cfr(node.children[a], player, child_reach, child_values, arena);
      for (std::size_t h = 0; h < my_hands; ++h) out[h] += child_values[h];
    }
  }

  arena.release(mark);
}

void Solver::evaluate_node(std::uint32_t node_index, int player, const float* opponent_reach,
                           float* out, bool best_response, Arena& arena) const {
  const Node& node = game_.tree.nodes[node_index];
  if (terminal_values(node, player, opponent_reach, out)) return;

  const std::size_t mark = arena.mark();
  const std::size_t actions = node.actions.size();
  const std::size_t my_hands = num_hands(player);
  const std::size_t actor_hands = num_hands(node.player);

  float* strategy = arena.alloc(actions * actor_hands);
  average_strategy(node_index, strategy);

  if (node.player == player) {
    float* action_values = arena.alloc(actions * my_hands);
    for (std::size_t a = 0; a < actions; ++a) {
      evaluate_node(node.children[a], player, opponent_reach, action_values + a * my_hands,
                    best_response, arena);
    }
    if (best_response) {
      std::copy(action_values, action_values + my_hands, out);
      for (std::size_t a = 1; a < actions; ++a) {
        for (std::size_t h = 0; h < my_hands; ++h) {
          out[h] = std::max(out[h], action_values[a * my_hands + h]);
        }
      }
    } else {
      std::fill(out, out + my_hands, 0.0f);
      for (std::size_t a = 0; a < actions; ++a) {
        for (std::size_t h = 0; h < my_hands; ++h) {
          out[h] += strategy[a * my_hands + h] * action_values[a * my_hands + h];
        }
      }
    }
  } else {
    float* child_reach = arena.alloc(actor_hands);
    float* child_values = arena.alloc(my_hands);
    std::fill(out, out + my_hands, 0.0f);
    for (std::size_t a = 0; a < actions; ++a) {
      for (std::size_t h = 0; h < actor_hands; ++h) {
        child_reach[h] = opponent_reach[h] * strategy[a * actor_hands + h];
      }
      evaluate_node(node.children[a], player, child_reach, child_values, best_response, arena);
      for (std::size_t h = 0; h < my_hands; ++h) out[h] += child_values[h];
    }
  }

  arena.release(mark);
}

double Solver::root_value(int player, const float* values) const {
  const auto& reach = initial_reach_[static_cast<std::size_t>(player)];
  double total = 0;
  for (std::size_t h = 0; h < reach.size(); ++h) {
    total += static_cast<double>(reach[h]) * values[h];
  }
  return total / pair_weight_;
}

void Solver::iterate() {
  ++iteration_;
  // Discount what earlier iterations accumulated before adding this one.
  const double previous = iteration_ - 1;
  const double weight = std::pow(previous, kAlpha);
  positive_discount_ = static_cast<float>(weight / (weight + 1));
  negative_discount_ = kNegativeDiscount;
  strategy_discount_ = static_cast<float>(std::pow(previous / iteration_, kGamma));

  Arena arena(arena_size_);
  for (int player = 0; player < 2; ++player) {
    std::vector<float> values(num_hands(player));
    cfr(GameTree::kRoot, player, initial_reach_[static_cast<std::size_t>(1 - player)].data(),
        values.data(), arena);
  }
}

Evaluation Solver::evaluate() const {
  Evaluation result;
  Arena arena(arena_size_);
  for (int player = 0; player < 2; ++player) {
    const auto p = static_cast<std::size_t>(player);
    const float* opponent_reach = initial_reach_[1 - p].data();
    std::vector<float> values(num_hands(player));

    evaluate_node(GameTree::kRoot, player, opponent_reach, values.data(), false, arena);
    result.ev[p] = root_value(player, values.data());

    evaluate_node(GameTree::kRoot, player, opponent_reach, values.data(), true, arena);
    result.best_response[p] = root_value(player, values.data());
  }
  result.exploitability =
      ((result.best_response[0] - result.ev[0]) + (result.best_response[1] - result.ev[1])) / 2;
  result.exploitability_pct = 100.0 * result.exploitability / game_.tree.pot;
  return result;
}

SolveResult Solver::solve(const SolveOptions& options) {
  const int interval = std::max(1, options.check_interval);
  const int start = iteration_;
  SolveResult result;
  result.iterations = iteration_;

  while (iteration_ - start < options.max_iterations) {
    iterate();
    const int done = iteration_ - start;
    if (done % interval != 0 && done != options.max_iterations) continue;

    result.evaluation = evaluate();
    result.iterations = iteration_;
    if (options.on_progress && !options.on_progress({iteration_, result.evaluation})) break;
    if (result.evaluation.exploitability_pct <= options.target_exploitability_pct) break;
  }

  if (result.iterations != iteration_ || iteration_ == start) {
    result.evaluation = evaluate();
    result.iterations = iteration_;
  }
  return result;
}

NodeReport Solver::report(std::uint32_t node_index) const {
  const auto& nodes = game_.tree.nodes;
  const Node& node = nodes[node_index];

  NodeReport report;
  report.type = node.type;
  report.player = node.player;
  report.actions = node.actions;
  report.reach = initial_reach_;

  // Walk down from the root, scaling each player's reach by their own choices.
  std::vector<std::uint32_t> path;
  for (std::uint32_t current = node_index; current != GameTree::kRoot;
       current = nodes[current].parent) {
    path.push_back(current);
  }
  std::vector<float> strategy;
  for (auto it = path.rbegin(); it != path.rend(); ++it) {
    const Node& child = nodes[*it];
    const Node& parent = nodes[child.parent];
    const std::size_t hands = num_hands(parent.player);
    strategy.resize(parent.actions.size() * hands);
    average_strategy(child.parent, strategy.data());
    auto& reach = report.reach[parent.player];
    for (std::size_t h = 0; h < hands; ++h) {
      reach[h] *= strategy[child.parent_action * hands + h];
    }
  }

  if (node.type == NodeType::Action) {
    report.strategy.resize(block_size(game_, node));
    average_strategy(node_index, report.strategy.data());
  }

  Arena arena(arena_size_);
  for (int player = 0; player < 2; ++player) {
    const auto p = static_cast<std::size_t>(player);
    const float* opponent_reach = report.reach[1 - p].data();
    std::vector<float> compatible(num_hands(player));
    terminal_.compatible_reach(player, opponent_reach, compatible.data());

    auto& ev = report.ev[p];
    ev.resize(num_hands(player));
    evaluate_node(node_index, player, opponent_reach, ev.data(), false, arena);
    for (std::size_t h = 0; h < ev.size(); ++h) {
      ev[h] = compatible[h] > 0 ? ev[h] / compatible[h] : 0.0f;
    }
  }
  return report;
}

}  // namespace gto
