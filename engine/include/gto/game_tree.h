#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "gto/card.h"

namespace gto {

/// Players are numbered by position: 0 acts first postflop (out of position).
inline constexpr int kOop = 0;
inline constexpr int kIp = 1;

inline constexpr std::uint8_t kNoCard = 0xFF;
/// Card ids are below this; Hold'em uses 0…51.
inline constexpr int kMaxCardIds = 64;

enum class Street : std::uint8_t { Flop = 0, Turn = 1, River = 2 };

enum class ActionType : std::uint8_t { Fold, Check, Call, Bet, Raise, AllIn };

struct Action {
  ActionType type = ActionType::Check;
  /// Chips the acting player has put in on this street after the action
  /// (the "to" amount). 0 for fold and check.
  double amount = 0;
};

/// "fold", "check", "call", "bet50", "raise150", "allin". Amounts are chips.
std::string action_label(const Action& action);

/// Bet and raise options for one player on one street.
struct BetSizing {
  /// Bet sizes as fractions of the pot (0.5 = half pot).
  std::vector<double> bet;
  /// Raise sizes as fractions of the pot after calling (1.0 = pot-size raise).
  std::vector<double> raise;
  /// Also offer all-in as a bet and as a raise.
  bool add_allin = false;
};

struct TreeConfig {
  /// Pot at the start of the spot, in chips (any unit, typically big blinds).
  double pot = 0;
  /// Chips each player has behind at the start of the spot.
  double effective_stack = 0;
  /// Indexed [street][player].
  std::array<std::array<BetSizing, 2>, 3> sizing;
  /// Most raises allowed on a street after the opening bet.
  int raise_cap = 3;
  /// A bet or raise that puts in at least this fraction of the player's
  /// remaining stack becomes all-in. 0 disables the rule.
  double allin_threshold = 0.67;
  /// Building stops with an error beyond this many nodes.
  std::size_t max_nodes = 100'000'000;
};

enum class NodeType : std::uint8_t { Action, Chance, Fold, Showdown };

/// One node of the tree. Kept small: flop trees have millions of them.
struct Node {
  NodeType type = NodeType::Action;
  /// Action: the player to act. Fold: the player who folded.
  std::uint8_t player = 0;
  /// The card dealt to get here, if the parent is a chance node.
  std::uint8_t card = kNoCard;
  std::uint16_t num_children = 0;
  /// Index of this node among its parent's children.
  std::uint16_t parent_action = 0;
  std::uint32_t parent = 0;
  /// Start of this node's children in GameTree::children.
  std::uint32_t first_child = 0;
  /// Action nodes: start of this node's actions in GameTree::actions.
  std::uint32_t first_action = 0;
  /// Showdown: index into GameTree::runouts. Chance: index into GameTree::chances.
  std::uint32_t aux = 0;
  /// Chips each player has put in since the start of the spot.
  std::array<double, 2> committed{0, 0};
};

/// A card that need not be solved separately: dealing `card` is the same
/// situation as the child `child` of the chance node with every hand's cards
/// mapped through the swap `swap`.
struct IsoDeal {
  std::uint8_t card = kNoCard;
  std::uint8_t swap = 0;
  std::uint16_t child = 0;
};

struct ChanceInfo {
  std::uint32_t first_iso = 0;
  std::uint32_t num_iso = 0;
};

/// A permutation of card ids.
using CardMap = std::array<std::uint8_t, kMaxCardIds>;
CardMap identity_card_map() noexcept;

/// A node, together with how the cards the user talks about map to the cards
/// stored in the tree. The two differ below an isomorphic deal: asking for the
/// turn 7d on a board where diamonds and clubs are interchangeable leads to the
/// stored 7c subtree with diamonds and clubs swapped.
struct NodeRef {
  std::uint32_t node = 0;
  CardMap card_map = identity_card_map();
  bool remapped = false;
};

/// Which pairs of suits can be swapped without changing either player's range.
struct SuitSymmetry {
  std::array<std::array<bool, 4>, 4> interchangeable{};
};

struct GameTree {
  static constexpr std::uint32_t kRoot = 0;

  double pot = 0;
  double effective_stack = 0;
  std::vector<Node> nodes;
  std::vector<std::uint32_t> children;
  std::vector<Action> actions;
  std::vector<ChanceInfo> chances;
  std::vector<IsoDeal> iso_deals;
  /// Cards dealt during the hand for each distinct showdown board.
  std::vector<std::array<std::uint8_t, 2>> runouts;
  /// Card permutations referenced by IsoDeal::swap.
  std::vector<CardMap> swaps;

  std::span<const std::uint32_t> children_of(std::uint32_t node) const noexcept {
    const Node& n = nodes[node];
    return {children.data() + n.first_child, n.num_children};
  }
  std::span<const Action> actions_of(std::uint32_t node) const noexcept {
    const Node& n = nodes[node];
    if (n.type != NodeType::Action) return {};
    return {actions.data() + n.first_action, n.num_children};
  }
  std::span<const IsoDeal> iso_of(std::uint32_t node) const noexcept {
    const ChanceInfo& info = chances[nodes[node].aux];
    return {iso_deals.data() + info.first_iso, info.num_iso};
  }

  // --- Construction. build_tree() uses these; tests build toy games by hand. ---

  std::uint32_t add_node(const Node& node);
  /// Makes `node` an action node with these actions leading to these children.
  void set_actions(std::uint32_t node, std::span<const Action> node_actions,
                   std::span<const std::uint32_t> node_children);
  /// Makes `node` a chance node. Each child must have its `card` set.
  void set_deals(std::uint32_t node, std::span<const std::uint32_t> node_children,
                 std::span<const IsoDeal> iso = {});

  // --- Queries ---

  std::size_t num_action_nodes() const noexcept;
  /// Memory held by the tree itself.
  std::size_t memory_bytes() const noexcept;

  /// Child reached by the action with this label, compared without case.
  std::optional<std::uint32_t> child(std::uint32_t node, std::string_view label) const;

  /// Follows a '/'-separated line from the root, e.g. "check/bet50/call/Ts/check".
  /// Actions are named by their label, dealt cards by the card. An empty path
  /// is the root.
  std::optional<NodeRef> find(std::string_view path, std::string* error = nullptr) const;

  /// The line that find() resolves to this node.
  std::string path(std::uint32_t node) const;
};

/// Builds the betting tree for a toy game without further cards: one betting
/// round ending in showdown.
std::optional<GameTree> build_tree(const TreeConfig& config, std::string* error = nullptr);

/// Builds the tree for a Hold'em spot on `board` (three to five cards),
/// dealing the remaining streets. Cards whose suit is interchangeable with a
/// lower suit, given the board so far and `symmetry`, are not expanded.
std::optional<GameTree> build_tree(const TreeConfig& config, std::span<const Card> board,
                                   const SuitSymmetry& symmetry, std::string* error = nullptr);

/// Size of a tree, measured without building it.
struct TreeStats {
  std::uint64_t nodes = 0;
  std::uint64_t action_nodes = 0;
  /// Total number of actions over each player's decision nodes.
  std::array<std::uint64_t, 2> actions{0, 0};
  /// Counting stopped at TreeConfig::max_nodes; the numbers are lower bounds.
  bool too_large = false;

  /// Memory the built tree would take.
  std::uint64_t tree_bytes() const noexcept;
};

/// Counts the tree build_tree() would build for a Hold'em spot.
std::optional<TreeStats> count_tree(const TreeConfig& config, std::span<const Card> board,
                                    const SuitSymmetry& symmetry, std::string* error = nullptr);

}  // namespace gto
