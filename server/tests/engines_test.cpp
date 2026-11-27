// PLO4, the engine choice, and the multiway / preflop API ahead of the Monte
// Carlo engine.

#include <algorithm>
#include <atomic>
#include <functional>
#include <tuple>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "convert.h"
#include "job_manager.h"
#include "mc_engine.h"
#include "solve_engine.h"
#include "test_spots.h"

namespace gto::server {
namespace {

using testing::multiway_spot;
using testing::plo_spot;
using testing::preflop_spot;
using testing::quick_spot;
using testing::slow_spot;
using testing::wait_until;

namespace fs = std::filesystem;

JobManagerConfig memory_config() {
  JobManagerConfig config;
  config.threads = 2;
  return config;
}

v1::SubmitJobRequest request_for(const v1::Spot& spot, std::uint32_t max_iterations = 0,
                                 double target = -1) {
  v1::SubmitJobRequest request;
  *request.mutable_spot() = spot;
  if (max_iterations > 0) request.mutable_settings()->set_max_iterations(max_iterations);
  if (target >= 0) request.mutable_settings()->set_target_exploitability_pct(target);
  return request;
}

v1::Job job_of(const JobManager& jobs, const std::string& id) {
  v1::Job job;
  EXPECT_FALSE(jobs.get(id, &job)) << id;
  return job;
}

bool wait_for_state(const JobManager& jobs, const std::string& id, v1::JobState state) {
  return wait_until([&] { return job_of(jobs, id).state() == state; });
}

/// Solves a spot to the end and returns the finished job.
v1::Job solve(JobManager& jobs, const v1::SubmitJobRequest& request) {
  v1::Job job;
  const Error error = jobs.submit(request, &job);
  EXPECT_FALSE(error) << error.message;
  EXPECT_TRUE(wait_for_state(jobs, job.id(), v1::JOB_STATE_DONE));
  return job_of(jobs, job.id());
}

class TempDir {
 public:
  TempDir() {
    static std::atomic<int> counter{0};
    path_ = fs::temp_directory_path() /
            ("gto-engines-test-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
             std::to_string(counter++));
    fs::create_directories(path_);
  }
  ~TempDir() {
    std::error_code ec;
    fs::remove_all(path_, ec);
  }
  const fs::path& path() const { return path_; }

