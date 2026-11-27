#include "mc_engine.h"

#include <cmath>
#include <set>

#include "convert.h"
#include "gto/omaha.h"
#include "gto/range.h"

namespace gto::server {

namespace {

Error invalid(std::string message) {
  return make_error(ErrorKind::InvalidArgument, std::move(message));
}

Error missing() { return make_error(ErrorKind::FailedPrecondition, kMonteCarloMissing); }

bool positive(double value) { return std::isfinite(value) && value > 0; }

Error check_range(const std::string& field, const std::string& text, v1::Variant variant) {
  std::string error;
  bool empty = false;
  if (variant == v1::VARIANT_OMAHA4) {
    const auto range = OmahaRange::parse(text, &error);
    if (!range) return invalid(field + ": " + error);
    empty = range->total_weight() <= 0;
  } else {
    const auto range = Range::parse(text, &error);
    if (!range) return invalid(field + ": " + error);
    empty = true;
    for (int combo = 0; combo < Range::kNumCombos && empty; ++combo) {
      empty = range->weight(combo) <= 0;
    }
  }
  if (empty) return invalid(field + ": the range is empty");
  return {};
}

Error check_abstraction(const v1::Abstraction& abstraction) {
  const struct {
    const char* name;
    bool set;
    std::uint32_t value;
  } streets[] = {
      {"preflop_buckets", abstraction.has_preflop_buckets(), abstraction.preflop_buckets()},
      {"flop_buckets", abstraction.has_flop_buckets(), abstraction.flop_buckets()},
      {"turn_buckets", abstraction.has_turn_buckets(), abstraction.turn_buckets()},
      {"river_buckets", abstraction.has_river_buckets(), abstraction.river_buckets()},
  };
  for (const auto& street : streets) {
    if (street.set && street.value > 1'000'000) {
      return invalid(std::string("abstraction.") + street.name + ": at most 1,000,000");
    }
  }
  return {};
}

Error check_preflop(const v1::Spot& spot) {
  const v1::PreflopSettings& preflop = spot.preflop();
  if (preflop.players() < 2 || preflop.players() > 9) {
    return invalid("preflop.players: a table has 2 to 9 players");
  }
  if (!positive(preflop.stack_bb())) return invalid("preflop.stack_bb: must be positive");
  const double small_blind = preflop.has_small_blind() ? preflop.small_blind() : 0.5;
  const double big_blind = preflop.has_big_blind() ? preflop.big_blind() : 1.0;
  if (!positive(small_blind) || !positive(big_blind) || small_blind > big_blind) {
    return invalid("preflop: the blinds must be positive, the small blind at most the big one");
  }
  if (!(preflop.ante() >= 0) || !std::isfinite(preflop.ante())) {
    return invalid("preflop.ante: cannot be negative");
  }
  for (const double size : preflop.open_sizes_bb()) {
    if (!(size > big_blind) || !std::isfinite(size)) {
      return invalid("preflop.open_sizes_bb: an open must be larger than the big blind");
    }
  }
  for (const double size : preflop.sb_open_sizes_bb()) {
    if (!(size > big_blind) || !std::isfinite(size)) {
      return invalid("preflop.sb_open_sizes_bb: an open must be larger than the big blind");
    }
  }
  const struct {
    const char* name;
    bool set;
    double value;
  } multipliers[] = {
      {"three_bet_ip_multiplier", preflop.has_three_bet_ip_multiplier(),
       preflop.three_bet_ip_multiplier()},
      {"three_bet_oop_multiplier", preflop.has_three_bet_oop_multiplier(),
       preflop.three_bet_oop_multiplier()},
      {"four_bet_multiplier", preflop.has_four_bet_multiplier(), preflop.four_bet_multiplier()},
  };
  for (const auto& multiplier : multipliers) {
    if (multiplier.set && (!(multiplier.value > 1) || !std::isfinite(multiplier.value))) {
      return invalid(std::string("preflop.") + multiplier.name + ": must be larger than 1");
    }
  }
  if (preflop.has_raise_cap() && (preflop.raise_cap() < 1 || preflop.raise_cap() > 10)) {
    return invalid("preflop.raise_cap: 1 to 10");
  }
  return {};
}

Error check_multiway(const v1::Spot& spot) {
  const int players = spot.players_size();
  if (players < 2 || players > 9) return invalid("players: a spot has 2 to 9 players");

  const auto board = parse_cards(spot.board());
  if (!board) return invalid("board: cannot read the cards");
  if (board->size() < 3 || board->size() > 5) {
    return invalid("board: a postflop spot needs three to five cards");
  }
  std::set<int> distinct;
  for (const Card card : *board) distinct.insert(card.index());
  if (distinct.size() != board->size()) return invalid("board: a card appears twice");

  if (!positive(spot.pot())) return invalid("pot: must be positive");

  std::vector<std::string> names;
  bool named = false;
  for (int i = 0; i < players; ++i) {
    const v1::PlayerSpot& player = spot.players(i);
    const std::string field = "players[" + std::to_string(i) + "]";
    if (Error error = check_range(field + ".range", player.range(), spot.variant())) return error;
    const double stack = player.stack() > 0 ? player.stack() : spot.effective_stack();
    if (!positive(stack) || player.stack() < 0) {
      return invalid(field + ".stack: needs a positive stack (or Spot.effective_stack)");
    }
    names.push_back(player.position());
    named = named || !player.position().empty();
  }
  if (named) {
    std::string error;
    if (!check_positions(names, &error)) return invalid(error);
  }

  const v1::TreeSettings& tree = spot.tree();
  for (const v1::StreetSizing* street : {&tree.flop(), &tree.turn(), &tree.river()}) {
    if (street->players_size() > players) {
      return invalid("tree: sizes for more players than the spot has");
    }
  }
  return {};
}

}  // namespace

Error McEngine::validate(const v1::Spot& spot) {
  if (spot.variant() != v1::VARIANT_HOLDEM && spot.variant() != v1::VARIANT_OMAHA4) {
    return invalid("variant: unknown value " + std::to_string(static_cast<int>(spot.variant())));
  }
  if (spot.rake_pct() < 0 || spot.rake_pct() > 100 || spot.rake_cap() < 0) {
    return invalid("rake: percent 0 to 100, cap not negative");
  }
  if (Error error = check_abstraction(spot.abstraction())) return error;
  if (spot.has_preflop()) return check_preflop(spot);
  return check_multiway(spot);
}

Error McEngine::estimate(const v1::Spot& spot, const EngineLimits&, v1::EstimateTreeResponse*) {
  if (Error error = validate(spot)) return error;
  return missing();
}

std::unique_ptr<McEngine> McEngine::create(const v1::Spot&, const EngineLimits&,
                                           std::string* error) {
  *error = kMonteCarloMissing;
  return nullptr;
}

void McEngine::run_chunk(std::uint64_t) {}

bool McEngine::measurement_due(int) const { return true; }

EngineEvaluation McEngine::evaluate() const {
  EngineEvaluation evaluation;
  evaluation.exploitability_kind = players_ > 2 ? v1::EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY
                                                : v1::EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER;
  evaluation.evs.assign(players_, 0.0);
  evaluation.estimated = true;
  return evaluation;
}

Error McEngine::node(std::string_view, const NodeOptions&, v1::GetNodeResponse*) const {
  return missing();
}

Error McEngine::runouts(std::string_view, v1::GetRunoutReportResponse*) const { return missing(); }

Error McEngine::preflop_scenarios(std::uint32_t, v1::ListPreflopScenariosResponse*) const {
  return missing();
}

Error McEngine::preflop_ranges(std::string_view, v1::GetPreflopRangesResponse*) const {
  return missing();
}

bool McEngine::save(std::ostream&) const { return false; }

bool McEngine::load(std::istream&, std::string* error) {
  *error = kMonteCarloMissing;
  return false;
}

}  // namespace gto::server
