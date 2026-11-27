// Node reports for browsing a solved mc game.

#include <algorithm>
#include <cmath>
#include <limits>

#include "gto/range.h"
#include "solver_impl.h"

namespace gto::mc {

std::optional<NodeReport> Solver::report(std::string_view path, const ReportOptions& options,
                                         std::string* error) const {
  const auto found = tree_.find(path, config_.board, error);
  if (!found) return std::nullopt;
  return report(*found, options, error);
}

std::optional<NodeReport> Solver::report(const BettingTree::Found& found,
                                         const ReportOptions& options, std::string* error) const {
  const auto fail = [&](std::string message) -> std::optional<NodeReport> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };
  const Impl& impl = *impl_;
  const int n = config_.num_players;
  const std::uint32_t target = found.node;
  const TreeNode& node = tree_.node(target);

  NodeReport r;
  r.node = target;
  r.kind = node.kind;
  r.street = node.street;
  r.player = node.player;
  for (const Action& a : tree_.actions(target)) {
    r.actions.push_back(a);
    r.labels.push_back(action_label(a));
  }
  r.pot = tree_.pot(target);
  for (int p = 0; p < n; ++p) {
    r.committed.push_back(tree_.committed(target)[static_cast<std::size_t>(p)]);
    r.folded.push_back((node.folded >> p) & 1);
    r.allin.push_back((node.allin >> p) & 1);
  }
  r.board = config_.board;
  r.board.insert(r.board.end(), found.dealt.begin(), found.dealt.end());
  r.cards_named = found.cards_named;
  r.num_combos = config_.num_combos();
  const std::size_t combos = static_cast<std::size_t>(r.num_combos);

  std::uint64_t board_mask = 0;
  std::uint8_t board[5] = {};
  for (std::size_t i = 0; i < r.board.size() && i < 5; ++i) {
    board_mask |= r.board[i].mask();
    board[i] = r.board[i].index();
  }
  const int known = static_cast<int>(r.board.size());

  // The line from the root: (node, action taken).
  std::vector<std::pair<std::uint32_t, int>> line;
  for (std::uint32_t at = target; at != BettingTree::kRoot;) {
    const TreeNode& child = tree_.node(at);
    line.push_back({child.parent, child.parent_action});
    at = child.parent;
  }
  std::reverse(line.begin(), line.end());

  const auto needs_board = [&](Street street) {
    return impl.total_board > 0 && board_size(street) > known;
  };

  // Reach per player and live hand.
  std::vector<std::vector<double>> live_reach(static_cast<std::size_t>(n));
  r.reach.assign(static_cast<std::size_t>(n), std::vector<float>(combos, 0.0f));
  for (int p = 0; p < n; ++p) {
    const auto& list = hands_[static_cast<std::size_t>(p)].hands;
    auto& lr = live_reach[static_cast<std::size_t>(p)];
    lr.assign(list.size(), 0.0);
    for (std::size_t h = 0; h < list.size(); ++h) {
      if (list[h].mask & board_mask) continue;
      lr[h] = list[h].weight;
    }
    for (const auto& [id, action] : line) {
      const TreeNode& at = tree_.node(id);
      if (at.kind != NodeKind::Action || at.player != p) continue;
      if (needs_board(at.street)) return fail("name the dealt cards to report this node");
      const BucketFunction& fn = abstraction_.street(at.street);
      float sigma[kMaxActions];
      for (std::size_t h = 0; h < list.size(); ++h) {
        if (lr[h] == 0) continue;
        impl.average(id, fn.bucket(p, list[h], static_cast<int>(h), board), sigma);
        lr[h] *= sigma[action];
      }
    }
    for (std::size_t h = 0; h < list.size(); ++h) {
      r.reach[static_cast<std::size_t>(p)][static_cast<std::size_t>(list[h].combo)] = static_cast<float>(lr[h]);
    }
  }

  // The actor's strategy.
  if (node.kind == NodeKind::Action) {
    if (needs_board(node.street)) return fail("name the dealt cards to report this node");
    const int na = node.num_children;
    r.strategy.assign(combos * static_cast<std::size_t>(na), 0.0f);
    const auto& list = hands_[static_cast<std::size_t>(node.player)].hands;
    const BucketFunction& fn = abstraction_.street(node.street);
    float sigma[kMaxActions];
    for (std::size_t h = 0; h < list.size(); ++h) {
      if (list[h].mask & board_mask) continue;
      impl.average(target, fn.bucket(node.player, list[h], static_cast<int>(h), board), sigma);
      for (int a = 0; a < na; ++a) {
        r.strategy[static_cast<std::size_t>(a) * combos + static_cast<std::size_t>(list[h].combo)] = sigma[a];
      }
    }
  }

