#include "gto/mc/game.h"

#include <cmath>

namespace gto::mc {

const char* street_name(Street street) noexcept {
  switch (street) {
    case Street::Preflop: return "preflop";
    case Street::Flop: return "flop";
    case Street::Turn: return "turn";
    case Street::River: return "river";
  }
  return "?";
}

double GameConfig::starting_pot() const noexcept {
  double pot = dead_pot;
  for (const double c : committed) pot += c;
  return pot;
}

int GameConfig::num_combos() const noexcept {
  return toy_deck_size > 0 ? toy_deck_size : combo_count(variant);
}

int GameConfig::hole_cards() const noexcept {
  return toy_deck_size > 0 ? 1 : hole_card_count(variant);
}

namespace {

bool valid_sizes(const std::vector<double>& sizes) {
  for (const double s : sizes) {
    if (!(s > 0) || !std::isfinite(s)) return false;
  }
  return true;
}

}  // namespace

bool GameConfig::validate(std::string* error) const {
  const auto fail = [&](std::string message) {
    if (error) *error = std::move(message);
    return false;
  };
  if (num_players < 2 || num_players > kMaxPlayers) return fail("the game needs 2 to 9 players");
  if (toy_deck_size < 0) return fail("bad toy deck size");
  if (toy_deck_size > 0) {
    if (toy_deck_size < num_players || toy_deck_size > 52) return fail("the toy deck is too small");
    if (!board.empty() || start_street != Street::River || last_street != Street::River) {
      return fail("toy deck games have no board and start on the river");
    }
  } else {
    if (static_cast<int>(board.size()) != board_size(start_street)) {
      return fail(std::string("a spot starting on the ") + street_name(start_street) + " needs " +
                  std::to_string(board_size(start_street)) + " board cards");
    }
    std::uint64_t seen = 0;
    for (const Card card : board) {
      if (card.index() >= Card::kDeckSize || (seen & card.mask()) != 0) {
        return fail("the board has an invalid or repeated card");
      }
      seen |= card.mask();
    }
  }
  if (last_street < start_street) return fail("the last street comes before the first");
  if (static_cast<int>(stacks.size()) != num_players) return fail("need one stack per player");
  if (!committed.empty() && static_cast<int>(committed.size()) != num_players) {
    return fail("need one committed amount per player (or none)");
  }
  for (int p = 0; p < num_players; ++p) {
    const double stack = stacks[static_cast<std::size_t>(p)];
    const double in = committed_of(p);
    if (!(stack > 0) || !std::isfinite(stack)) return fail("stacks must be positive");
    if (!(in >= 0) || in > stack + 1e-9) return fail("a player committed more than their stack");
  }
  if (!(dead_pot >= 0) || !std::isfinite(dead_pot)) return fail("the dead pot cannot be negative");
  if (!(starting_pot() > 0)) return fail("the starting pot must be positive");
  if (!(big_blind > 0)) return fail("the big blind must be positive");
  if (!ranges.empty() && static_cast<int>(ranges.size()) != num_players) {
    return fail("need one range per player (or none)");
  }
  for (const auto& range : ranges) {
    if (range.empty()) continue;
    if (static_cast<int>(range.size()) != num_combos()) {
      return fail("a range has " + std::to_string(range.size()) + " weights, expected " +
                  std::to_string(num_combos()));
    }
    for (const float w : range) {
      if (!(w >= 0) || !std::isfinite(w)) return fail("range weights must be non-negative");
    }
  }
  for (int s = 1; s < kNumStreets; ++s) {
    const StreetSizing& street = sizing[static_cast<std::size_t>(s)];
    if (!street.per_player.empty() && static_cast<int>(street.per_player.size()) != num_players) {
      return fail("per-player sizing needs one entry per player");
    }
    if (!valid_sizes(street.sizing.bet) || !valid_sizes(street.sizing.raise)) {
      return fail("bet sizes must be positive");
    }
    for (const auto& p : street.per_player) {
      if (p && (!valid_sizes(p->bet) || !valid_sizes(p->raise))) {
        return fail("bet sizes must be positive");
      }
    }
  }
  if (!valid_sizes(preflop.open_bb) || !valid_sizes(preflop.sb_open_bb) ||
      !valid_sizes(preflop.threebet_ip) || !valid_sizes(preflop.threebet_oop) ||
      !valid_sizes(preflop.fourbet) || !(preflop.iso_per_limper_bb >= 0) ||
      !(preflop.per_caller >= 0)) {
    return fail("preflop sizes must be positive");
  }
  if (raise_cap < 0 || preflop.raise_cap < 0) return fail("the raise cap cannot be negative");
  if (!(allin_threshold >= 0 && allin_threshold <= 1)) {
    return fail("the all-in threshold must be between 0 and 1");
  }
  if (!(rake.rate >= 0 && rake.rate < 1)) return fail("rake must be between 0 and 1");
  if (!(rake.cap >= 0)) return fail("the rake cap cannot be negative");
  for (const StreetAbstraction& a : abstraction.streets) {
    if (a.strength_bins < 1 || a.potential_bins < 1 || a.strength_bins * a.potential_bins > 65535) {
      return fail("bucket counts must be between 1 and 65535");
    }
  }
  if (abstraction.omaha_preflop_buckets < 1 || abstraction.omaha_preflop_buckets > 270725) {
    return fail("bad PLO preflop bucket count");
  }
  if (abstraction.omaha_strength_samples < 16 || abstraction.omaha_potential_cards < 1) {
    return fail("bad PLO strength sampling settings");
  }
  if (max_nodes == 0 || max_nodes > 0xFFFFFFF0u) return fail("bad node limit");
  return true;
}

GameConfig GameConfig::preflop_game(int num_players, double stack_bb, double sb_bb,
                                    double ante_bb) {
  GameConfig config;
  config.num_players = num_players;
  config.start_street = Street::Preflop;
  config.last_street = Street::Preflop;
  config.big_blind = 1;
  const auto n = static_cast<std::size_t>(num_players < 2 ? 2 : num_players);
  config.stacks.assign(n, stack_bb - ante_bb);
  config.committed.assign(n, 0.0);
  if (n == 2) {
    config.committed[0] = sb_bb;
    config.committed[1] = 1;
  } else {
    config.committed[n - 2] = sb_bb;
    config.committed[n - 1] = 1;
  }
  config.dead_pot = ante_bb * static_cast<double>(n);
  return config;
}

std::vector<int> action_order(int num_players, Street start, Street street) {
  std::vector<int> order;
  if (start != Street::Preflop || street == Street::Preflop) {
    for (int p = 0; p < num_players; ++p) order.push_back(p);
    return order;
  }
  if (num_players == 2) return {1, 0};
  order.push_back(num_players - 2);
  order.push_back(num_players - 1);
  for (int p = 0; p < num_players - 2; ++p) order.push_back(p);
  return order;
}

std::string seat_name(int num_players, Street start, int player) {
  if (start != Street::Preflop) return "P" + std::to_string(player);
  if (num_players == 2) return player == 0 ? "SB" : "BB";
  if (player == num_players - 1) return "BB";
  if (player == num_players - 2) return "SB";
  const int from_button = num_players - 3 - player;  // 0 = BTN
  static const char* const kBack[] = {"BTN", "CO", "HJ"};
  if (from_button < 3) return kBack[from_button];
  const int front = num_players - 5;  // seats before the HJ
  static const char* const kFront[4][4] = {
      {"UTG"}, {"UTG", "LJ"}, {"UTG", "UTG1", "LJ"}, {"UTG", "UTG1", "UTG2", "LJ"}};
  return kFront[front - 1][player];
}

}  // namespace gto::mc
