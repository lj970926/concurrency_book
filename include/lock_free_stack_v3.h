/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#include <atomic>
#include <memory>
#include <thread>

/**
 * @brief The lock-free stack use reference couter.
 */
template <typename T>
class LockFreeStackV3 {
public:
    void push(const T& val) {
        RefcountPtr new_head;
        new_head.external_count = 1;
        new_head.ptr = new Node(val);
        new_head.ptr->next = head_.load();
        while (!head_.compare_exchange_weak(new_head.ptr->next, new_head)) {
            std::this_thread::yield();
        }
    }

    std::shared_ptr<T> pop() {
        // do {
        //     auto old_head = head_.load();
        //     increase_head_count(old_head);
        // } while (!head_.compare_exchange_strong(old_head, ))
    }

private:
    struct Node;
    struct RefcountPtr {
        Node* ptr = nullptr;
        int external_count = 0;
    };

    struct Node {
        std::shared_ptr<T> data;
        RefcountPtr next;
        int internal_count = 0;
        Node(const T& d): data(std::make_shared<T>(d)) {}
    };

    void increase_head_count(RefcountPtr& old_head) {
        do {
             RefcountPtr new_head = old_head;
            new_head.external_count++;
        } while (!head_.compare_exchange_strong(old_head, new_head));
        old_head.external_count++;
    }

    std::atomic<RefcountPtr> head_ = nullptr;

};