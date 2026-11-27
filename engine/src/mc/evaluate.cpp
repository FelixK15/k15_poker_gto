// Evaluation of the average strategies: EVs and best-response gains,
// estimated by sampling or exact by enumerating deals.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>

#include "solver_impl.h"

namespace gto::mc {

namespace {

double hash_uniform(std::uint64_t seed, std::uint64_t node) {
  return static_cast<double>(mix_seed(seed, node) >> 11) * 0x1.0p-53;
}

int pick(const float* sigma, int count, double u) {
  double acc = 0;
  for (int a = 0; a < count - 1; ++a) {
    acc += sigma[a];
    if (u < acc) return a;
  }
  for (int a = count - 1; a > 0; --a) {
    if (sigma[a] > 0) return a;
  }
  return 0;
}

struct Moments {
  double sum = 0, sum2 = 0;
  std::uint64_t count = 0;
  void add(double v) {
    sum += v;
    sum2 += v * v;
    ++count;
  }
  void merge(const Moments& o) {
    sum += o.sum;
    sum2 += o.sum2;
    count += o.count;
  }
  double mean() const { return count ? sum / static_cast<double>(count) : 0.0; }
  double std_error() const {
    if (count < 2) return 0;
    const double m = mean();
    const double var = std::max(0.0, sum2 / static_cast<double>(count) - m * m);
    return std::sqrt(var / static_cast<double>(count - 1));
  }
};

constexpr std::uint8_t kNoPolicy = 255;

/// Best response of one player on a fixed set of sampled scenarios.
///
/// Per information set of the player it keeps, over the scenarios, the sum
/// of each action's advantage over the average strategy's value, the sum of
/// squared advantages and the visit count. With `significance` z > 0 the
/// response deviates from the average strategy at an information set only
/// where the best action's mean advantage exceeds z standard errors; this
/// keeps noise from being counted as gain (z = 0: plain argmax).
class SampledBestResponse {
 public:
  SampledBestResponse(const Solver::Impl& impl, int player, ThreadPool& pool, double significance)
      : impl_(impl), player_(player), pool_(pool), z_(significance) {
    const BettingTree& tree = impl.tree;
    const TableLayout& layout = impl.layout;
    local_.assign(tree.num_action_nodes(), kNotMine);
    std::uint64_t size = 0;
    for (std::size_t i = 0; i < tree.num_action_nodes(); ++i) {
      if (tree.node(tree.action_node(i)).player != player) continue;
      local_[i] = size;
      size += std::uint64_t{static_cast<std::uint64_t>(layout.actions(static_cast<std::uint32_t>(i)))} *
              layout.buckets(static_cast<std::uint32_t>(i));
    }
    entries_ = size;
    // Indexed by an information set's first entry.
    policy_.assign(static_cast<std::size_t>(size), kNoPolicy);
  }

  /// Policy iteration on `samples` scenarios from `seed`. Returns whether it
  /// settled; `gain` holds the in-sample gain of the final policy.
  bool solve(std::uint64_t samples, std::uint64_t seed, Moments& gain) {
    q_.assign(static_cast<std::size_t>(entries_), 0.0f);
    q2_.assign(static_cast<std::size_t>(entries_), 0.0f);
    visits_.assign(static_cast<std::size_t>(entries_), 0.0f);
    // Passes needed: the most decisions of this player on any line, + 1.
    const BettingTree& tree = impl_.tree;
    std::vector<int> height(tree.size(), 0);
    for (std::size_t id = tree.size(); id-- > 0;) {
      int h = 0;
      for (const std::uint32_t c : tree.children(static_cast<std::uint32_t>(id))) h = std::max(h, height[c]);
      const TreeNode& node = tree.node(static_cast<std::uint32_t>(id));
      height[id] = h + (node.kind == NodeKind::Action && node.player == player_ ? 1 : 0);
    }
    const int passes = std::min(height[0] + 1, 16);
    bool settled = false;
    bool use_policy = false;
    for (int pass = 0; pass < passes; ++pass) {
      std::fill(q_.begin(), q_.end(), 0.0f);
      std::fill(q2_.begin(), q2_.end(), 0.0f);
      std::fill(visits_.begin(), visits_.end(), 0.0f);
      gain = score(samples, seed, use_policy, true);
      const std::uint64_t changed = update_policy();
      if (use_policy && changed == 0) {
        settled = true;
        break;
      }
      use_policy = true;
    }
    if (!settled) gain = score(samples, seed, true, false);
    return settled;
  }

