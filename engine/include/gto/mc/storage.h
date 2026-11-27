#pragma once

// Storage of regrets and average-strategy sums for the mc engine
// (PLAN.md 3.6 "storage interface").
//
// Entries are addressed by (decision node, bucket, action) through a
// TableLayout: every decision node owns one block of buckets × actions
// entries, buckets of the acting player on that node's street. A Storage
// keeps two numbers per entry, the cumulative regret and the strategy sum.
//
// Concurrency: training threads share one table without locks. The hot-path
// accessors of DenseFloatStorage are relaxed atomic loads and stores
// (std::atomic_ref), so concurrent updates of the same entry can lose an
// increment now and then (the usual "Hogwild" trade-off of parallel MCCFR)
// but never tear a value. With one thread everything is deterministic.

#include <atomic>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace gto {
class ThreadPool;
}

namespace gto::mc {

class BettingTree;
class Abstraction;

/// Where each information set's entries live.
class TableLayout {
 public:
  TableLayout() = default;
  /// One block per decision node of `tree`, buckets from `abstraction`.
  TableLayout(const BettingTree& tree, const Abstraction& abstraction);
  /// Generic form: actions and buckets per decision node (dense index).
  TableLayout(std::vector<std::uint8_t> actions, std::vector<std::uint32_t> buckets);

  std::size_t num_nodes() const noexcept { return actions_.size(); }
  int actions(std::uint32_t node_index) const noexcept { return actions_[node_index]; }
  std::uint32_t buckets(std::uint32_t node_index) const noexcept { return buckets_[node_index]; }
  std::uint64_t offset(std::uint32_t node_index) const noexcept { return offset_[node_index]; }
  /// First entry of the information set (node, bucket).
  std::uint64_t entry(std::uint32_t node_index, std::uint32_t bucket) const noexcept {
    return offset_[node_index] + std::uint64_t{bucket} * actions_[node_index];
  }
  /// Total entries.
  std::uint64_t size() const noexcept { return size_; }
  /// Number of information sets (node, bucket pairs).
  std::uint64_t info_sets() const noexcept { return info_sets_; }
  /// A fingerprint of the shape, to refuse loading data of another game.
  std::uint64_t shape_hash() const noexcept { return hash_; }

 private:
  void finish();
  std::vector<std::uint8_t> actions_;
  std::vector<std::uint32_t> buckets_;
  std::vector<std::uint64_t> offset_;
  std::uint64_t size_ = 0;
  std::uint64_t info_sets_ = 0;
  std::uint64_t hash_ = 0;
};

/// Regrets and strategy sums behind an interface, so that a compressed
/// (e.g. 16-bit) table can replace the float one. The solver's inner loop
/// is written against a concrete type (see DenseFloatStorage) for speed;
/// everything else (save, load, discounting, reports) goes through here.
class Storage {
 public:
  virtual ~Storage() = default;
  virtual std::uint64_t size() const noexcept = 0;
  virtual void read_regrets(std::uint64_t first, int count, float* out) const = 0;
  virtual void read_strategy(std::uint64_t first, int count, float* out) const = 0;
  virtual void add_regrets(std::uint64_t first, int count, const float* delta) = 0;
  virtual void add_strategy(std::uint64_t first, int count, const float* delta) = 0;
  /// Multiplies all regrets and all strategy sums (discounting). Uses the
  /// pool when given.
  virtual void scale(float regret_factor, float strategy_factor, ThreadPool* pool) = 0;
  virtual void clear() = 0;
  /// Raw float form for save/load: regrets then strategy sums.
  virtual void write(std::ostream& out) const = 0;
  virtual bool read(std::istream& in) = 0;
  virtual std::size_t memory_bytes() const noexcept = 0;
};

/// Two float arrays.
class DenseFloatStorage final : public Storage {
 public:
  explicit DenseFloatStorage(std::uint64_t size)
      : regrets_(static_cast<std::size_t>(size), 0.0f),
        strategy_(static_cast<std::size_t>(size), 0.0f) {}

  // Hot path (not virtual).
  float regret(std::uint64_t i) const noexcept {
    return std::atomic_ref<float>(regrets_[static_cast<std::size_t>(i)]).load(std::memory_order_relaxed);
  }
  void add_regret(std::uint64_t i, float delta) noexcept {
    std::atomic_ref<float> r(regrets_[static_cast<std::size_t>(i)]);
    r.store(r.load(std::memory_order_relaxed) + delta, std::memory_order_relaxed);
  }
  float strategy(std::uint64_t i) const noexcept {
    return std::atomic_ref<float>(strategy_[static_cast<std::size_t>(i)]).load(std::memory_order_relaxed);
  }
  void add_strategy(std::uint64_t i, float delta) noexcept {
    std::atomic_ref<float> s(strategy_[static_cast<std::size_t>(i)]);
    s.store(s.load(std::memory_order_relaxed) + delta, std::memory_order_relaxed);
  }

  std::uint64_t size() const noexcept override { return regrets_.size(); }
  void read_regrets(std::uint64_t first, int count, float* out) const override;
  void read_strategy(std::uint64_t first, int count, float* out) const override;
  void add_regrets(std::uint64_t first, int count, const float* delta) override;
  void add_strategy(std::uint64_t first, int count, const float* delta) override;
  void scale(float regret_factor, float strategy_factor, ThreadPool* pool) override;
  void clear() override;
  void write(std::ostream& out) const override;
  bool read(std::istream& in) override;
  std::size_t memory_bytes() const noexcept override {
    return (regrets_.capacity() + strategy_.capacity()) * sizeof(float);
  }

  /// Bytes for a table of `entries` entries.
  static std::uint64_t bytes_for(std::uint64_t entries) noexcept { return entries * 2 * sizeof(float); }

 private:
  mutable std::vector<float> regrets_;
  mutable std::vector<float> strategy_;
};

/// Regret matching: strategy proportional to positive regrets, uniform if
/// none is positive.
void regret_matching(const float* regrets, int count, float* out) noexcept;
/// Average strategy from strategy sums, uniform if they are all zero.
void normalize_strategy(const float* sums, int count, float* out) noexcept;

}  // namespace gto::mc
