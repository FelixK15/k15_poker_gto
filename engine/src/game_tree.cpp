#include "gto/game_tree.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>

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

struct TreeTooLarge {};

struct State {
  std::array<double, 2> committed{0, 0};
  std::array<double, 2> street_bet{0, 0};
  int player = kOop;
  int raises = 0;
  /// Size of the last bet or raise increment; a raise must be at least this much.
  double last_increment = 0;
  int street = static_cast<int>(Street::River);
  /// Cards dealt since the start of the spot.
  std::array<std::uint8_t, 2> dealt{kNoCard, kNoCard};
  int num_dealt = 0;
};

class Builder {
 public:
  /// `board` is null for a toy game without cards to come.
  /// With `stats` set the builder only counts: nothing is stored in `tree`.
  Builder(const TreeConfig& config, const std::vector<Card>* board, const SuitSymmetry& symmetry,
          GameTree& tree, TreeStats* stats = nullptr)
      : config_(config), board_(board), symmetry_(symmetry), tree_(tree), stats_(stats) {}

  std::uint32_t action_node(const State& state) {
    const std::uint32_t index = add(NodeType::Action, state.player, state);
    const auto& sizing = config_.sizing[static_cast<std::size_t>(state.street)];

    const int player = state.player;
    const int opponent = 1 - player;
    const double remaining = config_.effective_stack - state.committed[player];
    const double to_call = state.street_bet[opponent] - state.street_bet[player];
    const double pot = config_.pot + state.committed[0] + state.committed[1];

    std::vector<Action> actions;
    std::vector<std::uint32_t> children;

    if (to_call <= kEpsilon) {
      // Nobody has bet yet.
      actions.push_back({ActionType::Check, 0});
      if (player == kOop) {
        State next = state;
        next.player = kIp;
        children.push_back(action_node(next));
      } else {
        children.push_back(close_round(state));
      }

      if (remaining > kEpsilon) {
        std::vector<double> amounts;
        // Pot limit: a bet is at most the pot.
        const double cap = pot;
        for (const double fraction : sizing[player].bet) {
          add_amount(amounts, snap_to_allin(fraction * pot, remaining, cap));
        }
        if (sizing[player].add_allin) {
          add_amount(amounts, config_.pot_limit ? std::min(remaining, cap) : remaining);
        }
        std::sort(amounts.begin(), amounts.end());

        for (const double amount : amounts) {
          const bool allin = amount >= remaining - kEpsilon;
          actions.push_back({allin ? ActionType::AllIn : ActionType::Bet, amount});
          State next = state;
          next.committed[player] += amount;
          next.street_bet[player] = amount;
          next.player = opponent;
          next.last_increment = amount;
          children.push_back(action_node(next));
        }
      }
    } else {
      // Facing a bet or raise.
      actions.push_back({ActionType::Fold, 0});
      children.push_back(add(NodeType::Fold, player, state));

      {
        actions.push_back({ActionType::Call, state.street_bet[opponent]});
        State next = state;
        next.committed[player] += to_call;
        next.street_bet[player] = state.street_bet[opponent];
        children.push_back(close_round(next));
      }

      const double opponent_remaining = config_.effective_stack - state.committed[opponent];
      const bool can_raise = opponent_remaining > kEpsilon && remaining > to_call + kEpsilon &&
                             state.raises < config_.raise_cap;
      if (can_raise) {
        const double pot_after_call = pot + to_call;
        const double allin_to = state.street_bet[player] + remaining;
        // Pot limit: call, then raise by at most the pot after the call.
        const double max_raise_to = state.street_bet[opponent] + pot_after_call;

        std::vector<double> amounts;
        for (const double fraction : sizing[player].raise) {
          const double increment = std::max(fraction * pot_after_call, state.last_increment);
          const double raise_to = state.street_bet[opponent] + increment;
          const double added = raise_to - state.street_bet[player];
          add_amount(amounts,
                     state.street_bet[player] +
                         snap_to_allin(added, remaining, max_raise_to - state.street_bet[player]));
        }
        if (sizing[player].add_allin) {
          add_amount(amounts, config_.pot_limit ? std::min(allin_to, max_raise_to) : allin_to);
        }
        std::sort(amounts.begin(), amounts.end());

        for (const double raise_to : amounts) {
          const bool allin = raise_to >= allin_to - kEpsilon;
          actions.push_back({allin ? ActionType::AllIn : ActionType::Raise, raise_to});
          State next = state;
          next.committed[player] += raise_to - state.street_bet[player];
          next.last_increment = raise_to - state.street_bet[opponent];
          next.street_bet[player] = raise_to;
          next.player = opponent;
          next.raises = state.raises + 1;
          children.push_back(action_node(next));
        }
      }
    }

    if (stats_ != nullptr) {
      ++stats_->action_nodes;
      stats_->actions[static_cast<std::size_t>(player)] += actions.size();
    } else {
      tree_.set_actions(index, actions, children);
    }
    return index;
  }

