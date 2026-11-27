// N-player betting tree builder, path lookup and pot settlement.

#include "gto/mc/tree.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace gto::mc {

namespace {

constexpr double kEps = 1e-9;

bool equals_ignore_case(std::string_view a, std::string_view b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
           return std::tolower(static_cast<unsigned char>(x)) ==
                  std::tolower(static_cast<unsigned char>(y));
         });
}

struct TooLarge {};

/// Betting state while building.
struct State {
  Street street = Street::Preflop;
  std::array<double, kMaxPlayers> committed{};  // total since before the spot
  std::array<double, kMaxPlayers> round{};      // this betting round
  std::uint16_t folded = 0;
  std::uint16_t allin = 0;
  /// Acted since the last full bet or raise.
  std::uint16_t acted = 0;
  /// Put chips in voluntarily this round (preflop: called or raised).
  std::uint16_t voluntary = 0;
  int player = 0;
  double bet = 0;            // largest round amount
  double last_increment = 0; // size of the last full bet or raise
  int raises = 0;            // postflop: raises after the opening bet; preflop: raise level
  // Preflop bookkeeping.
  int limpers = 0;
  int last_raiser = -1;
  double last_raise_to = 0;
  int callers = 0;           // calls of the last raise
};

}  // namespace

class TreeBuilder {
 public:
  TreeBuilder(const GameConfig& config, BettingTree* tree, TreeCounts* counts)
      : config_(config), tree_(tree), counts_(counts), n_(config.num_players) {
    for (int s = 0; s < kNumStreets; ++s) {
      orders_[static_cast<std::size_t>(s)] =
          action_order(n_, config.start_street, static_cast<Street>(s));
    }
    // Position of each seat in the postflop order, for in/out of position.
    const auto& post = orders_[static_cast<std::size_t>(Street::Flop)];
    for (std::size_t i = 0; i < post.size(); ++i) {
      postflop_position_[static_cast<std::size_t>(post[i])] = static_cast<int>(i);
    }
    if (tree_ != nullptr) {
      tree_->num_players_ = n_;
      tree_->start_ = config.start_street;
      tree_->last_ = config.last_street;
      tree_->dead_pot_ = config.dead_pot;
      tree_->stacks_ = config.stacks;
      tree_->initial_.assign(static_cast<std::size_t>(n_), 0.0);
      for (int p = 0; p < n_; ++p) tree_->initial_[static_cast<std::size_t>(p)] = config.committed_of(p);
    }
  }

  void build() {
    State state;
    state.street = config_.start_street;
    for (int p = 0; p < n_; ++p) {
      const auto i = static_cast<std::size_t>(p);
      state.committed[i] = config_.committed_of(p);
      state.round[i] = state.committed[i];
      if (state.committed[i] >= config_.stacks[i] - kEps) state.allin |= bit(p);
      state.bet = std::max(state.bet, state.round[i]);
    }
    if (config_.start_street == Street::Preflop) {
      state.last_increment = config_.big_blind;
    }
    start_round(state, /*first=*/true, kNoParent);
  }

 private:
  static constexpr std::uint32_t kNoParent = 0xFFFFFFFFu;
  static std::uint16_t bit(int p) { return static_cast<std::uint16_t>(1u << p); }

  double stack(int p) const { return config_.stacks[static_cast<std::size_t>(p)]; }
  double behind(const State& s, int p) const {
    return stack(p) - s.committed[static_cast<std::size_t>(p)];
  }
  double pot(const State& s) const {
    double pot = config_.dead_pot;
    for (int p = 0; p < n_; ++p) pot += s.committed[static_cast<std::size_t>(p)];
    return pot;
  }
  bool can_act(const State& s, int p) const { return ((s.folded | s.allin) & bit(p)) == 0; }
  int count_bits(std::uint16_t m) const {
    int c = 0;
    for (int p = 0; p < n_; ++p) c += (m >> p) & 1;
    return c;
  }
  int remaining(const State& s) const { return n_ - count_bits(s.folded); }
  int able(const State& s) const { return n_ - count_bits(static_cast<std::uint16_t>(s.folded | s.allin)); }

