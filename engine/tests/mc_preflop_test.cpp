// Preflop presets, positional paths, scenarios and the range library.

#include <gtest/gtest.h>

#include <sstream>

#include "gto/mc/preflop.h"

namespace gto::mc {
namespace {

std::vector<std::string> labels(const BettingTree& tree, std::uint32_t node) {
  std::vector<std::string> out;
  for (const Action& a : tree.actions(node)) out.push_back(action_label(a));
  return out;
}

std::uint32_t at(const BettingTree& tree, std::string_view path) {
  std::string error;
  const auto found = resolve_preflop_path(tree, path, &error);
  EXPECT_TRUE(found.has_value()) << path << ": " << error;
  return found ? found->node : 0;
}

int cls(std::string_view name) {
  for (int c = 0; c < 169; ++c) {
    if (holdem_class_name(c) == name) return c;
  }
  ADD_FAILURE() << "no class " << name;
  return 0;
}

GameConfig preset_config(std::string_view format, double stack) {
  const auto preset = preflop_preset(format, stack);
  EXPECT_TRUE(preset.has_value());
  std::string error;
  const auto config = make_preflop_config(preset->settings, &error);
  EXPECT_TRUE(config.has_value()) << error;
  return *config;
}

TEST(McPreflop, PositionNames) {
  EXPECT_EQ(preflop_position_name(2, 0), "BTN");
  EXPECT_EQ(preflop_position_name(2, 1), "BB");
  const std::vector<std::string> six{"UTG", "HJ", "CO", "BTN", "SB", "BB"};
  for (int p = 0; p < 6; ++p) EXPECT_EQ(preflop_position_name(6, p), six[static_cast<std::size_t>(p)]);
  const std::vector<std::string> nine{"UTG", "UTG1", "UTG2", "LJ", "HJ", "CO", "BTN", "SB", "BB"};
  for (int p = 0; p < 9; ++p) EXPECT_EQ(preflop_position_name(9, p), nine[static_cast<std::size_t>(p)]);
  EXPECT_EQ(preflop_seat(9, "utg+1"), 1);
  EXPECT_EQ(preflop_seat(2, "SB"), 0);
  EXPECT_EQ(preflop_seat(6, "LJ"), -1);
}

TEST(McPreflop, PresetTreesHaveExpectedSeatsAndSizes) {
  {
    const GameConfig c = preset_config("6max", 100);
    EXPECT_EQ(c.num_players, 6);
    EXPECT_DOUBLE_EQ(c.committed[4], 0.5);
    EXPECT_DOUBLE_EQ(c.committed[5], 1.0);
    EXPECT_DOUBLE_EQ(c.stacks[0], 100.0);
    const auto tree = build_tree(c);
    ASSERT_TRUE(tree);
    EXPECT_EQ(tree->node(0).player, 0);  // UTG first
    EXPECT_EQ(labels(*tree, 0), (std::vector<std::string>{"fold", "raise2.5"}));
    const std::uint32_t btn = at(*tree, "UTG:fold/HJ:fold/CO:fold");
    EXPECT_EQ(tree->node(btn).player, 3);
    EXPECT_EQ(labels(*tree, btn), (std::vector<std::string>{"fold", "raise2.3"}));
    const std::uint32_t sb = at(*tree, "BTN:fold");
    EXPECT_EQ(tree->node(sb).player, 4);
    EXPECT_EQ(labels(*tree, sb), (std::vector<std::string>{"fold", "raise3"}));
    // BB 3-bets OOP 4x vs the BTN, the BTN 4-bets 2.3x, then all-in.
    EXPECT_EQ(labels(*tree, at(*tree, "BTN:raise2.3")), (std::vector<std::string>{"fold", "call", "raise9.2"}));
    EXPECT_EQ(labels(*tree, at(*tree, "BTN:raise2.3/BB:raise9.2")),
              (std::vector<std::string>{"fold", "call", "raise21.16"}));
    EXPECT_EQ(labels(*tree, at(*tree, "BTN:raise2.3/BB:raise9.2/BTN:raise21.16")),
              (std::vector<std::string>{"fold", "call", "allin"}));
    // HJ cold calls an open; CO cannot cold call the 3-bet.
    EXPECT_EQ(labels(*tree, at(*tree, "UTG:raise2.5")), (std::vector<std::string>{"fold", "call", "raise7.5"}));
    EXPECT_EQ(labels(*tree, at(*tree, "UTG:raise2.5/HJ:raise7.5")), (std::vector<std::string>{"fold", "raise17.25"}));
    const auto e = estimate_game(c);
    ASSERT_TRUE(e);
    EXPECT_GT(e->info_sets, 1'000'000u);
    EXPECT_LT(e->info_sets, 3'000'000u);
  }
  {
    const GameConfig c = preset_config("9max", 100);
    EXPECT_EQ(c.num_players, 9);
    const auto tree = build_tree(c);
    ASSERT_TRUE(tree);
    // One cold caller at most (the BB may always call).
    EXPECT_EQ(labels(*tree, at(*tree, "UTG:raise2.5/UTG1:call")), (std::vector<std::string>{"fold", "raise10"}));
    EXPECT_EQ(labels(*tree, at(*tree, "UTG:raise2.5/UTG1:call/SB:fold")).size(), 3u);
    const auto e = estimate_game(c);
    ASSERT_TRUE(e);
    EXPECT_LT(e->info_sets, 3'000'000u);
  }
  {
    const GameConfig c = preset_config("hu", 100);
    const auto tree = build_tree(c);
    ASSERT_TRUE(tree);
    EXPECT_EQ(labels(*tree, 0), (std::vector<std::string>{"fold", "call", "raise2.5"}));
    EXPECT_EQ(labels(*tree, at(*tree, "BTN:raise2.5")), (std::vector<std::string>{"fold", "call", "raise10"}));
    EXPECT_EQ(labels(*tree, at(*tree, "BTN:call")), (std::vector<std::string>{"check", "raise3.5"}));
  }
  EXPECT_EQ(library_presets().size(), 6u);
  const auto deep = preflop_preset("6-max", 300);
  ASSERT_TRUE(deep);
  EXPECT_EQ(deep->name, "6max_300bb");
  EXPECT_DOUBLE_EQ(*deep->settings.three_bet_oop_multiplier, 5.0);
  EXPECT_FALSE(preflop_preset("7max", 100));
}

TEST(McPreflop, SettingsMapToTheGame) {
  PreflopSettings s;
  s.players = 6;
  s.stack_bb = 50;
  s.ante = 0.125;
  auto c = make_preflop_config(s);
  ASSERT_TRUE(c);
  EXPECT_DOUBLE_EQ(c->dead_pot, 0.75);
  EXPECT_DOUBLE_EQ(c->stacks[0], 49.875);
  s.bb_ante = true;
  s.ante = 1;
  c = make_preflop_config(s);
  ASSERT_TRUE(c);
  EXPECT_DOUBLE_EQ(c->dead_pot, 1.0);
  EXPECT_DOUBLE_EQ(c->stacks[5], 49.0);
  EXPECT_DOUBLE_EQ(c->stacks[4], 50.0);
  EXPECT_FALSE(c->preflop.cold_call_open);
  EXPECT_EQ(c->preflop.open_bb, std::vector<double>{2.5});

  std::string error;
  s.open_sizes_by_position["LJ"] = {2.2};
  EXPECT_FALSE(make_preflop_config(s, &error));
  EXPECT_NE(error.find("LJ"), std::string::npos);
  s.open_sizes_by_position.clear();
  s.raise_cap = 0;
  EXPECT_FALSE(make_preflop_config(s));
  s.raise_cap = 4;
  s.players = 10;
  EXPECT_FALSE(make_preflop_config(s));
}

TEST(McPreflop, PositionalPaths) {
  const GameConfig c = preset_config("6max", 100);
  const auto tree = build_tree(c);
  ASSERT_TRUE(tree);
  std::string error;
  const auto p = resolve_preflop_path(*tree, "BTN:raise2.3/BB:call", &error);
  ASSERT_TRUE(p) << error;
  EXPECT_EQ(p->short_path, "BTN:raise2.3/BB:call");
  EXPECT_EQ(p->full_path, "UTG:fold/HJ:fold/CO:fold/BTN:raise2.3/SB:fold/BB:call");
  EXPECT_EQ(p->engine_path, "fold/fold/fold/raise2.3/fold/call");
  EXPECT_EQ(tree->node(p->node).kind, NodeKind::Showdown);
  // Plain labels and mixed forms reach the same node.
  EXPECT_EQ(at(*tree, "fold/fold/fold/raise2.3/fold/call"), p->node);
  EXPECT_EQ(at(*tree, "btn:RAISE2.3/fold/bb:call"), p->node);
  // A trailing fold stays in the short form.
  const auto q = resolve_preflop_path(*tree, "UTG:raise2.5/HJ:fold");
  ASSERT_TRUE(q);
  EXPECT_EQ(q->short_path, "UTG:raise2.5/HJ:fold");
  EXPECT_EQ(tree->node(q->node).player, 2);
  EXPECT_EQ(preflop_path_of(*tree, p->node).short_path, p->short_path);
  EXPECT_EQ(resolve_preflop_path(*tree, "")->node, 0u);

  EXPECT_FALSE(resolve_preflop_path(*tree, "BTN:raise9", &error));
  EXPECT_NE(error.find("raise2.3"), std::string::npos);  // lists what is available
  EXPECT_FALSE(resolve_preflop_path(*tree, "XX:call", &error));
  EXPECT_FALSE(resolve_preflop_path(*tree, "BTN:raise2.3/CO:call", &error));  // CO already folded
  EXPECT_FALSE(resolve_preflop_path(*tree, "BTN:raise2.3/BB:call/UTG:call", &error));
}

// Heads-up push/fold at 10 BB against the known Nash chart (HoldemResources
// style): the SB pushes about 58% of hands, the BB calls about 37%.
TEST(McPreflop, HeadsUpPushFoldMatchesTheNashChart) {
  PreflopSettings s;
  s.players = 2;
  s.stack_bb = 10;
  s.open_sizes_by_position["BTN"] = {10};  // all-in
  s.raise_cap = 1;
  auto config = make_preflop_config(s);
  ASSERT_TRUE(config);
  auto solver = Solver::create(*config, {.threads = 0, .seed = 3});
  ASSERT_TRUE(solver);
  EXPECT_EQ(labels(solver->tree(), 0), (std::vector<std::string>{"fold", "allin"}));
  solver->run(3'000'000);

  std::string error;
  const auto push = preflop_node_strategy(*solver, "", &error);
  ASSERT_TRUE(push) << error;
  const auto call = preflop_node_strategy(*solver, "BTN:allin", &error);
  ASSERT_TRUE(call) << error;
  const auto freq = [](const PreflopNodeStrategy& st, std::string_view hand) {
    return st.strategy[static_cast<std::size_t>(cls(hand)) * 2 + 1];
  };
  for (const char* hand : {"AA", "KK", "22", "AKo", "A2o", "K5s", "Q8o", "J9s", "T8s", "K9o"}) {
    EXPECT_GT(freq(*push, hand), 0.9) << "push " << hand;
  }
  for (const char* hand : {"72o", "32o", "82o", "93o", "43o"}) {
    EXPECT_LT(freq(*push, hand), 0.1) << "push " << hand;
  }
  for (const char* hand : {"AA", "KK", "77", "A9o", "A5s", "KJo"}) {
    EXPECT_GT(freq(*call, hand), 0.9) << "call " << hand;
  }
  for (const char* hand : {"72o", "83o", "J4o", "T6o", "95s"}) {
    EXPECT_LT(freq(*call, hand), 0.1) << "call " << hand;
  }
  EXPECT_GT(push->action_frequency[1], 0.50);
  EXPECT_LT(push->action_frequency[1], 0.66);
  EXPECT_GT(call->action_frequency[1], 0.30);
  EXPECT_LT(call->action_frequency[1], 0.45);
  // Monotone in pairs (higher pairs push at least as often, up to noise).
  for (int r = 1; r < 13; ++r) {
    EXPECT_GE(push->strategy[static_cast<std::size_t>((r - 1) * 14) * 2 + 1] + 0.02f,
              push->strategy[static_cast<std::size_t>(r * 14) * 2 + 1]);
  }
}

/// A small 3-handed game: 15 BB, open 2.5, 3-bets, no 4-bets.
std::unique_ptr<Solver> small_game(PreflopSettings* out = nullptr) {
  PreflopSettings s;
  s.players = 3;
  s.stack_bb = 15;
  s.allow_cold_calls = true;
  s.raise_cap = 2;
  if (out) *out = s;
  auto config = make_preflop_config(s);
  EXPECT_TRUE(config);
  auto solver = Solver::create(*config, {.threads = 0, .seed = 5});
  EXPECT_TRUE(solver);
  solver->run(300'000);
  return solver;
}

TEST(McPreflop, ScenarioRangesFollowTheStrategies) {
  const auto solver = small_game();
  const auto scenarios = list_preflop_scenarios(*solver, 4);
  ASSERT_FALSE(scenarios.empty());
  std::string error;
  bool saw_srp = false, saw_3bet = false;
  for (const PreflopScenario& s : scenarios) {
    EXPECT_NE(s.seats[0], s.seats[1]);
    EXPECT_GT(s.frequency, 0.0);
    EXPECT_GT(s.pot, 0.0);
    saw_srp |= s.kind == PreflopScenarioKind::SingleRaised;
    saw_3bet |= s.kind == PreflopScenarioKind::ThreeBet;
  }
  EXPECT_TRUE(saw_srp);
  EXPECT_TRUE(saw_3bet);

  // BTN opens, BB calls (SB folds): BTN = raise frequency, BB = call frequency.
  const auto it = std::find_if(scenarios.begin(), scenarios.end(), [](const PreflopScenario& s) {
    return s.path.short_path == "BTN:raise2.5/BB:call";
  });
  ASSERT_NE(it, scenarios.end());
  EXPECT_EQ(it->positions[0], "BB");
  EXPECT_EQ(it->positions[1], "BTN");
  EXPECT_EQ(it->aggressor_position, "BTN");
  EXPECT_DOUBLE_EQ(it->pot, 5.5);  // 2.5 + 2.5 + the dead SB
  EXPECT_DOUBLE_EQ(it->effective_stack, 12.5);
  EXPECT_EQ(it->name, "BTN open 2.5, BB call");

  const auto ranges = preflop_ranges(*solver, it->path.short_path, &error, 0.0f);
  ASSERT_TRUE(ranges) << error;
  ASSERT_EQ(ranges->players.size(), 2u);
  EXPECT_TRUE(ranges->complete);
  const auto open = preflop_node_strategy(*solver, "");
  const auto sb = preflop_node_strategy(*solver, "BTN:raise2.5");
  const auto bb = preflop_node_strategy(*solver, "BTN:raise2.5/SB:fold");
  ASSERT_TRUE(open && sb && bb);
  ASSERT_EQ(bb->actions[1], "call");
  double fraction[3] = {};
  for (int combo = 0; combo < Range::kNumCombos; ++combo) {
    const auto [a, b] = Range::combo_cards(combo);
    const auto c = static_cast<std::size_t>(holdem_class(a, b));
    const float btn_w = open->strategy[c * open->actions.size() + 1];
    const float bb_w = bb->strategy[c * bb->actions.size() + 1];
    EXPECT_NEAR(ranges->players[1].range.weight(combo), btn_w, 1e-5);
    EXPECT_NEAR(ranges->players[0].range.weight(combo), bb_w, 1e-5);
    fraction[0] += btn_w;
    fraction[1] += sb->strategy[c * sb->actions.size()];  // fold
    fraction[2] += bb_w;
  }
  const double expected = fraction[0] / 1326 * fraction[1] / 1326 * fraction[2] / 1326;
  EXPECT_NEAR(it->frequency, expected, 1e-6);
  EXPECT_NEAR(ranges->players[1].class_weight[static_cast<std::size_t>(cls("AA"))],
              open->strategy[static_cast<std::size_t>(cls("AA")) * 2 + 1], 1e-5);

  // A node in the middle: not complete, both still to act are listed.
  const auto mid = preflop_ranges(*solver, "BTN:raise2.5", &error);
  ASSERT_TRUE(mid) << error;
  EXPECT_FALSE(mid->complete);
  EXPECT_EQ(mid->players.size(), 3u);
}

TEST(McPreflop, LibraryRoundTrip) {
  PreflopSettings settings;
  const auto solver = small_game(&settings);
  const PreflopLibrary lib = make_preflop_library(*solver, settings, "3max", 1.25);
  ASSERT_FALSE(lib.entries.empty());
  ASSERT_EQ(lib.open_frequency.size(), 2u);
  EXPECT_EQ(lib.open_frequency[0].first, "BTN");

  std::stringstream text;
  write_preflop_library(text, lib);
  std::string error;
  const auto back = read_preflop_library(text, &error);
  ASSERT_TRUE(back) << error;
  EXPECT_EQ(back->format, "3max");
  EXPECT_EQ(back->players, 3);
  EXPECT_DOUBLE_EQ(back->stack_bb, 15);
  EXPECT_EQ(back->iterations, solver->iterations());
  EXPECT_NEAR(back->exploitability_pct, 1.25, 1e-9);
  EXPECT_EQ(back->settings, lib.settings);
  ASSERT_EQ(back->open_frequency.size(), lib.open_frequency.size());
  EXPECT_NEAR(back->open_frequency[1].second, lib.open_frequency[1].second, 1e-3);
  ASSERT_EQ(back->entries.size(), lib.entries.size());
  for (std::size_t i = 0; i < lib.entries.size(); ++i) {
    const auto& x = lib.entries[i];
    const auto& y = back->entries[i];
    EXPECT_EQ(x.path, y.path);
    EXPECT_EQ(x.name, y.name);
    EXPECT_EQ(x.kind, y.kind);
    EXPECT_EQ(x.oop, y.oop);
    EXPECT_EQ(x.ip, y.ip);
    EXPECT_EQ(x.aggressor, y.aggressor);
    EXPECT_NEAR(x.pot, y.pot, 1e-6);
    EXPECT_NEAR(x.effective_stack, y.effective_stack, 1e-6);
    EXPECT_NEAR(x.frequency, y.frequency, 1e-3 * x.frequency + 1e-9);
    for (int combo = 0; combo < Range::kNumCombos; ++combo) {
      EXPECT_NEAR(x.oop_range.weight(combo), y.oop_range.weight(combo), 1e-4);
      EXPECT_NEAR(x.ip_range.weight(combo), y.ip_range.weight(combo), 1e-4);
    }
  }
  // Every path resolves in the solved game.
  for (const auto& e : back->entries) {
    EXPECT_TRUE(resolve_preflop_path(solver->tree(), e.path, &error)) << e.path << ": " << error;
  }

  std::stringstream bad("hello\n");
  EXPECT_FALSE(read_preflop_library(bad, &error));
  std::stringstream newer("k15-preflop-library 2\n");
  EXPECT_FALSE(read_preflop_library(newer, &error));
  std::stringstream broken("k15-preflop-library 1\nscenario: BTN:raise2.5\noop_range: QQ+,XY\n");
  EXPECT_FALSE(read_preflop_library(broken, &error));
  EXPECT_NE(error.find("line 3"), std::string::npos);
}

}  // namespace
}  // namespace gto::mc
