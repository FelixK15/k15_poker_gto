#include "job_manager.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <system_error>

#include <spdlog/spdlog.h>

#include "convert.h"

namespace gto::server {

namespace {

namespace fs = std::filesystem;

std::int64_t now_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

Error make_error(ErrorKind kind, std::string message) { return {kind, std::move(message)}; }

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

v1::ActionType to_proto(ActionType type) {
  switch (type) {
    case ActionType::Fold: return v1::ACTION_TYPE_FOLD;
    case ActionType::Check: return v1::ACTION_TYPE_CHECK;
    case ActionType::Call: return v1::ACTION_TYPE_CALL;
    case ActionType::Bet: return v1::ACTION_TYPE_BET;
    case ActionType::Raise: return v1::ACTION_TYPE_RAISE;
    case ActionType::AllIn: return v1::ACTION_TYPE_ALLIN;
  }
  return v1::ACTION_TYPE_UNSPECIFIED;
}

v1::NodeType to_proto(NodeType type) {
  switch (type) {
    case NodeType::Action: return v1::NODE_TYPE_ACTION;
    case NodeType::Chance: return v1::NODE_TYPE_CHANCE;
    case NodeType::Fold: return v1::NODE_TYPE_FOLD;
    case NodeType::Showdown: return v1::NODE_TYPE_SHOWDOWN;
  }
  return v1::NODE_TYPE_UNSPECIFIED;
}

/// Adds the node's actions with how often the acting range takes each.
template <typename AddAction>
void fill_actions(const NodeReport& report, AddAction add_action) {
  if (report.type != NodeType::Action) return;
  const auto& reach = report.reach[static_cast<std::size_t>(report.player)];
  double total_reach = 0;
  for (const float value : reach) total_reach += value;

  for (std::size_t a = 0; a < report.actions.size(); ++a) {
    const Action& action = report.actions[a];
    v1::NodeAction* proto = add_action();
    proto->set_type(to_proto(action.type));
    proto->set_amount(action.amount);
    proto->set_label(action_label(action));
    double weighted = 0;
    for (std::size_t h = 0; h < reach.size(); ++h) {
      weighted += static_cast<double>(reach[h]) * report.frequency(a, h);
    }
    proto->set_frequency(total_reach > 0 ? weighted / total_reach : 0.0);
  }
}

/// The cards that can come at a chance node, named as the client knows them.
std::vector<std::string> cards_at(const GameTree& tree, const NodeRef& ref) {
  // The tree may hold this line with suits swapped.
  CardMap to_client = identity_card_map();
  for (std::size_t card = 0; card < to_client.size(); ++card) {
    to_client[ref.card_map[card]] = static_cast<std::uint8_t>(card);
  }
  std::vector<std::uint8_t> cards;
  for (const std::uint32_t child : tree.children_of(ref.node)) {
    cards.push_back(to_client[tree.nodes[child].card]);
  }
  for (const IsoDeal& iso : tree.iso_of(ref.node)) cards.push_back(to_client[iso.card]);
  std::sort(cards.begin(), cards.end());
  std::vector<std::string> names;
  for (const std::uint8_t card : cards) names.push_back(Card(card).to_string());
  return names;
}

/// Reads a lock from the wire against a solver's tree and hands.
bool read_lock(const Solver& solver, const v1::NodeLock& lock, NodeRef* ref,
               std::vector<std::uint8_t>* locked, std::vector<float>* strategy,
               std::string* error) {
  const Game& game = solver.game();
  const std::string where = "lock on '" + lock.path() + "': ";

  std::string find_error;
  const auto found = game.tree.find(lock.path(), &find_error);
  if (!found) {
    *error = where + find_error;
    return false;
  }
  const Node& node = game.tree.nodes[found->node];
  if (node.type != NodeType::Action) {
    *error = where + "only a decision can be locked";
    return false;
  }
  if (lock.hands().empty()) {
    *error = where + "it names no hands";
    return false;
  }

  const auto& hands = game.hands[node.player];
  const std::size_t actions = node.num_children;
  std::unordered_map<std::string, std::size_t> index;
  for (std::size_t h = 0; h < hands.size(); ++h) index.emplace(holdem_hand_name(hands[h]), h);

  locked->assign(hands.size(), 0);
  strategy->assign(actions * hands.size(), 0.0f);
  for (const v1::HandLock& hand : lock.hands()) {
    const auto it = index.find(hand.hand());
    if (it == index.end()) {
      *error = where + "the hand '" + hand.hand() + "' is not in the " +
               (node.player == kOop ? "OOP" : "IP") + " range";
      return false;
    }
    if (static_cast<std::size_t>(hand.frequencies_size()) != actions) {
      *error = where + "the hand '" + hand.hand() + "' needs " + std::to_string(actions) +
               " frequencies, one per action";
      return false;
    }
    float sum = 0;
    for (std::size_t a = 0; a < actions; ++a) {
      const float value = hand.frequencies(static_cast<int>(a));
      if (!(value >= 0)) {
        *error = where + "frequencies cannot be negative";
        return false;
      }
      (*strategy)[a * hands.size() + it->second] = value;
      sum += value;
    }
    if (!(sum > 0)) {
      *error = where + "the frequencies of '" + hand.hand() + "' add up to zero";
      return false;
    }
    (*locked)[it->second] = 1;
  }
  *ref = *found;
  return true;
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
    solution.solver->save(out);
    return static_cast<bool>(out);
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

    Entry entry;
    entry.job = std::move(job);
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
  std::string error;
  auto config = to_spot_config(spot, &error);
  if (!config) return make_error(ErrorKind::InvalidArgument, error);
  config->tree.max_nodes = config_.max_nodes;

  const auto estimate = estimate_spot(*config, &error);
  if (!estimate) return make_error(ErrorKind::InvalidArgument, error);

  out->set_nodes(estimate->tree.nodes);
  out->set_decision_nodes(estimate->tree.action_nodes);
  out->set_oop_hands(static_cast<std::uint32_t>(estimate->hands[kOop]));
  out->set_ip_hands(static_cast<std::uint32_t>(estimate->hands[kIp]));
  // The memory reported is what the solve will take, packed or not.
  const bool compress = should_compress(*estimate, config_.memory_budget_bytes);
  out->set_strategy_bytes(estimate->strategy_bytes_for(compress));
  out->set_tree_bytes(estimate->tree.tree_bytes());
  out->set_too_large(estimate->tree.too_large);
  out->set_memory_budget_bytes(config_.memory_budget_bytes);
  out->set_can_solve(!estimate->tree.too_large &&
                     (config_.memory_budget_bytes == 0 ||
                      estimate->total_bytes(compress) <= config_.memory_budget_bytes));
  return {};
}

Error JobManager::submit(const v1::SubmitJobRequest& request, v1::Job* out) {
  v1::EstimateTreeResponse estimate;
  if (Error error = this->estimate(request.spot(), &estimate)) return error;
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

  const std::lock_guard lock(mutex_);
  if (stopping_) return make_error(ErrorKind::FailedPrecondition, "the server is shutting down");

  Entry entry;
  v1::Job& job = entry.job;
  job.set_id(new_id());
  job.set_name(request.name());
  job.set_state(v1::JOB_STATE_QUEUED);
  *job.mutable_spot() = request.spot();
  // Store the settings with defaults filled in, so a job always shows what
  // it will actually do.
  const ResolvedSettings settings = gto::server::resolve(request.settings());
  job.mutable_settings()->set_max_iterations(static_cast<std::uint32_t>(settings.max_iterations));
  job.mutable_settings()->set_target_exploitability_pct(settings.target_exploitability_pct);
  job.set_created_ms(now_ms());
  job.set_memory_bytes(needed);

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
  auto config = to_spot_config(job.spot(), error);
  if (!config) return nullptr;
  config->tree.max_nodes = config_.max_nodes;
  // Decided the same way as for the estimate the job was accepted on.
  const auto estimate = estimate_spot(*config, error);
  if (!estimate) return nullptr;
  auto game = make_game(*config, error);
  if (!game) return nullptr;

  auto solution = std::make_shared<Solution>();
  SolverConfig solver_config;
  solver_config.threads = config_.threads;
  solver_config.compress = should_compress(*estimate, config_.memory_budget_bytes);
  solution->solver = std::make_unique<Solver>(std::move(*game), solver_config);

  Solver& solver = *solution->solver;
  std::error_code ec;
  const bool have_file = !config_.data_dir.empty() && fs::exists(state_file(job.id()), ec);
  std::string load_error;
  bool loaded = true;
  if (!initial_state.empty()) {
    std::istringstream in(initial_state, std::ios::binary);
    loaded = solver.load(in, &load_error);
  } else if (have_file) {
    std::ifstream in(state_file(job.id()), std::ios::binary);
    loaded = in && solver.load(in, &load_error);
  } else if (job.has_solution() && !config_.data_dir.empty()) {
    loaded = false;
  }
  if (!loaded) {
    *error = "cannot read the saved solution: " +
             (load_error.empty() ? std::string("the file is missing") : load_error);
    return nullptr;
  }

  // Locks are part of the job, not of the saved state.
  for (const v1::NodeLock& lock : job.locks()) {
    NodeRef ref;
    std::vector<std::uint8_t> locked;
    std::vector<float> strategy;
    if (!read_lock(solver, lock, &ref, &locked, &strategy, error) ||
        !solver.set_lock(ref, locked, strategy, error)) {
      return nullptr;
    }
  }
  // A re-solve keeps what the parent learned about each action, but its
  // average strategy has to be built anew: the parent's is of another game.
  if (job.warm_start_pending()) solver.warm_restart();
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

  Request request = Request::None;
  bool stopped_by_shutdown = false;
  bool done = false;
  for (;;) {
    {
      const std::lock_guard lock(mutex_);
      Entry* entry = find(id);
      if (entry == nullptr) return;
      request = entry->request;
      stopped_by_shutdown = stopping_;
    }
    if (request != Request::None || stopped_by_shutdown) break;

    int iteration = 0;
    bool measured = false;
    Evaluation evaluation;
    const auto started = std::chrono::steady_clock::now();
    {
      // Browsers wait here for at most one iteration.
      const std::lock_guard solve_lock(solution->mutex);
      Solver& solver = *solution->solver;
      if (solver.iterations() < settings.max_iterations) solver.iterate();
      iteration = solver.iterations();
      if (iteration % interval == 0 || iteration >= settings.max_iterations) {
        evaluation = solver.evaluate();
        measured = true;
      }
    }
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started;

    {
      const std::lock_guard lock(mutex_);
      Entry* entry = find(id);
      if (entry == nullptr) return;
      v1::Job& live = entry->job;
      live.set_iteration(static_cast<std::uint32_t>(iteration));
      live.set_has_solution(true);
      live.set_solve_seconds(live.solve_seconds() + elapsed.count());
      if (measured) {
        live.set_measured(true);
        live.set_exploitability_pct(evaluation.exploitability_pct);
        live.set_oop_ev(evaluation.ev[kOop]);
        live.set_ip_ev(evaluation.ev[kIp]);
      }
      changed(*entry);
    }

    if (iteration >= settings.max_iterations ||
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
    if (solution->solver->iterations() > 0) saved = write_state(id, *solution);
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

Error JobManager::node(const std::string& id, const std::string& path, v1::GetNodeResponse* out) {
  std::shared_ptr<Solution> solution;
  if (Error error = acquire_solution(id, &solution)) return error;

  const std::lock_guard solve_lock(solution->mutex);
  const Solver& solver = *solution->solver;
  const Game& game = solver.game();
  const GameTree& tree = game.tree;

  std::string find_error;
  const auto ref = tree.find(path, &find_error);
  if (!ref) return make_error(ErrorKind::NotFound, "path '" + path + "': " + find_error);

  const NodeReport report = solver.report(*ref);
  const Node& node = tree.nodes[ref->node];

  out->set_type(to_proto(report.type));
  out->set_player(report.player == kOop ? v1::PLAYER_OOP : v1::PLAYER_IP);
  out->set_path(path);
  out->set_iteration(static_cast<std::uint32_t>(solver.iterations()));
  out->set_pot(tree.pot + node.committed[kOop] + node.committed[kIp]);
  out->set_oop_committed(node.committed[kOop]);
  out->set_ip_committed(node.committed[kIp]);

  for (std::size_t player = 0; player < 2; ++player) {
    v1::PlayerRange* range = player == kOop ? out->mutable_oop() : out->mutable_ip();
    const auto& hands = game.hands[player];
    range->mutable_hands()->Reserve(static_cast<int>(hands.size()));
    for (const Hand& hand : hands) range->add_hands(holdem_hand_name(hand));
    range->mutable_reach()->Add(report.reach[player].begin(), report.reach[player].end());
    range->mutable_ev()->Add(report.ev[player].begin(), report.ev[player].end());
    range->set_range_ev(report.range_ev[player]);
    range->mutable_equity()->Add(report.equity[player].begin(), report.equity[player].end());
    range->set_range_equity(report.range_equity[player]);
  }

  if (report.type == NodeType::Action) {
    fill_actions(report, [&] { return out->add_actions(); });
    out->mutable_strategy()->Add(report.strategy.begin(), report.strategy.end());
    for (const std::uint8_t flag : report.locked) out->add_locked(flag != 0);
  }

  if (report.type == NodeType::Chance) {
    for (const std::string& card : cards_at(tree, *ref)) out->add_next_cards(card);
  }
  return {};
}

Error JobManager::runouts(const std::string& id, const std::string& path,
                          v1::GetRunoutReportResponse* out) {
  std::shared_ptr<Solution> solution;
  if (Error error = acquire_solution(id, &solution)) return error;

  const std::lock_guard solve_lock(solution->mutex);
  const Solver& solver = *solution->solver;
  const GameTree& tree = solver.game().tree;

  std::string find_error;
  const auto ref = tree.find(path, &find_error);
  if (!ref) return make_error(ErrorKind::NotFound, "path '" + path + "': " + find_error);
  if (tree.nodes[ref->node].type != NodeType::Chance) {
    return make_error(ErrorKind::InvalidArgument, "no card is dealt at '" + path + "'");
  }

  for (const std::string& card : cards_at(tree, *ref)) {
    const auto child = tree.find(path.empty() ? card : path + "/" + card);
    if (!child) continue;
    const NodeReport report = solver.report(*child);
    v1::Runout* runout = out->add_runouts();
    runout->set_card(card);
    runout->set_oop_ev(report.range_ev[kOop]);
    runout->set_ip_ev(report.range_ev[kIp]);
    runout->set_oop_equity(report.range_equity[kOop]);
    runout->set_ip_equity(report.range_equity[kIp]);
    runout->set_type(to_proto(report.type));
    runout->set_player(report.player == kOop ? v1::PLAYER_OOP : v1::PLAYER_IP);
    fill_actions(report, [&] { return runout->add_actions(); });
  }
  return {};
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
    for (const v1::NodeLock& lock : request.locks()) {
      NodeRef ref;
      std::vector<std::uint8_t> locked;
      std::vector<float> strategy;
      std::string error;
      if (!read_lock(*solution->solver, lock, &ref, &locked, &strategy, &error)) {
        return make_error(ErrorKind::InvalidArgument, error);
      }
    }
    if (config_.data_dir.empty()) {
      std::ostringstream buffer(std::ios::binary);
      solution->solver->save(buffer);
      if (!buffer) {
        return make_error(ErrorKind::ResourceExhausted,
                          "the solution is too large to copy in memory; start the server "
                          "with a data directory to re-solve it");
      }
      state = std::move(buffer).str();
    } else {
      const bool ok = write_atomically(state_file(id), [&](std::ostream& file) {
        solution->solver->save(file);
        return static_cast<bool>(file);
      });
      if (!ok) return make_error(ErrorKind::Internal, "cannot write the new job to disk");
    }
    parent_iterations = solution->solver->iterations();
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
