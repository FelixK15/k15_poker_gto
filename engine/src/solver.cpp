#include "gto/solver.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <istream>
#include <ostream>

namespace gto {

namespace {

// Discounted CFR parameters recommended by Brown & Sandholm.
constexpr double kAlpha = 1.5;  // positive regrets
constexpr double kGamma = 2.0;  // average strategy
constexpr float kNegativeDiscount = 0.5f;  // beta = 0

std::size_t block_size(const Game& game, const Node& node) {
  return static_cast<std::size_t>(node.num_children) * game.hands[node.player].size();
}

std::uint64_t mapped_mask(const Hand& hand, const CardMap& map) {
  std::uint64_t mask = 0;
  for (int c = 0; c < hand.num_cards; ++c) mask |= std::uint64_t{1} << map[hand.cards[c]];
  return mask;
}

}  // namespace

Solver::Solver(Game game, const SolverConfig& config)
    : game_(std::move(game)), terminal_(game_.hands), pool_(config.threads) {
  const GameTree& tree = game_.tree;
  const auto& nodes = tree.nodes;

  for (ShowdownTable& table : game_.showdowns) terminal_.prepare(table);

  offset_.assign(nodes.size(), 0);
  std::uint64_t total = 0;
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].type != NodeType::Action) continue;
    offset_[i] = total;
    total += block_size(game_, nodes[i]);
  }
  regrets_.assign(total, 0.0f);
  strategy_sum_.assign(total, 0.0f);

  // Find the chance nodes that have no chance node above them.
  top_level_chance_.assign(tree.chances.size(), false);
  {
    std::vector<std::uint32_t> stack = {GameTree::kRoot};
    while (!stack.empty()) {
      const std::uint32_t index = stack.back();
      stack.pop_back();
      if (nodes[index].type == NodeType::Chance) {
        top_level_chance_[nodes[index].aux] = true;
        continue;
      }
      for (const std::uint32_t child : tree.children_of(index)) stack.push_back(child);
    }
  }

  const std::size_t scratch = scratch_need(GameTree::kRoot);
  main_arena_.resize(scratch);
  worker_arenas_.resize(pool_.size());
  for (Arena& arena : worker_arenas_) arena.resize(scratch);

  for (std::size_t player = 0; player < 2; ++player) {
    const auto& hands = game_.hands[player];
    for (std::size_t h = 0; h < hands.size(); ++h) {
      initial_reach_[player].push_back(hands[h].weight);
      hand_index_[player].emplace(hands[h].mask, static_cast<std::uint32_t>(h));
    }
  }

  for (const CardMap& map : tree.swaps) {
    auto& swapped = swap_hands_.emplace_back();
    for (std::size_t player = 0; player < 2; ++player) {
      const auto& hands = game_.hands[player];
      swapped[player].resize(hands.size());
      for (std::size_t h = 0; h < hands.size(); ++h) {
        const auto it = hand_index_[player].find(mapped_mask(hands[h], map));
        // The tree only uses swaps that keep both ranges intact.
        assert(it != hand_index_[player].end());
        swapped[player][h] =
            it != hand_index_[player].end() ? it->second : static_cast<std::uint32_t>(h);
      }
    }
  }

  std::vector<float> compatible(num_hands(0));
  terminal_.compatible_reach(0, initial_reach_[1].data(), compatible.data());
  for (std::size_t h = 0; h < compatible.size(); ++h) {
    pair_weight_ += static_cast<double>(initial_reach_[0][h]) * compatible[h];
  }
  assert(pair_weight_ > 0);
}

