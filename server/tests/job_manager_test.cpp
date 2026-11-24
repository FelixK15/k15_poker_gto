#include "job_manager.h"

#include <atomic>
#include <filesystem>
#include <numeric>

#include <gtest/gtest.h>

#include "test_spots.h"

namespace gto::server {
namespace {

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

/// A job that only stops when told to.
v1::SubmitJobRequest endless_request() { return request_for(slow_spot(), 1'000'000, 0); }

v1::Job job_of(const JobManager& jobs, const std::string& id) {
  v1::Job job;
  EXPECT_FALSE(jobs.get(id, &job)) << id;
  return job;
}

bool wait_for_state(const JobManager& jobs, const std::string& id, v1::JobState state) {
  return wait_until([&] { return job_of(jobs, id).state() == state; });
}

/// A directory under the system temp folder that is removed again.
class TempDir {
 public:
  TempDir() {
    static std::atomic<int> counter{0};
    path_ = fs::temp_directory_path() /
            ("gto-test-" + std::to_string(std::chrono::steady_clock::now()
                                              .time_since_epoch()
                                              .count()) +
             "-" + std::to_string(counter++));
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

TEST(JobManager, SolvesASubmittedSpot) {
  JobManager jobs(memory_config());
  v1::Job job;
  ASSERT_FALSE(jobs.submit(request_for(quick_spot(), 2000, 0.01), &job));
  EXPECT_FALSE(job.id().empty());
  EXPECT_GT(job.created_ms(), 0);
  // Defaults are stored resolved.
  EXPECT_EQ(job.settings().max_iterations(), 2000u);
  EXPECT_DOUBLE_EQ(job.settings().target_exploitability_pct(), 0.01);

  ASSERT_TRUE(wait_for_state(jobs, job.id(), v1::JOB_STATE_DONE));
  job = job_of(jobs, job.id());
  EXPECT_GT(job.iteration(), 0u);
  EXPECT_TRUE(job.measured());
  EXPECT_TRUE(job.has_solution());
  EXPECT_LE(job.exploitability_pct(), 0.01);
  EXPECT_NEAR(job.oop_ev(), 70.0, 0.2);
  EXPECT_NEAR(job.ip_ev(), 30.0, 0.2);
  EXPECT_GT(job.finished_ms(), 0);
  EXPECT_TRUE(job.error().empty());
}

TEST(JobManager, FillsInDefaultSettings) {
  JobManager jobs(memory_config());
  v1::Job job;
  ASSERT_FALSE(jobs.submit(request_for(quick_spot()), &job));
  EXPECT_EQ(job.settings().max_iterations(), 1000u);
  EXPECT_DOUBLE_EQ(job.settings().target_exploitability_pct(), 0.3);
}

TEST(JobManager, ServesNodesOfASolution) {
  JobManager jobs(memory_config());
  v1::Job job;
  ASSERT_FALSE(jobs.submit(request_for(quick_spot(), 3000, 0), &job));
  ASSERT_TRUE(wait_for_state(jobs, job.id(), v1::JOB_STATE_DONE));

  v1::GetNodeResponse root;
  ASSERT_FALSE(jobs.node(job.id(), "", &root));
  EXPECT_EQ(root.type(), v1::NODE_TYPE_ACTION);
  EXPECT_EQ(root.player(), v1::PLAYER_OOP);
  ASSERT_EQ(root.actions_size(), 1);
  EXPECT_EQ(root.actions(0).label(), "check");
  EXPECT_DOUBLE_EQ(root.pot(), 100);
  EXPECT_EQ(root.iteration(), 3000u);

  v1::GetNodeResponse node;
  ASSERT_FALSE(jobs.node(job.id(), "check", &node));
  EXPECT_EQ(node.type(), v1::NODE_TYPE_ACTION);
  EXPECT_EQ(node.player(), v1::PLAYER_IP);
  ASSERT_EQ(node.actions_size(), 2);
  EXPECT_EQ(node.actions(0).type(), v1::ACTION_TYPE_CHECK);
  EXPECT_EQ(node.actions(1).type(), v1::ACTION_TYPE_ALLIN);
  EXPECT_EQ(node.actions(1).label(), "allin");
  EXPECT_DOUBLE_EQ(node.actions(1).amount(), 100);
  EXPECT_NEAR(node.actions(0).frequency(), 0.7, 0.01);
  EXPECT_NEAR(node.actions(1).frequency(), 0.3, 0.01);

  ASSERT_EQ(node.ip().hands_size(), 15);
  EXPECT_EQ(node.ip().reach_size(), 15);
  EXPECT_EQ(node.ip().ev_size(), 15);
  EXPECT_EQ(node.strategy_size(), 30);
  EXPECT_EQ(node.oop().hands_size(), 3);
  EXPECT_EQ(node.oop().hands(0).size(), 4u);
  EXPECT_NEAR(node.ip().range_ev(), 30.0, 0.2);
  EXPECT_NEAR(node.oop().range_ev(), 70.0, 0.2);
  // Every hand's frequencies add up to one.
  for (int hand = 0; hand < 15; ++hand) {
    EXPECT_NEAR(node.strategy(hand) + node.strategy(15 + hand), 1.0, 1e-5);
  }

  v1::GetNodeResponse after_bet;
  ASSERT_FALSE(jobs.node(job.id(), "check/allin", &after_bet));
  EXPECT_DOUBLE_EQ(after_bet.pot(), 200);
  EXPECT_DOUBLE_EQ(after_bet.ip_committed(), 100);
  EXPECT_DOUBLE_EQ(after_bet.oop_committed(), 0);

  v1::GetNodeResponse fold;
  ASSERT_FALSE(jobs.node(job.id(), "check/allin/fold", &fold));
  EXPECT_EQ(fold.type(), v1::NODE_TYPE_FOLD);
  EXPECT_EQ(fold.player(), v1::PLAYER_OOP);
  EXPECT_EQ(fold.actions_size(), 0);

  v1::GetNodeResponse unused;
  const Error bad_path = jobs.node(job.id(), "check/bet50", &unused);
  EXPECT_EQ(bad_path.kind, ErrorKind::NotFound);
  EXPECT_NE(bad_path.message.find("allin"), std::string::npos) << bad_path.message;
  EXPECT_EQ(jobs.node("nope", "", &unused).kind, ErrorKind::NotFound);
}

TEST(JobManager, ListsDealableCardsAtChanceNodes) {
  JobManager jobs(memory_config());
  v1::Job job;
  ASSERT_FALSE(jobs.submit(request_for(slow_spot(), 5, 0), &job));
  ASSERT_TRUE(wait_for_state(jobs, job.id(), v1::JOB_STATE_DONE));

  v1::GetNodeResponse chance;
  ASSERT_FALSE(jobs.node(job.id(), "check/check", &chance));
  EXPECT_EQ(chance.type(), v1::NODE_TYPE_CHANCE);
  // Every card not on the four-card board, including the ones the tree merges.
  EXPECT_EQ(chance.next_cards_size(), 48);

  // 7d is merged into 7c on this board; both must be reachable and agree.
  v1::GetNodeResponse clubs, diamonds;
  ASSERT_FALSE(jobs.node(job.id(), "check/check/7c", &clubs));
  ASSERT_FALSE(jobs.node(job.id(), "check/check/7d", &diamonds));
  EXPECT_EQ(clubs.type(), v1::NODE_TYPE_ACTION);
  ASSERT_EQ(clubs.actions_size(), diamonds.actions_size());
  for (int a = 0; a < clubs.actions_size(); ++a) {
    EXPECT_NEAR(clubs.actions(a).frequency(), diamonds.actions(a).frequency(), 1e-5);
  }
  EXPECT_NEAR(clubs.oop().range_ev(), diamonds.oop().range_ev(), 1e-3);

  v1::GetNodeResponse unused;
  EXPECT_EQ(jobs.node(job.id(), "check/check/Ks", &unused).kind, ErrorKind::NotFound);
}

TEST(JobManager, EstimatesAndRejects) {
  JobManagerConfig config = memory_config();
  config.memory_budget_bytes = 64ull << 20;
  JobManager jobs(config);

  v1::EstimateTreeResponse estimate;
  ASSERT_FALSE(jobs.estimate(quick_spot(), &estimate));
  EXPECT_EQ(estimate.nodes(), 6u);
  EXPECT_EQ(estimate.decision_nodes(), 3u);
  EXPECT_EQ(estimate.oop_hands(), 3u);
  EXPECT_EQ(estimate.ip_hands(), 15u);
  // OOP: one action at the root and two facing the bet, three hands each.
  // IP: two actions, fifteen hands. Two floats per action and hand.
  EXPECT_EQ(estimate.strategy_bytes(), (3u * 3 + 2 * 15) * 2 * sizeof(float));
  EXPECT_TRUE(estimate.can_solve());
  EXPECT_FALSE(estimate.too_large());
  EXPECT_EQ(estimate.memory_budget_bytes(), 64ull << 20);

  v1::Spot bad_board = quick_spot();
  bad_board.set_board("Kc 8d");
  EXPECT_EQ(jobs.estimate(bad_board, &estimate).kind, ErrorKind::InvalidArgument);

  v1::Spot bad_range = quick_spot();
  bad_range.set_ip_range("KK, banana");
  const Error range_error = jobs.estimate(bad_range, &estimate);
  EXPECT_EQ(range_error.kind, ErrorKind::InvalidArgument);
  EXPECT_NE(range_error.message.find("ip_range"), std::string::npos) << range_error.message;

  v1::Spot no_pot = quick_spot();
  no_pot.set_pot(0);
  EXPECT_EQ(jobs.estimate(no_pot, &estimate).kind, ErrorKind::InvalidArgument);

  v1::Job job;
  EXPECT_EQ(jobs.submit(request_for(bad_range), &job).kind, ErrorKind::InvalidArgument);
  EXPECT_TRUE(jobs.list().empty());
}

TEST(JobManager, EstimateMatchesTheBuiltGame) {
  JobManager jobs(memory_config());
  v1::EstimateTreeResponse estimate;
  ASSERT_FALSE(jobs.estimate(slow_spot(), &estimate));

  v1::Job job;
  ASSERT_FALSE(jobs.submit(request_for(slow_spot(), 1, 0), &job));
  EXPECT_EQ(job.memory_bytes(), estimate.strategy_bytes() + estimate.tree_bytes());
  ASSERT_TRUE(wait_for_state(jobs, job.id(), v1::JOB_STATE_DONE));
}

TEST(JobManager, RefusesWhatDoesNotFit) {
  JobManagerConfig small_memory = memory_config();
  small_memory.memory_budget_bytes = 1024;
  {
    JobManager jobs(small_memory);
    v1::EstimateTreeResponse estimate;
    ASSERT_FALSE(jobs.estimate(slow_spot(), &estimate));
    EXPECT_FALSE(estimate.can_solve());
    EXPECT_FALSE(estimate.too_large());

    v1::Job job;
    const Error error = jobs.submit(request_for(slow_spot()), &job);
    EXPECT_EQ(error.kind, ErrorKind::ResourceExhausted);
    EXPECT_NE(error.message.find("MB"), std::string::npos) << error.message;
  }

  JobManagerConfig few_nodes = memory_config();
  few_nodes.max_nodes = 100;
  {
    JobManager jobs(few_nodes);
    v1::EstimateTreeResponse estimate;
    ASSERT_FALSE(jobs.estimate(slow_spot(), &estimate));
    EXPECT_TRUE(estimate.too_large());
    EXPECT_FALSE(estimate.can_solve());

    v1::Job job;
    EXPECT_EQ(jobs.submit(request_for(slow_spot()), &job).kind, ErrorKind::ResourceExhausted);
  }
}

TEST(JobManager, RunsOneJobAtATimeAndCanPause) {
  JobManager jobs(memory_config());
  v1::Job first, second;
  ASSERT_FALSE(jobs.submit(endless_request(), &first));
  ASSERT_FALSE(jobs.submit(request_for(quick_spot(), 500, 0), &second));

  ASSERT_TRUE(wait_until([&] { return job_of(jobs, first.id()).iteration() >= 3; }));
  EXPECT_EQ(job_of(jobs, first.id()).state(), v1::JOB_STATE_RUNNING);
  EXPECT_EQ(job_of(jobs, second.id()).state(), v1::JOB_STATE_QUEUED);

  // Newest first.
  const auto listed = jobs.list();
  ASSERT_EQ(listed.size(), 2u);
  EXPECT_EQ(listed[0].id(), second.id());
  EXPECT_EQ(listed[1].id(), first.id());

  // The solution can be browsed while the solve is running.
  v1::GetNodeResponse node;
  ASSERT_FALSE(jobs.node(first.id(), "", &node));
  EXPECT_EQ(node.type(), v1::NODE_TYPE_ACTION);
  // …but a job that has not started has nothing to show.
  EXPECT_EQ(jobs.node(second.id(), "", &node).kind, ErrorKind::FailedPrecondition);

  v1::Job paused;
  ASSERT_FALSE(jobs.pause(first.id(), &paused));
  ASSERT_TRUE(wait_for_state(jobs, first.id(), v1::JOB_STATE_PAUSED));
  const std::uint32_t paused_at = job_of(jobs, first.id()).iteration();
  EXPECT_GE(paused_at, 3u);
  EXPECT_EQ(job_of(jobs, first.id()).finished_ms(), 0);

  // Pausing frees the worker for the next job.
  ASSERT_TRUE(wait_for_state(jobs, second.id(), v1::JOB_STATE_DONE));
  EXPECT_EQ(job_of(jobs, first.id()).iteration(), paused_at);

  // Resuming continues where it stopped.
  v1::Job resumed;
  ASSERT_FALSE(jobs.resume(first.id(), &resumed));
  ASSERT_TRUE(wait_until([&] { return job_of(jobs, first.id()).iteration() > paused_at + 2; }));
  EXPECT_EQ(job_of(jobs, first.id()).state(), v1::JOB_STATE_RUNNING);

  v1::Job cancelled;
  ASSERT_FALSE(jobs.cancel(first.id(), &cancelled));
  ASSERT_TRUE(wait_for_state(jobs, first.id(), v1::JOB_STATE_CANCELLED));
  EXPECT_GT(job_of(jobs, first.id()).finished_ms(), 0);
  // What it computed stays browsable.
  ASSERT_FALSE(jobs.node(first.id(), "", &node));

  // Finished jobs cannot be paused; a done job cannot be resumed.
  v1::Job unused;
  EXPECT_EQ(jobs.pause(second.id(), &unused).kind, ErrorKind::FailedPrecondition);
  EXPECT_EQ(jobs.resume(second.id(), &unused).kind, ErrorKind::FailedPrecondition);
  EXPECT_EQ(jobs.cancel(second.id(), &unused).kind, ErrorKind::FailedPrecondition);
  EXPECT_EQ(jobs.pause("nope", &unused).kind, ErrorKind::NotFound);
}

TEST(JobManager, PausesAndCancelsQueuedJobs) {
  JobManager jobs(memory_config());
  v1::Job running, queued_a, queued_b;
  ASSERT_FALSE(jobs.submit(endless_request(), &running));
  ASSERT_FALSE(jobs.submit(endless_request(), &queued_a));
  ASSERT_FALSE(jobs.submit(endless_request(), &queued_b));
  ASSERT_TRUE(wait_for_state(jobs, running.id(), v1::JOB_STATE_RUNNING));

  v1::Job result;
  ASSERT_FALSE(jobs.pause(queued_a.id(), &result));
  EXPECT_EQ(result.state(), v1::JOB_STATE_PAUSED);
  ASSERT_FALSE(jobs.cancel(queued_b.id(), &result));
  EXPECT_EQ(result.state(), v1::JOB_STATE_CANCELLED);

  // Neither starts when the worker becomes free.
  ASSERT_FALSE(jobs.cancel(running.id(), &result));
  ASSERT_TRUE(wait_for_state(jobs, running.id(), v1::JOB_STATE_CANCELLED));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(job_of(jobs, queued_a.id()).state(), v1::JOB_STATE_PAUSED);
  EXPECT_EQ(job_of(jobs, queued_b.id()).state(), v1::JOB_STATE_CANCELLED);
  EXPECT_EQ(job_of(jobs, queued_a.id()).iteration(), 0u);
}

TEST(JobManager, DeletesJobs) {
  JobManager jobs(memory_config());
  v1::Job running, done;
  ASSERT_FALSE(jobs.submit(request_for(quick_spot(), 50, 0), &done));
  ASSERT_TRUE(wait_for_state(jobs, done.id(), v1::JOB_STATE_DONE));
  ASSERT_FALSE(jobs.submit(endless_request(), &running));
  ASSERT_TRUE(wait_until([&] { return job_of(jobs, running.id()).iteration() >= 1; }));

  EXPECT_FALSE(jobs.remove(done.id()));
  v1::Job unused;
  EXPECT_EQ(jobs.get(done.id(), &unused).kind, ErrorKind::NotFound);

  // A running job goes away after its current iteration.
  EXPECT_FALSE(jobs.remove(running.id()));
  ASSERT_TRUE(wait_until([&] { return jobs.list().empty(); }));
  EXPECT_EQ(jobs.remove(running.id()).kind, ErrorKind::NotFound);
}

TEST(JobManager, ReportsChangesToWatchers) {
  JobManager jobs(memory_config());
  v1::Job job;
  ASSERT_FALSE(jobs.submit(request_for(quick_spot(), 200, 0), &job));

  std::uint64_t version = 0;
  bool gone = false;
  v1::Job seen;
  std::uint32_t last_iteration = 0;
  int updates = 0;
  while (seen.state() != v1::JOB_STATE_DONE) {
    ASSERT_TRUE(jobs.wait_for_change(job.id(), &version, &seen, &gone, std::chrono::seconds(30)));
    ASSERT_FALSE(gone);
    EXPECT_GE(seen.iteration(), last_iteration);
    last_iteration = seen.iteration();
    ++updates;
  }
  EXPECT_GT(updates, 1);
  EXPECT_EQ(seen.iteration(), 200u);

  // Nothing changes any more: the wait times out.
  EXPECT_FALSE(
      jobs.wait_for_change(job.id(), &version, &seen, &gone, std::chrono::milliseconds(50)));

  // A deleted job is reported as gone.
  ASSERT_FALSE(jobs.remove(job.id()));
  EXPECT_TRUE(jobs.wait_for_change(job.id(), &version, &seen, &gone, std::chrono::seconds(1)));
  EXPECT_TRUE(gone);
}

TEST(JobManager, KeepsJobsAndSolutionsAcrossRestarts) {
  const TempDir dir;
  JobManagerConfig config = memory_config();
  config.data_dir = dir.path();

  std::string done_id, paused_id, queued_id;
  v1::GetNodeResponse before;
  std::uint32_t paused_at = 0;
  {
    JobManager jobs(config);
    v1::Job done, paused, queued;
    ASSERT_FALSE(jobs.submit(request_for(quick_spot(), 300, 0), &done));
    ASSERT_TRUE(wait_for_state(jobs, done.id(), v1::JOB_STATE_DONE));
    ASSERT_FALSE(jobs.node(done.id(), "check", &before));

    // One job caught mid-solve and one still waiting when the server stops.
    ASSERT_FALSE(jobs.submit(endless_request(), &paused));
    ASSERT_FALSE(jobs.submit(request_for(quick_spot(), 100, 0), &queued));
    ASSERT_TRUE(wait_until([&] { return job_of(jobs, paused.id()).iteration() >= 5; }));

    done_id = done.id();
    paused_id = paused.id();
    queued_id = queued.id();
    jobs.shutdown();
    paused_at = job_of(jobs, paused_id).iteration();
    EXPECT_EQ(job_of(jobs, paused_id).state(), v1::JOB_STATE_PAUSED);
  }
  EXPECT_TRUE(fs::exists(dir.path() / (done_id + ".job")));
  EXPECT_TRUE(fs::exists(dir.path() / (done_id + ".state")));
  EXPECT_TRUE(fs::exists(dir.path() / (paused_id + ".state")));

  JobManager jobs(config);
  EXPECT_EQ(jobs.list().size(), 3u);

  // The finished solution reads back exactly as it was.
  const v1::Job done = job_of(jobs, done_id);
  EXPECT_EQ(done.state(), v1::JOB_STATE_DONE);
  EXPECT_EQ(done.iteration(), 300u);
  v1::GetNodeResponse after;
  ASSERT_FALSE(jobs.node(done_id, "check", &after));
  ASSERT_EQ(after.strategy_size(), before.strategy_size());
  for (int i = 0; i < after.strategy_size(); ++i) {
    EXPECT_EQ(after.strategy(i), before.strategy(i)) << i;
  }
  EXPECT_EQ(after.ip().range_ev(), before.ip().range_ev());
  EXPECT_EQ(after.iteration(), 300u);

  // Nothing restarts by itself.
  EXPECT_EQ(job_of(jobs, paused_id).state(), v1::JOB_STATE_PAUSED);
  EXPECT_EQ(job_of(jobs, paused_id).iteration(), paused_at);
  EXPECT_EQ(job_of(jobs, queued_id).state(), v1::JOB_STATE_PAUSED);
  EXPECT_FALSE(job_of(jobs, queued_id).has_solution());

  // The interrupted solve continues from its saved iteration.
  v1::Job resumed;
  ASSERT_FALSE(jobs.resume(paused_id, &resumed));
  ASSERT_TRUE(wait_until([&] { return job_of(jobs, paused_id).iteration() > paused_at + 2; }));
  ASSERT_FALSE(jobs.cancel(paused_id, &resumed));
  ASSERT_TRUE(wait_for_state(jobs, paused_id, v1::JOB_STATE_CANCELLED));

  // The one that never ran starts from scratch.
  ASSERT_FALSE(jobs.resume(queued_id, &resumed));
  ASSERT_TRUE(wait_for_state(jobs, queued_id, v1::JOB_STATE_DONE));
  EXPECT_EQ(job_of(jobs, queued_id).iteration(), 100u);

  // Deleting removes the files.
  ASSERT_FALSE(jobs.remove(done_id));
  EXPECT_FALSE(fs::exists(dir.path() / (done_id + ".job")));
  EXPECT_FALSE(fs::exists(dir.path() / (done_id + ".state")));
}

TEST(JobManager, ReloadsEvictedSolutionsFromDisk) {
  const TempDir dir;
  JobManagerConfig config = memory_config();
  config.data_dir = dir.path();
  config.max_loaded = 1;
  JobManager jobs(config);

  std::vector<std::string> ids;
  std::vector<v1::GetNodeResponse> first_read;
  for (int i = 0; i < 3; ++i) {
    v1::Job job;
    ASSERT_FALSE(jobs.submit(request_for(quick_spot(), 100 + 50 * i, 0), &job));
    ASSERT_TRUE(wait_for_state(jobs, job.id(), v1::JOB_STATE_DONE));
    ids.push_back(job.id());
    ASSERT_FALSE(jobs.node(job.id(), "check", &first_read.emplace_back()));
  }
  // Only one solution stays in memory, so these come back from disk.
  for (int round = 0; round < 2; ++round) {
    for (std::size_t i = 0; i < ids.size(); ++i) {
      v1::GetNodeResponse node;
      ASSERT_FALSE(jobs.node(ids[i], "check", &node));
      EXPECT_EQ(node.iteration(), first_read[i].iteration());
      ASSERT_EQ(node.strategy_size(), first_read[i].strategy_size());
      for (int s = 0; s < node.strategy_size(); ++s) {
        EXPECT_EQ(node.strategy(s), first_read[i].strategy(s));
      }
    }
  }
}

TEST(JobManager, FailsAJobWhoseSavedSolutionIsDamaged) {
  const TempDir dir;
  JobManagerConfig config = memory_config();
  config.data_dir = dir.path();

  std::string id;
  {
    JobManager jobs(config);
    v1::Job job;
    ASSERT_FALSE(jobs.submit(endless_request(), &job));
    ASSERT_TRUE(wait_until([&] { return job_of(jobs, job.id()).iteration() >= 2; }));
    id = job.id();
  }
  // Cut the saved state short.
  const fs::path state = dir.path() / (id + ".state");
  ASSERT_TRUE(fs::exists(state));
  fs::resize_file(state, fs::file_size(state) / 2);

  JobManager jobs(config);
  v1::GetNodeResponse node;
  EXPECT_EQ(jobs.node(id, "", &node).kind, ErrorKind::Internal);

  v1::Job resumed;
  ASSERT_FALSE(jobs.resume(id, &resumed));
  ASSERT_TRUE(wait_for_state(jobs, id, v1::JOB_STATE_FAILED));
  EXPECT_NE(job_of(jobs, id).error().find("saved solution"), std::string::npos);
  EXPECT_EQ(jobs.resume(id, &resumed).kind, ErrorKind::FailedPrecondition);
}

}  // namespace
}  // namespace gto::server