  /// Mean gain (policy value - average strategy value) over scenarios.
  Moments score(std::uint64_t samples, std::uint64_t seed, bool use_policy, bool learn) {
    constexpr std::uint64_t kBlock = 256;
    const std::uint64_t blocks = (samples + kBlock - 1) / kBlock;
    std::vector<Moments> parts(static_cast<std::size_t>(blocks));
    pool_.parallel_for(static_cast<std::size_t>(blocks), [&](std::size_t block, unsigned) {
      Deal deal;
      const std::uint64_t begin = block * kBlock;
      const std::uint64_t end = std::min(samples, begin + kBlock);
      for (std::uint64_t k = begin; k < end; ++k) {
        const std::uint64_t scenario = mix_seed(seed, k);
        Rng rng(scenario);
        if (!impl_.sample_deal(rng, deal)) continue;
        impl_.compute_buckets(deal);
        double pol = 0, avg = 0;
        walk(deal, scenario, BettingTree::kRoot, use_policy, learn, pol, avg);
        parts[block].add(pol - avg);
      }
    });
    Moments total;
    for (const Moments& m : parts) total.merge(m);
    return total;
  }

 private:
  static constexpr std::uint64_t kNotMine = ~std::uint64_t{0};

  static void add(std::vector<float>& v, std::uint64_t i, double x) {
    std::atomic_ref<float>(v[static_cast<std::size_t>(i)])
        .fetch_add(static_cast<float>(x), std::memory_order_relaxed);
  }

  void walk(Deal& deal, std::uint64_t scenario, std::uint32_t id, bool use_policy, bool learn,
            double& pol, double& avg) {
    const BettingTree& tree = impl_.tree;
    const TreeNode* node = &tree.node(id);
    while (node->kind == NodeKind::Chance) {
      id = tree.children(id)[0];
      node = &tree.node(id);
    }
    if (node->terminal()) {
      pol = avg = impl_.terminal_value(deal, impl_.terminals[node->index], player_);
      return;
    }
    const int p = node->player;
    const int na = node->num_children;
    const int b = deal.bucket[static_cast<std::size_t>(p)][static_cast<std::size_t>(node->street)];
    float sigma[kMaxActions];
    impl_.average(id, b, sigma);
    const auto kids = tree.children(id);
    if (p != player_) {
      const int a = pick(sigma, na, hash_uniform(scenario, id));
      walk(deal, scenario, kids[static_cast<std::size_t>(a)], use_policy, learn, pol, avg);
      return;
    }
    const std::uint64_t e =
        local_[node->index] + static_cast<std::uint64_t>(b) * static_cast<std::uint64_t>(na);
    double pa[kMaxActions], va[kMaxActions];
    for (int a = 0; a < na; ++a) {
      walk(deal, scenario, kids[static_cast<std::size_t>(a)], use_policy, learn, pa[a], va[a]);
    }
    avg = 0;
    double mixed = 0;
    for (int a = 0; a < na; ++a) {
      avg += sigma[a] * va[a];
      mixed += sigma[a] * pa[a];
    }
    if (learn) {
      add(visits_, e, 1.0);
      for (int a = 0; a < na; ++a) {
        const double adv = pa[a] - mixed;
        add(q_, e + static_cast<std::uint64_t>(a), adv);
        add(q2_, e + static_cast<std::uint64_t>(a), adv * adv);
      }
    }
    const std::uint8_t choice = policy_[static_cast<std::size_t>(e)];
    pol = use_policy && choice != kNoPolicy ? pa[choice] : mixed;
  }