 private:
  /// The betting on this street ended without a fold.
  std::uint32_t close_round(const State& state) {
    if (board_ == nullptr || state.street == static_cast<int>(Street::River)) {
      const std::uint32_t index = add(NodeType::Showdown, 0, state);
      if (stats_ == nullptr) tree_.nodes[index].aux = runout_id(state);
      return index;
    }
    return chance_node(state);
  }

  std::uint32_t chance_node(const State& state) {
    const std::uint32_t index = add(NodeType::Chance, 0, state);

    // Rank masks of the cards on the table, per suit.
    std::array<unsigned, 4> suit_ranks{};
    std::uint64_t used = 0;
    const auto place = [&](Card card) {
      suit_ranks[static_cast<std::size_t>(card.suit())] |= 1u << card.rank();
      used |= card.mask();
    };
    for (const Card card : *board_) place(card);
    for (int i = 0; i < state.num_dealt; ++i) place(Card(state.dealt[static_cast<std::size_t>(i)]));

    // A suit is represented by the lowest suit it is interchangeable with:
    // same cards on the table, and neither range tells the two apart.
    std::array<int, 4> representative{0, 1, 2, 3};
    for (std::size_t suit = 1; suit < 4; ++suit) {
      for (std::size_t lower = 0; lower < suit; ++lower) {
        if (representative[lower] == static_cast<int>(lower) &&
            suit_ranks[lower] == suit_ranks[suit] && symmetry_.interchangeable[lower][suit]) {
          representative[suit] = static_cast<int>(lower);
          break;
        }
      }
    }

    const bool all_in = config_.effective_stack - state.committed[kOop] <= kEpsilon;

    std::vector<std::uint32_t> children;
    std::array<int, Card::kDeckSize> child_of_card;
    child_of_card.fill(-1);
    std::vector<IsoDeal> iso;

    for (int id = 0; id < Card::kDeckSize; ++id) {
      const Card card(static_cast<std::uint8_t>(id));
      if ((used & card.mask()) != 0) continue;
      const int rep_suit = representative[static_cast<std::size_t>(card.suit())];
      if (rep_suit != card.suit()) {
        if (stats_ != nullptr) continue;
        // Lower suits come first within a rank, so the representative exists.
        const Card rep(card.rank(), rep_suit);
        iso.push_back({card.index(), swap_id(rep_suit, card.suit()),
                       static_cast<std::uint16_t>(child_of_card[rep.index()])});
        continue;
      }

      State next = state;
      next.street = state.street + 1;
      next.dealt[static_cast<std::size_t>(next.num_dealt++)] = card.index();
      next.street_bet = {0, 0};
      next.player = kOop;
      next.raises = 0;
      next.last_increment = 0;

      // With the stacks in the middle there is nothing left to decide.
      const std::uint32_t child = all_in ? close_round(next) : action_node(next);
      if (stats_ == nullptr) tree_.nodes[child].card = card.index();
      child_of_card[static_cast<std::size_t>(id)] = static_cast<int>(children.size());
      children.push_back(child);
    }

    if (stats_ == nullptr) tree_.set_deals(index, children, iso);
    return index;
  }

  std::uint32_t add(NodeType type, int player, const State& state) {
    if (stats_ != nullptr) {
      if (stats_->nodes >= config_.max_nodes) throw TreeTooLarge{};
      ++stats_->nodes;
      return 0;
    }
    if (tree_.nodes.size() >= config_.max_nodes) throw TreeTooLarge{};
    Node node;
    node.type = type;
    node.player = static_cast<std::uint8_t>(player);
    node.committed = state.committed;
    return tree_.add_node(node);
  }

  std::uint32_t runout_id(const State& state) {
    const std::uint8_t low = std::min(state.dealt[0], state.dealt[1]);
    const std::uint8_t high = std::max(state.dealt[0], state.dealt[1]);
    const int key = low * 256 + high;
    const auto [it, inserted] = runout_ids_.try_emplace(key, 0u);
    if (inserted) {
      it->second = static_cast<std::uint32_t>(tree_.runouts.size());
      tree_.runouts.push_back(state.dealt);
    }
    return it->second;
  }

  std::uint8_t swap_id(int suit_a, int suit_b) {
    const int key = suit_a * 4 + suit_b;
    const auto [it, inserted] = swap_ids_.try_emplace(key, std::uint8_t{0});
    if (inserted) {
      CardMap map = identity_card_map();
      for (int rank = 0; rank < Card::kNumRanks; ++rank) {
        const Card a(rank, suit_a), b(rank, suit_b);
        map[a.index()] = b.index();
        map[b.index()] = a.index();
      }
      it->second = static_cast<std::uint8_t>(tree_.swaps.size());
      tree_.swaps.push_back(map);
    }
    return it->second;
  }