  /// Adds a node; returns its id. In counting mode only counts.
  std::uint32_t add(NodeKind kind, const State& s, std::uint32_t parent, int player) {
    const std::uint64_t count = counts_ ? counts_->nodes : tree_->nodes_.size();
    if (count >= config_.max_nodes) throw TooLarge{};
    if (counts_) {
      ++counts_->nodes;
      if (kind == NodeKind::Action) ++counts_->action_nodes;
      else if (kind == NodeKind::Chance) ++counts_->chance_nodes;
      else ++counts_->terminals;
      return static_cast<std::uint32_t>(counts_->nodes - 1);
    }
    TreeNode node;
    node.kind = kind;
    node.street = s.street;
    node.player = static_cast<std::uint8_t>(player);
    node.folded = s.folded;
    node.allin = s.allin;
    node.parent = parent == kNoParent ? 0 : parent;
    const auto id = static_cast<std::uint32_t>(tree_->nodes_.size());
    if (kind == NodeKind::Action) {
      node.index = static_cast<std::uint32_t>(tree_->action_nodes_.size());
      tree_->action_nodes_.push_back(id);
    } else if (node.terminal()) {
      node.index = static_cast<std::uint32_t>(tree_->terminals_.size());
      tree_->terminals_.push_back(id);
    }
    tree_->nodes_.push_back(node);
    for (int p = 0; p < n_; ++p) tree_->committed_.push_back(s.committed[static_cast<std::size_t>(p)]);
    return id;
  }

  void link(std::uint32_t id, const std::vector<Action>& actions,
            const std::vector<std::uint32_t>& children) {
    if (counts_) return;
    TreeNode& node = tree_->nodes_[id];
    node.first_child = static_cast<std::uint32_t>(tree_->children_.size());
    node.num_children = static_cast<std::uint8_t>(children.size());
    for (std::size_t i = 0; i < children.size(); ++i) {
      tree_->children_.push_back(children[i]);
      tree_->actions_.push_back(i < actions.size() ? actions[i] : Action{});
      tree_->nodes_[children[i]].parent_action = static_cast<std::uint16_t>(i);
    }
  }

  /// Starts a betting round (or skips it when nobody can bet).
  std::uint32_t start_round(State s, bool first, std::uint32_t parent) {
    if (!first) {
      s.round.fill(0);
      s.bet = 0;
      s.last_increment = 0;
      s.raises = 0;
      s.acted = 0;
      s.voluntary = 0;
    }
    // Nobody left to bet against: straight to showdown.
    const int first_player = next_to_act(s, -1);
    if (first_player < 0 || (able(s) <= 1 && pending(s, first_player) <= kEps)) {
      return close_round(s, parent, /*skip_betting=*/true);
    }
    s.player = first_player;
    return action_node(s, parent);
  }

  double pending(const State& s, int p) const {
    return s.bet - s.round[static_cast<std::size_t>(p)];
  }

  /// Next player in the street's order after `after` (-1: from the start)
  /// who still has to act, or -1.
  int next_to_act(const State& s, int after) const {
    const auto& order = orders_[static_cast<std::size_t>(s.street)];
    const int size = static_cast<int>(order.size());
    int start = 0;
    if (after >= 0) {
      for (int i = 0; i < size; ++i) {
        if (order[static_cast<std::size_t>(i)] == after) start = i + 1;
      }
    }
    for (int k = 0; k < size; ++k) {
      const int p = order[static_cast<std::size_t>((start + k) % size)];
      if (!can_act(s, p)) continue;
      if ((s.acted & bit(p)) == 0 || pending(s, p) > kEps) return p;
    }
    return -1;
  }

  /// After player `p` acted in `s`: the next node.
  std::uint32_t continue_after(State s, int p, std::uint32_t parent) {
    if (remaining(s) == 1) {
      return add(NodeKind::FoldOut, s, parent, p);
    }
    const int next = next_to_act(s, p);
    if (next < 0) return close_round(s, parent, false);
    // A lone player able to act with nothing to call has no decision.
    if (able(s) <= 1 && pending(s, next) <= kEps) return close_round(s, parent, false);
    s.player = next;
    return action_node(s, parent);
  }

  std::uint32_t close_round(const State& s, std::uint32_t parent, bool /*skip_betting*/) {
    if (s.street >= config_.last_street || able(s) <= 1) {
      return add(NodeKind::Showdown, s, parent, 0);
    }
    State next = s;
    next.street = static_cast<Street>(static_cast<int>(s.street) + 1);
    const std::uint32_t id = add(NodeKind::Chance, next, parent, 0);
    const std::uint32_t child = start_round(next, false, id);
    link(id, {}, {child});
    return id;
  }

