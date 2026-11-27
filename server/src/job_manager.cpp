#include "job_manager.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <system_error>

#include <spdlog/spdlog.h>

#include "convert.h"
#include "zstd_stream.h"

namespace gto::server {

namespace {

namespace fs = std::filesystem;

std::int64_t now_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

Error not_found(const std::string& id) {
  return make_error(ErrorKind::NotFound, "there is no job '" + id + "'");
}

bool is_final(v1::JobState state) {
  return state == v1::JOB_STATE_DONE || state == v1::JOB_STATE_CANCELLED ||
         state == v1::JOB_STATE_FAILED;
}

/// Job ids become file names, so only accept what new_id() produces.
bool valid_id(const std::string& id) {
  return !id.empty() && id.size() <= 32 && std::all_of(id.begin(), id.end(), [](char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
  });
}

/// Writes a file so that a crash never leaves a half-written one behind.
bool write_atomically(const fs::path& path, const std::function<bool(std::ostream&)>& write) {
  fs::path temporary = path;
  temporary += ".tmp";
  {
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out || !write(out)) return false;
    out.flush();
    if (!out) return false;
  }
  std::error_code ec;
  fs::rename(temporary, path, ec);
  if (ec) {
    fs::remove(temporary, ec);
    return false;
  }
  return true;
}

/// Saved solver states are zstd-compressed. States from before compression
/// still load.
bool save_state(const SolveEngine& engine, std::ostream& out) {
  return write_compressed(out, [&](std::ostream& compressed) {
    return engine.save(compressed) && static_cast<bool>(compressed);
  });
}

bool load_state(SolveEngine& engine, std::istream& in, std::string* error) {
  return read_maybe_compressed(in, [&](std::istream& state) { return engine.load(state, error); });
}

/// Fills in what jobs saved before engines and multiway existed lack: they
/// all ran on the exact engine, heads-up.
void complete_old_job(v1::Job& job) {
  if (job.engine() == v1::ENGINE_AUTO) job.set_engine(v1::ENGINE_EXACT);
  if (job.player_count() == 0) {
    const std::vector<std::string> positions = spot_positions(job.spot());
    job.set_player_count(static_cast<std::uint32_t>(positions.size()));
    for (const std::string& position : positions) job.add_positions(position);
  }
  if (job.exploitability_kind() == v1::EXPLOITABILITY_KIND_UNSPECIFIED) {
    job.set_exploitability_kind(job.locks().empty() ? v1::EXPLOITABILITY_KIND_EXACT
                                                    : v1::EXPLOITABILITY_KIND_LOCKED_GAME);
  }
  if (job.measured() && job.evs().empty()) {
    job.add_evs(job.oop_ev());
    job.add_evs(job.ip_ev());
  }
}

/// The iterations a job's settings allow on its engine.
std::uint64_t budget_of(const SolveEngine& engine, const ResolvedSettings& settings) {
  return engine.kind() == v1::ENGINE_MONTE_CARLO
             ? settings.mc_iterations
             : static_cast<std::uint64_t>(settings.max_iterations);
}

}  // namespace

JobManager::JobManager(JobManagerConfig config) : config_(std::move(config)) {
  if (!config_.data_dir.empty()) {
    std::error_code ec;
    fs::create_directories(config_.data_dir, ec);
    if (ec) {
      spdlog::error("cannot create data directory {}: {}", config_.data_dir.string(),
                    ec.message());
    }
    load_from_disk();
  }
  worker_ = std::thread([this] { worker_loop(); });
}

JobManager::~JobManager() { shutdown(); }

void JobManager::shutdown() {
  {
    const std::lock_guard lock(mutex_);
    if (stopping_) return;
    stopping_ = true;
  }
  queue_cv_.notify_all();
  change_cv_.notify_all();
  if (worker_.joinable()) worker_.join();

  // Jobs still waiting would otherwise come back as "queued" with nobody
  // having asked for them after the restart.
  const std::lock_guard lock(mutex_);
  for (const std::string& id : queue_) {
    if (Entry* entry = find(id)) {
      entry->job.set_state(v1::JOB_STATE_PAUSED);
      write_job(entry->job);
    }
  }
  queue_.clear();
}

