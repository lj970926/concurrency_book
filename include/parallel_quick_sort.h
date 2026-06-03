/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <algorithm>
#include <future>
#include <iterator>
#include <memory>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "thread_safe_stack.h"

template <typename ForwardIt>
class Sorter {
public:
    void sort(ForwardIt begin, ForwardIt end) {
        ChunkSortTask task = {begin, end, {}};
        std::future future = task.promise.get_future();
    }

private:
    using VT = typename std::iterator_traits<ForwardIt>::value_type;
    using ChunkSortResult = std::pair<ForwardIt, ForwardIt>;
    struct ChunkSortTask {
        ForwardIt first;
        ForwardIt last;
        std::promise<ChunkSortResult> promise;
    };
    ThreadSafeStack<ChunkSortTask> tasks_;
    std::vector<std::thread> workers_;
    inline static const int64_t MAX_THREAD_NUM =
        std::max<int64_t>(std::thread::hardware_concurrency() / 2, 1);

    void sort_worker_thread() {
        while (true) {
            std::shared_ptr<ChunkSortTask> task = tasks_.pop();
            if (task) {
                sort_chunk(task);
            }
            std::this_thread::yield();
        }
    }

    void sort_chunk(ChunkSortTask task) {
        auto pivot = *task.first;
        auto mid_left =
            std::partition(task.first, task.last, [&](const auto& x) { return x < pivot; });

        auto mid_right =
            std::partition(mid_left, task.last, [&](const auto& x) { return x > pivot; });
        ChunkSortTask left_sub_task = {task.first, mid_left, {}};
        tasks_.push(left_sub_task);
    }
};

template <typename ForwardIt>
void parallel_quick_sort(ForwardIt first, ForwardIt last) {}