  double snap(double added, double remaining_stack) const {
    if (added >= remaining_stack - kEps) return remaining_stack;
    if (config_.allin_threshold > 0 && added >= config_.allin_threshold * remaining_stack) {
      return remaining_stack;
    }
    return added;
  }

  static void add_amount(std::vector<double>& amounts, double amount) {
    for (const double a : amounts) {
      if (std::abs(a - amount) < 1e-6) return;
    }
    amounts.push_back(amount);
  }

  /// Turns wished-for raise-to amounts into legal ones: at least a min-raise,
  /// at most pot size under pot limit, snapped to all-in.
  std::vector<double> legalize(const State& s, int p, const std::vector<double>& wishes,
                               bool add_allin) const {
    const auto i = static_cast<std::size_t>(p);
    const double rem = behind(s, p);
    const double to_call = pending(s, p);
    const double allin_to = s.round[i] + rem;
    const double min_to = s.bet + std::max(s.last_increment, kEps);
    const double max_to = config_.pot_limit ? s.bet + pot(s) + to_call : allin_to;
    std::vector<double> amounts;
    for (double to : wishes) {
      to = std::max(to, min_to);
      to = std::min(to, max_to);
      double added = to - s.round[i];
      added = snap(added, rem);
      if (config_.pot_limit && s.round[i] + added > max_to + kEps) added = max_to - s.round[i];
      add_amount(amounts, s.round[i] + added);
    }
    if (add_allin && allin_to <= max_to + kEps) add_amount(amounts, allin_to);
    std::sort(amounts.begin(), amounts.end());
    // A raise must put in more than a call.
    std::erase_if(amounts, [&](double to) { return to <= s.bet + kEps; });
    return amounts;
  }

  std::uint32_t action_node(const State& s, std::uint32_t parent) {
    const int p = s.player;
    const auto i = static_cast<std::size_t>(p);
    const std::uint32_t id = add(NodeKind::Action, s, parent, p);
    const double to_call = pending(s, p);
    const double rem = behind(s, p);
    const bool preflop = s.street == Street::Preflop;

    // Someone else who could still respond to a raise?
    bool others_can_act = false;
    for (int q = 0; q < n_; ++q) {
      if (q != p && can_act(s, q)) others_can_act = true;
    }

    std::vector<Action> actions;
    std::vector<std::uint32_t> children;
    std::vector<double> raises;  // raise-to amounts
    bool raise_is_bet = false;

    if (to_call > kEps) {
      // Fold.
      {
        State next = s;
        next.folded |= bit(p);
        actions.push_back({ActionType::Fold, 0});
        children.push_back(continue_after(next, p, id));
      }
      // Call.
      if (!preflop || call_allowed(s, p)) {
        State next = s;
        const double added = std::min(to_call, rem);
        next.committed[i] += added;
        next.round[i] += added;
        if (added >= rem - kEps) next.allin |= bit(p);
        next.acted |= bit(p);
        next.voluntary |= bit(p);
        if (preflop) {
          if (s.raises == 0) ++next.limpers;
          else ++next.callers;
        }
        actions.push_back({ActionType::Call, next.round[i]});
        children.push_back(continue_after(next, p, id));
      }
    } else {
      State next = s;
      next.acted |= bit(p);
      actions.push_back({ActionType::Check, 0});
      children.push_back(continue_after(next, p, id));
    }

    if (rem > to_call + kEps && others_can_act) {
      if (preflop) {
        if (s.raises < config_.preflop.raise_cap) raises = preflop_raises(s, p);
      } else if (s.bet <= kEps) {
        raise_is_bet = true;
        const BetSizing& sizing = config_.sizing[static_cast<std::size_t>(s.street)].for_player(p);
        std::vector<double> wishes;
        for (const double f : sizing.bet) wishes.push_back(s.round[i] + f * pot(s));
        raises = legalize(s, p, wishes, sizing.add_allin);
      } else if (s.raises < config_.raise_cap) {
        const BetSizing& sizing = config_.sizing[static_cast<std::size_t>(s.street)].for_player(p);
        std::vector<double> wishes;
        const double pot_after_call = pot(s) + to_call;
        for (const double f : sizing.raise) {
          wishes.push_back(s.bet + std::max(f * pot_after_call, s.last_increment));
        }
        raises = legalize(s, p, wishes, sizing.add_allin);
      }
    }

    for (const double to : raises) {
      State next = s;
      const double added = to - s.round[i];
      const bool allin = added >= rem - kEps;
      next.committed[i] += added;
      next.round[i] = to;
      if (allin) next.allin |= bit(p);
      const double increment = to - s.bet;
      if (increment >= s.last_increment - kEps) {
        next.last_increment = increment;
        next.acted = bit(p);
      } else {
        next.acted |= bit(p);  // short all-in: the others still owe a call
      }
      next.bet = std::max(s.bet, to);
      next.voluntary |= bit(p);
      if (preflop || !raise_is_bet) ++next.raises;
      next.last_raiser = p;
      next.last_raise_to = to;
      next.callers = 0;
      const ActionType type = allin ? ActionType::AllIn
                              : (raise_is_bet ? ActionType::Bet : ActionType::Raise);
      actions.push_back({type, to});
      children.push_back(continue_after(next, p, id));
    }

    if (actions.size() > static_cast<std::size_t>(kMaxActions)) {
      throw std::runtime_error("too many actions at one node");
    }
    if (counts_) {
      const auto st = static_cast<std::size_t>(s.street);
      ++counts_->decisions[st][i];
      counts_->actions[st][i] += actions.size();
    }
    link(id, actions, children);
    return id;
  }

