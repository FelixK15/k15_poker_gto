#pragma once

// N-player betting tree and pot settlement for the mc engine.
//
// The tree holds betting only. Cards are not branched in storage: a street
// change is a Chance node with exactly one child, and which cards fell is
// part of a sampled deal (or, when browsing, named in the path). After the
// last street with betting, or once at most one player can still bet, the
// rest of the board is dealt at a Showdown node (all-in runouts included).
//
// Paths: '/'-separated labels from the root. Actions are named like the
// heads-up tree (gto::action_label): "fold", "check", "call", "bet50",
// "raise150", "allin"; amounts are the acting player's total for the street
// after the action ("to" amounts), in chips, so preflop "raise2.5" is a raise
// to 2.5. At a Chance node the next entry names the dealt cards: the whole
// flop as ONE entry of three cards ("Ks7d2c", any order), the turn and the
// river as one card each ("Th"). Example (3-way flop spot):
//   "check/bet6/call/fold/Th/check/check"
// Labels compare without case. A chance entry may also be "*" (cards not
// named); reports then cannot place hands into buckets below it.

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "gto/mc/game.h"

namespace gto::mc {

enum class NodeKind : std::uint8_t {
  /// A player decides.
  Action,
  /// The next street's cards are dealt; exactly one child.
  Chance,
  /// Everybody but one player folded.
  FoldOut,
  /// The remaining players show down after the rest of the board is dealt.
  Showdown,
};

struct TreeNode {
  NodeKind kind = NodeKind::Action;
  Street street = Street::Preflop;
  /// Action: the player to act. FoldOut: the player who folded last.
  std::uint8_t player = 0;
  std::uint8_t num_children = 0;
  /// Bit p set: player p has folded / is all-in.
  std::uint16_t folded = 0;
  std::uint16_t allin = 0;
  /// Index of this node among its parent's children.
  std::uint16_t parent_action = 0;
  std::uint32_t parent = 0;
  /// Start of the children in BettingTree::children (and, for action nodes,
  /// of the matching actions in BettingTree::actions).
  std::uint32_t first_child = 0;
  /// Action nodes: dense index 0..num_action_nodes-1 (storage key).
  /// Terminals: dense index 0..num_terminals-1.
  std::uint32_t index = 0;

  bool terminal() const noexcept { return kind == NodeKind::FoldOut || kind == NodeKind::Showdown; }
};

/// The betting tree. Nodes are in depth-first preorder: every child has a
/// larger index than its parent, the root is node 0.
class BettingTree {
 public:
  static constexpr std::uint32_t kRoot = 0;

  int num_players() const noexcept { return num_players_; }
  Street start_street() const noexcept { return start_; }
  Street last_street() const noexcept { return last_; }
  double dead_pot() const noexcept { return dead_pot_; }
  std::span<const double> stacks() const noexcept { return stacks_; }
  /// Chips each player had in at the start of the spot.
  std::span<const double> initial_committed() const noexcept { return initial_; }
  double starting_pot() const noexcept;

  std::size_t size() const noexcept { return nodes_.size(); }
  const TreeNode& node(std::uint32_t id) const noexcept { return nodes_[id]; }
  std::span<const TreeNode> nodes() const noexcept { return nodes_; }
  std::span<const std::uint32_t> children(std::uint32_t id) const noexcept {
    const TreeNode& n = nodes_[id];
    return {children_.data() + n.first_child, n.num_children};
  }
  /// Actions of an action node, parallel to children(); empty otherwise.
  std::span<const Action> actions(std::uint32_t id) const noexcept {
    const TreeNode& n = nodes_[id];
    if (n.kind != NodeKind::Action) return {};
    return {actions_.data() + n.first_child, n.num_children};
  }
  /// Chips each player has put in since before the spot (includes `committed`
  /// of the config), at this node.
  std::span<const double> committed(std::uint32_t id) const noexcept {
    return {committed_.data() + std::size_t{id} * static_cast<std::size_t>(num_players_),
            static_cast<std::size_t>(num_players_)};
  }
  /// dead pot plus everybody's committed chips.
  double pot(std::uint32_t id) const noexcept;

  std::size_t num_action_nodes() const noexcept { return action_nodes_.size(); }
  std::size_t num_terminals() const noexcept { return terminals_.size(); }
  /// Node id of the action node with dense index i.
  std::uint32_t action_node(std::size_t i) const noexcept { return action_nodes_[i]; }
  std::uint32_t terminal_node(std::size_t i) const noexcept { return terminals_[i]; }