  /// Caps `chips` at the remaining stack and applies the all-in threshold.
  /// With pot-limit betting `chips` is first cut to `cap`, and the threshold
  /// only moves it to all-in if that is within the cap.
  double snap_to_allin(double chips, double remaining, double cap) const {
    bool allin_allowed = true;
    if (config_.pot_limit) {
      chips = std::min(chips, cap);
      allin_allowed = remaining <= cap + kEpsilon;
    }
    if (chips >= remaining - kEpsilon) return remaining;
    if (config_.allin_threshold > 0 && chips >= config_.allin_threshold * remaining &&
        allin_allowed) {
      return remaining;
    }
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
  const std::vector<Card>* board_;
  const SuitSymmetry& symmetry_;
  GameTree& tree_;
  TreeStats* stats_;
  std::map<int, std::uint32_t> runout_ids_;
  std::map<int, std::uint8_t> swap_ids_;
};

bool validate(const TreeConfig& config, std::string* error) {
  const auto fail = [&](const char* message) {
    if (error) *error = message;
    return false;
  };
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
  return true;
}

std::optional<GameTree> build(const TreeConfig& config, const std::vector<Card>* board,
                              const SuitSymmetry& symmetry, Street start, std::string* error) {
  if (!validate(config, error)) return std::nullopt;

  GameTree tree;
  tree.pot = config.pot;
  tree.effective_stack = config.effective_stack;
  State root;
  root.street = static_cast<int>(start);
  try {
    Builder(config, board, symmetry, tree).action_node(root);
  } catch (const TreeTooLarge&) {
    if (error) {
      *error = "the tree has more than " + std::to_string(config.max_nodes) +
               " nodes; use fewer bet sizes or a lower raise cap";
    }
    return std::nullopt;
  }
  return tree;
}

}  // namespace

CardMap identity_card_map() noexcept {
  CardMap map{};
  for (std::size_t i = 0; i < map.size(); ++i) map[i] = static_cast<std::uint8_t>(i);
  return map;
}

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

std::uint32_t GameTree::add_node(const Node& node) {
  nodes.push_back(node);
  return static_cast<std::uint32_t>(nodes.size() - 1);
}

void GameTree::set_actions(std::uint32_t node, std::span<const Action> node_actions,
                           std::span<const std::uint32_t> node_children) {
  assert(node_actions.size() == node_children.size());
  Node& n = nodes[node];
  n.type = NodeType::Action;
  n.first_child = static_cast<std::uint32_t>(children.size());
  n.first_action = static_cast<std::uint32_t>(actions.size());
  n.num_children = static_cast<std::uint16_t>(node_children.size());
  children.insert(children.end(), node_children.begin(), node_children.end());
  actions.insert(actions.end(), node_actions.begin(), node_actions.end());
  for (std::size_t i = 0; i < node_children.size(); ++i) {
    nodes[node_children[i]].parent = node;
    nodes[node_children[i]].parent_action = static_cast<std::uint16_t>(i);
  }
}

void GameTree::set_deals(std::uint32_t node, std::span<const std::uint32_t> node_children,
                         std::span<const IsoDeal> iso) {
  Node& n = nodes[node];
  n.type = NodeType::Chance;
  n.first_child = static_cast<std::uint32_t>(children.size());
  n.num_children = static_cast<std::uint16_t>(node_children.size());
  n.aux = static_cast<std::uint32_t>(chances.size());
  chances.push_back({static_cast<std::uint32_t>(iso_deals.size()),
                     static_cast<std::uint32_t>(iso.size())});
  children.insert(children.end(), node_children.begin(), node_children.end());
  iso_deals.insert(iso_deals.end(), iso.begin(), iso.end());
  for (std::size_t i = 0; i < node_children.size(); ++i) {
    assert(nodes[node_children[i]].card != kNoCard);
    nodes[node_children[i]].parent = node;
    nodes[node_children[i]].parent_action = static_cast<std::uint16_t>(i);
  }
}

std::uint64_t TreeStats::tree_bytes() const noexcept {
  return nodes * (sizeof(Node) + sizeof(std::uint32_t)) + (actions[0] + actions[1]) * sizeof(Action);
}

std::size_t GameTree::num_action_nodes() const noexcept {
  return static_cast<std::size_t>(std::count_if(
      nodes.begin(), nodes.end(), [](const Node& node) { return node.type == NodeType::Action; }));
}

