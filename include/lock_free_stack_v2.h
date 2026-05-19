/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <atomic>
#include <memory>
#include <thread>

#include "hp_manager.h"

/**
 * @brief Lock-free Treiber stack that reclaims removed nodes with hazard pointers.
 *
 * @tparam T Value type stored in the stack.
 *
 * Each successful pop protects the observed head node with the current thread's
 * hazard pointer before reading the node's next pointer. Removed nodes are
 * either deleted immediately or deferred until no hazard pointer references
 * them.
 */
template <typename T>
class LockFreeStackV2 {
private:
    /**
     * @brief Singly linked stack node owned by the stack.
     */
    struct Node {
        std::shared_ptr<T> data;
        Node* next = nullptr;
        Node(std::shared_ptr<T> d) : data(d) {}
    };

    std::atomic<Node*> head_ = nullptr;

public:
    /**
     * @brief Push a value onto the stack.
     *
     * @param val Value to copy into the newly allocated node.
     */
    void push(const T& val) {
        auto new_node = new Node(std::make_shared<T>(val));
        new_node->next = head_.load();
        while (!head_.compare_exchange_weak(new_node->next, new_node)) {
            std::this_thread::yield();
        }
    }

    /**
     * @brief Pop the current top value from the stack.
     *
     * @return Shared pointer to the popped value, or nullptr if the stack is
     * empty.
     *
     * The head node is published through a hazard pointer before the compare
     * exchange. This prevents another thread from deleting and reusing that
     * address while this thread is still reading it.
     */
    std::shared_ptr<T> pop() {
        Node* old_head = head_.load();
        auto& hp = get_harzard_pointer_for_current_thread();
        do {
            Node* tmp;
            do {
                tmp = old_head;
                hp.store(tmp);
                old_head = head_.load();
            } while (old_head != tmp);
            // The cost of fake failed of weak is high, so we use strong here.
        } while (old_head && !head_.compare_exchange_strong(old_head, old_head->next));

        hp.store(nullptr);
        std::shared_ptr<T> res;
        if (old_head) {
            res = old_head->data;
            if (hp_can_delete(old_head)) {
                delete old_head;
            } else {
                hp_add_to_pending_list(old_head, [](void* p) { delete static_cast<Node*>(p); });
            }
            hp_delete_unused_pending_pointers();
        }
        return res;
    }
};