 private:
  fs::path path_;
};

// --- Heads-up PLO on the exact engine -----------------------------------------

TEST(Plo, SolvesAndServesFourCardHands) {
  JobManager jobs(memory_config());
  const v1::Job job = solve(jobs, request_for(plo_spot(), 1000, 0.5));
  EXPECT_EQ(job.engine(), v1::ENGINE_EXACT);
  EXPECT_EQ(job.exploitability_kind(), v1::EXPLOITABILITY_KIND_EXACT);
  EXPECT_EQ(job.player_count(), 2u);
  ASSERT_EQ(job.positions_size(), 2);
  EXPECT_EQ(job.positions(0), "OOP");
  EXPECT_EQ(job.positions(1), "IP");
  EXPECT_TRUE(job.measured());
  EXPECT_LE(job.exploitability_pct(), 0.5);
  ASSERT_EQ(job.evs_size(), 2);
  EXPECT_DOUBLE_EQ(job.evs(0), job.oop_ev());
  EXPECT_DOUBLE_EQ(job.evs(1), job.ip_ev());
  EXPECT_NEAR(job.evs(0) + job.evs(1), 100.0, 0.1);

  v1::GetNodeResponse root;
  ASSERT_FALSE(jobs.node(job.id(), "", &root));
  EXPECT_EQ(root.type(), v1::NODE_TYPE_ACTION);
  EXPECT_EQ(root.acting_player(), 0u);
  EXPECT_EQ(root.board(), "Kd 9s 6h 4c 2s");
  EXPECT_FALSE(root.ev_estimated());
  // Pot limit: nothing above the pot; the 150% bet became a pot-size bet.
  bool pot_bet = false;
  for (const v1::NodeAction& action : root.actions()) {
    EXPECT_LE(action.amount(), 100.0 + 1e-9) << action.label();
    pot_bet = pot_bet || action.amount() == 100.0;
  }
  EXPECT_TRUE(pot_bet);

  ASSERT_EQ(root.players_size(), 2);
  for (int p = 0; p < 2; ++p) {
    const v1::PlayerRange& range = root.players(p);
    ASSERT_GT(range.hands_size(), 10);
    // By default at most 2,000 hands a player: IP has more.
    EXPECT_EQ(static_cast<std::size_t>(range.hands_size()),
              std::min<std::size_t>(range.total_hands(), kDefaultOmahaHands));
    if (range.total_hands() <= kDefaultOmahaHands) {
      EXPECT_DOUBLE_EQ(range.shown_reach_fraction(), 1.0);
    }
    EXPECT_EQ(range.reach_size(), range.hands_size());
    for (const std::string& hand : range.hands()) ASSERT_EQ(hand.size(), 8u) << hand;
  }
  EXPECT_EQ(root.players(0).position(), "OOP");
  // Older clients read the same through oop / ip.
  EXPECT_EQ(root.oop().SerializeAsString(), root.players(0).SerializeAsString());
  EXPECT_EQ(root.ip().SerializeAsString(), root.players(1).SerializeAsString());
  EXPECT_EQ(root.strategy_size(), root.actions_size() * root.players(0).hands_size());
  // IP holds more than the default limit, so the default cut applies.
  EXPECT_GT(root.players(1).total_hands(), kDefaultOmahaHands);
  EXPECT_EQ(root.players(1).hands_size(), static_cast<int>(kDefaultOmahaHands));
  ASSERT_EQ(root.committed_size(), 2);
  EXPECT_FALSE(root.folded(0));
  EXPECT_FALSE(root.all_in(0));

  // All-in and called: both all-in at the showdown.
  v1::GetNodeResponse showdown;
  ASSERT_FALSE(jobs.node(job.id(), "bet100/allin/call", &showdown));
  EXPECT_EQ(showdown.type(), v1::NODE_TYPE_SHOWDOWN);
  EXPECT_TRUE(showdown.all_in(0));
  EXPECT_TRUE(showdown.all_in(1));

  v1::GetNodeResponse folded;
  ASSERT_FALSE(jobs.node(job.id(), "bet50/fold", &folded));
  EXPECT_EQ(folded.type(), v1::NODE_TYPE_FOLD);
  EXPECT_TRUE(folded.folded(1));
  EXPECT_FALSE(folded.folded(0));
  EXPECT_EQ(folded.acting_player(), 1u);
}

TEST(Plo, CutsLargeRangesToTheHandsWithTheMostReach) {
  JobManager jobs(memory_config());
  const v1::Job job = solve(jobs, request_for(plo_spot(), 100, 0.5));

  NodeOptions all;
  all.max_hands = kMaxHandsPerPlayer;
  v1::GetNodeResponse full;
  ASSERT_FALSE(jobs.node(job.id(), "check", all, &full));
  ASSERT_EQ(full.acting_player(), 1u);
  const v1::PlayerRange& whole = full.players(1);
  ASSERT_GT(whole.hands_size(), 20);

  NodeOptions few;
  few.max_hands = 20;
  v1::GetNodeResponse cut;
  ASSERT_FALSE(jobs.node(job.id(), "check", few, &cut));
  const v1::PlayerRange& top = cut.players(1);
  ASSERT_EQ(top.hands_size(), 20);
  EXPECT_EQ(top.total_hands(), static_cast<std::uint32_t>(whole.hands_size()));
  EXPECT_GT(top.shown_reach_fraction(), 0.0);
  EXPECT_LT(top.shown_reach_fraction(), 1.0);
  EXPECT_EQ(cut.strategy_size(), cut.actions_size() * 20);
  // Action frequencies still cover the whole range.
  for (int a = 0; a < cut.actions_size(); ++a) {
    EXPECT_DOUBLE_EQ(cut.actions(a).frequency(), full.actions(a).frequency());
  }

  // The listed hands are the ones with the most reach, in range order, with
  // the strategy laid out for them.
  std::vector<float> reaches(whole.reach().begin(), whole.reach().end());
  std::sort(reaches.begin(), reaches.end(), std::greater<>());
  int position_in_full = 0;
  for (int i = 0; i < top.hands_size(); ++i) {
    EXPECT_GE(top.reach(i), reaches[19]);
    while (whole.hands(position_in_full) != top.hands(i)) ++position_in_full;
    for (int a = 0; a < cut.actions_size(); ++a) {
      EXPECT_FLOAT_EQ(cut.strategy(a * 20 + i),
                      full.strategy(a * whole.hands_size() + position_in_full));
    }
  }

  // PLO has no 169 classes.
  NodeOptions classes;
  classes.detail = v1::HAND_DETAIL_CLASSES;
  EXPECT_EQ(jobs.node(job.id(), "check", classes, &cut).kind, ErrorKind::InvalidArgument);
}

TEST(Plo, DefaultHandLimit) {
  EXPECT_EQ(hand_limit(0, v1::VARIANT_OMAHA4), kDefaultOmahaHands);
  EXPECT_EQ(hand_limit(0, v1::VARIANT_HOLDEM), kMaxHandsPerPlayer);
  EXPECT_EQ(hand_limit(5, v1::VARIANT_HOLDEM), 5u);
  EXPECT_EQ(hand_limit(1'000'000, v1::VARIANT_OMAHA4), kMaxHandsPerPlayer);
}

TEST(Plo, EstimatesAndRejectsBadRanges) {
  JobManager jobs(memory_config());
  v1::EstimateTreeResponse estimate;
  ASSERT_FALSE(jobs.estimate(plo_spot(), &estimate));
  EXPECT_EQ(estimate.engine(), v1::ENGINE_EXACT);
  EXPECT_EQ(estimate.exploitability_kind(), v1::EXPLOITABILITY_KIND_EXACT);
  EXPECT_EQ(estimate.player_count(), 2u);
  ASSERT_EQ(estimate.player_hands_size(), 2);
  EXPECT_EQ(estimate.player_hands(0), estimate.oop_hands());
  EXPECT_EQ(estimate.player_hands(1), estimate.ip_hands());
  EXPECT_GT(estimate.oop_hands(), 100u);
  EXPECT_GT(estimate.strategy_entries(), 0u);
  EXPECT_EQ(estimate.info_sets(), 0u);
  EXPECT_TRUE(estimate.can_solve());

  v1::Spot bad = plo_spot();
  bad.set_oop_range("AAxx, banana");
  Error error = jobs.estimate(bad, &estimate);
  EXPECT_EQ(error.kind, ErrorKind::InvalidArgument);
  EXPECT_NE(error.message.find("oop_range"), std::string::npos) << error.message;

  // A Hold'em range is no PLO range.
  bad = plo_spot();
  bad.set_ip_range("AKs, QQ+");
  error = jobs.estimate(bad, &estimate);
  EXPECT_EQ(error.kind, ErrorKind::InvalidArgument);
  EXPECT_NE(error.message.find("ip_range"), std::string::npos) << error.message;

  bad = plo_spot();
  bad.set_variant(static_cast<v1::Variant>(7));
  EXPECT_EQ(jobs.estimate(bad, &estimate).kind, ErrorKind::InvalidArgument);

  v1::Job job;
  EXPECT_EQ(jobs.submit(request_for(bad), &job).kind, ErrorKind::InvalidArgument);
  EXPECT_TRUE(jobs.list().empty());
}

TEST(Plo, ReuseKeysTellVariantAndPotLimitApart) {
  std::string error;
  const auto holdem = to_spot_config(quick_spot(), &error);
  ASSERT_TRUE(holdem) << error;
  SpotConfig as_omaha = *holdem;
  as_omaha.variant = Variant::Omaha4;
  EXPECT_NE(spot_key(*holdem), spot_key(as_omaha));
  SpotConfig pot_limit = *holdem;
  pot_limit.tree.pot_limit = true;
  EXPECT_NE(spot_key(*holdem), spot_key(pot_limit));

  // PLO defaults to pot limit: saying so changes nothing, turning it off
  // makes another game.
  v1::Spot explicit_on = plo_spot();
  explicit_on.set_pot_limit(true);
  v1::Spot no_limit = plo_spot();
  no_limit.set_pot_limit(false);
  const std::string key = reuse_key(plo_spot(), v1::ENGINE_EXACT, &error);
  ASSERT_FALSE(key.empty()) << error;
  EXPECT_EQ(reuse_key(explicit_on, v1::ENGINE_EXACT, &error), key);
  EXPECT_NE(reuse_key(no_limit, v1::ENGINE_EXACT, &error), key);
  // PLO ranges count by weight: another weight is another spot.
  v1::Spot other_weight = plo_spot();
  other_weight.set_ip_range("KQJx:0.4, 9966, 7653, AAKK, 5432ds");
  EXPECT_NE(reuse_key(other_weight, v1::ENGINE_EXACT, &error), key);
  // Combos the board blocks do not count.
  v1::Spot blocked = plo_spot();
  blocked.set_ip_range("KQJx:0.3, 9966, 7653, AAKK, 5432ds, KdKcQhQs");
  EXPECT_EQ(reuse_key(blocked, v1::ENGINE_EXACT, &error), key);

  JobManager jobs(memory_config());
  const v1::Job first = solve(jobs, request_for(plo_spot(), 50, 0.5));
  v1::Job again;
  ASSERT_FALSE(jobs.submit(request_for(explicit_on, 50, 0.5), &again));
  EXPECT_TRUE(again.reused());
  EXPECT_EQ(again.id(), first.id());
  ASSERT_FALSE(jobs.submit(request_for(no_limit, 50, 0.5), &again));
  EXPECT_FALSE(again.reused());
  EXPECT_NE(again.id(), first.id());
}

TEST(Plo, LocksFourCardHands) {
  JobManager jobs(memory_config());
  const v1::Job parent = solve(jobs, request_for(plo_spot(), 100, 0.5));

  // Every hand, so that indices match between the two reads.
  NodeOptions all;
  all.max_hands = kMaxHandsPerPlayer;
  v1::GetNodeResponse node;
  ASSERT_FALSE(jobs.node(parent.id(), "check", all, &node));
  ASSERT_EQ(node.acting_player(), 1u);
  const std::string hand = node.players(1).hands(0);
  ASSERT_EQ(hand.size(), 8u);

  v1::ResolveRequest request;
  request.set_parent_job_id(parent.id());
  v1::NodeLock* lock = request.add_locks();
  lock->set_path("check");
  v1::HandLock* hand_lock = lock->add_hands();
  hand_lock->set_hand(hand);
  for (int a = 0; a < node.actions_size(); ++a) hand_lock->add_frequencies(a == 0 ? 1.0f : 0.0f);
  request.mutable_settings()->set_max_iterations(50);

  v1::Job child;
  ASSERT_FALSE(jobs.resolve(request, &child));
  EXPECT_EQ(child.exploitability_kind(), v1::EXPLOITABILITY_KIND_LOCKED_GAME);
  EXPECT_EQ(child.engine(), v1::ENGINE_EXACT);
  ASSERT_TRUE(wait_for_state(jobs, child.id(), v1::JOB_STATE_DONE));
  EXPECT_EQ(job_of(jobs, child.id()).exploitability_kind(),
            v1::EXPLOITABILITY_KIND_LOCKED_GAME);

  ASSERT_FALSE(jobs.node(child.id(), "check", all, &node));
  ASSERT_EQ(node.players(1).hands(0), hand);
  ASSERT_GT(node.locked_size(), 0);
  EXPECT_TRUE(node.locked(0));
  EXPECT_FALSE(node.locked(1));
  EXPECT_FLOAT_EQ(node.strategy(0), 1.0f);

  // Hold'em names are not PLO hands.
  hand_lock->set_hand("AhKd");
  const Error error = jobs.resolve(request, &child);
  EXPECT_EQ(error.kind, ErrorKind::InvalidArgument);
  EXPECT_NE(error.message.find("AhKd"), std::string::npos) << error.message;
}

// --- Hold'em details that came with the new API ----------------------------------

TEST(Holdem, SummarisesRangesInHandClasses) {
  JobManager jobs(memory_config());
  const v1::Job job = solve(jobs, request_for(quick_spot(), 500, 0));

  NodeOptions options;
  options.detail = v1::HAND_DETAIL_CLASSES;
  v1::GetNodeResponse node;
  ASSERT_FALSE(jobs.node(job.id(), "check", options, &node));
  const v1::PlayerRange& ip = node.players(1);
  EXPECT_EQ(ip.hands_size(), 0);
  EXPECT_EQ(node.strategy_size(), 0);
  ASSERT_EQ(ip.classes_size(), 169);
  EXPECT_EQ(ip.classes(0).name(), "AA");
  EXPECT_EQ(ip.classes(1).name(), "AKs");
  EXPECT_EQ(ip.classes(13).name(), "AKo");
  EXPECT_EQ(ip.classes(14).name(), "KK");
  EXPECT_EQ(ip.classes(168).name(), "22");
  EXPECT_EQ(ip.total_hands(), 15u);

  const auto find = [](const v1::PlayerRange& range, const std::string& name) {
    for (const v1::HandClass& c : range.classes()) {
      if (c.name() == name) return c;
    }
    return v1::HandClass();
  };
  // "KK, QJo" on a board with the Kc: three kings left, all twelve QJo.
  EXPECT_FLOAT_EQ(find(ip, "KK").weight(), 1.0f);
  EXPECT_FLOAT_EQ(find(ip, "KK").combos(), 3.0f);
  EXPECT_FLOAT_EQ(find(ip, "QJo").combos(), 12.0f);
  EXPECT_FLOAT_EQ(find(ip, "AA").combos(), 0.0f);
  // IP acts: classes carry the strategy. The nuts always bet.
  ASSERT_EQ(find(ip, "KK").strategy_size(), node.actions_size());
  EXPECT_NEAR(find(ip, "KK").strategy(1), 1.0, 0.01);
  EXPECT_EQ(find(node.players(0), "88").strategy_size(), 0);

  options.detail = v1::HAND_DETAIL_HANDS_AND_CLASSES;
  ASSERT_FALSE(jobs.node(job.id(), "check", options, &node));
  EXPECT_EQ(node.players(1).hands_size(), 15);
  EXPECT_EQ(node.players(1).classes_size(), 169);
  EXPECT_EQ(node.strategy_size(), 30);
}

TEST(Holdem, TakesTheTwoPlayersAsAList) {
  JobManager jobs(memory_config());
  const v1::Job plain = solve(jobs, request_for(quick_spot(), 300, 0));

  v1::Spot listed = quick_spot();
  listed.clear_oop_range();
  listed.clear_ip_range();
  listed.set_effective_stack(0);
  for (const auto& [position, range, stack] :
       {std::tuple{"BB", "88", 100.0}, std::tuple{"BTN", "KK, QJo", 250.0}}) {
    v1::PlayerSpot* player = listed.add_players();
    player->set_position(position);
    player->set_range(range);
    player->set_stack(stack);
  }
  // Sizes for everyone instead of per player.
  listed.mutable_tree()->mutable_river()->clear_ip();
  listed.mutable_tree()->mutable_river()->mutable_oop();  // set, and empty: no bets
  listed.mutable_tree()->mutable_river()->mutable_all()->add_bet_pct(100);

  // The same game, so the plain job answers it.
  v1::Job job;
  ASSERT_FALSE(jobs.submit(request_for(listed, 300, 0), &job));
  EXPECT_TRUE(job.reused());
  EXPECT_EQ(job.id(), plain.id());

  // Forced to be new: its positions are the listed names.
  v1::SubmitJobRequest request = request_for(listed, 300, 0);
  request.set_force_new(true);
  job = solve(jobs, request);
  ASSERT_EQ(job.positions_size(), 2);
  EXPECT_EQ(job.positions(0), "BB");
  EXPECT_EQ(job.positions(1), "BTN");
  v1::GetNodeResponse node;
  ASSERT_FALSE(jobs.node(job.id(), "", &node));
  EXPECT_EQ(node.players(1).position(), "BTN");
  EXPECT_NEAR(job.evs(0), 70.0, 0.5);

  listed.mutable_players(1)->set_position("BB");
  v1::EstimateTreeResponse estimate;
  const Error error = jobs.estimate(listed, &estimate);
  EXPECT_EQ(error.kind, ErrorKind::InvalidArgument);
  EXPECT_NE(error.message.find("twice"), std::string::npos) << error.message;
}

TEST(Holdem, StopsAtTheTimeLimit) {
  JobManager jobs(memory_config());
  v1::SubmitJobRequest request = request_for(slow_spot(), 1'000'000, 0);
  request.mutable_settings()->set_max_seconds(0.3);
  v1::Job job;
  ASSERT_FALSE(jobs.submit(request, &job));
  EXPECT_DOUBLE_EQ(job.settings().max_seconds(), 0.3);
  ASSERT_TRUE(wait_for_state(jobs, job.id(), v1::JOB_STATE_DONE));
  job = job_of(jobs, job.id());
  EXPECT_GE(job.solve_seconds(), 0.3);
  EXPECT_LT(job.iteration(), 1'000'000u);
  EXPECT_TRUE(job.measured());
}

TEST(Holdem, PreflopCallsNeedAPreflopSolve) {
  JobManager jobs(memory_config());
  const v1::Job job = solve(jobs, request_for(quick_spot(), 10, 0));
  v1::ListPreflopScenariosResponse scenarios;
  EXPECT_EQ(jobs.preflop_scenarios(job.id(), 4, &scenarios).kind,
            ErrorKind::FailedPrecondition);
  v1::GetPreflopRangesResponse ranges;
  EXPECT_EQ(jobs.preflop_ranges(job.id(), "", &ranges).kind, ErrorKind::FailedPrecondition);
}

TEST(Holdem, CompletesJobsSavedByOlderServers) {
  const TempDir dir;
  JobManagerConfig config = memory_config();
  config.data_dir = dir.path();

  std::string id;
  {
    JobManager jobs(config);
    id = solve(jobs, request_for(quick_spot(), 300, 0)).id();
  }
  // Strip what older servers did not write.
  const fs::path file = dir.path() / (id + ".job");
  v1::Job old;
  {
    std::ifstream in(file, std::ios::binary);
    ASSERT_TRUE(old.ParseFromIstream(&in));
  }
  old.clear_engine();
  old.clear_exploitability_kind();
  old.clear_player_count();
  old.clear_positions();
  old.clear_evs();
  {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(old.SerializeToOstream(&out));
  }

  JobManager jobs(config);
  const v1::Job job = job_of(jobs, id);
  EXPECT_EQ(job.engine(), v1::ENGINE_EXACT);
  EXPECT_EQ(job.exploitability_kind(), v1::EXPLOITABILITY_KIND_EXACT);
  EXPECT_EQ(job.player_count(), 2u);
  ASSERT_EQ(job.positions_size(), 2);
  ASSERT_EQ(job.evs_size(), 2);
  EXPECT_DOUBLE_EQ(job.evs(0), job.oop_ev());

  v1::GetNodeResponse node;
  ASSERT_FALSE(jobs.node(id, "check", &node));
  EXPECT_EQ(node.ip().hands_size(), 15);
  // Still found for reuse.
  v1::Job again;
  ASSERT_FALSE(jobs.submit(request_for(quick_spot(), 300, 0), &again));
  EXPECT_TRUE(again.reused());
}

// --- Multiway and preflop until the Monte Carlo engine exists ---------------------

TEST(MonteCarlo, IsNotBuiltYet) { EXPECT_FALSE(kMonteCarloAvailable); }

TEST(MonteCarlo, ValidatesMultiwaySpotsThenRefuses) {
  JobManager jobs(memory_config());
  v1::EstimateTreeResponse estimate;
  Error error = jobs.estimate(multiway_spot(), &estimate);
  EXPECT_EQ(error.kind, ErrorKind::FailedPrecondition);
  EXPECT_NE(error.message.find("Monte Carlo"), std::string::npos) << error.message;

  v1::Job job;
  EXPECT_EQ(jobs.submit(request_for(multiway_spot()), &job).kind, ErrorKind::FailedPrecondition);

  v1::Spot spot = multiway_spot();
  spot.mutable_players(1)->set_range("22+, banana");
  error = jobs.estimate(spot, &estimate);
  EXPECT_EQ(error.kind, ErrorKind::InvalidArgument);
  EXPECT_NE(error.message.find("players[1].range"), std::string::npos) << error.message;

  spot = multiway_spot();
  spot.mutable_players(2)->set_position("SB");
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);