// --- Files -----------------------------------------------------------------

fs::path JobManager::job_file(const std::string& id) const {
  return config_.data_dir / (id + ".job");
}

fs::path JobManager::state_file(const std::string& id) const {
  return config_.data_dir / (id + ".state");
}

void JobManager::write_job(const v1::Job& job) const {
  if (config_.data_dir.empty()) return;
  const bool ok = write_atomically(
      job_file(job.id()), [&](std::ostream& out) { return job.SerializeToOstream(&out); });
  if (!ok) spdlog::error("cannot write job {} to {}", job.id(), config_.data_dir.string());
}

bool JobManager::write_state(const std::string& id, const Solution& solution) const {
  if (config_.data_dir.empty()) return true;
  const bool ok = write_atomically(state_file(id), [&](std::ostream& out) {
    return save_state(*solution.engine, out);
  });
  if (!ok) spdlog::error("cannot write the solution of job {}", id);
  return ok;
}

void JobManager::erase_files(const std::string& id) const {
  if (config_.data_dir.empty()) return;
  std::error_code ec;
  fs::remove(job_file(id), ec);
  fs::remove(state_file(id), ec);
}

void JobManager::load_from_disk() {
  std::error_code ec;
  for (const fs::directory_entry& file : fs::directory_iterator(config_.data_dir, ec)) {
    if (file.path().extension() != ".job") continue;
    std::ifstream in(file.path(), std::ios::binary);
    v1::Job job;
    if (!in || !job.ParseFromIstream(&in) || !valid_id(job.id()) ||
        file.path().stem().string() != job.id()) {
      spdlog::warn("ignoring unreadable job file {}", file.path().string());
      continue;
    }

    // Whatever was in flight when the server stopped did not finish.
    if (job.state() == v1::JOB_STATE_RUNNING || job.state() == v1::JOB_STATE_QUEUED) {
      job.set_state(v1::JOB_STATE_PAUSED);
    }
    if (job.has_solution() && !fs::exists(state_file(job.id()), ec)) {
      // The solve never got saved; it starts over when resumed.
      job.set_has_solution(false);
      job.set_iteration(0);
      job.set_measured(false);
      if (job.state() == v1::JOB_STATE_DONE) job.set_state(v1::JOB_STATE_PAUSED);
    }

    complete_old_job(job);
    Entry entry;
    entry.job = std::move(job);
    if (entry.job.locks().empty()) {
      std::string error;
      entry.spot_key = reuse_key(entry.job.spot(), entry.job.engine(), &error);
    }
    jobs_.emplace(entry.job.id(), std::move(entry));
  }
  if (!jobs_.empty()) spdlog::info("loaded {} job(s) from {}", jobs_.size(),
                                   config_.data_dir.string());
}

// --- Helpers under mutex_ --------------------------------------------------

JobManager::Entry* JobManager::find(const std::string& id) {
  const auto it = jobs_.find(id);
  return it == jobs_.end() ? nullptr : &it->second;
}

const JobManager::Entry* JobManager::find(const std::string& id) const {
  const auto it = jobs_.find(id);
  return it == jobs_.end() ? nullptr : &it->second;
}

void JobManager::changed(Entry& entry) {
  ++entry.version;
  change_cv_.notify_all();
}

std::string JobManager::new_id() {
  // Creation time, then a counter for jobs submitted within one millisecond:
  // ids sort in the order the jobs were created.
  for (;;) {
    const std::int64_t now = std::max(now_ms(), last_id_ms_);
    last_id_sequence_ = now == last_id_ms_ ? last_id_sequence_ + 1 : 0;
    last_id_ms_ = now;
    if (last_id_sequence_ > 0xFFFF) {
      // More than 65536 jobs in a millisecond: move on to the next one.
      ++last_id_ms_;
      last_id_sequence_ = 0;
    }
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%012llx%04x",
                  static_cast<unsigned long long>(last_id_ms_), last_id_sequence_);
    // Jobs loaded from disk may already hold an id from this millisecond.
    if (jobs_.find(buffer) == jobs_.end()) return buffer;
  }
}

