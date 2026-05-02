// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include <atomic>
#include <thread>
#include <cassert>


std::atomic<bool> sync1 = false, sync2 = false;
std::atomic<int> data[5] = {-1, -1, -1, -1, -1};

void thread_1() {
    for (int i = 0; i < 5; ++i) {
        data[i].store(i, std::memory_order_relaxed);
    }
    sync1.store(true, std::memory_order_release);
}

void thread_2() {
    while (!sync1.load(std::memory_order_acquire)) ;
    sync2.store(true, std::memory_order_release);
}

void thread_3() {
    while (!sync2.load(std::memory_order_acquire)) ;
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