#include "gto/game_tree.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace gto {

namespace {

constexpr double kEpsilon = 1e-9;

std::string format_amount(double amount) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.2f", amount);
  std::string text = buffer;
  while (text.back() == '0') text.pop_back();
  if (text.back() == '.') text.pop_back();
  return text;
}

bool equals_ignore_case(std::string_view a, std::string_view b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
           return std::tolower(static_cast<unsigned char>(x)) ==
                  std::tolower(static_cast<unsigned char>(y));
         });
}

struct State {
  std::array<double, 2> committed{0, 0};
  std::array<double, 2> street_bet{0, 0};
  int player = kOop;
  int raises = 0;
  /// Size of the last bet or raise increment; a raise must be at least this much.
  double last_increment = 0;
};

class Builder {
 public:
  Builder(const TreeConfig& config, Street street, GameTree& tree)
      : config_(config), sizing_(config.sizing[static_cast<std::size_t>(street)]), tree_(tree) {}

  std::uint32_t action_node(const State& state, std::uint32_t parent, std::uint32_t parent_action) {
    const std::uint32_t index = add(NodeType::Action, state.player, state, parent, parent_action);

    const int player = state.player;
    const int opponent = 1 - player;
    const double remaining = config_.effective_stack - state.committed[player];
    const double to_call = state.street_bet[opponent] - state.street_bet[player];
    const double pot = config_.pot + state.committed[0] + state.committed[1];

    std::vector<Action> actions;
    std::vector<std::uint32_t> children;
    const auto next_index = [&] { return static_cast<std::uint32_t>(actions.size()); };

    if (to_call <= kEpsilon) {
      // Nobody has bet yet.
      {
        const std::uint32_t slot = next_index();
        actions.push_back({ActionType::Check, 0});
        if (player == kOop) {
          State next = state;
          next.player = kIp;
          children.push_back(action_node(next, index, slot));
        } else {
          children.push_back(add(NodeType::Showdown, 0, state, index, slot));
        }
      }

      if (remaining > kEpsilon) {
        std::vector<double> amounts;
        for (const double fraction : sizing_[player].bet) {
          add_amount(amounts, snap_to_allin(fraction * pot, remaining, remaining));
        }
        if (sizing_[player].add_allin) add_amount(amounts, remaining);
        std::sort(amounts.begin(), amounts.end());

        for (const double amount : amounts) {
          const std::uint32_t slot = next_index();
          const bool allin = amount >= remaining - kEpsilon;
          actions.push_back({allin ? ActionType::AllIn : ActionType::Bet, amount});
          State next = state;
          next.committed[player] += amount;
          next.street_bet[player] = amount;
          next.player = opponent;
          next.last_increment = amount;
          children.push_back(action_node(next, index, slot));
        }
      }
    } else {
      // Facing a bet or raise.
      {
        const std::uint32_t slot = next_index();
        actions.push_back({ActionType::Fold, 0});
        children.push_back(add(NodeType::Fold, player, state, index, slot));
      }
      {
        const std::uint32_t slot = next_index();
        actions.push_back({ActionType::Call, state.street_bet[opponent]});
        State next = state;
        next.committed[player] += to_call;
        next.street_bet[player] = state.street_bet[opponent];
        children.push_back(add(NodeType::Showdown, 0, next, index, slot));
      }

      const double opponent_remaining = config_.effective_stack - state.committed[opponent];
      const bool can_raise = opponent_remaining > kEpsilon && remaining > to_call + kEpsilon &&
                             state.raises < config_.raise_cap;
      if (can_raise) {
        const double pot_after_call = pot + to_call;
        const double allin_to = state.street_bet[player] + remaining;

        std::vector<double> amounts;
        for (const double fraction : sizing_[player].raise) {
          const double increment = std::max(fraction * pot_after_call, state.last_increment);
          const double raise_to = state.street_bet[opponent] + increment;
          const double added = raise_to - state.street_bet[player];
          add_amount(amounts,
                     state.street_bet[player] + snap_to_allin(added, remaining, remaining));
        }
        if (sizing_[player].add_allin) add_amount(amounts, allin_to);
        std::sort(amounts.begin(), amounts.end());

        for (const double raise_to : amounts) {
          const std::uint32_t slot = next_index();
          const bool allin = raise_to >= allin_to - kEpsilon;
          actions.push_back({allin ? ActionType::AllIn : ActionType::Raise, raise_to});
          State next = state;
          next.committed[player] += raise_to - state.street_bet[player];
          next.last_increment = raise_to - state.street_bet[opponent];
          next.street_bet[player] = raise_to;
          next.player = opponent;
          next.raises = state.raises + 1;
          children.push_back(action_node(next, index, slot));
        }
      }
    }

    tree_.nodes[index].actions = std::move(actions);
    tree_.nodes[index].children = std::move(children);
    return index;
  }

