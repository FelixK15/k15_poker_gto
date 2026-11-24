#include "gto/thread_pool.h"

#include <atomic>
#include <vector>

#include <gtest/gtest.h>

namespace gto {
namespace {

TEST(ThreadPool, RunsEveryIndexExactlyOnce) {
  ThreadPool pool(4);
  EXPECT_EQ(pool.size(), 4u);

  // Repeated loops of different lengths, to catch workers that miss or
  // re-join a loop.
  for (const std::size_t count : {1u, 2u, 3u, 49u, 1000u, 5u, 0u, 257u}) {
    std::vector<std::atomic<int>> hits(count);
    std::atomic<bool> bad_worker{false};
    pool.parallel_for(count, [&](std::size_t index, unsigned worker) {
      if (worker >= pool.size()) bad_worker = true;
      ++hits[index];
    });
    EXPECT_FALSE(bad_worker);
    for (std::size_t i = 0; i < count; ++i) EXPECT_EQ(hits[i].load(), 1) << count << ":" << i;
  }
}

TEST(ThreadPool, WorkerIdsAreExclusive) {
  ThreadPool pool(4);
  std::vector<std::atomic<int>> in_use(pool.size());
  std::atomic<bool> clash{false};
  pool.parallel_for(2000, [&](std::size_t, unsigned worker) {
    if (in_use[worker].fetch_add(1) != 0) clash = true;
    volatile int spin = 0;
    for (int i = 0; i < 200; ++i) spin = spin + i;
    in_use[worker].fetch_sub(1);
  });
  EXPECT_FALSE(clash);
}

TEST(ThreadPool, SingleThreadRunsInline) {
  ThreadPool pool(1);
  EXPECT_EQ(pool.size(), 1u);
  std::vector<std::size_t> order;
  pool.parallel_for(5, [&](std::size_t index, unsigned worker) {
    EXPECT_EQ(worker, 0u);
    order.push_back(index);
  });
  EXPECT_EQ(order, (std::vector<std::size_t>{0, 1, 2, 3, 4}));
}

TEST(ThreadPool, DefaultUsesAtLeastOneThread) {
  ThreadPool pool;
  EXPECT_GE(pool.size(), 1u);
}

}  // namespace
}  // namespace gto