const JobManager::Entry* JobManager::find_reusable(const std::string& key,
                                                   const ResolvedSettings& wanted) const {
  if (key.empty()) return nullptr;
  const Entry* best = nullptr;
  int best_rank = 0;
  // Oldest first, so that among equals the newest one wins.
  for (const auto& [id, entry] : jobs_) {
    const v1::Job& job = entry.job;
    // A job about to be paused, cancelled or deleted will not get there.
    if (entry.spot_key != key || entry.request != Request::None || !job.error().empty()) {
      continue;
    }
    int rank = 0;
    if (job.state() == v1::JOB_STATE_DONE) {
      // Got as far as a new solve with these settings would.
      const bool reached =
          (job.measured() && job.exploitability_pct() <= wanted.target_exploitability_pct) ||
          (job.engine() == v1::ENGINE_MONTE_CARLO
               ? job.mc_iterations() >= wanted.mc_iterations
               : job.iteration() >= static_cast<std::uint32_t>(wanted.max_iterations));
      if (reached && job.has_solution()) rank = 2;
    } else if (job.state() == v1::JOB_STATE_QUEUED || job.state() == v1::JOB_STATE_RUNNING) {
      const ResolvedSettings settings = gto::server::resolve(job.settings());
      // No time limit beats any; otherwise the longer one.
      const bool long_enough =
          settings.max_seconds == 0 ||
          (wanted.max_seconds > 0 && settings.max_seconds >= wanted.max_seconds);
      if (settings.target_exploitability_pct <= wanted.target_exploitability_pct &&
          settings.max_iterations >= wanted.max_iterations &&
          settings.mc_iterations >= wanted.mc_iterations && long_enough) {
        rank = 1;
      }
    }
    if (rank > 0 && rank >= best_rank) {
      best = &entry;
      best_rank = rank;
    }
  }
  return best;
}

void JobManager::evict_idle_solutions() {
  // Without a data directory memory is the only copy.
  if (config_.data_dir.empty()) return;
  for (;;) {
    Entry* oldest = nullptr;
    std::size_t loaded = 0;
    for (auto& [id, entry] : jobs_) {
      if (!entry.solution || entry.job.state() == v1::JOB_STATE_RUNNING) continue;
      ++loaded;
      if (oldest == nullptr || entry.last_used < oldest->last_used) oldest = &entry;
    }
    if (loaded <= config_.max_loaded || oldest == nullptr) return;
    // A browser still reading it keeps its own reference until it is done.
    oldest->solution.reset();
  }
}

// --- Estimating and submitting ---------------------------------------------

Error JobManager::estimate(const v1::Spot& spot, v1::EstimateTreeResponse* out) const {
  SpotPlan plan;
  if (Error error = plan_spot(spot, limits(), &plan)) return error;
  *out = std::move(plan.estimate);
  return {};
}