 private:
  std::uint32_t add(NodeType type, int player, const State& state, std::uint32_t parent,
                    std::uint32_t parent_action) {
    Node node;
    node.type = type;
    node.player = static_cast<std::uint8_t>(player);
    node.committed = state.committed;
    node.parent = parent;
    node.parent_action = parent_action;
    tree_.nodes.push_back(std::move(node));
    return static_cast<std::uint32_t>(tree_.nodes.size() - 1);
  }

  /// Caps `chips` at the stack and applies the all-in threshold.
  double snap_to_allin(double chips, double remaining, double allin) const {
    if (chips >= remaining - kEpsilon) return allin;
    if (config_.allin_threshold > 0 && chips >= config_.allin_threshold * remaining) return allin;
    return chips;
  }

  static void add_amount(std::vector<double>& amounts, double amount) {
    if (amount <= kEpsilon) return;
    for (const double existing : amounts) {
      if (std::abs(existing - amount) < 1e-6) return;
    }
    amounts.push_back(amount);
  }

  const TreeConfig& config_;
  const std::array<BetSizing, 2>& sizing_;
  GameTree& tree_;
};

}  // namespace

std::string action_label(const Action& action) {
  switch (action.type) {
    case ActionType::Fold: return "fold";
    case ActionType::Check: return "check";
    case ActionType::Call: return "call";
    case ActionType::Bet: return "bet" + format_amount(action.amount);
    case ActionType::Raise: return "raise" + format_amount(action.amount);
    case ActionType::AllIn: return "allin";
  }
  return "?";
}

std::size_t GameTree::num_action_nodes() const noexcept {
  return static_cast<std::size_t>(std::count_if(
      nodes.begin(), nodes.end(), [](const Node& node) { return node.type == NodeType::Action; }));
}

std::optional<std::uint32_t> GameTree::child(std::uint32_t node, std::string_view label) const {
  const Node& parent = nodes[node];
  for (std::size_t i = 0; i < parent.actions.size(); ++i) {
    if (equals_ignore_case(action_label(parent.actions[i]), label)) return parent.children[i];
  }
  return std::nullopt;
}

std::optional<std::uint32_t> GameTree::find(std::string_view path, std::string* error) const {
  std::uint32_t node = kRoot;
  std::size_t start = 0;
  while (start < path.size()) {
    std::size_t end = path.find('/', start);
    if (end == std::string_view::npos) end = path.size();
    const std::string_view label = path.substr(start, end - start);
    start = end + 1;
    if (label.empty()) continue;

    const auto next = child(node, label);
    if (!next) {
      if (error) {
        *error = "no action '" + std::string(label) + "' here; available:";
        for (const Action& action : nodes[node].actions) *error += " " + action_label(action);
        if (nodes[node].actions.empty()) *error += " none (the hand is over)";
      }
      return std::nullopt;
    }
    node = *next;
  }
  return node;
}

std::string GameTree::path(std::uint32_t node) const {
  std::vector<std::string> labels;
  while (node != kRoot) {
    const Node& current = nodes[node];
    labels.push_back(action_label(nodes[current.parent].actions[current.parent_action]));
    node = current.parent;
  }
  std::string result;
  for (auto it = labels.rbegin(); it != labels.rend(); ++it) {
    if (!result.empty()) result += '/';
    result += *it;
  }
  return result;
}

std::optional<GameTree> build_tree(const TreeConfig& config, Street start, std::string* error) {
  const auto fail = [&](const char* message) -> std::optional<GameTree> {
    if (error) *error = message;
    return std::nullopt;
  };

  if (start != Street::River) return fail("only river spots are supported so far");
  if (!(config.pot > 0)) return fail("the pot must be greater than 0");
  if (!(config.effective_stack >= 0)) return fail("the effective stack cannot be negative");
  if (config.raise_cap < 0) return fail("the raise cap cannot be negative");
  if (!(config.allin_threshold >= 0 && config.allin_threshold <= 1)) {
    return fail("the all-in threshold must be between 0 and 1");
  }
  for (const auto& street : config.sizing) {
    for (const BetSizing& sizing : street) {
      for (const double size : sizing.bet) {
        if (!(size > 0)) return fail("bet sizes must be greater than 0");
      }
      for (const double size : sizing.raise) {
        if (!(size > 0)) return fail("raise sizes must be greater than 0");
      }
    }
  }

  GameTree tree;
  tree.pot = config.pot;
  tree.effective_stack = config.effective_stack;
  Builder(config, start, tree).action_node(State{}, GameTree::kRoot, 0);
  return tree;
}

}  // namespace gto
