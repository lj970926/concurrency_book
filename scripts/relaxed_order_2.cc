// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include <atomic>
#include <iostream>
#include <thread>

constexpr int LOOP_CNT = 10;

struct ValueSnapshot {
    int x, y, z;
};

std::atomic<bool> go = false;
std::atomic<int> x = 0, y = 0, z = 0;

ValueSnapshot values1[LOOP_CNT];
ValueSnapshot values2[LOOP_CNT];
ValueSnapshot values3[LOOP_CNT];
ValueSnapshot values4[LOOP_CNT];
ValueSnapshot values5[LOOP_CNT];

void increment_vals(std::atomic<int>* vals_to_write, ValueSnapshot* vals_snapshot) {
    while (!go) {
        std::this_thread::yield();
    }

    for (int i = 0; i < LOOP_CNT; ++i) {
        vals_snapshot[i].x = x.load(std::memory_order_relaxed);
        vals_snapshot[i].y = y.load(std::memory_order_relaxed);
        vals_snapshot[i].z = z.load(std::memory_order_relaxed);
        vals_to_write->store(i + 1, std::memory_order_relaxed);
    }

    return;
}

void read_vals(ValueSnapshot* vals_snapshot) {
    while (!go) {
        std::this_thread::yield();
    }
    for (int i = 0; i < LOOP_CNT; ++i) {
        vals_snapshot[i].x = x.load(std::memory_order_relaxed);
        vals_snapshot[i].y = y.load(std::memory_order_relaxed);
        vals_snapshot[i].z = z.load(std::memory_order_relaxed);
    }
}

void print_vals(ValueSnapshot* vals_snapshot) {
    for (int i = 0; i < LOOP_CNT; ++i) {
        if (i != 0) {
            std::cout << ", ";
        }
        std::cout << "(" << vals_snapshot[i].x << ", " << vals_snapshot[i].y << ", "
                  << vals_snapshot[i].z << ")";
    }
    std::cout << std::endl;
}

int main() {
    std::thread t1(increment_vals, &x, values1);
    std::thread t2(increment_vals, &y, values2);
    std::thread t3(increment_vals, &z, values3);
    std::thread t4(read_vals, values4);
    std::thread t5(read_vals, values5);
    go = true;
    t5.join();
    t4.join();
    t3.join();
    t2.join();
    t1.join();
    print_vals(values1);
    print_vals(values2);
    print_vals(values3);
    print_vals(values4);
    print_vals(values5);
    return 0;
}