  std::uint64_t update_policy() {
    const BettingTree& tree = impl_.tree;
    const TableLayout& layout = impl_.layout;
    std::uint64_t changed = 0;
    for (std::size_t i = 0; i < tree.num_action_nodes(); ++i) {
      if (local_[i] == kNotMine) continue;
      const int na = layout.actions(static_cast<std::uint32_t>(i));
      for (std::uint32_t b = 0; b < layout.buckets(static_cast<std::uint32_t>(i)); ++b) {
        const auto e = static_cast<std::size_t>(local_[i] + std::uint64_t{b} * static_cast<std::uint64_t>(na));
        const double count = visits_[e];
        if (count <= 0) continue;  // not seen: keep the previous choice
        int best = -1;
        double best_sum = 0;
        for (int a = 0; a < na; ++a) {
          const double sum = q_[e + static_cast<std::size_t>(a)];
          if (sum <= best_sum) continue;
          if (z_ > 0) {
            const double mean = sum / count;
            const double var = std::max(0.0, q2_[e + static_cast<std::size_t>(a)] / count - mean * mean);
            // Significant if the mean exceeds z standard errors; a single
            // visit never is.
            if (count < 2 || mean <= z_ * std::sqrt(var / (count - 1))) continue;
          }
          best = a;
          best_sum = sum;
        }
        const std::uint8_t choice = best < 0 ? kNoPolicy : static_cast<std::uint8_t>(best);
        if (policy_[e] != choice) {
          ++changed;
          policy_[e] = choice;
        }
      }
    }
    return changed;
  }

