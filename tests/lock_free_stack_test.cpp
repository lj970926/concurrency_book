// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include "lock_free_stack.h"

#include <vector>
#include <thread>
#include <cassert>


static void smoke_test() {
    LockFreeStack<int> st;
    st.push(0);
    st.push(1);
    st.pop();
    st.pop();
}

static void concurrent_push_test() {
    std::vector<int> values_t1 = {0, 1, 2, 3};
    std::vector<int> values_t2 = {4, 5, 6, 7};
    LockFreeStack<int> st;
    auto insert_vals = [&st](const std::vector<int>& values){
        for (int v: values) {
            st.push(v);
            st.pop();
        }
    };

    std::thread t1(insert_vals, values_t1);
    std::thread t2(insert_vals, values_t2);
    t1.join();
    t2.join();
    assert(st.size() == 8);
}

int main() {
    smoke_test();
    concurrent_push_test();
    return 0;
}
