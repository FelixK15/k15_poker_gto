// External-sampling MCCFR: construction, training, persistence, estimates.

#include "gto/mc/solver.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <istream>
#include <ostream>

#include "gto/hand_evaluator.h"
#include "solver_impl.h"
#include "gto/omaha.h"

namespace gto::mc {

// --- AliasTable ---------------------------------------------------------------

AliasTable::AliasTable(std::span<const double> weights) {
  const std::size_t n = weights.size();
  double total = 0;
  for (const double w : weights) total += w;
  if (n == 0 || !(total > 0)) return;
  prob_.assign(n, 0.0);
  alias_.assign(n, 0);
  std::vector<double> scaled(n);
  std::vector<int> small, large;
  for (std::size_t i = 0; i < n; ++i) {
    scaled[i] = weights[i] * static_cast<double>(n) / total;
    (scaled[i] < 1.0 ? small : large).push_back(static_cast<int>(i));
  }
  while (!small.empty() && !large.empty()) {
    const int l = small.back();
    small.pop_back();
    const int g = large.back();
    prob_[static_cast<std::size_t>(l)] = scaled[static_cast<std::size_t>(l)];
    alias_[static_cast<std::size_t>(l)] = g;
    scaled[static_cast<std::size_t>(g)] += scaled[static_cast<std::size_t>(l)] - 1.0;
    if (scaled[static_cast<std::size_t>(g)] < 1.0) {
      large.pop_back();
      small.push_back(g);
    }
  }
  for (const int i : large) prob_[static_cast<std::size_t>(i)] = 1.0;
  for (const int i : small) {
    // Only rounding leftovers; a zero weight must stay unreachable.
    prob_[static_cast<std::size_t>(i)] = weights[static_cast<std::size_t>(i)] > 0 ? 1.0 : 0.0;
    alias_[static_cast<std::size_t>(i)] = large.empty() ? i : large.front();
  }
}

// --- Impl -----------------------------------------------------------------------

Solver::Impl::Impl(const Solver& solver)
    : s(solver), config(solver.config_), tree(solver.tree_), layout(solver.layout_),
      store(*solver.storage_), n(solver.config_.num_players) {
  start_size = static_cast<int>(config.board.size());
  total_board = config.toy_deck_size > 0 ? 0 : 5;
  for (const Card c : config.board) start_mask |= c.mask();

  terminals.resize(tree.num_terminals());
  for (std::size_t i = 0; i < tree.num_terminals(); ++i) {
    const std::uint32_t id = tree.terminal_node(i);
    const TreeNode& node = tree.node(id);
    const auto committed = tree.committed(id);
    TerminalData& t = terminals[i];
    for (int p = 0; p < n; ++p) {
      t.net[static_cast<std::size_t>(p)] =
          -(committed[static_cast<std::size_t>(p)] - tree.initial_committed()[static_cast<std::size_t>(p)]);
    }
    const bool ended_preflop = node.kind == NodeKind::FoldOut && node.street == Street::Preflop;
    std::vector<PotLayer> pot = pot_layers(committed, tree.dead_pot(), node.folded);
    take_rake(pot, config.rake, ended_preflop);
    if (node.kind == NodeKind::FoldOut) {
      t.fold = true;
      for (int p = 0; p < n; ++p) {
        if ((node.folded & (1u << p)) == 0) t.winner = p;
      }
      for (const PotLayer& l : pot) t.win += l.amount;
    } else {
      t.first_layer = static_cast<std::uint32_t>(layers.size());
      t.num_layers = static_cast<std::uint32_t>(pot.size());
      for (const PotLayer& l : pot) layers.push_back({l.amount, l.eligible});
    }
  }

  for (int p = 0; p < n; ++p) {
    std::vector<double> w;
    for (const HoleHand& h : solver.hands_[static_cast<std::size_t>(p)].hands) w.push_back(h.weight);
    range_tables.emplace_back(w);
  }
}

std::uint64_t Solver::Impl::hands_mask(const Deal& deal) const {
  std::uint64_t used = 0;
  for (int p = 0; p < n; ++p) {
    used |= s.hands_[static_cast<std::size_t>(p)].hands[static_cast<std::size_t>(deal.hand[static_cast<std::size_t>(p)])].mask;
  }
  return used;
}

bool Solver::Impl::sample_hands(Rng& rng, Deal& deal, std::span<const AliasTable> tables,
                                std::uint64_t used, int fixed_player, int fixed_index) const {
  constexpr int kMaxTries = 100'000;
  for (int tries = 0; tries < kMaxTries; ++tries) {
    std::uint64_t mask = used;
    bool ok = true;
    if (fixed_player >= 0) {
      const std::uint64_t m = s.hands_[static_cast<std::size_t>(fixed_player)].hands[static_cast<std::size_t>(fixed_index)].mask;
      if (m & mask) return false;
      mask |= m;
      deal.hand[static_cast<std::size_t>(fixed_player)] = fixed_index;
    }
    for (int p = 0; p < n && ok; ++p) {
      if (p == fixed_player) continue;
      const AliasTable& table = tables[static_cast<std::size_t>(p)];
      if (table.empty()) return false;
      const int h = table.sample(rng);
      const std::uint64_t m = s.hands_[static_cast<std::size_t>(p)].hands[static_cast<std::size_t>(h)].mask;
      if (m & mask) {
        ok = false;
        break;
      }
      mask |= m;
      deal.hand[static_cast<std::size_t>(p)] = h;
    }
    if (ok) return true;
  }
  return false;
}

void Solver::Impl::sample_board(Rng& rng, Deal& deal, int known, std::uint64_t used) const {
  for (int i = 0; i < known; ++i) used |= std::uint64_t{1} << deal.board[static_cast<std::size_t>(i)];
  for (int i = known; i < total_board; ++i) {
    std::uint32_t c;
    do {
      c = rng.below(52);
    } while (used & (std::uint64_t{1} << c));
    used |= std::uint64_t{1} << c;
    deal.board[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(c);
  }
}

bool Solver::Impl::sample_deal(Rng& rng, Deal& deal) const {
  if (!sample_hands(rng, deal, range_tables, start_mask)) return false;
  for (int i = 0; i < start_size; ++i) deal.board[static_cast<std::size_t>(i)] = config.board[static_cast<std::size_t>(i)].index();
  sample_board(rng, deal, start_size, start_mask | hands_mask(deal));
  deal.strength_ready = false;
  return true;
}

void Solver::Impl::compute_buckets(Deal& deal) const {
  for (int st = static_cast<int>(config.start_street); st <= static_cast<int>(config.last_street); ++st) {
    const BucketFunction& fn = s.abstraction_.street(static_cast<Street>(st));
    for (int p = 0; p < n; ++p) {
      const int h = deal.hand[static_cast<std::size_t>(p)];
      deal.bucket[static_cast<std::size_t>(p)][static_cast<std::size_t>(st)] =
          fn.bucket(p, s.hands_[static_cast<std::size_t>(p)].hands[static_cast<std::size_t>(h)], h, deal.board.data());
    }
  }
}

void Solver::Impl::compute_strength(Deal& deal) const {
  for (int p = 0; p < n; ++p) {
    const HoleHand& hand = s.hands_[static_cast<std::size_t>(p)].hands[static_cast<std::size_t>(deal.hand[static_cast<std::size_t>(p)])];
    std::uint32_t strength = 0;
    if (config.toy_deck_size > 0) {
      strength = hand.cards[0];
    } else if (config.variant == Variant::Holdem) {
      Card cards[7];
      for (int i = 0; i < 5; ++i) cards[i] = Card(deal.board[static_cast<std::size_t>(i)]);
      cards[5] = Card(hand.cards[0]);
      cards[6] = Card(hand.cards[1]);
      strength = evaluate_hand(std::span<const Card>(cards, 7));
    } else {
      Card board[5];
      for (int i = 0; i < 5; ++i) board[i] = Card(deal.board[static_cast<std::size_t>(i)]);
      const std::array<Card, 4> hole{Card(hand.cards[0]), Card(hand.cards[1]), Card(hand.cards[2]),
                                     Card(hand.cards[3])};
      strength = evaluate_omaha(hole, std::span<const Card>(board, 5));
    }
    deal.strength[static_cast<std::size_t>(p)] = strength;
  }
  deal.strength_ready = true;
}

double Solver::Impl::terminal_value(Deal& deal, const TerminalData& t, int player) const {
  double v = t.net[static_cast<std::size_t>(player)];
  if (t.fold) return player == t.winner ? v + t.win : v;
  if (!deal.strength_ready) compute_strength(deal);
  for (std::uint32_t l = 0; l < t.num_layers; ++l) {
    const Layer& layer = layers[t.first_layer + l];
    if ((layer.eligible & (1u << player)) == 0) continue;
    const std::uint32_t mine = deal.strength[static_cast<std::size_t>(player)];
    int ties = 0;
    bool beaten = false;
    for (int p = 0; p < n; ++p) {
      if ((layer.eligible & (1u << p)) == 0) continue;
      const std::uint32_t other = deal.strength[static_cast<std::size_t>(p)];
      if (other > mine) {
        beaten = true;
        break;
      }
      if (other == mine) ++ties;
    }
    if (!beaten) v += layer.amount / ties;
  }
  return v;
}

void Solver::Impl::terminal_values(Deal& deal, const TerminalData& t, double* out) const {
  for (int p = 0; p < n; ++p) out[p] += t.net[static_cast<std::size_t>(p)];
  if (t.fold) {
    out[t.winner] += t.win;
    return;
  }
  if (!deal.strength_ready) compute_strength(deal);
  for (std::uint32_t l = 0; l < t.num_layers; ++l) {
    const Layer& layer = layers[t.first_layer + l];
    std::uint32_t best = 0;
    int count = 0;
    std::uint16_t winners = 0;
    for (int p = 0; p < n; ++p) {
      if ((layer.eligible & (1u << p)) == 0) continue;
      const std::uint32_t v = deal.strength[static_cast<std::size_t>(p)];
      if (count == 0 || v > best) {
        best = v;
        count = 1;
        winners = static_cast<std::uint16_t>(1u << p);
      } else if (v == best) {
        ++count;
        winners |= static_cast<std::uint16_t>(1u << p);
      }
    }
    for (int p = 0; p < n; ++p) {
      if (winners & (1u << p)) out[p] += layer.amount / count;
    }
  }
}

void Solver::Impl::average(std::uint32_t node, int bucket, float* out) const {
  const TreeNode& nd = tree.node(node);
  const std::uint64_t e = layout.entry(nd.index, static_cast<std::uint32_t>(bucket));
  float sums[kMaxActions];
  for (int a = 0; a < nd.num_children; ++a) sums[a] = store.strategy(e + static_cast<std::uint64_t>(a));
  normalize_strategy(sums, nd.num_children, out);
}

void Solver::Impl::current(std::uint32_t node, int bucket, float* out) const {
  const TreeNode& nd = tree.node(node);
  const std::uint64_t e = layout.entry(nd.index, static_cast<std::uint32_t>(bucket));
  float r[kMaxActions];
  for (int a = 0; a < nd.num_children; ++a) r[a] = store.regret(e + static_cast<std::uint64_t>(a));
  regret_matching(r, nd.num_children, out);
}

namespace {

int sample_action(const float* sigma, int count, double u) {
  double acc = 0;
  for (int a = 0; a < count - 1; ++a) {
    acc += sigma[a];
    if (u < acc) return a;
  }
  // Rounding: the last action with positive probability.
  for (int a = count - 1; a > 0; --a) {
    if (sigma[a] > 0) return a;
  }
  return 0;
}

}  // namespace

double Solver::Impl::walk_train(TrainCtx& c, Rng& rng, std::uint32_t id) const {
  const TreeNode* node = &tree.node(id);
  while (node->kind == NodeKind::Chance) {
    id = tree.children(id)[0];
    node = &tree.node(id);
  }
  if (node->terminal()) return terminal_value(c.deal, terminals[node->index], c.traverser);

  const int p = node->player;
  const int na = node->num_children;
  const int b = c.deal.bucket[static_cast<std::size_t>(p)][static_cast<std::size_t>(node->street)];
  const std::uint64_t e = layout.entry(node->index, static_cast<std::uint32_t>(b));
  float sigma[kMaxActions];
  {
    float r[kMaxActions];
    for (int a = 0; a < na; ++a) r[a] = store.regret(e + static_cast<std::uint64_t>(a));
    regret_matching(r, na, sigma);
  }
  const auto kids = tree.children(id);

  if (p == c.traverser) {
    double v[kMaxActions];
    double ev = 0;
    for (int a = 0; a < na; ++a) {
      v[a] = walk_train(c, rng, kids[static_cast<std::size_t>(a)]);
      ev += sigma[a] * v[a];
    }
    for (int a = 0; a < na; ++a) store.add_regret(e + static_cast<std::uint64_t>(a), static_cast<float>(v[a] - ev));
    return ev;
  }

  if (c.update_average) {
    double third = 1;
    for (int q = 0; q < n; ++q) {
      if (q != p && q != c.traverser) third *= c.reach[static_cast<std::size_t>(q)];
    }
    // third > 0: only actions with positive probability are sampled.
    const float w = static_cast<float>(std::min(1.0 / third, 1e6));
    for (int a = 0; a < na; ++a) store.add_strategy(e + static_cast<std::uint64_t>(a), sigma[a] * w);
  }
  const int a = sample_action(sigma, na, rng.uniform());
  const double saved = c.reach[static_cast<std::size_t>(p)];
  c.reach[static_cast<std::size_t>(p)] = saved * sigma[a];
  const double v = walk_train(c, rng, kids[static_cast<std::size_t>(a)]);
  c.reach[static_cast<std::size_t>(p)] = saved;
  return v;
}

void Solver::Impl::walk_average(Deal& deal, Rng& rng, std::uint32_t id, double* out) const {
  for (;;) {
    const TreeNode& node = tree.node(id);
    if (node.kind == NodeKind::Chance) {
      id = tree.children(id)[0];
      continue;
    }
    if (node.terminal()) {
      terminal_values(deal, terminals[node.index], out);
      return;
    }
    const int p = node.player;
    float sigma[kMaxActions];
    average(id, deal.bucket[static_cast<std::size_t>(p)][static_cast<std::size_t>(node.street)], sigma);
    id = tree.children(id)[static_cast<std::size_t>(sample_action(sigma, node.num_children, rng.uniform()))];
  }
}

// --- Solver -----------------------------------------------------------------------

Solver::~Solver() = default;

std::unique_ptr<Solver> Solver::create(const GameConfig& config, const SolverConfig& solver,
                                       std::string* error) {
  const auto fail = [&](std::string message) -> std::unique_ptr<Solver> {
    if (error) *error = std::move(message);
    return nullptr;
  };
  std::unique_ptr<Solver> s(new Solver);
  s->config_ = config;
  s->solver_config_ = solver;
  auto tree = build_tree(config, error);
  if (!tree) return nullptr;
  s->tree_ = std::move(*tree);
  s->hands_ = live_hands(config);
  for (int p = 0; p < config.num_players; ++p) {
    if (s->hands_[static_cast<std::size_t>(p)].hands.empty()) {
      return fail("player " + std::to_string(p) + " has no hands that are possible on this board");
    }
  }
  auto abstraction = Abstraction::create(config, s->hands_, error);
  if (!abstraction) return nullptr;
  s->abstraction_ = std::move(*abstraction);
  s->layout_ = TableLayout(s->tree_, s->abstraction_);
  s->storage_ = std::make_unique<DenseFloatStorage>(s->layout_.size());
  s->pool_ = std::make_unique<ThreadPool>(solver.threads);
  s->impl_ = std::make_unique<Impl>(*s);
  s->discount_interval_ = solver.discount_interval > 0
                              ? solver.discount_interval
                              : std::clamp<std::uint64_t>(s->layout_.size() / 4, 1000, 1'000'000);
  // The ranges must be able to meet.
  Rng rng(solver.seed);
  Deal deal;
  if (!s->impl_->sample_hands(rng, deal, s->impl_->range_tables, s->impl_->start_mask)) {
    return fail("the ranges cannot be held at the same time (no deal found)");
  }
  return s;
}

void Solver::run_batch(std::uint64_t first, std::uint64_t count) {
  constexpr std::uint64_t kBlock = 16;
  const std::uint64_t blocks = (count + kBlock - 1) / kBlock;
  const Impl& impl = *impl_;
  const int n = config_.num_players;
  pool_->parallel_for(static_cast<std::size_t>(blocks), [&](std::size_t block, unsigned) {
    Impl::TrainCtx ctx;
    const std::uint64_t begin = first + block * kBlock;
    const std::uint64_t end = std::min(first + count, begin + kBlock);
    for (std::uint64_t it = begin; it < end; ++it) {
      Rng rng(mix_seed(solver_config_.seed, it));
      ctx.update_average = it >= solver_config_.averaging_delay;
      for (int t = 0; t < n; ++t) {
        if (!impl.sample_deal(rng, ctx.deal)) continue;
        impl.compute_buckets(ctx.deal);
        ctx.traverser = t;
        ctx.reach.fill(1.0);
        impl.walk_train(ctx, rng, BettingTree::kRoot);
      }
    }
  });
}

void Solver::discount_if_due() {
  if (iterations_ == 0 || iterations_ % discount_interval_ != 0) return;
  if (iterations_ > solver_config_.discount_until) return;
  const double k = static_cast<double>(iterations_ / discount_interval_);
  const auto factor = static_cast<float>(k / (k + 1.0));
  storage_->scale(factor, factor, pool_.get());
}

void Solver::run(std::uint64_t count) {
  while (count > 0) {
    const std::uint64_t to_boundary = discount_interval_ - iterations_ % discount_interval_;
    const std::uint64_t chunk = std::min(count, to_boundary);
    run_batch(iterations_, chunk);
    iterations_ += chunk;
    count -= chunk;
    discount_if_due();
  }
}

SolveResult Solver::solve(const SolveOptions& options) {
  using Clock = std::chrono::steady_clock;
  const auto start = Clock::now();
  auto last_progress = start;
  SolveResult result;
  std::uint64_t chunk = std::uint64_t{64} * pool_->size();
  const auto seconds_since = [](Clock::time_point t) {
    return std::chrono::duration<double>(Clock::now() - t).count();
  };
  for (;;) {
    if (options.cancel && options.cancel->load(std::memory_order_relaxed)) {
      result.stopped = true;
      break;
    }
    if (options.max_iterations > 0 && result.iterations >= options.max_iterations) break;
    const double elapsed = seconds_since(start);
    if (options.max_seconds > 0 && elapsed >= options.max_seconds) break;
    std::uint64_t todo = chunk;
    if (options.max_iterations > 0) todo = std::min(todo, options.max_iterations - result.iterations);
    todo = std::min(todo, discount_interval_ - iterations_ % discount_interval_);
    const auto t0 = Clock::now();
    run_batch(iterations_, todo);
    iterations_ += todo;
    result.iterations += todo;
    discount_if_due();
    // Aim at chunks of about 50 ms.
    const double took = seconds_since(t0);
    if (took < 0.025 && chunk < (std::uint64_t{1} << 30)) chunk *= 2;
    else if (took > 0.1 && chunk > 16) chunk /= 2;
    if (options.on_progress && seconds_since(last_progress) >= options.progress_interval_seconds) {
      last_progress = Clock::now();
      Progress progress;
      progress.iterations = iterations_;
      progress.seconds = seconds_since(start);
      progress.iterations_per_second = static_cast<double>(result.iterations) / progress.seconds;
      if (!options.on_progress(progress)) {
        result.stopped = true;
        break;
      }
    }
  }
  result.seconds = seconds_since(start);
  result.total_iterations = iterations_;
  return result;
}

void Solver::average_strategy(std::uint32_t node, int bucket, float* out) const {
  impl_->average(node, bucket, out);
}
void Solver::current_strategy(std::uint32_t node, int bucket, float* out) const {
  impl_->current(node, bucket, out);
}

int Solver::bucket(Street street, int player, int live_index, std::span<const Card> board) const {
  std::uint8_t cards[5] = {};
  for (std::size_t i = 0; i < board.size() && i < 5; ++i) cards[i] = board[i].index();
  return abstraction_.street(street).bucket(
      player, hands_[static_cast<std::size_t>(player)].hands[static_cast<std::size_t>(live_index)],
      live_index, cards);
}

namespace {
constexpr char kMagic[8] = {'K', '1', '5', 'M', 'C', 'C', 'F', 'R'};
constexpr std::uint32_t kVersion = 1;
}  // namespace

void Solver::save(std::ostream& out) const {
  out.write(kMagic, sizeof(kMagic));
  const std::uint32_t version = kVersion;
  const std::uint64_t hash = layout_.shape_hash();
  const std::uint64_t size = layout_.size();
  const std::uint64_t iterations = iterations_;
  out.write(reinterpret_cast<const char*>(&version), sizeof(version));
  out.write(reinterpret_cast<const char*>(&hash), sizeof(hash));
  out.write(reinterpret_cast<const char*>(&size), sizeof(size));
  out.write(reinterpret_cast<const char*>(&iterations), sizeof(iterations));
  storage_->write(out);
}

bool Solver::load(std::istream& in, std::string* error) {
  const auto fail = [&](const char* message) {
    if (error) *error = message;
    return false;
  };
  char magic[8];
  std::uint32_t version = 0;
  std::uint64_t hash = 0, size = 0, iterations = 0;
  in.read(magic, sizeof(magic));
  in.read(reinterpret_cast<char*>(&version), sizeof(version));
  in.read(reinterpret_cast<char*>(&hash), sizeof(hash));
  in.read(reinterpret_cast<char*>(&size), sizeof(size));
  in.read(reinterpret_cast<char*>(&iterations), sizeof(iterations));
  if (!in || std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) return fail("not a saved mc solver state");
  if (version != kVersion) return fail("unsupported mc solver state version");
  if (hash != layout_.shape_hash() || size != layout_.size()) {
    return fail("the saved state belongs to a differently shaped game");
  }
  if (!storage_->read(in)) return fail("the saved state is truncated");
  iterations_ = iterations;
  return true;
}

std::size_t Solver::memory_bytes() const {
  std::size_t hands = 0;
  for (const PlayerHands& ph : hands_) {
    hands += ph.hands.capacity() * sizeof(HoleHand) + ph.index_of_combo.capacity() * sizeof(std::int32_t);
  }
  return storage_->memory_bytes() + tree_.memory_bytes() + abstraction_.memory_bytes() + hands;
}

std::optional<GameEstimate> estimate_game(const GameConfig& config, std::string* error) {
  auto counts = count_tree(config, error);
  if (!counts) return std::nullopt;
  GameEstimate e;
  e.tree = *counts;
  e.live_hands = live_counts(config);
  e.abstraction = plan_abstraction(config, e.live_hands);
  for (int s = 0; s < kNumStreets; ++s) {
    for (int p = 0; p < config.num_players; ++p) {
      const std::uint64_t b = e.abstraction[static_cast<std::size_t>(s)].buckets[static_cast<std::size_t>(p)];
      e.info_sets += e.tree.decisions[static_cast<std::size_t>(s)][static_cast<std::size_t>(p)] * b;
      e.table_entries += e.tree.actions[static_cast<std::size_t>(s)][static_cast<std::size_t>(p)] * b;
    }
    e.bucket_bytes += e.abstraction[static_cast<std::size_t>(s)].cache_bytes;
  }
  e.storage_bytes = DenseFloatStorage::bytes_for(e.table_entries);
  e.tree_bytes = e.tree.tree_bytes(config.num_players);
  for (const std::size_t h : e.live_hands) e.hands_bytes += h * sizeof(HoleHand);
  e.hands_bytes += static_cast<std::uint64_t>(config.num_players) *
                   static_cast<std::uint64_t>(config.num_combos()) * sizeof(std::int32_t);
  return e;
}

}  // namespace gto::mc
