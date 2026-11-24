// Turns a Hold'em spot description into a Game.

#include "gto/solver.h"

#include <algorithm>

namespace gto {

namespace {

/// Whether swapping two suits leaves every weight of the range unchanged.
bool invariant_under_swap(const Range& range, int suit_a, int suit_b) {
  const auto swap = [&](Card card) {
    if (card.suit() == suit_a) return Card(card.rank(), suit_b);
    if (card.suit() == suit_b) return Card(card.rank(), suit_a);
    return card;
  };
  for (int combo = 0; combo < Range::kNumCombos; ++combo) {
    const auto [a, b] = Range::combo_cards(combo);
    if (range.weight(combo) != range.weight(swap(a), swap(b))) return false;
  }
  return true;
}


SuitSymmetry range_symmetry(const SpotConfig& config) {
  SuitSymmetry symmetry;
  if (!config.use_isomorphism) return symmetry;
  for (int a = 0; a < 4; ++a) {
    for (int b = a + 1; b < 4; ++b) {
      const bool same = invariant_under_swap(config.ranges[0], a, b) &&
                        invariant_under_swap(config.ranges[1], a, b);
      symmetry.interchangeable[static_cast<std::size_t>(a)][static_cast<std::size_t>(b)] = same;
      symmetry.interchangeable[static_cast<std::size_t>(b)][static_cast<std::size_t>(a)] = same;
    }
  }
  return symmetry;
}

/// Checks everything about a spot except its tree and lists both players'
/// hands. The board must already be known to be valid.
bool make_hands(const SpotConfig& config, Hands& hands, std::string* error) {
  const auto fail = [&](std::string message) {
    if (error) *error = std::move(message);
    return false;
  };
  if (!(config.rake_rate >= 0 && config.rake_rate < 1)) return fail("rake must be between 0 and 1");
  if (!(config.rake_cap >= 0)) return fail("the rake cap cannot be negative");

  std::uint64_t board_mask = 0;
  for (const Card card : config.board) board_mask |= card.mask();

  for (std::size_t player = 0; player < 2; ++player) {
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      const float weight = config.ranges[player].weight(combo);
      if (!(weight > 0)) continue;
      const auto [a, b] = Range::combo_cards(combo);
      if (((a.mask() | b.mask()) & board_mask) != 0) continue;
      hands[player].push_back(make_hand({a.index(), b.index()}, weight));
    }
    if (hands[player].empty()) {
      return fail(std::string(player == kOop ? "the OOP" : "the IP") +
                  " range has no hands that are possible on this board");
    }
  }

  // Both ranges may be non-empty and still never meet, e.g. AsAh against AsAd.
  bool compatible = false;
  for (const Hand& a : hands[0]) {
    compatible = std::any_of(hands[1].begin(), hands[1].end(),
                             [&](const Hand& b) { return (a.mask & b.mask) == 0; });
    if (compatible) break;
  }
  if (!compatible) return fail("the two ranges cannot be held at the same time");
  return true;
}

}  // namespace

std::optional<SpotEstimate> estimate_spot(const SpotConfig& config, std::string* error) {
  // Also validates the board, which make_hands() relies on.
  const auto stats = count_tree(config.tree, config.board, range_symmetry(config), error);
  if (!stats) return std::nullopt;

  Hands hands;
  if (!make_hands(config, hands, error)) return std::nullopt;

  SpotEstimate estimate;
  estimate.tree = *stats;
  for (std::size_t player = 0; player < 2; ++player) {
    estimate.hands[player] = hands[player].size();
    // One regret and one strategy value per action and hand.
    estimate.strategy_bytes += stats->actions[player] * hands[player].size() * 2 * sizeof(float);
  }
  return estimate;
}

std::optional<Game> make_game(const SpotConfig& config, std::string* error) {
  Game game;
  game.rake_rate = config.rake_rate;
  game.rake_cap = config.rake_cap;

  // Also validates the board, which make_hands() relies on.
  auto tree = build_tree(config.tree, config.board, range_symmetry(config), error);
  if (!tree) return std::nullopt;
  game.tree = std::move(*tree);

  if (!make_hands(config, game.hands, error)) return std::nullopt;

  // Hand strengths on every board the hand can end on.
  game.showdowns.reserve(game.tree.runouts.size());
  std::vector<Card> cards;
  for (const auto& runout : game.tree.runouts) {
    cards.assign(config.board.begin(), config.board.end());
    std::uint64_t dealt_mask = 0;
    for (const std::uint8_t id : runout) {
      if (id == kNoCard) continue;
      cards.emplace_back(id);
      dealt_mask |= std::uint64_t{1} << id;
    }
    const std::size_t board_size = cards.size();
    cards.resize(board_size + 2);

    std::array<std::vector<HandStrength>, 2> strength;
    for (std::size_t player = 0; player < 2; ++player) {
      strength[player].reserve(game.hands[player].size());
      for (const Hand& hand : game.hands[player]) {
        if ((hand.mask & dealt_mask) != 0) {
          strength[player].push_back(0);  // impossible on this board, never used
          continue;
        }
        cards[board_size] = Card(hand.cards[0]);
        cards[board_size + 1] = Card(hand.cards[1]);
        strength[player].push_back(evaluate_hand(cards));
      }
    }
    game.showdowns.push_back(make_showdown_table(std::move(strength)));
  }

  return game;
}

}  // namespace gto
