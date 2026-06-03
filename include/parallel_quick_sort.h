/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <algorithm>
#include <atomic>
#include <cassert>
#include <future>
#include <iterator>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "thread_safe_stack.h"

/**
 * @brief A parallel quicksort implementation backed by a thread pool.
 *
 * Partitions the input range around a pivot element using the
 * Dutch-national-flag three-way partition. Sub-ranges below a
 * configurable size threshold are sorted sequentially with std::sort
 * to avoid excessive task creation overhead.
 *
 * @tparam ForwardIt A forward-iterator type whose value_type is
 *         default-constructible and supports operator< and operator==.
 */
template <typename ForwardIt>
class ParallelQuickSorter {
public:
    /**
     * @brief Constructs the sorter and launches worker threads.
     *
     * @param thread_num Number of worker threads (must be >= 1).
     */
    explicit ParallelQuickSorter(int thread_num) {
        assert(thread_num >= 1);
        workers_.reserve(thread_num);
        for (int i = 0; i < thread_num; ++i) {
            workers_.emplace_back(&ParallelQuickSorter::sort_worker_thread, this);
        }
    }

    /** @brief Signals workers to stop and joins all threads. */
    ~ParallelQuickSorter() {
        should_terminate_.store(true, std::memory_order_release);
        for (auto& w : workers_) {
            w.join();
        }
    }

    ParallelQuickSorter(const ParallelQuickSorter&) = delete;
    ParallelQuickSorter& operator=(const ParallelQuickSorter&) = delete;

    /**
     * @brief Sorts the range [begin, end) in ascending order.
     *
     * Blocks until the entire range has been sorted.
     *
     * @param begin Iterator to the first element.
     * @param end   Past-the-end iterator.
     */
    void sort(ForwardIt begin, ForwardIt end) {
        ChunkSortTask task = {begin, end, {}};
        std::future<void> future = task.promise.get_future();
        tasks_.push(std::move(task));
        future.get();
    }

private:
    static constexpr int kMinChunkSize = 1024;  ///< Sequential sort threshold.

    struct ChunkSortTask {
        ForwardIt first;
        ForwardIt last;
        std::promise<void> promise;
    };

    ThreadSafeStack<ChunkSortTask> tasks_;
    std::vector<std::thread> workers_;
    std::atomic<bool> should_terminate_{false};

    /** @brief Worker loop: pops and executes tasks until termination. */
    void sort_worker_thread() {
        while (!should_terminate_.load(std::memory_order_acquire)) {
            std::shared_ptr<ChunkSortTask> task = tasks_.pop();
            if (task) {
                sort_chunk(*task);
            } else {
                std::this_thread::yield();
            }
        }
    }

    /**
     * @brief Sorts a single chunk, splitting into sub-tasks when large.
     *
     * Uses a three-way partition (< pivot, == pivot, > pivot) to handle
     * duplicate elements efficiently. The left sub-range is dispatched as
     * a pool task; the right sub-range is recursed on the current thread.
     *
     * @param task The chunk to sort; its promise is set on completion.
     */
    void sort_chunk(ChunkSortTask& task) {
        auto dist = std::distance(task.first, task.last);
        if (dist <= 1) {
            task.promise.set_value();
            return;
        }

        // Small chunks: sort sequentially to avoid per-element task overhead.
        if (dist < kMinChunkSize) {
            std::sort(task.first, task.last);
            task.promise.set_value();
            return;
        }

        auto pivot = *task.first;
        // Three-way partition: [< pivot] [== pivot] [> pivot]
        auto mid_left =
            std::partition(task.first, task.last, [&](const auto& x) { return x < pivot; });
        auto mid_right =
            std::partition(mid_left, task.last, [&](const auto& x) { return !(pivot < x); });

        ChunkSortTask left_sub_task = {task.first, mid_left, {}};
        auto left_future = left_sub_task.promise.get_future();
        tasks_.push(std::move(left_sub_task));

        ChunkSortTask right_sub_task = {mid_right, task.last, {}};
        sort_chunk(right_sub_task);

        left_future.get();
        task.promise.set_value();
    }
};

/**
 * @brief Sorts the range [first, last) in ascending order using a thread pool.
 *
 * Creates a ParallelQuickSorter with std::thread::hardware_concurrency()
 * threads (minimum 2) and sorts the range.
 *
 * @tparam ForwardIt A forward-iterator type.
 * @param first Iterator to the first element.
 * @param last  Past-the-end iterator.
 */
template <typename ForwardIt>
void parallel_quick_sort(ForwardIt first, ForwardIt last) {
    int thread_num = static_cast<int>(std::thread::hardware_concurrency());
    if (thread_num < 2) thread_num = 2;
    ParallelQuickSorter<ForwardIt> sorter(thread_num);
    sorter.sort(first, last);
}