Error JobManager::submit(const v1::SubmitJobRequest& request, v1::Job* out) {
  SpotPlan plan;
  if (Error error = plan_spot(request.spot(), limits(), &plan)) return error;
  const v1::EstimateTreeResponse& estimate = plan.estimate;
  if (estimate.too_large()) {
    return make_error(ErrorKind::ResourceExhausted,
                      "the tree has more than " + std::to_string(config_.max_nodes) +
                          " nodes; use fewer bet sizes or a lower raise cap");
  }
  const std::uint64_t needed = estimate.strategy_bytes() + estimate.tree_bytes();
  if (!estimate.can_solve()) {
    return make_error(ErrorKind::ResourceExhausted,
                      "the solve needs about " + std::to_string(needed >> 20) +
                          " MB, more than the server's limit of " +
                          std::to_string(config_.memory_budget_bytes >> 20) + " MB");
  }
  if (request.name().size() > 200) {
    return make_error(ErrorKind::InvalidArgument, "the name is longer than 200 characters");
  }
  std::string key = std::move(plan.key);
  const ResolvedSettings settings = gto::server::resolve(request.settings());

  const std::lock_guard lock(mutex_);
  if (stopping_) return make_error(ErrorKind::FailedPrecondition, "the server is shutting down");

  if (!request.force_new()) {
    if (const Entry* existing = find_reusable(key, settings)) {
      *out = existing->job;
      out->set_reused(true);
      spdlog::info("submit answered by job {}, which solves the same spot", existing->job.id());
      return {};
    }
  }

  Entry entry;
  entry.spot_key = std::move(key);
  v1::Job& job = entry.job;
  job.set_id(new_id());
  job.set_name(request.name());
  job.set_state(v1::JOB_STATE_QUEUED);
  *job.mutable_spot() = request.spot();
  // Store the settings with defaults filled in, so a job always shows what
  // it will actually do.
  job.mutable_settings()->set_max_iterations(static_cast<std::uint32_t>(settings.max_iterations));
  job.mutable_settings()->set_target_exploitability_pct(settings.target_exploitability_pct);
  job.mutable_settings()->set_max_seconds(settings.max_seconds);
  if (plan.engine == v1::ENGINE_MONTE_CARLO) {
    job.mutable_settings()->set_mc_iterations(settings.mc_iterations);
  }
  job.set_created_ms(now_ms());
  job.set_memory_bytes(needed);
  job.set_engine(plan.engine);
  job.set_exploitability_kind(estimate.exploitability_kind());
  job.set_player_count(estimate.player_count());
  *job.mutable_positions() = estimate.positions();

  write_job(job);
  *out = job;
  queue_.push_back(job.id());
  jobs_.emplace(job.id(), std::move(entry));
  queue_cv_.notify_one();
  return {};
}

// --- Reading ---------------------------------------------------------------

std::vector<v1::Job> JobManager::list() const {
  const std::lock_guard lock(mutex_);
  std::vector<v1::Job> jobs;
  jobs.reserve(jobs_.size());
  // Ids start with the creation time, so reverse map order is newest first.
  for (auto it = jobs_.rbegin(); it != jobs_.rend(); ++it) jobs.push_back(it->second.job);
  return jobs;
}

Error JobManager::get(const std::string& id, v1::Job* out) const {
  const std::lock_guard lock(mutex_);
  const Entry* entry = find(id);
  if (entry == nullptr) return not_found(id);
  *out = entry->job;
  return {};
}

bool JobManager::wait_for_change(const std::string& id, std::uint64_t* version, v1::Job* out,
                                 bool* gone, std::chrono::milliseconds timeout) const {
  std::unique_lock lock(mutex_);
  *gone = false;
  const auto differs = [&] {
    const Entry* entry = find(id);
    return stopping_ || entry == nullptr || entry->version != *version;
  };
  if (!change_cv_.wait_for(lock, timeout, differs)) return false;

  const Entry* entry = find(id);
  if (entry == nullptr) {
    *gone = true;
    return true;
  }
  if (entry->version == *version) {
    // Woken only because the server is stopping.
    *gone = true;
    return true;
  }
  *version = entry->version;
  *out = entry->job;
  return true;
}

// --- Controlling -----------------------------------------------------------

Error JobManager::pause(const std::string& id, v1::Job* out) {
  const std::lock_guard lock(mutex_);
  Entry* entry = find(id);
  if (entry == nullptr) return not_found(id);

  switch (entry->job.state()) {
    case v1::JOB_STATE_QUEUED:
      queue_.erase(std::remove(queue_.begin(), queue_.end(), id), queue_.end());
      entry->job.set_state(v1::JOB_STATE_PAUSED);
      write_job(entry->job);
      changed(*entry);
      break;
    case v1::JOB_STATE_RUNNING:
      // Takes effect after the current iteration; watchers see it then.
      entry->request = Request::Pause;
      break;
    case v1::JOB_STATE_PAUSED:
      break;
    default:
      return make_error(ErrorKind::FailedPrecondition, "the job has already ended");
  }
  *out = entry->job;
  return {};
}

