#include "exact_engine.h"

#include <algorithm>
#include <array>
#include <istream>
#include <numeric>
#include <ostream>
#include <unordered_map>

#include "convert.h"

namespace gto::server {

namespace {

v1::ActionType to_proto(ActionType type) {
  switch (type) {
    case ActionType::Fold: return v1::ACTION_TYPE_FOLD;
    case ActionType::Check: return v1::ACTION_TYPE_CHECK;
    case ActionType::Call: return v1::ACTION_TYPE_CALL;
    case ActionType::Bet: return v1::ACTION_TYPE_BET;
    case ActionType::Raise: return v1::ACTION_TYPE_RAISE;
    case ActionType::AllIn: return v1::ACTION_TYPE_ALLIN;
  }
  return v1::ACTION_TYPE_UNSPECIFIED;
}

v1::NodeType to_proto(NodeType type) {
  switch (type) {
    case NodeType::Action: return v1::NODE_TYPE_ACTION;
    case NodeType::Chance: return v1::NODE_TYPE_CHANCE;
    case NodeType::Fold: return v1::NODE_TYPE_FOLD;
    case NodeType::Showdown: return v1::NODE_TYPE_SHOWDOWN;
  }
  return v1::NODE_TYPE_UNSPECIFIED;
}

/// Adds the node's actions with how often the acting range takes each.
template <typename AddAction>
void fill_actions(const NodeReport& report, AddAction add_action) {
  if (report.type != NodeType::Action) return;
  const auto& reach = report.reach[static_cast<std::size_t>(report.player)];
  double total_reach = 0;
  for (const float value : reach) total_reach += value;

  for (std::size_t a = 0; a < report.actions.size(); ++a) {
    const Action& action = report.actions[a];
    v1::NodeAction* proto = add_action();
    proto->set_type(to_proto(action.type));
    proto->set_amount(action.amount);
    proto->set_label(action_label(action));
    double weighted = 0;
    for (std::size_t h = 0; h < reach.size(); ++h) {
      weighted += static_cast<double>(reach[h]) * report.frequency(a, h);
    }
    proto->set_frequency(total_reach > 0 ? weighted / total_reach : 0.0);
  }
}

/// The cards that can come at a chance node, named as the client knows them.
std::vector<std::string> cards_at(const GameTree& tree, const NodeRef& ref) {
  // The tree may hold this line with suits swapped.
  CardMap to_client = identity_card_map();
  for (std::size_t card = 0; card < to_client.size(); ++card) {
    to_client[ref.card_map[card]] = static_cast<std::uint8_t>(card);
  }
  std::vector<std::uint8_t> cards;
  for (const std::uint32_t child : tree.children_of(ref.node)) {
    cards.push_back(to_client[tree.nodes[child].card]);
  }
  for (const IsoDeal& iso : tree.iso_of(ref.node)) cards.push_back(to_client[iso.card]);
  std::sort(cards.begin(), cards.end());
  std::vector<std::string> names;
  for (const std::uint8_t card : cards) names.push_back(Card(card).to_string());
  return names;
}

/// The cards a path deals, as the client named them. Action labels are
/// longer than two characters, so every two-character segment is a card.
std::vector<Card> cards_on_path(std::string_view path) {
  std::vector<Card> cards;
  while (!path.empty()) {
    const std::size_t slash = path.find('/');
    const std::string_view segment = path.substr(0, slash);
    if (segment.size() == 2) {
      if (const auto card = Card::parse(segment)) cards.push_back(*card);
    }
    if (slash == std::string_view::npos) break;
    path.remove_prefix(slash + 1);
  }
  return cards;
}

/// Reads a lock from the wire against a solver's tree and hands.
bool read_lock(const Solver& solver, const v1::NodeLock& lock, NodeRef* ref,
               std::vector<std::uint8_t>* locked, std::vector<float>* strategy,
               std::string* error) {
  const Game& game = solver.game();
  const std::string where = "lock on '" + lock.path() + "': ";

  std::string find_error;
  const auto found = game.tree.find(lock.path(), &find_error);
  if (!found) {
    *error = where + find_error;
    return false;
  }
  const Node& node = game.tree.nodes[found->node];
  if (node.type != NodeType::Action) {
    *error = where + "only a decision can be locked";
    return false;
  }
  if (lock.hands().empty()) {
    *error = where + "it names no hands";
    return false;
  }

  const auto& hands = game.hands[node.player];
  const std::size_t actions = node.num_children;
  std::unordered_map<std::string, std::size_t> index;
  for (std::size_t h = 0; h < hands.size(); ++h) index.emplace(hand_name(hands[h]), h);

  locked->assign(hands.size(), 0);
  strategy->assign(actions * hands.size(), 0.0f);
  for (const v1::HandLock& hand : lock.hands()) {
    const auto it = index.find(hand.hand());
    if (it == index.end()) {
      *error = where + "the hand '" + hand.hand() + "' is not in the " +
               (node.player == kOop ? "OOP" : "IP") + " range";
      return false;
    }
    if (static_cast<std::size_t>(hand.frequencies_size()) != actions) {
      *error = where + "the hand '" + hand.hand() + "' needs " + std::to_string(actions) +
               " frequencies, one per action";
      return false;
    }
    float sum = 0;
    for (std::size_t a = 0; a < actions; ++a) {
      const float value = hand.frequencies(static_cast<int>(a));
      if (!(value >= 0)) {
        *error = where + "frequencies cannot be negative";
        return false;
      }
      (*strategy)[a * hands.size() + it->second] = value;
      sum += value;
    }
    if (!(sum > 0)) {
      *error = where + "the frequencies of '" + hand.hand() + "' add up to zero";
      return false;
    }
    (*locked)[it->second] = 1;
  }
  *ref = *found;
  return true;
}

// --- Hold'em hand classes --------------------------------------------------

constexpr int kNumClasses = 169;

/// Cell of the 13x13 grid: row by row from aces, suited right of the
/// diagonal, offsuit left of it.
int class_of(Card a, Card b) {
  const int hi = std::max(a.rank(), b.rank());
  const int lo = std::min(a.rank(), b.rank());
  if (hi == lo) return (12 - hi) * 13 + (12 - hi);
  if (a.suit() == b.suit()) return (12 - hi) * 13 + (12 - lo);
  return (12 - lo) * 13 + (12 - hi);
}

std::string class_name(int index) {
  const int row = index / 13;
  const int col = index % 13;
  std::string name;
  if (row == col) {
    name = {rank_to_char(12 - row), rank_to_char(12 - row)};
  } else if (col > row) {
    name = {rank_to_char(12 - row), rank_to_char(12 - col), 's'};
  } else {
    name = {rank_to_char(12 - col), rank_to_char(12 - row), 'o'};
  }
  return name;
}

/// Fills `out` with all 169 classes of one player. `actions` is the number
/// of actions when the player acts here, else 0.
void fill_classes(const std::vector<Hand>& hands, const NodeReport& report, std::size_t player,
                  std::size_t actions, std::uint64_t dead, v1::PlayerRange* out) {
  std::array<int, kNumClasses> possible{};
  for (int combo = 0; combo < Range::kNumCombos; ++combo) {
    const auto [a, b] = Range::combo_cards(combo);
    if (((a.mask() | b.mask()) & dead) == 0) ++possible[static_cast<std::size_t>(class_of(a, b))];
  }
  struct Sum {
    double reach = 0, ev = 0, equity = 0;
    std::vector<double> strategy;
  };
  std::vector<Sum> sums(kNumClasses);
  for (Sum& sum : sums) sum.strategy.assign(actions, 0.0);
  const auto& reach = report.reach[player];
  for (std::size_t h = 0; h < hands.size(); ++h) {
    const double r = reach[h];
    if (r <= 0) continue;
    Sum& sum = sums[static_cast<std::size_t>(class_of(Card(hands[h].cards[0]), Card(hands[h].cards[1])))];
    sum.reach += r;
    sum.ev += r * report.ev[player][h];
    sum.equity += r * report.equity[player][h];
    for (std::size_t a = 0; a < actions; ++a) sum.strategy[a] += r * report.frequency(a, h);
  }
  for (int c = 0; c < kNumClasses; ++c) {
    const Sum& sum = sums[static_cast<std::size_t>(c)];
    v1::HandClass* proto = out->add_classes();
    proto->set_name(class_name(c));
    const int count = possible[static_cast<std::size_t>(c)];
    proto->set_weight(count > 0 ? static_cast<float>(sum.reach / count) : 0.0f);
    proto->set_combos(static_cast<float>(sum.reach));
    if (sum.reach > 0) {
      proto->set_ev(static_cast<float>(sum.ev / sum.reach));
      proto->set_equity(static_cast<float>(sum.equity / sum.reach));
    }
    for (std::size_t a = 0; a < actions; ++a) {
      proto->add_strategy(sum.reach > 0 ? static_cast<float>(sum.strategy[a] / sum.reach) : 0.0f);
    }
  }
}

/// The hands to list: all of them, or the `limit` with the most reach, in
/// their own order.
std::vector<std::uint32_t> pick_hands(const std::vector<float>& reach, std::size_t limit) {
  std::vector<std::uint32_t> picked(reach.size());
  std::iota(picked.begin(), picked.end(), 0u);
  if (picked.size() <= limit) return picked;
  const auto more_reach = [&](std::uint32_t x, std::uint32_t y) {
    return reach[x] != reach[y] ? reach[x] > reach[y] : x < y;
  };
  std::nth_element(picked.begin(), picked.begin() + static_cast<std::ptrdiff_t>(limit),
                   picked.end(), more_reach);
  picked.resize(limit);
  std::sort(picked.begin(), picked.end());
  return picked;
}

}  // namespace

// --- Building ----------------------------------------------------------------

Error ExactEngine::estimate(const v1::Spot& spot, const EngineLimits& limits,
                            v1::EstimateTreeResponse* out) {
  std::string error;
  auto config = to_spot_config(spot, &error);
  if (!config) return make_error(ErrorKind::InvalidArgument, error);
  config->tree.max_nodes = limits.max_nodes;

  const auto estimate = estimate_spot(*config, &error);
  if (!estimate) return make_error(ErrorKind::InvalidArgument, error);

  out->set_nodes(estimate->tree.nodes);
  out->set_decision_nodes(estimate->tree.action_nodes);
  out->set_oop_hands(static_cast<std::uint32_t>(estimate->hands[kOop]));
  out->set_ip_hands(static_cast<std::uint32_t>(estimate->hands[kIp]));
  // The memory reported is what the solve will take, packed or not.
  const bool compress = should_compress(*estimate, limits.memory_budget_bytes);
  out->set_strategy_bytes(estimate->strategy_bytes_for(compress));
  out->set_tree_bytes(estimate->tree.tree_bytes());
  out->set_too_large(estimate->tree.too_large);
  out->set_memory_budget_bytes(limits.memory_budget_bytes);
  out->set_can_solve(!estimate->tree.too_large &&
                     (limits.memory_budget_bytes == 0 ||
                      estimate->total_bytes(compress) <= limits.memory_budget_bytes));

  out->add_player_hands(static_cast<std::uint32_t>(estimate->hands[kOop]));
  out->add_player_hands(static_cast<std::uint32_t>(estimate->hands[kIp]));
  out->set_strategy_entries(estimate->tree.actions[kOop] * estimate->hands[kOop] +
                            estimate->tree.actions[kIp] * estimate->hands[kIp]);
  out->set_engine(v1::ENGINE_EXACT);
  out->set_exploitability_kind(v1::EXPLOITABILITY_KIND_EXACT);
  out->set_player_count(2);
  for (const std::string& position : spot_positions(spot)) out->add_positions(position);
  return {};
}

std::unique_ptr<ExactEngine> ExactEngine::create(const v1::Spot& spot, const EngineLimits& limits,
                                                 std::string* error) {
  auto config = to_spot_config(spot, error);
  if (!config) return nullptr;
  config->tree.max_nodes = limits.max_nodes;
  // Decided the same way as for the estimate the job was accepted on.
  const auto estimate = estimate_spot(*config, error);
  if (!estimate) return nullptr;
  auto game = make_game(*config, error);
  if (!game) return nullptr;

  SolverConfig solver_config;
  solver_config.threads = limits.threads;
  solver_config.compress = should_compress(*estimate, limits.memory_budget_bytes);
  auto solver = std::make_unique<Solver>(std::move(*game), solver_config);
  return std::unique_ptr<ExactEngine>(new ExactEngine(std::move(solver), config->board,
                                                      config->variant, spot_positions(spot)));
}

ExactEngine::ExactEngine(std::unique_ptr<Solver> solver, std::vector<Card> board, Variant variant,
                         std::vector<std::string> positions)
    : solver_(std::move(solver)),
      board_(std::move(board)),
      variant_(variant),
      positions_(std::move(positions)) {}

// --- Solving -------------------------------------------------------------------

void ExactEngine::run_chunk(std::uint64_t budget) {
  if (iterations() < budget) solver_->iterate();
}

bool ExactEngine::measurement_due(int check_interval) const {
  return solver_->iterations() % std::max(1, check_interval) == 0;
}

EngineEvaluation ExactEngine::evaluate() const {
  const Evaluation evaluation = solver_->evaluate();
  EngineEvaluation result;
  result.exploitability_pct = evaluation.exploitability_pct;
  result.exploitability_kind = solver_->has_locks() ? v1::EXPLOITABILITY_KIND_LOCKED_GAME
                                                    : v1::EXPLOITABILITY_KIND_EXACT;
  result.evs = {evaluation.ev[kOop], evaluation.ev[kIp]};
  return result;
}

// --- Browsing ------------------------------------------------------------------

Error ExactEngine::node(std::string_view path, const NodeOptions& options,
                        v1::GetNodeResponse* out) const {
  const Game& game = solver_->game();
  const GameTree& tree = game.tree;
  const bool holdem = variant_ == Variant::Holdem;
  if (!holdem && options.detail != v1::HAND_DETAIL_HANDS) {
    return make_error(ErrorKind::InvalidArgument, "hand classes exist for Hold'em only");
  }

  std::string find_error;
  const auto ref = tree.find(path, &find_error);
  if (!ref) {
    return make_error(ErrorKind::NotFound, "path '" + std::string(path) + "': " + find_error);
  }

  const NodeReport report = solver_->report(*ref);
  const Node& node = tree.nodes[ref->node];
  const bool acting = report.type == NodeType::Action;

  out->set_type(to_proto(report.type));
  out->set_player(report.player == kOop ? v1::PLAYER_OOP : v1::PLAYER_IP);
  out->set_acting_player(static_cast<std::uint32_t>(report.player));
  out->set_path(std::string(path));
  out->set_iteration(static_cast<std::uint32_t>(solver_->iterations()));
  out->set_pot(tree.pot + node.committed[kOop] + node.committed[kIp]);
  out->set_oop_committed(node.committed[kOop]);
  out->set_ip_committed(node.committed[kIp]);

  std::vector<Card> board = board_;
  for (const Card card : cards_on_path(path)) board.push_back(card);
  std::uint64_t dead = 0;
  std::string board_text;
  for (const Card card : board) {
    dead |= card.mask();
    if (!board_text.empty()) board_text += ' ';
    board_text += card.to_string();
  }
  out->set_board(board_text);
  out->set_ev_estimated(false);

  const bool list_hands = options.detail != v1::HAND_DETAIL_CLASSES;
  const bool list_classes = options.detail != v1::HAND_DETAIL_HANDS;
  const std::size_t limit =
      hand_limit(options.max_hands, holdem ? v1::VARIANT_HOLDEM : v1::VARIANT_OMAHA4);
  const std::size_t action_count = acting ? report.actions.size() : 0;
  std::vector<std::uint32_t> actor_hands;

  for (std::size_t player = 0; player < 2; ++player) {
    out->add_committed(node.committed[player]);
    out->add_folded(report.type == NodeType::Fold &&
                    static_cast<std::size_t>(report.player) == player);
    out->add_all_in(tree.effective_stack - node.committed[player] <=
                    1e-9 * std::max(1.0, tree.effective_stack));

    v1::PlayerRange* range = out->add_players();
    const auto& hands = game.hands[player];
    const auto& reach = report.reach[player];
    range->set_position(player < positions_.size() ? positions_[player] : std::string());
    range->set_total_hands(static_cast<std::uint32_t>(hands.size()));
    range->set_range_ev(report.range_ev[player]);
    range->set_range_equity(report.range_equity[player]);

    if (list_hands) {
      const std::vector<std::uint32_t> picked = pick_hands(reach, limit);
      double shown = 0, total = 0;
      for (const float value : reach) total += value;
      range->mutable_hands()->Reserve(static_cast<int>(picked.size()));
      for (const std::uint32_t h : picked) {
        range->add_hands(hand_name(hands[h]));
        range->add_reach(reach[h]);
        range->add_ev(report.ev[player][h]);
        range->add_equity(report.equity[player][h]);
        shown += reach[h];
      }
      range->set_shown_reach_fraction(total > 0 ? shown / total : 1.0);
      if (acting && static_cast<std::size_t>(report.player) == player) actor_hands = picked;
    }
    if (list_classes) {
      const bool actor = acting && static_cast<std::size_t>(report.player) == player;
      fill_classes(hands, report, player, actor ? action_count : 0, dead, range);
    }
  }
  // The heads-up fields of older clients.
  *out->mutable_oop() = out->players(kOop);
  *out->mutable_ip() = out->players(kIp);

  if (acting) {
    fill_actions(report, [&] { return out->add_actions(); });
    if (list_hands) {
      out->mutable_strategy()->Reserve(static_cast<int>(action_count * actor_hands.size()));
      for (std::size_t a = 0; a < action_count; ++a) {
        for (const std::uint32_t h : actor_hands) out->add_strategy(report.frequency(a, h));
      }
      if (!report.locked.empty()) {
        for (const std::uint32_t h : actor_hands) out->add_locked(report.locked[h] != 0);
      }
    }
  }

  if (report.type == NodeType::Chance) {
    for (const std::string& card : cards_at(tree, *ref)) out->add_next_cards(card);
  }
  return {};
}

Error ExactEngine::runouts(std::string_view path, v1::GetRunoutReportResponse* out) const {
  const GameTree& tree = solver_->game().tree;

  std::string find_error;
  const auto ref = tree.find(path, &find_error);
  if (!ref) {
    return make_error(ErrorKind::NotFound, "path '" + std::string(path) + "': " + find_error);
  }
  if (tree.nodes[ref->node].type != NodeType::Chance) {
    return make_error(ErrorKind::InvalidArgument,
                      "no card is dealt at '" + std::string(path) + "'");
  }

  for (const std::string& card : cards_at(tree, *ref)) {
    const auto child = tree.find(path.empty() ? card : std::string(path) + "/" + card);
    if (!child) continue;
    const NodeReport report = solver_->report(*child);
    v1::Runout* runout = out->add_runouts();
    runout->set_card(card);
    runout->set_oop_ev(report.range_ev[kOop]);
    runout->set_ip_ev(report.range_ev[kIp]);
    runout->set_oop_equity(report.range_equity[kOop]);
    runout->set_ip_equity(report.range_equity[kIp]);
    for (std::size_t player = 0; player < 2; ++player) {
      runout->add_evs(report.range_ev[player]);
      runout->add_equities(report.range_equity[player]);
    }
    runout->set_type(to_proto(report.type));
    runout->set_player(report.player == kOop ? v1::PLAYER_OOP : v1::PLAYER_IP);
    runout->set_acting_player(static_cast<std::uint32_t>(report.player));
    fill_actions(report, [&] { return runout->add_actions(); });
  }
  return {};
}

// --- Locks ---------------------------------------------------------------------

Error ExactEngine::check_locks(
    const google::protobuf::RepeatedPtrField<v1::NodeLock>& locks) const {
  for (const v1::NodeLock& lock : locks) {
    NodeRef ref;
    std::vector<std::uint8_t> locked;
    std::vector<float> strategy;
    std::string error;
    if (!read_lock(*solver_, lock, &ref, &locked, &strategy, &error)) {
      return make_error(ErrorKind::InvalidArgument, error);
    }
  }
  return {};
}

Error ExactEngine::apply_locks(const google::protobuf::RepeatedPtrField<v1::NodeLock>& locks) {
  for (const v1::NodeLock& lock : locks) {
    NodeRef ref;
    std::vector<std::uint8_t> locked;
    std::vector<float> strategy;
    std::string error;
    if (!read_lock(*solver_, lock, &ref, &locked, &strategy, &error) ||
        !solver_->set_lock(ref, locked, strategy, &error)) {
      return make_error(ErrorKind::InvalidArgument, error);
    }
  }
  return {};
}

// --- State ---------------------------------------------------------------------

bool ExactEngine::save(std::ostream& out) const {
  solver_->save(out);
  return static_cast<bool>(out);
}

bool ExactEngine::load(std::istream& in, std::string* error) { return solver_->load(in, error); }

std::uint64_t ExactEngine::memory_bytes() const noexcept {
  return solver_->memory_bytes() + solver_->game().tree.memory_bytes();
}

}  // namespace gto::server