std::size_t Solver::scratch_need(std::uint32_t node_index) const {
  const Node& node = game_.tree.nodes[node_index];
  if (node.type != NodeType::Action && node.type != NodeType::Chance) return 0;

  std::size_t below = 0;
  for (const std::uint32_t child : game_.tree.children_of(node_index)) {
    below = std::max(below, scratch_need(child));
  }
  const std::size_t max_hands = std::max(num_hands(0), num_hands(1));
  const std::size_t children = node.num_children;
  // Action: a strategy and the action values. Chance: one value vector per
  // card and a reach vector.
  const std::size_t here = node.type == NodeType::Action ? 2 * children + 2 : children + 1;
  return here * max_hands + below;
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

namespace {

constexpr char kStateMagic[8] = {'K', '1', '5', 'G', 'T', 'O', 'S', '1'};

/// Fixed-size header of a saved solver state. Little-endian, as written by
/// every platform we build for.
struct StateHeader {
  char magic[8];
  std::uint32_t version;
  std::int32_t iteration;
  std::uint64_t nodes;
  std::uint64_t hands[2];
  std::uint64_t values;
};

constexpr std::uint32_t kStateVersion = 1;

}  // namespace

std::vector<std::uint32_t> Solver::stored_hands(int player, const CardMap& map) const {
  const auto p = static_cast<std::size_t>(player);
  const auto& hands = game_.hands[p];
  std::vector<std::uint32_t> stored(hands.size());
  for (std::size_t h = 0; h < hands.size(); ++h) {
    const auto it = hand_index_[p].find(mapped_mask(hands[h], map));
    stored[h] = it != hand_index_[p].end() ? it->second : static_cast<std::uint32_t>(h);
  }
  return stored;
}

bool Solver::set_lock(const NodeRef& ref, std::span<const std::uint8_t> locked,
                      std::span<const float> strategy, std::string* error) {
  const auto fail = [&](const char* message) {
    if (error) *error = message;
    return false;
  };
  const GameTree& tree = game_.tree;
  const Node& node = tree.nodes[ref.node];
  if (node.type != NodeType::Action) return fail("only a decision can be locked");

  const std::size_t hands = num_hands(node.player);
  const std::size_t actions = node.num_children;
  if (locked.size() != hands || strategy.size() != actions * hands) {
    return fail("the lock does not match the node's hands and actions");
  }

  NodeLock lock;
  lock.locked.assign(hands, 0);
  lock.strategy.assign(actions * hands, 0.0f);
  const std::vector<std::uint32_t> stored = stored_hands(node.player, ref.card_map);
  bool any = false;
  for (std::size_t h = 0; h < hands; ++h) {
    if (!locked[h]) continue;
    float sum = 0;
    for (std::size_t a = 0; a < actions; ++a) {
      const float value = strategy[a * hands + h];
      if (!(value >= 0)) return fail("lock frequencies cannot be negative");
      sum += value;
    }
    if (!(sum > 0)) return fail("the frequencies of a locked hand add up to zero");
    const std::size_t s = stored[h];
    lock.locked[s] = 1;
    for (std::size_t a = 0; a < actions; ++a) {
      lock.strategy[a * hands + s] = strategy[a * hands + h] / sum;
    }
    any = true;
  }
  if (!any) return fail("the lock names no hands");

  // Suit swaps that merge cards somewhere below this node.
  std::vector<bool> swap_used(tree.swaps.size(), false);
  {
    std::vector<std::uint32_t> stack = {ref.node};
    while (!stack.empty()) {
      const std::uint32_t index = stack.back();
      stack.pop_back();
      const Node& current = tree.nodes[index];
      if (current.type == NodeType::Chance) {
        for (const IsoDeal& iso : tree.iso_of(index)) swap_used[iso.swap] = true;
      }
      for (const std::uint32_t child : tree.children_of(index)) stack.push_back(child);
    }
  }

  // Group the hands those swaps turn into one another, and give each group
  // with a locked hand the average of its locked hands.
  std::vector<std::uint32_t> group(hands);
  for (std::size_t h = 0; h < hands; ++h) group[h] = static_cast<std::uint32_t>(h);
  const auto find = [&](std::uint32_t h) {
    while (group[h] != h) h = group[h] = group[group[h]];
    return h;
  };
  for (std::size_t swap = 0; swap < swap_used.size(); ++swap) {
    if (!swap_used[swap]) continue;
    const auto& swapped = swap_hands_[swap][node.player];
    for (std::size_t h = 0; h < hands; ++h) {
      const std::uint32_t a = find(static_cast<std::uint32_t>(h));
      const std::uint32_t b = find(swapped[h]);
      if (a != b) group[a] = b;
    }
  }
  std::vector<float> total(actions * hands, 0.0f);
  std::vector<int> count(hands, 0);
  for (std::size_t h = 0; h < hands; ++h) {
    if (!lock.locked[h]) continue;
    const std::uint32_t g = find(static_cast<std::uint32_t>(h));
    ++count[g];
    for (std::size_t a = 0; a < actions; ++a) total[a * hands + g] += lock.strategy[a * hands + h];
  }
  for (std::size_t h = 0; h < hands; ++h) {
    const std::uint32_t g = find(static_cast<std::uint32_t>(h));
    if (count[g] == 0) continue;
    lock.locked[h] = 1;
    for (std::size_t a = 0; a < actions; ++a) {
      lock.strategy[a * hands + h] = total[a * hands + g] / static_cast<float>(count[g]);
    }
  }

  if (lock_slot_.empty()) lock_slot_.assign(tree.nodes.size(), 0);
  if (const std::uint32_t slot = lock_slot_[ref.node]; slot != 0) {
    locks_[slot - 1] = std::move(lock);
  } else {
    locks_.push_back(std::move(lock));
    lock_slot_[ref.node] = static_cast<std::uint32_t>(locks_.size());
  }
  return true;
}

void Solver::clear_locks() {
  locks_.clear();
  lock_slot_.clear();
}

void Solver::warm_restart() { std::fill(strategy_sum_.begin(), strategy_sum_.end(), 0.0f); }

void Solver::save(std::ostream& out) const {
  StateHeader header{};
  std::memcpy(header.magic, kStateMagic, sizeof(kStateMagic));
  header.version = kStateVersion;
  header.iteration = iteration_;
  header.nodes = game_.tree.nodes.size();
  header.hands[0] = num_hands(0);
  header.hands[1] = num_hands(1);
  header.values = regrets_.size();

  const auto bytes = static_cast<std::streamsize>(regrets_.size() * sizeof(float));
  out.write(reinterpret_cast<const char*>(&header), sizeof(header));
  out.write(reinterpret_cast<const char*>(regrets_.data()), bytes);
  out.write(reinterpret_cast<const char*>(strategy_sum_.data()), bytes);
}

bool Solver::load(std::istream& in, std::string* error) {
  const auto fail = [&](const char* message) {
    if (error) *error = message;
    return false;
  };

  StateHeader header{};
  in.read(reinterpret_cast<char*>(&header), sizeof(header));
  if (!in || std::memcmp(header.magic, kStateMagic, sizeof(kStateMagic)) != 0) {
    return fail("not a saved solver state");
  }
  if (header.version != kStateVersion) return fail("saved by an incompatible version");
  if (header.nodes != game_.tree.nodes.size() || header.hands[0] != num_hands(0) ||
      header.hands[1] != num_hands(1) || header.values != regrets_.size() ||
      header.iteration < 0) {
    return fail("the saved state belongs to a different spot");
  }

  // Read into copies so that a truncated file leaves this solver as it was.
  std::vector<float> regrets(regrets_.size());
  std::vector<float> strategy_sum(strategy_sum_.size());
  const auto bytes = static_cast<std::streamsize>(regrets.size() * sizeof(float));
  in.read(reinterpret_cast<char*>(regrets.data()), bytes);
  in.read(reinterpret_cast<char*>(strategy_sum.data()), bytes);
  if (!in) return fail("the saved state is incomplete");

  regrets_ = std::move(regrets);
  strategy_sum_ = std::move(strategy_sum);
  iteration_ = header.iteration;
  return true;
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
      terminal_.showdown(game_.showdowns[node.aux], player, pot + theirs - taken,
                         (pot - taken) / 2, -mine, opponent_reach, out);
      return true;
    }
    case NodeType::Chance:
    case NodeType::Action:
      return false;
  }
  return false;
}