  bool is_blind(int p) const {
    if (config_.start_street != Street::Preflop) return false;
    if (n_ == 2) return true;
    return p >= n_ - 2;
  }
  bool is_sb(int p) const {
    if (config_.start_street != Street::Preflop) return false;
    return n_ == 2 ? p == 0 : p == n_ - 2;
  }

  bool call_allowed(const State& s, int p) const {
    const PreflopSizing& pf = config_.preflop;
    const bool vol = (s.voluntary & bit(p)) != 0;
    if (s.raises == 0) {
      // Limp / complete.
      if (is_sb(p) && s.limpers == 0) return pf.allow_sb_complete;
      return is_sb(p) ? (pf.allow_sb_complete || pf.allow_limp) : pf.allow_limp;
    }
    if (vol) return true;
    if (s.raises == 1) {
      const bool bb = is_blind(p) && !is_sb(p);
      if (bb) return true;
      if (pf.max_cold_callers >= 0 && s.callers >= pf.max_cold_callers) return false;
      return is_blind(p) || pf.cold_call_open;
    }
    if (s.raises == 2) return pf.cold_call_3bet;
    return pf.cold_call_4bet;
  }

  std::vector<double> preflop_raises(const State& s, int p) const {
    const PreflopSizing& pf = config_.preflop;
    const double bb = config_.big_blind;
    const int level = s.raises + 1;
    std::vector<double> wishes;
    bool allin = pf.allin_every_level;
    if (level == 1) {
      const bool sb_first_in = is_sb(p) && s.limpers == 0;
      const auto seat = static_cast<std::size_t>(p);
      const std::vector<double>& opens =
          sb_first_in ? pf.sb_open_bb
          : (seat < pf.open_bb_by_seat.size() && !pf.open_bb_by_seat[seat].empty())
              ? pf.open_bb_by_seat[seat]
              : pf.open_bb;
      for (const double size : opens) {
        wishes.push_back(size * bb + s.limpers * pf.iso_per_limper_bb * bb);
      }
    } else if (level == 2 || level == 3) {
      std::vector<double> mults;
      if (level == 2) {
        const bool ip = s.last_raiser >= 0 &&
                        postflop_position_[static_cast<std::size_t>(p)] >
                            postflop_position_[static_cast<std::size_t>(s.last_raiser)];
        mults = ip ? pf.threebet_ip : pf.threebet_oop;
      } else {
        mults = pf.fourbet;
      }
      for (const double m : mults) {
        wishes.push_back((m + pf.per_caller * s.callers) * s.last_raise_to);
      }
    } else {
      allin = true;
    }
    return legalize(s, p, wishes, allin);
  }

  const GameConfig& config_;
  BettingTree* tree_;
  TreeCounts* counts_;
  int n_;
  std::array<std::vector<int>, kNumStreets> orders_;
  std::array<int, kMaxPlayers> postflop_position_{};
};

std::optional<BettingTree> build_tree(const GameConfig& config, std::string* error) {
  if (!config.validate(error)) return std::nullopt;
  BettingTree tree;
  try {
    TreeBuilder(config, &tree, nullptr).build();
  } catch (const TooLarge&) {
    if (error) *error = "the tree has more than " + std::to_string(config.max_nodes) + " nodes";
    return std::nullopt;
  } catch (const std::exception& e) {
    if (error) *error = e.what();
    return std::nullopt;
  }
  return tree;
}

