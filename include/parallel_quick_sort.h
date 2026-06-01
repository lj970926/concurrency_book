/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <algorithm>
#include <thread>

#include "thread_safe_stack.h"

template <typename ForwardIt>
class Sorter {
public:
    void sort(ForwardIt begin, ForwardIt end) {}

private:
    struct ChunkSortTask {
        ForwardIt first;
        ForwardIt last;
    };
    ThreadSafeStack<ChunkSortTask> tasks_;
    inline static const int64_t MAX_THREAD_NUM =
        std::max<int64_t>(std::thread::hardware_concurrency() - 1, 1);
};

template <typename ForwardIt>
void parallel_quick_sort(ForwardIt first, ForwardIt last) {}