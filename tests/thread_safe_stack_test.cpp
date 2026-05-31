// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include "thread_safe_stack.h"

#include <cassert>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

/**
 * @brief Single-threaded smoke test for basic push/pop behavior.
 *
 * Verifies LIFO ordering and that pop returns nullptr on empty stack.
 */
void smoke_test() {
    ThreadSafeStack<int> stack;
    stack.push(1);
    stack.push(2);

    auto val = stack.pop();
    assert(val != nullptr && *val == 2);

    val = stack.pop();
    assert(val != nullptr && *val == 1);

    val = stack.pop();
    assert(val == nullptr);
}

/**
 * @brief Tests concurrent push and pop with one producer and one consumer.
 *
 * The producer pushes [0, MAX_LEN) while the consumer pops until all
 * values are received. Verifies no data is lost.
 */
void single_producer_single_consumer_test() {
    ThreadSafeStack<int> stack;
    constexpr int MAX_LEN = 1000;

    std::thread producer_thread([&] {
        for (int i = 0; i < MAX_LEN; ++i) {
            stack.push(i);
        }
    });

    std::unordered_set<int> recv_set;

    std::thread consumer_thread([&] {
        int recv_cnt = 0;
        while (recv_cnt < MAX_LEN) {
            std::shared_ptr<int> res;
            if ((res = stack.pop()) != nullptr) {
                recv_set.insert(*res);
                ++recv_cnt;
            }
        }
    });

    producer_thread.join();
    consumer_thread.join();

    assert(recv_set.size() == MAX_LEN);
}

/**
 * @brief Tests concurrent push and pop with multiple producers and consumers.
 *
 * Spawns 4 producers, each pushing 1000 unique values, and 4 consumers
 * that collectively drain all elements. Uses a mutex-protected set to
 * verify that every value is received exactly once.
 */
void multi_producer_multi_consumer_test() {
    ThreadSafeStack<int> stack;
    constexpr int PRODUCERS = 4;
    constexpr int ITEMS_PER_PRODUCER = 1000;
    constexpr int TOTAL = PRODUCERS * ITEMS_PER_PRODUCER;

    std::vector<std::thread> producers;
    for (int p = 0; p < PRODUCERS; ++p) {
        producers.emplace_back([&stack, p] {
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                stack.push(p * ITEMS_PER_PRODUCER + i);
            }
        });
    }

    std::mutex set_mutex;
    std::unordered_set<int> recv_set;

    std::vector<std::thread> consumers;
    for (int c = 0; c < PRODUCERS; ++c) {
        consumers.emplace_back([&] {
            int local_cnt = 0;
            while (local_cnt < ITEMS_PER_PRODUCER) {
                auto res = stack.pop();
                if (res) {
                    std::lock_guard guard(set_mutex);
                    recv_set.insert(*res);
                    ++local_cnt;
                }
            }
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    assert(recv_set.size() == TOTAL);
}

int main() {
    smoke_test();
    single_producer_single_consumer_test();
    multi_producer_multi_consumer_test();
    return 0;
}