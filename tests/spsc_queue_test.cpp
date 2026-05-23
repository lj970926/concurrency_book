// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include "spsc_queue.h"

#include <thread>
#include <cassert>

/// Verifies that items pushed in order are received in the same order.
void basic_test() {
    SPSCQueue<int> queue;
    constexpr int TEST_LEN = 10000;
    std::thread push_thread([&] {
        for (int i = 0; i < TEST_LEN; ++i) {
            queue.push(i);
        }
    });
    std::thread pop_thread([&] {
        int recv_cnt = 0;
        while (recv_cnt < TEST_LEN) {
            auto res = queue.pop();
            if (res) {
                assert(*res == recv_cnt++);
            }
        }
    });

    push_thread.join();
    pop_thread.join();
}

int main() {
    basic_test();
}