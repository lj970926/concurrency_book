// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include "mpmc_queue.h"

#include <cassert>
#include <memory>
#include <set>
#include <thread>
#include <vector>

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
            if (res) {
                assert(*res == recv_cnt);
                recv_cnt++;
            }
        }
    });
    produce_thread.join();
    consume_thread.join();
}

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

        std::set<int> recv_value_set(recv_values.begin(), recv_values.end());
        assert(recv_value_set.size() == NUM_PRODUCER_THREADS * NUM_VALUES_TO_PUSH_PER_THREAD);
    });
    for (int i = 0; i < producer_threads.size(); ++i) {
        producer_threads[i].join();
    }

    cosumer_thread.join();
}

int main() {
    single_producer_single_customer_test();
    multi_producer_single_consumer_test();
}