std::optional<TreeCounts> count_tree(const GameConfig& config, std::string* error) {
  if (!config.validate(error)) return std::nullopt;
  TreeCounts counts;
  try {
    TreeBuilder(config, nullptr, &counts).build();
  } catch (const TooLarge&) {
    counts.too_large = true;
  } catch (const std::exception& e) {
    if (error) *error = e.what();
    return std::nullopt;
  }
  return counts;
}

std::uint64_t TreeCounts::tree_bytes(int num_players) const noexcept {
  return nodes * (sizeof(TreeNode) + sizeof(std::uint32_t) + sizeof(Action) +
                  sizeof(double) * static_cast<std::uint64_t>(num_players)) +
         action_nodes * sizeof(std::uint32_t) + terminals * sizeof(std::uint32_t);
}

// --- BettingTree ----------------------------------------------------------

double BettingTree::starting_pot() const noexcept {
  double pot = dead_pot_;
  for (const double c : initial_) pot += c;
  return pot;
}

double BettingTree::pot(std::uint32_t id) const noexcept {
  double pot = dead_pot_;
  for (const double c : committed(id)) pot += c;
  return pot;
}

std::size_t BettingTree::memory_bytes() const noexcept {
  return nodes_.capacity() * sizeof(TreeNode) + children_.capacity() * sizeof(std::uint32_t) +
         actions_.capacity() * sizeof(Action) + committed_.capacity() * sizeof(double) +
         (action_nodes_.capacity() + terminals_.capacity()) * sizeof(std::uint32_t);
}

std::optional<std::uint32_t> BettingTree::child(std::uint32_t id, std::string_view label) const {
  const auto acts = actions(id);
  const auto kids = children(id);
  for (std::size_t i = 0; i < acts.size(); ++i) {
    if (equals_ignore_case(action_label(acts[i]), label)) return kids[i];
  }
  return std::nullopt;
}

std::optional<BettingTree::Found> BettingTree::find(std::string_view path,
                                                    std::span<const Card> board,
                                                    std::string* error) const {
  const auto fail = [&](std::string message) -> std::optional<Found> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };
  Found found;
  std::uint64_t used = 0;
  for (const Card c : board) used |= c.mask();
  std::size_t start = 0;
  while (start < path.size()) {
    std::size_t end = path.find('/', start);
    if (end == std::string_view::npos) end = path.size();
    const std::string_view label = path.substr(start, end - start);
    start = end + 1;
    if (label.empty()) continue;
    const TreeNode& n = nodes_[found.node];
    if (n.kind == NodeKind::Chance) {
      const std::uint32_t next = children_[n.first_child];
      const Street street = nodes_[next].street;
      const int count = street == Street::Flop ? 3 : 1;
      if (label == "*") {
        found.cards_named = false;
      } else {
        const auto cards = parse_cards(label);
        if (!cards || static_cast<int>(cards->size()) != count) {
          return fail("expected " + std::to_string(count) + " card(s) for the " +
                      street_name(street) + ", got '" + std::string(label) + "'");
        }
        for (const Card c : *cards) {
          if ((used & c.mask()) != 0) return fail("card " + c.to_string() + " is already out");
          used |= c.mask();
          found.dealt.push_back(c);
        }
      }
      found.node = next;
      continue;
    }
    const auto child_id = child(found.node, label);
    if (!child_id) return fail("no action '" + std::string(label) + "' at this node");
    found.node = *child_id;
  }
  return found;
}

std::string BettingTree::path(std::uint32_t id, std::span<const Card> dealt) const {
  std::vector<std::uint32_t> line;
  for (std::uint32_t at = id; at != kRoot; at = nodes_[at].parent) line.push_back(at);
  std::reverse(line.begin(), line.end());
  std::string text;
  std::size_t card = 0;
  for (const std::uint32_t at : line) {
    const TreeNode& n = nodes_[at];
    const TreeNode& parent = nodes_[n.parent];
    if (!text.empty()) text += '/';
    if (parent.kind == NodeKind::Chance) {
      const std::size_t count = n.street == Street::Flop ? 3 : 1;
      if (card + count <= dealt.size()) {
        for (std::size_t k = 0; k < count; ++k) text += dealt[card + k].to_string();
      } else {
        text += '*';
      }
      card += count;
    } else {
      text += action_label(actions_[parent.first_child + n.parent_action]);
    }
  }
  return text;
}

