#include "gto/thread_pool.h"

#include <algorithm>

namespace gto {

ThreadPool::ThreadPool(unsigned threads) {
  if (threads == 0) threads = std::max(1u, std::thread::hardware_concurrency());
  threads_.reserve(threads - 1);
  for (unsigned worker = 1; worker < threads; ++worker) {
    threads_.emplace_back([this, worker] { worker_loop(worker); });
  }
}

ThreadPool::~ThreadPool() {
  {
    const std::lock_guard lock(mutex_);
    stopping_ = true;
  }
  wake_.notify_all();
  for (std::thread& thread : threads_) thread.join();
}

void ThreadPool::drain(unsigned worker) {
  for (;;) {
    const std::size_t index = next_.fetch_add(1, std::memory_order_relaxed);
    if (index >= count_) return;
    (*task_)(index, worker);
  }
}

void ThreadPool::parallel_for(std::size_t count, const Task& task) {
  if (count == 0) return;
  if (threads_.empty() || count == 1) {
    for (std::size_t index = 0; index < count; ++index) task(index, 0);
    return;
  }

  {
    const std::lock_guard lock(mutex_);
    task_ = &task;
    count_ = count;
    next_.store(0, std::memory_order_relaxed);
    busy_ = static_cast<unsigned>(threads_.size());
    ++generation_;
  }
  wake_.notify_all();

  drain(0);

  std::unique_lock lock(mutex_);
  done_.wait(lock, [this] { return busy_ == 0; });
  task_ = nullptr;
}

void ThreadPool::worker_loop(unsigned worker) {
  unsigned seen = 0;
  std::unique_lock lock(mutex_);
  for (;;) {
    wake_.wait(lock, [&] { return stopping_ || generation_ != seen; });
    if (stopping_) return;
    seen = generation_;

    lock.unlock();
    drain(worker);
    lock.lock();

    if (--busy_ == 0) done_.notify_one();
  }
}

}  // namespace gto