  const Solver::Impl& impl_;
  int player_;
  ThreadPool& pool_;
  double z_;
  std::vector<std::uint64_t> local_;
  std::uint64_t entries_ = 0;
  std::vector<std::uint8_t> policy_;
  std::vector<float> q_, q2_, visits_;
};

void finish(Evaluation& e, const GameConfig& config) {
  e.total_gain = 0;
  for (const double g : e.br_gain) e.total_gain += g;
  e.exploitability = e.br_gain.empty() ? 0 : e.total_gain / static_cast<double>(e.br_gain.size());
  e.exploitability_pct = 100.0 * e.exploitability / config.starting_pot();
  e.nash_guarantee = config.num_players == 2 && config.rake.rate <= 0;
}

}  // namespace

Evaluation Solver::evaluate(const EvalOptions& options) const {
  const Impl& impl = *impl_;
  const int n = config_.num_players;
  Evaluation result;
  result.accuracy = Accuracy::Estimated;

  // EVs from sampled paths.
  {
    constexpr std::uint64_t kBlock = 1024;
    const std::uint64_t samples = options.ev_samples;
    const std::uint64_t blocks = (samples + kBlock - 1) / kBlock;
    std::vector<std::array<Moments, kMaxPlayers>> parts(static_cast<std::size_t>(blocks));
    pool_->parallel_for(static_cast<std::size_t>(blocks), [&](std::size_t block, unsigned) {
      Deal deal;
      const std::uint64_t begin = block * kBlock;
      const std::uint64_t end = std::min(samples, begin + kBlock);
      for (std::uint64_t k = begin; k < end; ++k) {
        Rng rng(mix_seed(options.seed, k));
        if (!impl.sample_deal(rng, deal)) continue;
        impl.compute_buckets(deal);
        double values[kMaxPlayers] = {};
        impl.walk_average(deal, rng, BettingTree::kRoot, values);
        for (int p = 0; p < n; ++p) parts[block][static_cast<std::size_t>(p)].add(values[p]);
      }
    });
    std::array<Moments, kMaxPlayers> total;
    for (const auto& part : parts) {
      for (int p = 0; p < n; ++p) total[static_cast<std::size_t>(p)].merge(part[static_cast<std::size_t>(p)]);
    }
    for (int p = 0; p < n; ++p) {
      result.ev.push_back(total[static_cast<std::size_t>(p)].mean());
      result.ev_std_error.push_back(total[static_cast<std::size_t>(p)].std_error());
    }
    result.ev_samples = samples;
  }

  // Information sets per player.
  std::vector<std::uint64_t> info_sets(static_cast<std::size_t>(n), 0);
  for (std::size_t i = 0; i < tree_.num_action_nodes(); ++i) {
    info_sets[tree_.node(tree_.action_node(i)).player] += layout_.buckets(static_cast<std::uint32_t>(i));
  }
  result.br_samples_per_info_set = std::numeric_limits<double>::infinity();
  for (int p = 0; p < n; ++p) {
    if (!options.best_response) break;
    const std::uint64_t sets = std::max<std::uint64_t>(1, info_sets[static_cast<std::size_t>(p)]);
    const std::uint64_t samples =
        options.br_samples > 0
            ? options.br_samples
            : std::clamp<std::uint64_t>(64 * sets, 100'000, std::max<std::uint64_t>(options.br_samples_cap, 1));
    result.br_samples = std::max(result.br_samples, samples);
    result.br_samples_per_info_set =
        std::min(result.br_samples_per_info_set, static_cast<double>(samples) / static_cast<double>(sets));
    SampledBestResponse br(impl, p, *pool_, options.br_significance);
    Moments gain;
    const std::uint64_t seed = mix_seed(options.seed, 1000 + static_cast<std::uint64_t>(p));
    if (!br.solve(samples, seed, gain)) result.br_converged = false;
    const Moments holdout = br.score(samples, mix_seed(seed, 0xB0B), true, false);
    result.br_gain.push_back(gain.mean());
    result.br_gain_std_error.push_back(gain.std_error());
    result.br_gain_holdout.push_back(holdout.mean());
  }
  if (!options.best_response) result.br_samples_per_info_set = 0;
  finish(result, config_);
  return result;
}

// --- Exact evaluation -----------------------------------------------------------

namespace {

/// Every deal with its probability.
struct DealSet {
  std::vector<Deal> deals;
  std::vector<double> prob;
};

}  // namespace

std::optional<Evaluation> Solver::evaluate_exact(std::uint64_t max_work, std::string* error) const {
  const Impl& impl = *impl_;
  const int n = config_.num_players;
  const auto fail = [&](std::string message) -> std::optional<Evaluation> {
    if (error) *error = std::move(message);
    return std::nullopt;
  };

  // Upper bound on deals: product of live hands × board completions.
  double bound = 1;
  for (int p = 0; p < n; ++p) bound *= static_cast<double>(hands_[static_cast<std::size_t>(p)].hands.size());
  {
    int deck = 52 - impl.start_size;
    for (int s = static_cast<int>(config_.start_street) + 1; s <= 3 && impl.total_board > 0; ++s) {
      if (s == 1) {
        bound *= static_cast<double>(deck) * (deck - 1) * (deck - 2) / 6.0;
        deck -= 3;
      } else {
        bound *= deck;
        deck -= 1;
      }
    }
  }
  if (bound * static_cast<double>(tree_.size()) > static_cast<double>(max_work)) {
    return fail("too many deals for an exact evaluation");
  }

  DealSet set;
  double total_weight = 0;
  {
    Deal deal;
    std::vector<Deal> completions;
    const auto complete = [&](const Deal& base, std::uint64_t used) {
      completions.clear();
      if (impl.total_board == 0) {
        completions.push_back(base);
        return;
      }
      // Fill positions start_size..4: flop as a set, turn and river singly.
      std::vector<std::pair<Deal, std::uint64_t>> frontier{{base, used}};
      int pos = impl.start_size;
      while (pos < 5) {
        std::vector<std::pair<Deal, std::uint64_t>> next;
        for (const auto& [d, u] : frontier) {
          if (pos == 0) {
            for (int a = 0; a < 52; ++a) {
              if (u >> a & 1) continue;
              for (int b = a + 1; b < 52; ++b) {
                if (u >> b & 1) continue;
                for (int c = b + 1; c < 52; ++c) {
                  if (u >> c & 1) continue;
                  Deal x = d;
                  x.board[0] = static_cast<std::uint8_t>(a);
                  x.board[1] = static_cast<std::uint8_t>(b);
                  x.board[2] = static_cast<std::uint8_t>(c);
                  next.push_back({x, u | (1ull << a) | (1ull << b) | (1ull << c)});
                }
              }
            }
          } else {
            for (int a = 0; a < 52; ++a) {
              if (u >> a & 1) continue;
              Deal x = d;
              x.board[static_cast<std::size_t>(pos)] = static_cast<std::uint8_t>(a);
              next.push_back({x, u | (1ull << a)});
            }
          }
        }
        frontier = std::move(next);
        pos += pos == 0 ? 3 : 1;
      }
      for (auto& [d, u] : frontier) completions.push_back(d);
    };
    for (int i = 0; i < impl.start_size; ++i) deal.board[static_cast<std::size_t>(i)] = config_.board[static_cast<std::size_t>(i)].index();
    // Recursive enumeration of hand tuples.
    std::vector<int> idx(static_cast<std::size_t>(n), 0);
    const auto recurse = [&](auto&& self, int p, std::uint64_t used, double weight) -> void {
      if (p == n) {
        complete(deal, used);
        const double each = weight / static_cast<double>(completions.size());
        for (Deal& d : completions) {
          d.strength_ready = false;
          impl.compute_buckets(d);
          impl.compute_strength(d);
          set.deals.push_back(d);
          set.prob.push_back(each);
        }
        total_weight += weight;
        return;
      }
      const auto& list = hands_[static_cast<std::size_t>(p)].hands;
      for (std::size_t h = 0; h < list.size(); ++h) {
        if (list[h].mask & used) continue;
        deal.hand[static_cast<std::size_t>(p)] = static_cast<int>(h);
        self(self, p + 1, used | list[h].mask, weight * list[h].weight);
      }
    };
    recurse(recurse, 0, impl.start_mask, 1.0);
  }
  if (!(total_weight > 0)) return fail("no possible deal");
  for (double& p : set.prob) p /= total_weight;
  const std::size_t D = set.deals.size();

  Evaluation result;
  result.accuracy = Accuracy::Exact;

  // EVs: full expectation per deal.
  {
    std::vector<double> ev(static_cast<std::size_t>(n), 0.0);
    const auto rec = [&](auto&& self, Deal& d, std::uint32_t id, double w, double* out) -> void {
      const TreeNode* node = &tree_.node(id);
      while (node->kind == NodeKind::Chance) {
        id = tree_.children(id)[0];
        node = &tree_.node(id);
      }
      if (node->terminal()) {
        double tmp[kMaxPlayers] = {};
        impl.terminal_values(d, impl.terminals[node->index], tmp);
        for (int p = 0; p < n; ++p) out[p] += w * tmp[p];
        return;
      }
      float sigma[kMaxActions];
      impl.average(id, d.bucket[node->player][static_cast<std::size_t>(node->street)], sigma);
      const auto kids = tree_.children(id);
      for (int a = 0; a < node->num_children; ++a) {
        if (sigma[a] > 0) self(self, d, kids[static_cast<std::size_t>(a)], w * sigma[a], out);
      }
    };
    for (std::size_t k = 0; k < D; ++k) {
      double out[kMaxPlayers] = {};
      rec(rec, set.deals[k], BettingTree::kRoot, 1.0, out);
      for (int p = 0; p < n; ++p) ev[static_cast<std::size_t>(p)] += set.prob[k] * out[p];
    }
    result.ev = ev;
    result.ev_std_error.assign(static_cast<std::size_t>(n), 0.0);
  }

  // Best responses: vectors over all deals, bottom-up per information set.
  for (int i = 0; i < n; ++i) {
    const auto rec = [&](auto&& self, std::uint32_t id, const std::vector<double>& reach,
                         std::vector<double>& pol, std::vector<double>& avg) -> void {
      const TreeNode* node = &tree_.node(id);
      while (node->kind == NodeKind::Chance) {
        id = tree_.children(id)[0];
        node = &tree_.node(id);
      }
      pol.assign(D, 0.0);
      avg.assign(D, 0.0);
      if (node->terminal()) {
        for (std::size_t k = 0; k < D; ++k) {
          if (reach[k] == 0) continue;
          pol[k] = avg[k] = impl.terminal_value(set.deals[k], impl.terminals[node->index], i);
        }
        return;
      }
      const int p = node->player;
      const int na = node->num_children;
      const auto kids = tree_.children(id);
      const auto street = static_cast<std::size_t>(node->street);
      std::vector<float> sigma(D * static_cast<std::size_t>(na));
      for (std::size_t k = 0; k < D; ++k) {
        impl.average(id, set.deals[k].bucket[static_cast<std::size_t>(p)][street], &sigma[k * static_cast<std::size_t>(na)]);
      }
      std::vector<double> cp, ca;
      if (p != i) {
        std::vector<double> child_reach(D);
        for (int a = 0; a < na; ++a) {
          for (std::size_t k = 0; k < D; ++k) child_reach[k] = reach[k] * sigma[k * static_cast<std::size_t>(na) + static_cast<std::size_t>(a)];
          self(self, kids[static_cast<std::size_t>(a)], child_reach, cp, ca);
          for (std::size_t k = 0; k < D; ++k) {
            const double s = sigma[k * static_cast<std::size_t>(na) + static_cast<std::size_t>(a)];
            pol[k] += s * cp[k];
            avg[k] += s * ca[k];
          }
        }
        return;
      }
      std::vector<std::vector<double>> child_pol(static_cast<std::size_t>(na));
      const std::uint32_t buckets = layout_.buckets(node->index);
      std::vector<double> q(static_cast<std::size_t>(buckets) * static_cast<std::size_t>(na), 0.0);
      for (int a = 0; a < na; ++a) {
        self(self, kids[static_cast<std::size_t>(a)], reach, child_pol[static_cast<std::size_t>(a)], ca);
        for (std::size_t k = 0; k < D; ++k) {
          avg[k] += sigma[k * static_cast<std::size_t>(na) + static_cast<std::size_t>(a)] * ca[k];
          const int b = set.deals[k].bucket[static_cast<std::size_t>(i)][street];
          q[static_cast<std::size_t>(b) * static_cast<std::size_t>(na) + static_cast<std::size_t>(a)] += reach[k] * child_pol[static_cast<std::size_t>(a)][k];
        }
      }
      for (std::size_t k = 0; k < D; ++k) {
        const int b = set.deals[k].bucket[static_cast<std::size_t>(i)][street];
        const double* row = &q[static_cast<std::size_t>(b) * static_cast<std::size_t>(na)];
        int best = 0;
        for (int a = 1; a < na; ++a) {
          if (row[a] > row[best]) best = a;
        }
        pol[k] = child_pol[static_cast<std::size_t>(best)][k];
      }
    };
    std::vector<double> pol, avg;
    rec(rec, BettingTree::kRoot, set.prob, pol, avg);
    double br = 0, now = 0;
    for (std::size_t k = 0; k < D; ++k) {
      br += set.prob[k] * pol[k];
      now += set.prob[k] * avg[k];
    }
    result.br_gain.push_back(br - now);
    result.br_gain_std_error.push_back(0.0);
    result.br_gain_holdout.push_back(br - now);
  }
  finish(result, config_);
  return result;
}

}  // namespace gto::mc