namespace {

/// Scales each hand's non-negative values across the actions to add up to
/// one; a hand with nothing gets every action equally. `values` is laid out
/// [action * hands + hand] and `sum` is scratch for one value per hand.
///
/// The loops run over hands innermost, with no branches, so that the
/// compiler turns them into vector code.
void normalise(float* values, float* sum, std::size_t actions, std::size_t hands) {
  std::fill(sum, sum + hands, 0.0f);
  for (std::size_t a = 0; a < actions; ++a) {
    const float* row = values + a * hands;
    for (std::size_t h = 0; h < hands; ++h) sum[h] += row[h];
  }
  const float uniform = 1.0f / static_cast<float>(actions);
  for (std::size_t a = 0; a < actions; ++a) {
    float* row = values + a * hands;
    for (std::size_t h = 0; h < hands; ++h) {
      row[h] = sum[h] > 0 ? row[h] / sum[h] : uniform;
    }
  }
}

}  // namespace

void Solver::current_strategy(std::uint32_t node_index, float* out, float* scratch) const {
  const Node& node = game_.tree.nodes[node_index];
  const std::size_t actions = node.num_children;
  const std::size_t hands = num_hands(node.player);
  const float* regrets = regrets_.data() + offset_[node_index];

  // Regret matching: play each action in proportion to its positive regret.
  const std::size_t count = actions * hands;
  for (std::size_t i = 0; i < count; ++i) out[i] = std::max(regrets[i], 0.0f);
  normalise(out, scratch, actions, hands);

  if (const NodeLock* lock = lock_at(node_index)) {
    for (std::size_t h = 0; h < hands; ++h) {
      if (!lock->locked[h]) continue;
      for (std::size_t a = 0; a < actions; ++a) out[a * hands + h] = lock->strategy[a * hands + h];
    }
  }
}

