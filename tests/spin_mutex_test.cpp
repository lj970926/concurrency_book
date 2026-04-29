// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include "spin_mutex.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

namespace {

void lock_unlock_smoke_test() {
    SpinMutex mutex;

    mutex.lock();
    mutex.unlock();
}

void works_with_lock_guard_test() {
    SpinMutex mutex;
    int value = 0;

    {
        std::lock_guard<SpinMutex> lock(mutex);
        ++value;
    }

    assert(value == 1);
}

void protects_shared_state_under_contention_test() {
    constexpr std::size_t thread_count = 8;
    constexpr std::size_t iterations_per_thread = 10000;

    SpinMutex mutex;
    std::size_t counter = 0;
    std::atomic<bool> start{false};
    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (std::size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([&] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (std::size_t j = 0; j < iterations_per_thread; ++j) {
                std::lock_guard<SpinMutex> lock(mutex);
                ++counter;
            }
        });
    }

    start.store(true, std::memory_order_release);

    for (auto& thread : threads) {
        thread.join();
    }

    assert(counter == thread_count * iterations_per_thread);
}

void admits_only_one_thread_at_a_time_test() {
    constexpr std::size_t thread_count = 8;
    constexpr std::size_t iterations_per_thread = 2000;

    SpinMutex mutex;
    std::atomic<bool> start{false};
    std::atomic<int> inside{0};
    std::atomic<int> max_inside{0};
    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (std::size_t i = 0; i < thread_count; ++i) {
        threads.emplace_back([&] {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (std::size_t j = 0; j < iterations_per_thread; ++j) {
                std::lock_guard<SpinMutex> lock(mutex);
                const int current_inside = inside.fetch_add(1, std::memory_order_relaxed) + 1;
                int observed = max_inside.load(std::memory_order_relaxed);
                while (observed < current_inside &&
                       !max_inside.compare_exchange_weak(
                           observed, current_inside, std::memory_order_relaxed)) {
                }
                inside.fetch_sub(1, std::memory_order_relaxed);
            }
        });
    }

    start.store(true, std::memory_order_release);

    for (auto& thread : threads) {
        thread.join();
    }

    assert(max_inside.load(std::memory_order_relaxed) == 1);
}

}  // namespace

int main() {
    lock_unlock_smoke_test();
    works_with_lock_guard_test();
    protects_shared_state_under_contention_test();
    admits_only_one_thread_at_a_time_test();
}
