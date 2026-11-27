#pragma once

// Internals shared by the mc solver's source files.

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "gto/mc/solver.h"

namespace gto::mc {

inline std::uint64_t splitmix64(std::uint64_t& x) noexcept {
  std::uint64_t z = (x += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

inline std::uint64_t mix_seed(std::uint64_t a, std::uint64_t b) noexcept {
  std::uint64_t x = a * 0xD1B54A32D192ED03ull + b;
  splitmix64(x);
  return splitmix64(x);
}

/// xoshiro256+.
class Rng {
 public:
  explicit Rng(std::uint64_t seed) noexcept {
    for (auto& v : s_) v = splitmix64(seed);
  }
  std::uint64_t next() noexcept {
    const std::uint64_t result = s_[0] + s_[3];
    const std::uint64_t t = s_[1] << 17;
    s_[2] ^= s_[0];
    s_[3] ^= s_[1];
    s_[1] ^= s_[2];
    s_[0] ^= s_[3];
    s_[2] ^= t;
    s_[3] = (s_[3] << 45) | (s_[3] >> 19);
    return result;
  }
  /// Uniform in [0, 1).
  double uniform() noexcept { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
  std::uint32_t below(std::uint32_t n) noexcept {
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(next() >> 32) * n) >> 32);
  }

 private:
  std::array<std::uint64_t, 4> s_{};
};

/// Walker's alias method over non-negative weights.
class AliasTable {
 public:
  AliasTable() = default;
  explicit AliasTable(std::span<const double> weights);
  bool empty() const noexcept { return prob_.empty(); }
  std::size_t size() const noexcept { return prob_.size(); }
  int sample(Rng& rng) const noexcept {
    const std::uint32_t i = rng.below(static_cast<std::uint32_t>(prob_.size()));
    return rng.uniform() < prob_[i] ? static_cast<int>(i) : alias_[i];
  }

 private:
  std::vector<double> prob_;
  std::vector<int> alias_;
};

struct Deal {
  /// Live hand index per player.
  std::array<int, kMaxPlayers> hand{};
  /// Starting board, then the cards to come in street order.
  std::array<std::uint8_t, 5> board{};
  /// [player][street]
  std::array<std::array<int, kNumStreets>, kMaxPlayers> bucket{};
  std::array<std::uint32_t, kMaxPlayers> strength{};
  bool strength_ready = false;
};

struct TerminalData {
  bool fold = false;
  int winner = 0;
  double win = 0;  // fold: what the winner takes
  std::uint32_t first_layer = 0;
  std::uint32_t num_layers = 0;
  /// Minus what each player put in during the spot.
  std::array<double, kMaxPlayers> net{};
};

struct Layer {
  double amount = 0;
  std::uint16_t eligible = 0;
};

struct Solver::Impl {
  Impl(const Solver& solver);

  const Solver& s;
  const GameConfig& config;
  const BettingTree& tree;
  const TableLayout& layout;
  DenseFloatStorage& store;
  int n = 2;
  int start_size = 0;
  int total_board = 5;  // 0 for the toy deck
  std::vector<TerminalData> terminals;
  std::vector<Layer> layers;
  std::vector<AliasTable> range_tables;
  std::uint64_t start_mask = 0;

  /// Samples every player's hand from `tables` jointly with card removal;
  /// `fixed_player` (if >= 0) keeps `fixed_index`. `used` = cards already out.
  bool sample_hands(Rng& rng, Deal& deal, std::span<const AliasTable> tables,
                    std::uint64_t used, int fixed_player = -1, int fixed_index = 0) const;
  /// Fills deal.board from position `known` on with random cards.
  void sample_board(Rng& rng, Deal& deal, int known, std::uint64_t used) const;
  /// Full training deal: hands from the ranges, then the board.
  bool sample_deal(Rng& rng, Deal& deal) const;
  void compute_buckets(Deal& deal) const;
  void compute_strength(Deal& deal) const;
  std::uint64_t hands_mask(const Deal& deal) const;

  double terminal_value(Deal& deal, const TerminalData& t, int player) const;
  void terminal_values(Deal& deal, const TerminalData& t, double* out) const;

  void average(std::uint32_t node, int bucket, float* out) const;
  void current(std::uint32_t node, int bucket, float* out) const;

  struct TrainCtx {
    Deal deal;
    int traverser = 0;
    bool update_average = true;
    std::array<double, kMaxPlayers> reach{};
  };
  double walk_train(TrainCtx& c, Rng& rng, std::uint32_t id) const;
  /// One path under the average strategies from `id`; adds each player's
  /// value to out[].
  void walk_average(Deal& deal, Rng& rng, std::uint32_t id, double* out) const;
};

}  // namespace gto::mc
