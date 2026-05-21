// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include "lock_free_stack_v3.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>

static void smoke_test() {
    LockFreeStackV3<int> st;
    assert(st.pop() == nullptr);
    st.push(0);
    st.push(1);
    assert(*st.pop() == 1);
    assert(*st.pop() == 0);
    assert(st.pop() == nullptr);
}

// Many producers race to push, then a single thread drains the stack.
// Every pushed value must appear exactly once.
static void concurrent_push_then_drain_test() {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 5000;
    LockFreeStackV3<int> st;

    std::vector<std::thread> ts;
    ts.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        ts.emplace_back([&st, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                st.push(t * kPerThread + i);
            }
        });
    }
    for (auto& th : ts) th.join();

    std::vector<int> got;
    while (auto p = st.pop()) {
        got.push_back(*p);
    }
    assert(static_cast<int>(got.size()) == kThreads * kPerThread);
    std::sort(got.begin(), got.end());
    for (int i = 0; i < kThreads * kPerThread; ++i) {
        assert(got[i] == i);
    }
}

// Pre-fill the stack, then many consumers race to drain it.
// Every value must be popped exactly once across all consumers.
static void prefill_then_concurrent_pop_test() {
    constexpr int kThreads = 4;
    constexpr int kTotal = 20000;
    LockFreeStackV3<int> st;
    for (int i = 0; i < kTotal; ++i) {
        st.push(i);
    }

    std::vector<std::vector<int>> per_thread(kThreads);
    std::vector<std::thread> ts;
    ts.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        ts.emplace_back([&, t]() {
            while (auto p = st.pop()) {
                per_thread[t].push_back(*p);
            }
        });
    }
    for (auto& th : ts) th.join();

    std::vector<int> got;
    for (auto& v : per_thread) {
        got.insert(got.end(), v.begin(), v.end());
    }
    assert(static_cast<int>(got.size()) == kTotal);
    std::sort(got.begin(), got.end());
    for (int i = 0; i < kTotal; ++i) {
        assert(got[i] == i);
    }
}

// Producers and consumers run at the same time. Consumers retry on empty
// until every produced value has been observed. The popped multiset must
// match the pushed multiset.
static void producers_and_consumers_test() {
    constexpr int kProducers = 3;
    constexpr int kConsumers = 3;
    constexpr int kPerProducer = 5000;
    constexpr int kTotal = kProducers * kPerProducer;
    LockFreeStackV3<int> st;
    std::atomic<int> remaining{kTotal};
    std::vector<std::vector<int>> consumed(kConsumers);
    std::vector<std::thread> ts;
    ts.reserve(kProducers + kConsumers);

    for (int t = 0; t < kProducers; ++t) {
        ts.emplace_back([&, t]() {
            for (int i = 0; i < kPerProducer; ++i) {
                st.push(t * kPerProducer + i);
            }
        });
    }
    for (int c = 0; c < kConsumers; ++c) {
        ts.emplace_back([&, c]() {
            while (remaining.load() > 0) {
                if (auto p = st.pop()) {
                    consumed[c].push_back(*p);
                    remaining.fetch_sub(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }
    for (auto& th : ts) th.join();

    std::vector<int> got;
    for (auto& v : consumed) {
        got.insert(got.end(), v.begin(), v.end());
    }
    assert(static_cast<int>(got.size()) == kTotal);
    std::sort(got.begin(), got.end());
    for (int i = 0; i < kTotal; ++i) {
        assert(got[i] == i);
    }
}

int main() {
    smoke_test();
    concurrent_push_then_drain_test();
    prefill_then_concurrent_pop_test();
    producers_and_consumers_test();
    return 0;
}
