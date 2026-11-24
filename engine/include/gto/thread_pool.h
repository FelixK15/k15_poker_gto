#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace gto {

/// A fixed set of threads that run index loops in parallel.
///
/// The calling thread takes part as worker 0, so a pool of size 1 starts no
/// threads at all. parallel_for must not be called from inside a task.
class ThreadPool {
 public:
  using Task = std::function<void(std::size_t index, unsigned worker)>;

  /// `threads` counts the caller; 0 means one per hardware thread.
  explicit ThreadPool(unsigned threads = 0);
  ~ThreadPool();
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  /// Number of workers, including the calling thread.
  unsigned size() const noexcept { return static_cast<unsigned>(threads_.size()) + 1; }

  /// Calls task(i, worker) for every i in [0, count), each exactly once, and
  /// returns when all calls have finished. `worker` is in [0, size()) and no
  /// two concurrent calls share one, so it can index per-thread scratch data.
  void parallel_for(std::size_t count, const Task& task);

 private:
  void worker_loop(unsigned worker);
  void drain(unsigned worker);

  std::vector<std::thread> threads_;
  std::mutex mutex_;
  std::condition_variable wake_;
  std::condition_variable done_;
  const Task* task_ = nullptr;
  std::size_t count_ = 0;
  std::atomic<std::size_t> next_{0};
  /// Bumped for every parallel_for so that each worker joins each loop once.
  unsigned generation_ = 0;
  unsigned busy_ = 0;
  bool stopping_ = false;
};

}  // namespace gto
