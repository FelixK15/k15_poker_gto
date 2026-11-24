#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gto {

/// Players are numbered by position: 0 acts first postflop (out of position).
inline constexpr int kOop = 0;
inline constexpr int kIp = 1;

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
};

enum class NodeType : std::uint8_t { Action, Chance, Fold, Showdown };

struct Node {
  NodeType type = NodeType::Action;
  /// Action: the player to act. Fold: the player who folded.
  std::uint8_t player = 0;
  /// Chips each player has put in since the start of the spot.
  std::array<double, 2> committed{0, 0};
  /// Parallel arrays; empty for terminal nodes.
  std::vector<Action> actions;
  std::vector<std::uint32_t> children;
  std::uint32_t parent = 0;
  /// Index of the action in the parent that leads here.
  std::uint32_t parent_action = 0;
};

struct GameTree {
  static constexpr std::uint32_t kRoot = 0;

  double pot = 0;
  double effective_stack = 0;
  std::vector<Node> nodes;

  std::size_t num_action_nodes() const noexcept;

  /// Child reached by the action with this label, compared without case.
  std::optional<std::uint32_t> child(std::uint32_t node, std::string_view label) const;

  /// Follows a '/'-separated action path from the root, e.g. "check/bet50/call".
  /// An empty path is the root.
  std::optional<std::uint32_t> find(std::string_view path, std::string* error = nullptr) const;

  /// The path that find() resolves to this node.
  std::string path(std::uint32_t node) const;
};

/// Builds the betting tree for a spot that starts on `start`.
/// Only river spots are supported so far; flop and turn follow with chance nodes.
std::optional<GameTree> build_tree(const TreeConfig& config, Street start,
                                   std::string* error = nullptr);

}  // namespace gto