std::size_t GameTree::memory_bytes() const noexcept {
  return nodes.capacity() * sizeof(Node) + children.capacity() * sizeof(std::uint32_t) +
         actions.capacity() * sizeof(Action) + chances.capacity() * sizeof(ChanceInfo) +
         iso_deals.capacity() * sizeof(IsoDeal) + runouts.capacity() * 2 +
         swaps.capacity() * sizeof(CardMap);
}

std::optional<std::uint32_t> GameTree::child(std::uint32_t node, std::string_view label) const {
  const auto node_actions = actions_of(node);
  const auto node_children = children_of(node);
  for (std::size_t i = 0; i < node_actions.size(); ++i) {
    if (equals_ignore_case(action_label(node_actions[i]), label)) return node_children[i];
  }
  return std::nullopt;
}

std::optional<NodeRef> GameTree::find(std::string_view path, std::string* error) const {
  NodeRef ref;
  std::size_t start = 0;
  while (start < path.size()) {
    std::size_t end = path.find('/', start);
    if (end == std::string_view::npos) end = path.size();
    const std::string_view label = path.substr(start, end - start);
    start = end + 1;
    if (label.empty()) continue;

    const Node& node = nodes[ref.node];
    if (node.type == NodeType::Chance) {
      const auto card = Card::parse(label);
      if (!card) {
        if (error) *error = "a card is dealt here, but '" + std::string(label) + "' is not a card";
        return std::nullopt;
      }
      // Below an isomorphic deal the tree holds the swapped suits.
      const std::uint8_t stored = ref.card_map[card->index()];
      bool found = false;
      const auto node_children = children_of(ref.node);
      for (const std::uint32_t child_index : node_children) {
        if (nodes[child_index].card == stored) {
          ref.node = child_index;
          found = true;
          break;
        }
      }
      if (!found) {
        for (const IsoDeal& iso : iso_of(ref.node)) {
          if (iso.card != stored) continue;
          const CardMap& swap = swaps[iso.swap];
          for (std::uint8_t& mapped : ref.card_map) mapped = swap[mapped];
          ref.node = node_children[iso.child];
          ref.remapped = true;
          found = true;
          break;
        }
      }
      if (!found) {
        if (error) *error = "the card " + card->to_string() + " is already in play";
        return std::nullopt;
      }
      continue;
    }

    const auto next = child(ref.node, label);
    if (!next) {
      if (error) {
        *error = "no action '" + std::string(label) + "' here; available:";
        for (const Action& action : actions_of(ref.node)) *error += " " + action_label(action);
        if (node.type != NodeType::Action) *error += " none (the hand is over)";
      }
      return std::nullopt;
    }
    ref.node = *next;
  }
  return ref;
}

std::string GameTree::path(std::uint32_t node) const {
  std::vector<std::string> labels;
  while (node != kRoot) {
    const Node& current = nodes[node];
    const Node& parent = nodes[current.parent];
    if (parent.type == NodeType::Chance) {
      labels.push_back(Card(current.card).to_string());
    } else {
      labels.push_back(action_label(actions[parent.first_action + current.parent_action]));
    }
    node = current.parent;
  }
  std::string result;
  for (auto it = labels.rbegin(); it != labels.rend(); ++it) {
    if (!result.empty()) result += '/';
    result += *it;
  }
  return result;
}

std::optional<GameTree> build_tree(const TreeConfig& config, std::string* error) {
  return build(config, nullptr, SuitSymmetry{}, Street::River, error);
}

namespace {

bool validate_board(std::span<const Card> board, std::string* error) {
  if (board.size() < 3 || board.size() > 5) {
    if (error) *error = "the board must have three, four or five cards";
    return false;
  }
  std::uint64_t seen = 0;
  for (const Card card : board) {
    if ((seen & card.mask()) != 0) {
      if (error) *error = "the board contains a card twice";
      return false;
    }
    seen |= card.mask();
  }
  return true;
}

}  // namespace

std::optional<GameTree> build_tree(const TreeConfig& config, std::span<const Card> board,
                                   const SuitSymmetry& symmetry, std::string* error) {
  if (!validate_board(board, error)) return std::nullopt;
  const std::vector<Card> cards(board.begin(), board.end());
  const auto start = static_cast<Street>(board.size() - 3);
  return build(config, &cards, symmetry, start, error);
}

std::optional<TreeStats> count_tree(const TreeConfig& config, std::span<const Card> board,
                                    const SuitSymmetry& symmetry, std::string* error) {
  if (!validate_board(board, error) || !validate(config, error)) return std::nullopt;
  const std::vector<Card> cards(board.begin(), board.end());

  TreeStats stats;
  GameTree unused;
  State root;
  root.street = static_cast<int>(board.size() - 3);
  try {
    Builder(config, &cards, symmetry, unused, &stats).action_node(root);
  } catch (const TreeTooLarge&) {
    stats.too_large = true;
  }
  return stats;
}

}  // namespace gto
