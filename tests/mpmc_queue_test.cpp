// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include "mpmc_queue.h"

#include <atomic>
#include <cassert>
#include <memory>
#include <set>
#include <thread>
#include <vector>

// One producer, one consumer. With a single producer the consumer sees values
// in FIFO order, so each pop must equal the next expected integer.
void single_producer_single_customer_test() {
    MPMCQueue<int> queue;
    const int MAX_LEN = 100;

    std::thread produce_thread([&] {
        for (int i = 0; i < MAX_LEN; ++i) {
            queue.push(i);
        }
    });

    std::thread consume_thread([&] {
        int recv_cnt = 0;
        while (recv_cnt < MAX_LEN) {
            std::unique_ptr<int> res = queue.pop();
            // pop() returns nullptr when the queue is observed empty; spin
            // until the producer publishes the next value.
            if (res) {
                assert(*res == recv_cnt);
                recv_cnt++;
            }
        }
    });
    produce_thread.join();
    consume_thread.join();
}

// Many producers, one consumer. Each producer i pushes a disjoint integer
// range [i*N, i*N+N); the consumer collects every value. Interleaving means
// the order is unspecified, so correctness is checked by confirming the
// received multiset has the expected size and no duplicates.
void multi_producer_single_consumer_test() {
    MPMCQueue<int> queue;
    const int NUM_PRODUCER_THREADS = 100;
    const int NUM_VALUES_TO_PUSH_PER_THREAD = 100;
    std::vector<std::thread> producer_threads;
    for (int i = 0; i < NUM_PRODUCER_THREADS; ++i) {
        producer_threads.push_back(std::thread([&, i] {
            for (int j = 0; j < NUM_VALUES_TO_PUSH_PER_THREAD; ++j) {
                queue.push(i * NUM_VALUES_TO_PUSH_PER_THREAD + j);
            }
        }));
    }

    std::thread cosumer_thread([&] {
        std::vector<int> recv_values;
        while (recv_values.size() < NUM_PRODUCER_THREADS * NUM_VALUES_TO_PUSH_PER_THREAD) {
            auto res = queue.pop();
            if (res) {
                recv_values.push_back(*res);
            }
        }

        // A set strips duplicates; equal size confirms every value was unique.
        std::set<int> recv_value_set(recv_values.begin(), recv_values.end());
        assert(recv_value_set.size() == NUM_PRODUCER_THREADS * NUM_VALUES_TO_PUSH_PER_THREAD);
    });
    for (int i = 0; i < producer_threads.size(); ++i) {
        producer_threads[i].join();
    }

    cosumer_thread.join();
}

// One producer, many consumers. The producer pushes [0, TOTAL); consumers
// race on pop() and each appends to its own thread-local buffer to avoid
// contending on a shared collection. Buffers are merged after join and the
// union must equal the produced range exactly.
void single_producer_multi_customer_test() {
    MPMCQueue<int> queue;
    const int NUM_CONSUMER_THREADS = 8;
    const int TOTAL_VALUES = 1000;

    std::atomic<int> recv_cnt{0};
    std::vector<std::vector<int>> per_consumer(NUM_CONSUMER_THREADS);

    std::thread producer_thread([&] {
        for (int i = 0; i < TOTAL_VALUES; ++i) {
            queue.push(i);
        }
    });

    std::vector<std::thread> consumer_threads;
    for (int i = 0; i < NUM_CONSUMER_THREADS; ++i) {
        consumer_threads.push_back(std::thread([&, i] {
            while (recv_cnt.load() < TOTAL_VALUES) {
                auto res = queue.pop();
                if (res) {
                    per_consumer[i].push_back(*res);
                    recv_cnt.fetch_add(1);
                }
            }
        }));
    }

    producer_thread.join();
    for (auto& t : consumer_threads) {
        t.join();
    }

    std::set<int> recv_value_set;
    for (auto& bucket : per_consumer) {
        recv_value_set.insert(bucket.begin(), bucket.end());
    }
    // Equal size means every produced value was popped exactly once across
    // all consumers — no losses, no duplicates.
    assert(recv_value_set.size() == TOTAL_VALUES);
}

// Many producers, many consumers — the full lock-free contention case.
// Producers push disjoint integer ranges; consumers cooperatively drain the
// queue into thread-local buffers until the total received count matches
// what was pushed. The merged set verifies every value arrived exactly once.
void multi_producer_multi_customer_test() {
    MPMCQueue<int> queue;
    const int NUM_PRODUCER_THREADS = 16;
    const int NUM_CONSUMER_THREADS = 16;
    const int NUM_VALUES_TO_PUSH_PER_THREAD = 100;
    const int TOTAL_VALUES = NUM_PRODUCER_THREADS * NUM_VALUES_TO_PUSH_PER_THREAD;

    std::atomic<int> recv_cnt{0};
    std::vector<std::vector<int>> per_consumer(NUM_CONSUMER_THREADS);

    std::vector<std::thread> producer_threads;
    for (int i = 0; i < NUM_PRODUCER_THREADS; ++i) {
        producer_threads.push_back(std::thread([&, i] {
            for (int j = 0; j < NUM_VALUES_TO_PUSH_PER_THREAD; ++j) {
                queue.push(i * NUM_VALUES_TO_PUSH_PER_THREAD + j);
            }
        }));
    }

    std::vector<std::thread> consumer_threads;
    for (int i = 0; i < NUM_CONSUMER_THREADS; ++i) {
        consumer_threads.push_back(std::thread([&, i] {
            while (recv_cnt.load() < TOTAL_VALUES) {
                auto res = queue.pop();
                if (res) {
                    per_consumer[i].push_back(*res);
                    recv_cnt.fetch_add(1);
                }
            }
        }));
    }

    for (auto& t : producer_threads) {
        t.join();
    }
    for (auto& t : consumer_threads) {
        t.join();
    }

    std::set<int> recv_value_set;
    for (auto& bucket : per_consumer) {
        recv_value_set.insert(bucket.begin(), bucket.end());
    }
    assert(recv_value_set.size() == TOTAL_VALUES);
}

int main() {
    single_producer_single_customer_test();
    multi_producer_single_consumer_test();
    single_producer_multi_customer_test();
    multi_producer_multi_customer_test();
}