void Solver::average_strategy(std::uint32_t node_index, float* out, float* scratch) const {
  const Node& node = game_.tree.nodes[node_index];
  const std::size_t actions = node.num_children;
  const std::size_t hands = num_hands(node.player);
  const float* sums = strategy_sum_.data() + offset_[node_index];

  std::copy(sums, sums + actions * hands, out);
  normalise(out, scratch, actions, hands);

  // A locked hand's average is the lock itself, exactly.
  if (const NodeLock* lock = lock_at(node_index)) {
    for (std::size_t h = 0; h < hands; ++h) {
      if (!lock->locked[h]) continue;
      for (std::size_t a = 0; a < actions; ++a) out[a * hands + h] = lock->strategy[a * hands + h];
    }
  }
}

void Solver::traverse(Mode mode, std::uint32_t node_index, int player, const float* opponent_reach,
                      float* out, Arena& arena) const {
  const Node& node = game_.tree.nodes[node_index];
  if (mode == Mode::Equity) {
    switch (node.type) {
      case NodeType::Showdown:
        terminal_.showdown(game_.showdowns[node.aux], player, 1.0, 0.5, 0.0, opponent_reach, out);
        return;
      case NodeType::Fold:
        std::fill(out, out + num_hands(player), 0.0f);
        return;
      case NodeType::Action: {
        // Every decision offers a check or a call; take it.
        const auto actions = game_.tree.actions_of(node_index);
        std::size_t passive = 0;
        for (std::size_t a = 0; a < actions.size(); ++a) {
          if (actions[a].type == ActionType::Check || actions[a].type == ActionType::Call) {
            passive = a;
            break;
          }
        }
        traverse(mode, game_.tree.children_of(node_index)[passive], player, opponent_reach, out,
                 arena);
        return;
      }
      case NodeType::Chance:
        break;
    }
  }
  if (terminal_values(node, player, opponent_reach, out)) return;
  if (node.type == NodeType::Chance) {
    traverse_chance(mode, node_index, player, opponent_reach, out, arena);
    return;
  }

  const std::size_t mark = arena.mark();
  const auto children = game_.tree.children_of(node_index);
  const std::size_t actions = children.size();
  const std::size_t my_hands = num_hands(player);
  const std::size_t actor_hands = num_hands(node.player);

  float* strategy = arena.alloc(actions * actor_hands);
  float* scratch = arena.alloc(actor_hands);
  if (mode == Mode::Train) {
    current_strategy(node_index, strategy, scratch);
  } else {
    average_strategy(node_index, strategy, scratch);
  }

  if (node.player == player) {
    // Our decision: value every action.
    float* action_values = arena.alloc(actions * my_hands);
    for (std::size_t a = 0; a < actions; ++a) {
      traverse(mode, children[a], player, opponent_reach, action_values + a * my_hands, arena);
    }

    if (mode == Mode::BestResponse) {
      std::copy(action_values, action_values + my_hands, out);
      for (std::size_t a = 1; a < actions; ++a) {
        for (std::size_t h = 0; h < my_hands; ++h) {
          out[h] = std::max(out[h], action_values[a * my_hands + h]);
        }
      }
      // A locked hand cannot deviate: its best response is the lock.
      if (const NodeLock* lock = lock_at(node_index)) {
        for (std::size_t h = 0; h < my_hands; ++h) {
          if (!lock->locked[h]) continue;
          float value = 0;
          for (std::size_t a = 0; a < actions; ++a) {
            value += strategy[a * my_hands + h] * action_values[a * my_hands + h];
          }
          out[h] = value;
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

    if (mode == Mode::Train) {
      // Move regrets towards the actions that did better than the mix.
      float* regrets = regrets_.data() + offset_[node_index];
      const NodeLock* lock = lock_at(node_index);
      const float positive = positive_discount_;
      const float negative = negative_discount_;
      for (std::size_t a = 0; a < actions; ++a) {
        float* row = regrets + a * my_hands;
        const float* values = action_values + a * my_hands;
        if (lock == nullptr) {
          // The common case, kept free of anything that would stop the
          // compiler from vectorising it.
          for (std::size_t h = 0; h < my_hands; ++h) {
            const float regret = row[h];
            row[h] = regret * (regret > 0 ? positive : negative) + values[h] - out[h];
          }
        } else {
          for (std::size_t h = 0; h < my_hands; ++h) {
            // Locked hands have nothing to learn here.
            if (lock->locked[h]) continue;
            const float regret = row[h];
            row[h] = regret * (regret > 0 ? positive : negative) + values[h] - out[h];
          }
        }
      }
    }
  } else {
    // Opponent's decision: split their reach by their strategy. When training,
    // also record it in their average strategy, weighted by how often each
    // hand gets here.
    float* child_reach = arena.alloc(actor_hands);
    float* child_values = arena.alloc(my_hands);
    float* sums = strategy_sum_.data() + offset_[node_index];

    std::fill(out, out + my_hands, 0.0f);
    for (std::size_t a = 0; a < actions; ++a) {
      for (std::size_t h = 0; h < actor_hands; ++h) {
        child_reach[h] = opponent_reach[h] * strategy[a * actor_hands + h];
      }
      if (mode == Mode::Train) {
        for (std::size_t h = 0; h < actor_hands; ++h) {
          float& sum = sums[a * actor_hands + h];
          sum = sum * strategy_discount_ + child_reach[h];
        }
      }
      traverse(mode, children[a], player, child_reach, child_values, arena);
      for (std::size_t h = 0; h < my_hands; ++h) out[h] += child_values[h];
    }
  }

  arena.release(mark);
}

void Solver::deal(Mode mode, std::uint32_t child, int player, const float* opponent_reach,
                  float* result, Arena& scratch) const {
  const auto p = static_cast<std::size_t>(player);
  const auto& my_hands = game_.hands[p];
  const auto& opponent_hands = game_.hands[1 - p];

  const std::size_t mark = scratch.mark();
  float* reach = scratch.alloc(opponent_hands.size());
  const std::uint64_t card = std::uint64_t{1} << game_.tree.nodes[child].card;
  for (std::size_t h = 0; h < opponent_hands.size(); ++h) {
    reach[h] = (opponent_hands[h].mask & card) != 0 ? 0.0f : opponent_reach[h];
  }
  traverse(mode, child, player, reach, result, scratch);
  for (std::size_t h = 0; h < my_hands.size(); ++h) {
    if ((my_hands[h].mask & card) != 0) result[h] = 0.0f;
  }
  scratch.release(mark);
}

void Solver::traverse_chance(Mode mode, std::uint32_t node_index, int player,
                             const float* opponent_reach, float* out, Arena& arena) const {
  const GameTree& tree = game_.tree;
  const Node& node = tree.nodes[node_index];
  const auto children = tree.children_of(node_index);
  const auto iso = tree.iso_of(node_index);
  const auto p = static_cast<std::size_t>(player);
  const std::size_t my_count = game_.hands[p].size();
  const bool top_level = top_level_chance_[node.aux];

  const std::size_t mark = arena.mark();
  const float* values = nullptr;
  if (top_level && batch_cursor_ != SIZE_MAX) {
    // The batch at the start of this pass valued every card already.
    const DealBatch& batch = batches_[batch_cursor_++];
    assert(batch.node == node_index);
    values = batch.values.data();
  } else {
    float* computed = arena.alloc(children.size() * my_count);
    if (top_level && pool_.size() > 1 && children.size() > 1) {
      // The subtrees below different cards share no storage.
      pool_.parallel_for(children.size(), [&](std::size_t i, unsigned worker) {
        deal(mode, children[i], player, opponent_reach, computed + i * my_count,
             worker_arenas_[worker]);
      });
    } else {
      for (std::size_t i = 0; i < children.size(); ++i) {
        deal(mode, children[i], player, opponent_reach, computed + i * my_count, arena);
      }
    }
    values = computed;
  }

  std::fill(out, out + my_count, 0.0f);
  for (std::size_t i = 0; i < children.size(); ++i) {
    const float* result = values + i * my_count;
    for (std::size_t h = 0; h < my_count; ++h) out[h] += result[h];
  }
  // An interchangeable card is worth to a hand what its representative is
  // worth to the hand with the two suits swapped.
  for (const IsoDeal& deal_iso : iso) {
    const float* result = values + deal_iso.child * my_count;
    const auto& swapped = swap_hands_[deal_iso.swap][p];
    for (std::size_t h = 0; h < my_count; ++h) out[h] += result[swapped[h]];
  }

  // Every card that neither player holds is equally likely.
  const std::size_t unseen =
      children.size() + iso.size() - 2 * static_cast<std::size_t>(terminal_.hole_cards());
  const float probability = 1.0f / static_cast<float>(unseen);
  for (std::size_t h = 0; h < my_count; ++h) out[h] *= probability;

  arena.release(mark);
}

void Solver::collect_deals(Mode mode, std::uint32_t node_index, int player,
                           const float* opponent_reach, Arena& arena) const {
  const Node& node = game_.tree.nodes[node_index];
  if (node.type == NodeType::Chance) {
    // Reached only for first chance nodes: the walk stops here.
    if (batch_count_ == batches_.size()) batches_.emplace_back();
    DealBatch& batch = batches_[batch_count_];
    const std::size_t children = node.num_children;
    batch.node = node_index;
    batch.reach.assign(opponent_reach, opponent_reach + num_hands(1 - player));
    batch.values.resize(children * num_hands(player));
    for (std::size_t i = 0; i < children; ++i) {
      deal_tasks_.push_back(
          {static_cast<std::uint32_t>(batch_count_), static_cast<std::uint32_t>(i)});
    }
    ++batch_count_;
    return;
  }
  if (node.type != NodeType::Action) return;

  const auto children = game_.tree.children_of(node_index);
  if (node.player == player) {
    // Our own choices do not change what the opponent brings.
    for (const std::uint32_t child : children) {
      collect_deals(mode, child, player, opponent_reach, arena);
    }
    return;
  }

  const std::size_t mark = arena.mark();
  const std::size_t actions = children.size();
  const std::size_t hands = num_hands(node.player);
  float* strategy = arena.alloc(actions * hands);
  float* scratch = arena.alloc(hands);
  float* child_reach = arena.alloc(hands);
  // The same strategy the walk proper will compute: nothing in the top of
  // the tree changes before it gets there.
  if (mode == Mode::Train) {
    current_strategy(node_index, strategy, scratch);
  } else {
    average_strategy(node_index, strategy, scratch);
  }
  for (std::size_t a = 0; a < actions; ++a) {
    for (std::size_t h = 0; h < hands; ++h) {
      child_reach[h] = opponent_reach[h] * strategy[a * hands + h];
    }
    collect_deals(mode, children[a], player, child_reach, arena);
  }
  arena.release(mark);
}

void Solver::traverse_root(Mode mode, int player, float* out) const {
  const float* reach = initial_reach_[static_cast<std::size_t>(1 - player)].data();
  // Waking the threads once per chance node costs more than a small subtree
  // takes to solve; one batch for the whole pass wakes them once.
  const bool batched = pool_.size() > 1 && !top_level_chance_.empty() && mode != Mode::Equity;
  if (!batched) {
    traverse(mode, GameTree::kRoot, player, reach, out, main_arena_);
    return;
  }

  batch_count_ = 0;
  deal_tasks_.clear();
  collect_deals(mode, GameTree::kRoot, player, reach, main_arena_);

  const std::size_t my_count = num_hands(player);
  pool_.parallel_for(deal_tasks_.size(), [&](std::size_t t, unsigned worker) {
    const DealTask& task = deal_tasks_[t];
    DealBatch& batch = batches_[task.batch];
    const std::uint32_t child = game_.tree.children_of(batch.node)[task.child];
    deal(mode, child, player, batch.reach.data(), batch.values.data() + task.child * my_count,
         worker_arenas_[worker]);
  });

  batch_cursor_ = 0;
  traverse(mode, GameTree::kRoot, player, reach, out, main_arena_);
  assert(batch_cursor_ == batch_count_);
  batch_cursor_ = SIZE_MAX;
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

  for (int player = 0; player < 2; ++player) {
    std::vector<float> values(num_hands(player));
    traverse_root(Mode::Train, player, values.data());
  }
}

Evaluation Solver::evaluate() const {
  Evaluation result;
  for (int player = 0; player < 2; ++player) {
    const auto p = static_cast<std::size_t>(player);
    std::vector<float> values(num_hands(player));

    traverse_root(Mode::Average, player, values.data());
    result.ev[p] = root_value(player, values.data());

    traverse_root(Mode::BestResponse, player, values.data());
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

NodeReport Solver::report(const NodeRef& ref) const {
  const GameTree& tree = game_.tree;
  const auto& nodes = tree.nodes;
  const Node& node = nodes[ref.node];

  NodeReport report;
  report.type = node.type;
  report.player = node.player;
  const auto actions = tree.actions_of(ref.node);
  report.actions.assign(actions.begin(), actions.end());
  report.reach = initial_reach_;

  // Walk down from the root: a player's own choices scale their reach, a
  // dealt card removes the hands that contain it.
  std::vector<std::uint32_t> path;
  for (std::uint32_t current = ref.node; current != GameTree::kRoot;
       current = nodes[current].parent) {
    path.push_back(current);
  }
  std::vector<float> strategy;
  std::vector<float> scratch;
  std::uint64_t dealt = 0;
  for (auto it = path.rbegin(); it != path.rend(); ++it) {
    const Node& child = nodes[*it];
    const Node& parent = nodes[child.parent];
    if (parent.type == NodeType::Chance) {
      const std::uint64_t card = std::uint64_t{1} << child.card;
      dealt |= card;
      for (std::size_t player = 0; player < 2; ++player) {
        for (std::size_t h = 0; h < game_.hands[player].size(); ++h) {
          if ((game_.hands[player][h].mask & card) != 0) report.reach[player][h] = 0.0f;
        }
      }
      continue;
    }
    const std::size_t hands = num_hands(parent.player);
    strategy.resize(static_cast<std::size_t>(parent.num_children) * hands);
    scratch.resize(hands);
    average_strategy(child.parent, strategy.data(), scratch.data());
    auto& reach = report.reach[parent.player];
    for (std::size_t h = 0; h < hands; ++h) {
      reach[h] *= strategy[child.parent_action * hands + h];
    }
  }

  if (node.type == NodeType::Action) {
    report.strategy.resize(block_size(game_, node));
    scratch.resize(num_hands(node.player));
    average_strategy(ref.node, report.strategy.data(), scratch.data());
    report.locked.assign(num_hands(node.player), 0);
    if (const NodeLock* lock = lock_at(ref.node)) report.locked = lock->locked;
  }

  for (int player = 0; player < 2; ++player) {
    const auto p = static_cast<std::size_t>(player);
    const float* opponent_reach = report.reach[1 - p].data();
    std::vector<float> compatible(num_hands(player));
    terminal_.compatible_reach(player, opponent_reach, compatible.data());

    auto& ev = report.ev[p];
    ev.resize(num_hands(player));
    traverse(Mode::Average, ref.node, player, opponent_reach, ev.data(), main_arena_);
    double value = 0, weight = 0;
    for (std::size_t h = 0; h < ev.size(); ++h) {
      const bool possible = (game_.hands[p][h].mask & dealt) == 0;
      if (possible) {
        // Weighted by how often the hand is here against a hand it can face.
        value += static_cast<double>(report.reach[p][h]) * ev[h];
        weight += static_cast<double>(report.reach[p][h]) * compatible[h];
      }
      ev[h] = possible && compatible[h] > 0 ? ev[h] / compatible[h] : 0.0f;
    }
    report.range_ev[p] = weight > 0 ? value / weight : 0.0;

    auto& equity = report.equity[p];
    equity.resize(num_hands(player));
    traverse(Mode::Equity, ref.node, player, opponent_reach, equity.data(), main_arena_);
    double share = 0;
    for (std::size_t h = 0; h < equity.size(); ++h) {
      const bool possible = (game_.hands[p][h].mask & dealt) == 0;
      if (possible) share += static_cast<double>(report.reach[p][h]) * equity[h];
      equity[h] = possible && compatible[h] > 0 ? equity[h] / compatible[h] : 0.0f;
    }
    report.range_equity[p] = weight > 0 ? share / weight : 0.0;
  }

  if (!ref.remapped) return report;

  // The tree holds this line with suits swapped. Present it in the suits the
  // caller asked for: their hand h is the stored hand with mapped cards.
  for (std::size_t player = 0; player < 2; ++player) {
    const auto& hands = game_.hands[player];
    std::vector<std::uint32_t> stored(hands.size());
    for (std::size_t h = 0; h < hands.size(); ++h) {
      const auto it = hand_index_[player].find(mapped_mask(hands[h], ref.card_map));
      stored[h] = it != hand_index_[player].end() ? it->second : static_cast<std::uint32_t>(h);
    }
    const auto permute = [&](std::vector<float>& values, std::size_t rows) {
      const std::vector<float> original = values;
      for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t h = 0; h < hands.size(); ++h) {
          values[row * hands.size() + h] = original[row * hands.size() + stored[h]];
        }
      }
    };
    permute(report.reach[player], 1);
    permute(report.ev[player], 1);
    permute(report.equity[player], 1);
    if (node.type == NodeType::Action && node.player == player) {
      permute(report.strategy, report.actions.size());
      const std::vector<std::uint8_t> original = report.locked;
      for (std::size_t h = 0; h < hands.size(); ++h) report.locked[h] = original[stored[h]];
    }
  }
  return report;
}

}  // namespace gto
