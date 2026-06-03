/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <algorithm>
#include <cassert>
#include <future>
#include <iterator>
#include <memory>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "thread_safe_stack.h"

template <typename ForwardIt>
class ParallelQuickSorter {
public:
    ParallelQuickSorter(int thread_num) {
        assert(thread_num >= 1);
        workers_.resize(thread_num);
        for (int i = 0; i < thread_num; ++i) {
            workers_.emplace_back(&ParallelQuickSorter::sort_worker_thread, this);
        }
    }

    void sort(ForwardIt begin, ForwardIt end) {
        ChunkSortTask task = {begin, end, {}};
        std::future future = task.promise.get_future();
        tasks_.push(task);
        future.get();
    }

private:
    using VT = typename std::iterator_traits<ForwardIt>::value_type;
    struct ChunkSortTask {
        ForwardIt first;
        ForwardIt last;
        std::promise<void> promise;
    };
    ThreadSafeStack<ChunkSortTask> tasks_;
    std::vector<std::thread> workers_;

    void sort_worker_thread() {
        while (true) {
            std::shared_ptr<ChunkSortTask> task = tasks_.pop();
            if (task) {
                sort_chunk(*task);
            }
            std::this_thread::yield();
        }
    }

    void sort_chunk(const ChunkSortTask& task) {
        if (task.first == task.last) {
            task.promise.set_value();
            return;
        }
        auto pivot = *task.first;
        auto mid_left =
            std::partition(task.first, task.ast, [&](const auto& x) { return x < pivot; });

        auto mid_right =
            std::partition(mid_left, task.last, [&](const auto& x) { return x > pivot; });
        ChunkSortTask left_sub_task = {task.first, mid_left, {}};
        auto left_future = left_sub_task.promise.get_future();
        tasks_.push(left_sub_task);
        ChunkSortTask right_sub_task = {mid_right, task.last, {}};
        sort_chunk(right_sub_task);
        left_future.get();
        return;
    }
};

template <typename ForwardIt>
void parallel_quick_sort(ForwardIt first, ForwardIt last) {}