  spot = multiway_spot();
  spot.mutable_players(0)->set_position("S/B");
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);

  spot = multiway_spot();
  spot.set_board("Ks 7d");
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);

  spot = multiway_spot();
  spot.set_effective_stack(0);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);

  spot = multiway_spot();
  for (int i = 0; i < 7; ++i) *spot.add_players() = spot.players(0);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);

  // PLO multiway reads the Omaha syntax.
  spot = multiway_spot();
  spot.set_variant(v1::VARIANT_OMAHA4);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);
  for (int i = 0; i < 3; ++i) spot.mutable_players(i)->set_range("top20%");
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::FailedPrecondition);

  // The exact engine is heads-up only.
  spot = multiway_spot();
  spot.set_engine(v1::ENGINE_EXACT);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);

  // Heads-up on Monte Carlo when asked for.
  spot = quick_spot();
  spot.set_engine(v1::ENGINE_MONTE_CARLO);
  spot.add_players()->set_range("88");
  spot.add_players()->set_range("KK, QJo");
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::FailedPrecondition);

  EXPECT_TRUE(jobs.list().empty());
}

TEST(MonteCarlo, ValidatesPreflopSpotsThenRefuses) {
  JobManager jobs(memory_config());
  v1::EstimateTreeResponse estimate;
  EXPECT_EQ(jobs.estimate(preflop_spot(), &estimate).kind, ErrorKind::FailedPrecondition);
  v1::Job job;
  EXPECT_EQ(jobs.submit(request_for(preflop_spot()), &job).kind, ErrorKind::FailedPrecondition);

  v1::Spot spot = preflop_spot();
  spot.mutable_preflop()->set_players(10);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);
  spot = preflop_spot();
  spot.mutable_preflop()->set_stack_bb(0);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);
  spot = preflop_spot();
  spot.mutable_preflop()->add_open_sizes_bb(0.8);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);
  spot = preflop_spot();
  spot.mutable_preflop()->set_four_bet_multiplier(1.0);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);
  spot = preflop_spot();
  spot.mutable_abstraction()->set_flop_buckets(5'000'000);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);
  spot = preflop_spot();
  spot.set_engine(v1::ENGINE_EXACT);
  EXPECT_EQ(jobs.estimate(spot, &estimate).kind, ErrorKind::InvalidArgument);
  EXPECT_TRUE(jobs.list().empty());
}

TEST(MonteCarlo, NamesPositions) {
  EXPECT_EQ(spot_positions(preflop_spot()),
            (std::vector<std::string>{"SB", "BB", "UTG", "HJ", "CO", "BTN"}));
  EXPECT_EQ(preflop_positions(2), (std::vector<std::string>{"BB", "BTN"}));
  EXPECT_EQ(preflop_positions(9).size(), 9u);
  EXPECT_TRUE(preflop_positions(10).empty());
  EXPECT_EQ(spot_positions(multiway_spot()), (std::vector<std::string>{"SB", "BB", "BTN"}));
  v1::Spot unnamed = multiway_spot();
  for (v1::PlayerSpot& player : *unnamed.mutable_players()) player.clear_position();
  EXPECT_EQ(spot_positions(unnamed), (std::vector<std::string>{"P1", "P2", "P3"}));
  EXPECT_EQ(spot_positions(quick_spot()), (std::vector<std::string>{"OOP", "IP"}));
}

}  // namespace
}  // namespace gto::server
