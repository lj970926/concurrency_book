// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include <atomic>
#include <iostream>
#include <thread>

std::atomic<bool> x = false, y = false;
int z = 0;

void write_x() {
    x.store(true, std::memory_order_seq_cst);
}

void write_y() {
    y.store(true, std::memory_order_seq_cst);
}

void read_x_then_y() {
    while (!x.load(std::memory_order_seq_cst));
    if (y.load(std::memory_order_seq_cst)) {
        ++z;
    }
}

void read_y_then_x() {
    while (!y.load(std::memory_order_seq_cst));
    if (x.load(std::memory_order_seq_cst)) {
        ++z;
    }
}

int main() {
    std::thread c(read_x_then_y);
    std::thread d(read_y_then_x);
    std::thread a(write_x);
    std::thread b(write_y);
    a.join();
    b.join();
    c.join();
    d.join();
    std::cout << "The value of z is " << z << std::endl;
}