Error JobManager::resume(const std::string& id, v1::Job* out) {
  const std::lock_guard lock(mutex_);
  if (stopping_) return make_error(ErrorKind::FailedPrecondition, "the server is shutting down");
  Entry* entry = find(id);
  if (entry == nullptr) return not_found(id);

  switch (entry->job.state()) {
    case v1::JOB_STATE_PAUSED:
    case v1::JOB_STATE_CANCELLED:
      entry->job.set_state(v1::JOB_STATE_QUEUED);
      entry->job.set_finished_ms(0);
      entry->request = Request::None;
      write_job(entry->job);
      queue_.push_back(id);
      changed(*entry);
      queue_cv_.notify_one();
      break;
    case v1::JOB_STATE_QUEUED:
      break;
    case v1::JOB_STATE_RUNNING:
      // Withdraw a pause that has not taken effect yet.
      if (entry->request == Request::Pause) entry->request = Request::None;
      break;
    case v1::JOB_STATE_DONE:
      return make_error(ErrorKind::FailedPrecondition, "the job is already done");
    default:
      return make_error(ErrorKind::FailedPrecondition,
                        "the job failed and cannot be resumed: " + entry->job.error());
  }
  *out = entry->job;
  return {};
}

Error JobManager::cancel(const std::string& id, v1::Job* out) {
  const std::lock_guard lock(mutex_);
  Entry* entry = find(id);
  if (entry == nullptr) return not_found(id);

  switch (entry->job.state()) {
    case v1::JOB_STATE_QUEUED:
      queue_.erase(std::remove(queue_.begin(), queue_.end(), id), queue_.end());
      [[fallthrough]];
    case v1::JOB_STATE_PAUSED:
      entry->job.set_state(v1::JOB_STATE_CANCELLED);
      entry->job.set_finished_ms(now_ms());
      write_job(entry->job);
      changed(*entry);
      break;
    case v1::JOB_STATE_RUNNING:
      entry->request = Request::Cancel;
      break;
    case v1::JOB_STATE_CANCELLED:
      break;
    default:
      return make_error(ErrorKind::FailedPrecondition, "the job has already ended");
  }
  *out = entry->job;
  return {};
}

Error JobManager::remove(const std::string& id) {
  const std::lock_guard lock(mutex_);
  Entry* entry = find(id);
  if (entry == nullptr) return not_found(id);

  if (entry->job.state() == v1::JOB_STATE_RUNNING) {
    // The worker owns it right now and removes it after the current iteration.
    entry->request = Request::Delete;
    return {};
  }
  queue_.erase(std::remove(queue_.begin(), queue_.end(), id), queue_.end());
  erase_files(id);
  jobs_.erase(id);
  change_cv_.notify_all();
  return {};
}

// --- Solving ---------------------------------------------------------------

std::shared_ptr<JobManager::Solution> JobManager::load_solution(
    const v1::Job& job, const std::string& initial_state, std::string* error) const {
  auto solution = std::make_shared<Solution>();
  solution->engine = make_engine(job.spot(), job.engine(), limits(), error);
  if (!solution->engine) return nullptr;

  SolveEngine& engine = *solution->engine;
  std::error_code ec;
  const bool have_file = !config_.data_dir.empty() && fs::exists(state_file(job.id()), ec);
  std::string load_error;
  bool loaded = true;
  if (!initial_state.empty()) {
    std::istringstream in(initial_state, std::ios::binary);
    loaded = load_state(engine, in, &load_error);
  } else if (have_file) {
    std::ifstream in(state_file(job.id()), std::ios::binary);
    loaded = in && load_state(engine, in, &load_error);
  } else if (job.has_solution() && !config_.data_dir.empty()) {
    loaded = false;
  }
  if (!loaded) {
    *error = "cannot read the saved solution: " +
             (load_error.empty() ? std::string("the file is missing") : load_error);
    return nullptr;
  }

  // Locks are part of the job, not of the saved state.
  if (!job.locks().empty()) {
    if (Error lock_error = engine.apply_locks(job.locks())) {
      *error = lock_error.message;
      return nullptr;
    }
  }
  // A re-solve keeps what the parent learned about each action, but its
  // average strategy has to be built anew: the parent's is of another game.
  if (job.warm_start_pending()) engine.warm_restart();
  return solution;
}

void JobManager::worker_loop() {
  for (;;) {
    std::string id;
    {
      std::unique_lock lock(mutex_);
      queue_cv_.wait(lock, [&] { return stopping_ || !queue_.empty(); });
      if (stopping_) return;
      id = queue_.front();
      queue_.pop_front();
      Entry* entry = find(id);
      if (entry == nullptr) continue;
      entry->job.set_state(v1::JOB_STATE_RUNNING);
      entry->request = Request::None;
      entry->last_used = ++use_counter_;
      write_job(entry->job);
      changed(*entry);
    }
    run_job(id);
  }
}

