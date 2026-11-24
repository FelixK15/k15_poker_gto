#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "gto/solver.h"
#include "gto/v1/jobs.pb.h"
#include "gto/v1/locks.pb.h"
#include "gto/v1/solutions.pb.h"
#include "gto/v1/trees.pb.h"

namespace gto::server {

struct JobManagerConfig {
  /// Where jobs and their solutions are kept. Empty: memory only, nothing
  /// survives a restart.
  std::filesystem::path data_dir;
  /// Threads for a solve; 0 means one per hardware thread.
  unsigned threads = 0;
  /// Largest solve to accept, in bytes of tree plus strategies.
  std::uint64_t memory_budget_bytes = 0;
  /// Largest tree to accept.
  std::uint64_t max_nodes = 100'000'000;
  /// Finished or paused solutions kept in memory for browsing. Others are
  /// read back from disk when asked for. Ignored without a data directory.
  std::size_t max_loaded = 2;
  /// Iterations between exploitability measurements.
  int check_interval = 10;
};

enum class ErrorKind {
  None,
  InvalidArgument,
  NotFound,
  FailedPrecondition,
  ResourceExhausted,
  Internal,
};

/// Outcome of a JobManager call; the gRPC layer turns it into a status.
struct Error {
  ErrorKind kind = ErrorKind::None;
  std::string message;

  explicit operator bool() const noexcept { return kind != ErrorKind::None; }
};

/// Owns all jobs: queues them, solves one at a time on its own thread, keeps
/// their solutions for browsing and stores both on disk.
///
/// All methods are thread-safe.
class JobManager {
 public:
  explicit JobManager(JobManagerConfig config);
  /// Stops like shutdown().
  ~JobManager();
  JobManager(const JobManager&) = delete;
  JobManager& operator=(const JobManager&) = delete;

  const JobManagerConfig& config() const noexcept { return config_; }

  Error estimate(const v1::Spot& spot, v1::EstimateTreeResponse* out) const;

  Error submit(const v1::SubmitJobRequest& request, v1::Job* out);
  /// Newest first.
  std::vector<v1::Job> list() const;
  Error get(const std::string& id, v1::Job* out) const;
  Error pause(const std::string& id, v1::Job* out);
  Error resume(const std::string& id, v1::Job* out);
  Error cancel(const std::string& id, v1::Job* out);
  Error remove(const std::string& id);

  Error node(const std::string& id, const std::string& path, v1::GetNodeResponse* out);

  /// Queues a re-solve of a job under locks, as a new job that starts from
  /// the parent's solution.
  Error resolve(const v1::ResolveRequest& request, v1::Job* out);

  /// Waits until the job differs from the state identified by `version`
  /// (start with 0 to get it at once), or until the timeout. On a change,
  /// fills `out`, updates `version` and returns true. Sets `gone` when the
  /// job no longer exists or the manager is shutting down.
  bool wait_for_change(const std::string& id, std::uint64_t* version, v1::Job* out, bool* gone,
                       std::chrono::milliseconds timeout) const;

  /// Pauses the running job, saves it and stops the worker. Queued jobs stay
  /// queued on disk as paused. Safe to call more than once.
  void shutdown();

 private:
  /// A solver and the lock that serialises solving and browsing on it.
  struct Solution {
    std::mutex mutex;
    std::unique_ptr<Solver> solver;
  };

  enum class Request { None, Pause, Cancel, Delete };

  struct Entry {
    v1::Job job;
    std::uint64_t version = 1;
    /// Set while the solution is in memory.
    std::shared_ptr<Solution> solution;
    /// What the running job should do at its next iteration.
    Request request = Request::None;
    std::uint64_t last_used = 0;
    /// Without a data directory: the parent's state a re-solve starts from,
    /// held until the job has been loaded.
    std::string initial_state;
  };

  void worker_loop();
  void run_job(const std::string& id);
  /// Builds the job's solver and restores its saved state, if any.
  /// `initial_state`, if not empty, is used instead of the job's state file.
  std::shared_ptr<Solution> load_solution(const v1::Job& job, const std::string& initial_state,
                                          std::string* error) const;
  /// Returns the job's solution, reading it from disk if needed.
  Error acquire_solution(const std::string& id, std::shared_ptr<Solution>* out);

  // The following expect mutex_ to be held.
  Entry* find(const std::string& id);
  const Entry* find(const std::string& id) const;
  void changed(Entry& entry);
  void evict_idle_solutions();
  std::string new_id();

  void load_from_disk();
  void write_job(const v1::Job& job) const;
  bool write_state(const std::string& id, const Solution& solution) const;
  void erase_files(const std::string& id) const;
  std::filesystem::path job_file(const std::string& id) const;
  std::filesystem::path state_file(const std::string& id) const;

  const JobManagerConfig config_;

  mutable std::mutex mutex_;
  std::condition_variable queue_cv_;
  mutable std::condition_variable change_cv_;
  std::map<std::string, Entry> jobs_;
  std::deque<std::string> queue_;
  std::uint64_t use_counter_ = 0;
  /// Time and sequence number of the last id handed out.
  std::int64_t last_id_ms_ = 0;
  unsigned last_id_sequence_ = 0;
  bool stopping_ = false;

  /// Serialises reading solutions back from disk, so that two browsers of the
  /// same job do not both load it.
  std::mutex load_mutex_;

  std::thread worker_;
};

}  // namespace gto::server
