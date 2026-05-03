// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include <atomic>
#include <thread>
#include <cassert>


std::atomic<int> sync = 0;
std::atomic<int> data[5] = {-1, -1, -1, -1, -1};

void thread_1() {
    for (int i = 0; i < 5; ++i) {
        data[i].store(i, std::memory_order_relaxed);
    }
    sync.store(1, std::memory_order_release);
}

void thread_2() {
    int expected = 1;
    while (!sync.compare_exchange_strong(expected, 2, std::memory_order_acq_rel)) {
        expected = 1;
    }
}

void thread_3() {
    while (sync.load(std::memory_order_acquire) < 2
) ;
    for (int i = 0; i < 5; ++i) {
        assert(data[i] == i);
    }
}

int main() {
    std::thread t1(thread_1);
    std::thread t2(thread_2);
    std::thread t3(thread_3);

    t1.join();
    t2.join();
    t3.join();
    return 0;
}