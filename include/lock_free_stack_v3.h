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
        auto new_node = new Node(val);
        new_node->next = std::atomic_load(&head_);
        while (!std::atomic_compare_exchange_weak(&head_, &new_node->next, new_node)) {
            std::this_thread::yield();
        }
    }

    std::shared_ptr<T> pop() {
        auto old_head = std::atomic_load(head_);
        while (old_head && !std::atomic_compare_exchange_weak(&head_, &old_head, old_head->next)) {
            std::this_thread::yield();
        }
        if (old_head) {
            std::atomic_store(&(old_head->next), std::make_shared<Node>());
            return old_head->data;
        }
        return nullptr;
    }

private:
    struct Node {
        std::shared_ptr<T> data;
        std::shared_ptr<Node> next;
        Node(const T& d) : data(std::make_shared<T>(d)), next(nullptr) {}
    };
    std::shared_ptr<Node> head_ = nullptr;
};