void JobManager::run_job(const std::string& id) {
  v1::Job job;
  std::shared_ptr<Solution> solution;
  std::string initial_state;
  {
    const std::lock_guard lock(mutex_);
    Entry* entry = find(id);
    if (entry == nullptr) return;
    job = entry->job;
    solution = entry->solution;
    initial_state = std::move(entry->initial_state);
    entry->initial_state.clear();
  }

  // Applies the final state of this run. Returns false if the job is gone.
  const auto finish = [&](const std::function<void(v1::Job&)>& update) {
    const std::lock_guard lock(mutex_);
    Entry* entry = find(id);
    if (entry == nullptr) return false;
    update(entry->job);
    write_job(entry->job);
    changed(*entry);
    return true;
  };

  if (!solution) {
    std::string error;
    {
      const std::lock_guard load_lock(load_mutex_);
      solution = load_solution(job, initial_state, &error);
    }
    if (!solution) {
      spdlog::warn("job {} failed: {}", id, error);
      finish([&](v1::Job& j) {
        j.set_state(v1::JOB_STATE_FAILED);
        j.set_error(error);
        j.set_finished_ms(now_ms());
      });
      return;
    }
    const std::lock_guard lock(mutex_);
    Entry* entry = find(id);
    if (entry == nullptr) return;
    entry->solution = solution;
    evict_idle_solutions();
  }

  const ResolvedSettings settings = gto::server::resolve(job.settings());
  const int interval = std::max(1, config_.check_interval);
  spdlog::info("job {} started ({} MB)", id, job.memory_bytes() >> 20);

  const std::uint64_t budget = budget_of(*solution->engine, settings);

  Request request = Request::None;
  bool stopped_by_shutdown = false;
  bool done = false;
  for (;;) {
    double solved_seconds = 0;
    {
      const std::lock_guard lock(mutex_);
      Entry* entry = find(id);
      if (entry == nullptr) return;
      request = entry->request;
      stopped_by_shutdown = stopping_;
      solved_seconds = entry->job.solve_seconds();
    }
    if (request != Request::None || stopped_by_shutdown) break;

    std::uint64_t iteration = 0;
    bool measured = false;
    bool out_of_budget = false;
    EngineEvaluation evaluation;
    const auto started = std::chrono::steady_clock::now();
    {
      // Browsers wait here for at most one chunk of work.
      const std::lock_guard solve_lock(solution->mutex);
      SolveEngine& engine = *solution->engine;
      engine.run_chunk(budget);
      iteration = engine.iterations();
      const std::chrono::duration<double> so_far = std::chrono::steady_clock::now() - started;
      out_of_budget = iteration >= budget ||
                      (settings.max_seconds > 0 &&
                       solved_seconds + so_far.count() >= settings.max_seconds);
      if (out_of_budget || engine.measurement_due(interval)) {
        evaluation = engine.evaluate();
        measured = true;
      }
    }
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started;

    {
      const std::lock_guard lock(mutex_);
      Entry* entry = find(id);
      if (entry == nullptr) return;
      v1::Job& live = entry->job;
      live.set_iteration(static_cast<std::uint32_t>(
          std::min<std::uint64_t>(iteration, std::numeric_limits<std::uint32_t>::max())));
      if (solution->engine->kind() == v1::ENGINE_MONTE_CARLO) live.set_mc_iterations(iteration);
      live.set_has_solution(true);
      live.set_solve_seconds(live.solve_seconds() + elapsed.count());
      if (measured) {
        live.set_measured(true);
        live.set_exploitability_pct(evaluation.exploitability_pct);
        live.set_exploitability_kind(evaluation.exploitability_kind);
        live.set_ev_estimated(evaluation.estimated);
        live.clear_evs();
        for (const double ev : evaluation.evs) live.add_evs(ev);
        if (evaluation.evs.size() == 2) {
          live.set_oop_ev(evaluation.evs[0]);
          live.set_ip_ev(evaluation.evs[1]);
        }
      }
      changed(*entry);
    }

    if (out_of_budget ||
        (measured && evaluation.exploitability_pct <= settings.target_exploitability_pct)) {
      done = true;
      break;
    }
  }

  if (request == Request::Delete) {
    const std::lock_guard lock(mutex_);
    erase_files(id);
    jobs_.erase(id);
    change_cv_.notify_all();
    spdlog::info("job {} deleted", id);
    return;
  }

  bool saved = true;
  {
    const std::lock_guard solve_lock(solution->mutex);
    if (solution->engine->iterations() > 0) saved = write_state(id, *solution);
  }

  const v1::JobState state = done                          ? v1::JOB_STATE_DONE
                             : request == Request::Cancel ? v1::JOB_STATE_CANCELLED
                                                          : v1::JOB_STATE_PAUSED;
  finish([&](v1::Job& j) {
    j.set_state(state);
    // From here on the job continues from its own saved state.
    if (saved) j.set_warm_start_pending(false);
    if (is_final(state)) j.set_finished_ms(now_ms());
    if (!saved) j.set_error("the solution could not be saved to disk");
  });
  spdlog::info("job {} {}", id,
               state == v1::JOB_STATE_DONE        ? "done"
               : state == v1::JOB_STATE_CANCELLED ? "cancelled"
                                                  : "paused");
}