  /// Memory held by the tree.
  std::size_t memory_bytes() const noexcept;

  /// Child reached by the action with this label (no case), or nullopt.
  std::optional<std::uint32_t> child(std::uint32_t id, std::string_view label) const;

  struct Found {
    std::uint32_t node = kRoot;
    /// Cards named at chance nodes on the way, in the order dealt (flop
    /// cards in the order written).
    std::vector<Card> dealt;
    /// False if a chance entry was "*".
    bool cards_named = true;
  };
  /// Resolves a path (see the file comment). `board` is the starting board,
  /// used to reject named cards that are already out.
  std::optional<Found> find(std::string_view path, std::span<const Card> board,
                            std::string* error = nullptr) const;

  /// The path to a node. Chance entries are taken from `dealt` (in order;
  /// three cards for a flop) as far as given, "*" beyond.
  std::string path(std::uint32_t id, std::span<const Card> dealt = {}) const;

 private:
  friend class TreeBuilder;
  int num_players_ = 2;
  Street start_ = Street::River;
  Street last_ = Street::River;
  double dead_pot_ = 0;
  std::vector<double> stacks_;
  std::vector<double> initial_;
  std::vector<TreeNode> nodes_;
  std::vector<std::uint32_t> children_;
  std::vector<Action> actions_;
  std::vector<double> committed_;
  std::vector<std::uint32_t> action_nodes_;
  std::vector<std::uint32_t> terminals_;
};

/// Builds the betting tree of a game. Fails on an invalid config or beyond
/// config.max_nodes nodes.
std::optional<BettingTree> build_tree(const GameConfig& config, std::string* error = nullptr);

/// Size of a betting tree, counted without building it.
struct TreeCounts {
  std::uint64_t nodes = 0;
  std::uint64_t action_nodes = 0;
  std::uint64_t chance_nodes = 0;
  std::uint64_t terminals = 0;
  /// [street][player]: decision nodes, and actions summed over them.
  std::array<std::array<std::uint64_t, kMaxPlayers>, kNumStreets> decisions{};
  std::array<std::array<std::uint64_t, kMaxPlayers>, kNumStreets> actions{};
  /// Counting stopped at config.max_nodes; numbers are lower bounds.
  bool too_large = false;

  /// Memory the built tree takes for `num_players` players.
  std::uint64_t tree_bytes(int num_players) const noexcept;
};
std::optional<TreeCounts> count_tree(const GameConfig& config, std::string* error = nullptr);

// --- Pot settlement -------------------------------------------------------

/// One layer of the pot (main pot first, then side pots).
struct PotLayer {
  double amount = 0;
  /// Players who can win it: not folded and in for at least the layer's top.
  std::uint16_t eligible = 0;
  /// Players who put chips into it (folded or not).
  std::uint16_t contributors = 0;
  /// Whether dead money is in it (the main pot only).
  bool has_dead = false;
};

/// Splits the pot into layers by the players' total contributions. Dead
/// money goes into the main pot. Folded players' chips stay in the layers
/// they reached. A layer that no remaining player reached (cannot happen in
/// trees built here) is merged into the layer below.
std::vector<PotLayer> pot_layers(std::span<const double> committed, double dead_pot,
                                 std::uint16_t folded);

/// Rake for a settled pot: rate × the contested pot (every layer except an
/// uncalled top layer, i.e. one with a single contributor and no dead
/// money, which simply returns to its owner), capped; zero when
/// `ended_preflop` and rake.no_flop_no_drop. Taken from the contested layers
/// in proportion to their size; the layers are reduced in place. Returns the
/// rake.
double take_rake(std::vector<PotLayer>& layers, const RakeConfig& rake, bool ended_preflop);

/// Chips each player takes out of the pot. Each layer goes to the eligible
/// players with the highest strength, split equally on ties. `strength` is
/// ignored for players who folded (pass anything); a lone remaining player
/// wins everything. `ended_preflop`: no flop was dealt (a fold preflop).
std::vector<double> settle_pot(std::span<const double> committed, double dead_pot,
                               std::uint16_t folded, std::span<const std::uint32_t> strength,
                               const RakeConfig& rake, bool ended_preflop);

}  // namespace gto::mc
