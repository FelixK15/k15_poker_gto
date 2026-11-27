#include "gto/mc/storage.h"

#include <istream>
#include <ostream>

#include "gto/mc/abstraction.h"
#include "gto/mc/tree.h"
#include "gto/thread_pool.h"

namespace gto::mc {

TableLayout::TableLayout(const BettingTree& tree, const Abstraction& abstraction) {
  const std::size_t n = tree.num_action_nodes();
  actions_.resize(n);
  buckets_.resize(n);
  for (std::size_t i = 0; i < n; ++i) {
    const TreeNode& node = tree.node(tree.action_node(i));
    actions_[i] = node.num_children;
    buckets_[i] = static_cast<std::uint32_t>(abstraction.num_buckets(node.street, node.player));
  }
  finish();
}

TableLayout::TableLayout(std::vector<std::uint8_t> actions, std::vector<std::uint32_t> buckets)
    : actions_(std::move(actions)), buckets_(std::move(buckets)) {
  finish();
}

void TableLayout::finish() {
  offset_.resize(actions_.size());
  size_ = 0;
  info_sets_ = 0;
  hash_ = 0xcbf29ce484222325ull;
  const auto mix = [&](std::uint64_t v) {
    hash_ ^= v;
    hash_ *= 0x100000001b3ull;
  };
  for (std::size_t i = 0; i < actions_.size(); ++i) {
    offset_[i] = size_;
    size_ += std::uint64_t{actions_[i]} * buckets_[i];
    info_sets_ += buckets_[i];
    mix(actions_[i]);
    mix(buckets_[i]);
  }
  mix(actions_.size());
}

void DenseFloatStorage::read_regrets(std::uint64_t first, int count, float* out) const {
  for (int a = 0; a < count; ++a) out[a] = regret(first + static_cast<std::uint64_t>(a));
}
void DenseFloatStorage::read_strategy(std::uint64_t first, int count, float* out) const {
  for (int a = 0; a < count; ++a) out[a] = strategy(first + static_cast<std::uint64_t>(a));
}
void DenseFloatStorage::add_regrets(std::uint64_t first, int count, const float* delta) {
  for (int a = 0; a < count; ++a) add_regret(first + static_cast<std::uint64_t>(a), delta[a]);
}
void DenseFloatStorage::add_strategy(std::uint64_t first, int count, const float* delta) {
  for (int a = 0; a < count; ++a) add_strategy(first + static_cast<std::uint64_t>(a), delta[a]);
}

void DenseFloatStorage::scale(float regret_factor, float strategy_factor, ThreadPool* pool) {
  const std::size_t n = regrets_.size();
  constexpr std::size_t kChunk = 1 << 16;
  const std::size_t chunks = (n + kChunk - 1) / kChunk;
  const auto work = [&](std::size_t c, unsigned) {
    const std::size_t end = std::min(n, (c + 1) * kChunk);
    for (std::size_t i = c * kChunk; i < end; ++i) {
      regrets_[i] *= regret_factor;
      strategy_[i] *= strategy_factor;
    }
  };
  if (pool != nullptr && chunks > 1) {
    pool->parallel_for(chunks, work);
  } else {
    for (std::size_t c = 0; c < chunks; ++c) work(c, 0);
  }
}

void DenseFloatStorage::clear() {
  std::fill(regrets_.begin(), regrets_.end(), 0.0f);
  std::fill(strategy_.begin(), strategy_.end(), 0.0f);
}

void DenseFloatStorage::write(std::ostream& out) const {
  out.write(reinterpret_cast<const char*>(regrets_.data()),
            static_cast<std::streamsize>(regrets_.size() * sizeof(float)));
  out.write(reinterpret_cast<const char*>(strategy_.data()),
            static_cast<std::streamsize>(strategy_.size() * sizeof(float)));
}

bool DenseFloatStorage::read(std::istream& in) {
  std::vector<float> r(regrets_.size()), s(strategy_.size());
  in.read(reinterpret_cast<char*>(r.data()), static_cast<std::streamsize>(r.size() * sizeof(float)));
  in.read(reinterpret_cast<char*>(s.data()), static_cast<std::streamsize>(s.size() * sizeof(float)));
  if (!in) return false;
  regrets_ = std::move(r);
  strategy_ = std::move(s);
  return true;
}

void regret_matching(const float* regrets, int count, float* out) noexcept {
  float total = 0;
  for (int a = 0; a < count; ++a) {
    const float r = regrets[a] > 0 ? regrets[a] : 0.0f;
    out[a] = r;
    total += r;
  }
  if (total > 0) {
    const float inv = 1.0f / total;
    for (int a = 0; a < count; ++a) out[a] *= inv;
  } else {
    const float u = 1.0f / static_cast<float>(count);
    for (int a = 0; a < count; ++a) out[a] = u;
  }
}

void normalize_strategy(const float* sums, int count, float* out) noexcept {
  double total = 0;
  for (int a = 0; a < count; ++a) total += sums[a] > 0 ? sums[a] : 0.0f;
  if (total > 0) {
    for (int a = 0; a < count; ++a) {
      out[a] = static_cast<float>((sums[a] > 0 ? sums[a] : 0.0f) / total);
    }
  } else {
    const float u = 1.0f / static_cast<float>(count);
    for (int a = 0; a < count; ++a) out[a] = u;
  }
}

}  // namespace gto::mc