// --- Browsing --------------------------------------------------------------

Error JobManager::acquire_solution(const std::string& id, std::shared_ptr<Solution>* out) {
  v1::Job job;
  {
    const std::lock_guard lock(mutex_);
    Entry* entry = find(id);
    if (entry == nullptr) return not_found(id);
    if (!entry->job.has_solution()) {
      return make_error(ErrorKind::FailedPrecondition,
                        "the job has not started solving yet, so there is nothing to show");
    }
    entry->last_used = ++use_counter_;
    if (entry->solution) {
      *out = entry->solution;
      return {};
    }
    job = entry->job;
  }

  const std::lock_guard load_lock(load_mutex_);
  {
    // Someone else may have loaded it while we waited.
    const std::lock_guard lock(mutex_);
    Entry* entry = find(id);
    if (entry == nullptr) return not_found(id);
    if (entry->solution) {
      *out = entry->solution;
      return {};
    }
  }

  std::string error;
  std::shared_ptr<Solution> solution = load_solution(job, std::string(), &error);
  if (!solution) return make_error(ErrorKind::Internal, error);

  const std::lock_guard lock(mutex_);
  Entry* entry = find(id);
  if (entry == nullptr) return not_found(id);
  if (!entry->solution) {
    entry->solution = solution;
    evict_idle_solutions();
  }
  *out = entry->solution ? entry->solution : solution;
  return {};
}

Error JobManager::node(const std::string& id, const std::string& path,
                       const NodeOptions& options, v1::GetNodeResponse* out) {
  std::shared_ptr<Solution> solution;
  if (Error error = acquire_solution(id, &solution)) return error;
  const std::lock_guard solve_lock(solution->mutex);
  out->Clear();
  return solution->engine->node(path, options, out);
}

Error JobManager::runouts(const std::string& id, const std::string& path,
                          v1::GetRunoutReportResponse* out) {
  std::shared_ptr<Solution> solution;
  if (Error error = acquire_solution(id, &solution)) return error;
  const std::lock_guard solve_lock(solution->mutex);
  out->Clear();
  return solution->engine->runouts(path, out);
}

Error JobManager::preflop_scenarios(const std::string& id, std::uint32_t max_raises,
                                    v1::ListPreflopScenariosResponse* out) {
  std::shared_ptr<Solution> solution;
  if (Error error = acquire_solution(id, &solution)) return error;
  const std::lock_guard solve_lock(solution->mutex);
  out->Clear();
  return solution->engine->preflop_scenarios(max_raises, out);
}

Error JobManager::preflop_ranges(const std::string& id, const std::string& path,
                                 v1::GetPreflopRangesResponse* out) {
  std::shared_ptr<Solution> solution;
  if (Error error = acquire_solution(id, &solution)) return error;
  const std::lock_guard solve_lock(solution->mutex);
  out->Clear();
  return solution->engine->preflop_ranges(path, out);
}