  // EVs by sampling continuations.
  r.ev.assign(static_cast<std::size_t>(n), {});
  r.range_ev.assign(static_cast<std::size_t>(n), std::numeric_limits<double>::quiet_NaN());
  if (options.ev_samples > 0) {
    std::vector<AliasTable> tables;
    for (int p = 0; p < n; ++p) tables.emplace_back(live_reach[static_cast<std::size_t>(p)]);
    bool reachable = true;
    for (const AliasTable& t : tables) reachable = reachable && !t.empty();
    for (int p = 0; p < n && reachable; ++p) {
      const auto& lr = live_reach[static_cast<std::size_t>(p)];
      std::vector<int> todo;
      for (std::size_t h = 0; h < lr.size(); ++h) {
        if (lr[h] > 0) todo.push_back(static_cast<int>(h));
      }
      if (todo.size() > options.max_ev_combos) continue;
      std::vector<double> ev(todo.size(), std::numeric_limits<double>::quiet_NaN());
      pool_->parallel_for(todo.size(), [&](std::size_t k, unsigned) {
        const int h = todo[k];
        double sum = 0;
        std::uint64_t count = 0;
        Deal deal;
        for (std::uint64_t sample = 0; sample < options.ev_samples; ++sample) {
          Rng rng(mix_seed(mix_seed(options.seed, static_cast<std::uint64_t>(p) * 1'000'003 + static_cast<std::uint64_t>(h)), sample));
          if (!impl.sample_hands(rng, deal, tables, board_mask, p, h)) continue;
          for (int i = 0; i < known; ++i) deal.board[static_cast<std::size_t>(i)] = board[i];
          impl.sample_board(rng, deal, std::min(known, impl.total_board), board_mask | impl.hands_mask(deal));
          deal.strength_ready = false;
          impl.compute_buckets(deal);
          double values[kMaxPlayers] = {};
          impl.walk_average(deal, rng, target, values);
          sum += values[p];
          ++count;
        }
        if (count > 0) ev[k] = sum / static_cast<double>(count);
      });
      auto& out = r.ev[static_cast<std::size_t>(p)];
      out.assign(combos, std::numeric_limits<float>::quiet_NaN());
      double num = 0, den = 0;
      const auto& list = hands_[static_cast<std::size_t>(p)].hands;
      for (std::size_t k = 0; k < todo.size(); ++k) {
        const int h = todo[k];
        out[static_cast<std::size_t>(list[static_cast<std::size_t>(h)].combo)] = static_cast<float>(ev[k]);
        if (!std::isnan(ev[k])) {
          num += lr[static_cast<std::size_t>(h)] * ev[k];
          den += lr[static_cast<std::size_t>(h)];
        }
      }
      if (den > 0) r.range_ev[static_cast<std::size_t>(p)] = num / den;
    }
  }
  return r;
}

ClassStrategy preflop_class_strategy(const NodeReport& report) {
  ClassStrategy result;
  const std::size_t na = report.actions.size();
  for (int c = 0; c < 169; ++c) result.names.push_back(holdem_class_name(c));
  result.actions = report.labels;
  result.strategy.assign(169 * na, 0.0f);
  result.weight.assign(169, 0.0f);
  if (report.num_combos != 1326 || na == 0 || report.strategy.empty()) return result;
  std::vector<double> weighted(169 * na, 0.0), uniform(169 * na, 0.0), wsum(169, 0.0);
  std::vector<int> ucount(169, 0);
  const auto& reach = report.reach[static_cast<std::size_t>(report.player)];
  for (int combo = 0; combo < 1326; ++combo) {
    double total = 0;
    for (std::size_t a = 0; a < na; ++a) total += report.frequency(a, combo);
    if (total <= 0) continue;  // blocked or not in the range
    const auto [x, y] = Range::combo_cards(combo);
    const auto cls = static_cast<std::size_t>(holdem_class(x, y));
    const double w = reach[static_cast<std::size_t>(combo)];
    for (std::size_t a = 0; a < na; ++a) {
      weighted[cls * na + a] += w * report.frequency(a, combo);
      uniform[cls * na + a] += report.frequency(a, combo);
    }
    wsum[cls] += w;
    ++ucount[cls];
  }
  for (std::size_t c = 0; c < 169; ++c) {
    result.weight[c] = static_cast<float>(wsum[c]);
    for (std::size_t a = 0; a < na; ++a) {
      const double v = wsum[c] > 0 ? weighted[c * na + a] / wsum[c]
                                   : (ucount[c] > 0 ? uniform[c * na + a] / ucount[c] : 0.0);
      result.strategy[c * na + a] = static_cast<float>(v);
    }
  }
  return result;
}

}  // namespace gto::mc