// --- Pots -----------------------------------------------------------------

std::vector<PotLayer> pot_layers(std::span<const double> committed, double dead_pot,
                                 std::uint16_t folded) {
  const int n = static_cast<int>(committed.size());
  std::vector<double> levels;
  for (const double c : committed) {
    if (c > kEps) levels.push_back(c);
  }
  std::sort(levels.begin(), levels.end());
  levels.erase(std::unique(levels.begin(), levels.end(),
                           [](double a, double b) { return std::abs(a - b) < kEps; }),
               levels.end());
  std::vector<PotLayer> layers;
  double below = 0;
  for (const double level : levels) {
    PotLayer layer;
    for (int p = 0; p < n; ++p) {
      const double c = committed[static_cast<std::size_t>(p)];
      const double part = std::min(c, level) - std::min(c, below);
      if (part > kEps) {
        layer.amount += part;
        layer.contributors |= static_cast<std::uint16_t>(1u << p);
      }
      if (c >= level - kEps && (folded & (1u << p)) == 0) {
        layer.eligible |= static_cast<std::uint16_t>(1u << p);
      }
    }
    below = level;
    if (layer.eligible == 0 && !layers.empty()) {
      layers.back().amount += layer.amount;
      layers.back().contributors |= layer.contributors;
      continue;
    }
    layers.push_back(layer);
  }
  if (dead_pot > 0) {
    if (layers.empty()) {
      PotLayer layer;
      for (int p = 0; p < n; ++p) {
        if ((folded & (1u << p)) == 0) layer.eligible |= static_cast<std::uint16_t>(1u << p);
      }
      layers.push_back(layer);
    }
    layers.front().amount += dead_pot;
    layers.front().has_dead = true;
  }
  // A bottom layer nobody can win (everybody in it folded) cannot be built
  // by the tree; give it to the next layer's players.
  if (layers.size() > 1 && layers.front().eligible == 0) {
    layers[1].amount += layers[0].amount;
    layers[1].contributors |= layers[0].contributors;
    layers[1].has_dead = layers[1].has_dead || layers[0].has_dead;
    layers.erase(layers.begin());
  }
  return layers;
}

double take_rake(std::vector<PotLayer>& layers, const RakeConfig& rake, bool ended_preflop) {
  if (rake.rate <= 0) return 0;
  if (ended_preflop && rake.no_flop_no_drop) return 0;
  const auto contested = [](const PotLayer& layer) {
    return layer.has_dead || (layer.contributors & (layer.contributors - 1)) != 0;
  };
  double base = 0;
  for (const PotLayer& layer : layers) {
    if (contested(layer)) base += layer.amount;
  }
  if (base <= 0) return 0;
  double amount = rake.rate * base;
  if (rake.cap > 0) amount = std::min(amount, rake.cap);
  const double keep = 1.0 - amount / base;
  for (PotLayer& layer : layers) {
    if (contested(layer)) layer.amount *= keep;
  }
  return amount;
}

std::vector<double> settle_pot(std::span<const double> committed, double dead_pot,
                               std::uint16_t folded, std::span<const std::uint32_t> strength,
                               const RakeConfig& rake, bool ended_preflop) {
  const int n = static_cast<int>(committed.size());
  std::vector<PotLayer> layers = pot_layers(committed, dead_pot, folded);
  take_rake(layers, rake, ended_preflop);
  std::vector<double> payout(static_cast<std::size_t>(n), 0.0);
  for (const PotLayer& layer : layers) {
    std::uint32_t best = 0;
    int winners = 0;
    std::uint16_t mask = 0;
    for (int p = 0; p < n; ++p) {
      if ((layer.eligible & (1u << p)) == 0) continue;
      const std::uint32_t s = strength.empty() ? 0 : strength[static_cast<std::size_t>(p)];
      if (winners == 0 || s > best) {
        best = s;
        winners = 1;
        mask = static_cast<std::uint16_t>(1u << p);
      } else if (s == best) {
        ++winners;
        mask |= static_cast<std::uint16_t>(1u << p);
      }
    }
    if (winners == 0) continue;
    for (int p = 0; p < n; ++p) {
      if (mask & (1u << p)) payout[static_cast<std::size_t>(p)] += layer.amount / winners;
    }
  }
  return payout;
}

}  // namespace gto::mc