// --- Locks -----------------------------------------------------------------

Error JobManager::resolve(const v1::ResolveRequest& request, v1::Job* out) {
  if (request.locks().empty()) {
    return make_error(ErrorKind::InvalidArgument, "a re-solve needs at least one lock");
  }
  if (request.name().size() > 200) {
    return make_error(ErrorKind::InvalidArgument, "the name is longer than 200 characters");
  }

  v1::Job parent;
  {
    const std::lock_guard lock(mutex_);
    const Entry* entry = find(request.parent_job_id());
    if (entry == nullptr) return not_found(request.parent_job_id());
    parent = entry->job;
  }
  if (parent.state() == v1::JOB_STATE_RUNNING || parent.state() == v1::JOB_STATE_QUEUED) {
    return make_error(ErrorKind::FailedPrecondition,
                      "the job is still solving; pause it or wait until it is done");
  }

  std::string id;
  {
    const std::lock_guard lock(mutex_);
    if (stopping_) return make_error(ErrorKind::FailedPrecondition, "the server is shutting down");
    id = new_id();
  }

  // Check the locks against the parent's tree and copy its state for the new
  // job. With a data directory the state goes straight to the new job's file:
  // states can be several GB, more than an in-memory string stream holds
  // (MSVC's stops growing at 2 GB and only sets failbit).
  std::shared_ptr<Solution> solution;
  if (Error error = acquire_solution(parent.id(), &solution)) return error;
  std::string state;
  int parent_iterations = 0;
  {
    const std::lock_guard solve_lock(solution->mutex);
    if (!solution->engine->supports_locks()) {
      return make_error(ErrorKind::FailedPrecondition,
                        "locks are supported on exact (heads-up postflop) solutions only");
    }
    if (Error error = solution->engine->check_locks(request.locks())) return error;
    if (config_.data_dir.empty()) {
      std::ostringstream buffer(std::ios::binary);
      if (!save_state(*solution->engine, buffer)) {
        return make_error(ErrorKind::ResourceExhausted,
                          "the solution is too large to copy in memory; start the server "
                          "with a data directory to re-solve it");
      }
      state = std::move(buffer).str();
    } else {
      const bool ok = write_atomically(state_file(id), [&](std::ostream& file) {
        return save_state(*solution->engine, file);
      });
      if (!ok) return make_error(ErrorKind::Internal, "cannot write the new job to disk");
    }
    parent_iterations = static_cast<int>(solution->engine->iterations());
  }

  const std::lock_guard lock(mutex_);
  if (stopping_) {
    erase_files(id);
    return make_error(ErrorKind::FailedPrecondition, "the server is shutting down");
  }

  Entry entry;
  v1::Job& job = entry.job;
  job.set_id(id);
  job.set_name(request.name().empty() ? parent.name() + " (locked)" : request.name());
  job.set_state(v1::JOB_STATE_QUEUED);
  *job.mutable_spot() = parent.spot();
  const ResolvedSettings settings = gto::server::resolve(request.settings());
  job.mutable_settings()->set_max_iterations(
      static_cast<std::uint32_t>(parent_iterations + settings.max_iterations));
  job.mutable_settings()->set_target_exploitability_pct(settings.target_exploitability_pct);
  job.set_iteration(static_cast<std::uint32_t>(parent_iterations));
  job.set_created_ms(now_ms());
  job.set_memory_bytes(parent.memory_bytes());
  job.set_parent_id(parent.id());
  *job.mutable_locks() = request.locks();
  job.set_engine(parent.engine());
  job.set_exploitability_kind(v1::EXPLOITABILITY_KIND_LOCKED_GAME);
  job.set_player_count(parent.player_count());
  *job.mutable_positions() = parent.positions();
  job.set_warm_start_pending(true);

  entry.initial_state = std::move(state);
  write_job(job);
  *out = job;
  queue_.push_back(job.id());
  jobs_.emplace(job.id(), std::move(entry));
  queue_cv_.notify_one();
  return {};
}

}  // namespace gto::server
