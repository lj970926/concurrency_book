// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include <atomic>
#include <iostream>
#include <thread>

std::atomic<bool> x = false;
std::atomic<bool> y = false;
int z = 0;

void write_x_then_y() {
    x.store(true, std::memory_order_relaxed);
    y.store(true, std::memory_order_relaxed);
}

void read_y_then_x() {
    while (!y.load(std::memory_order_relaxed));
    if (x.load(std::memory_order_relaxed)) {
        ++z;
    }
}

int main() {
    std::thread a(write_x_then_y);
    std::thread b(read_y_then_x);

    a.join();
    b.join();

    std::cout << "Z=" << z << std::endl;
    return 0;
}