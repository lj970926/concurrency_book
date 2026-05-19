/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <atomic>
#include <cassert>
#include <memory>
#include <thread>

template <typename T>
class LockFreeStack {
private:
    struct Node {
        std::shared_ptr<T> data;
        Node* next = nullptr;
        Node(const T& d) : data(std::make_shared<T>(d)) {}
    };
    std::atomic<Node*> head_ = nullptr;
    std::atomic<int> size_ = 0;
    std::atomic<int> threads_in_pop_ = 0;
    std::atomic<Node*> nodes_to_delete_ = nullptr;

public:
    LockFreeStack() = default;
    void push(const T& value) {
        auto new_node = new Node(value);
        new_node->next = head_;
        while (!head_.compare_exchange_weak(new_node->next, new_node)) {
            std::this_thread::yield();
        }
        size_.fetch_add(1);
    }

    int size() {
        return size_.load();
    }

    std::shared_ptr<T> pop() {
        threads_in_pop_++;
        Node* old_head = head_.load();
        while (old_head && !head_.compare_exchange_weak(old_head, old_head->next)) {
            std::this_thread::yield();
        }
        // Pin data, avoid it to be removed.
        if (old_head) {
            std::shared_ptr<T> res = old_head->data;
            try_reclaim(old_head);
            return res;
        }

        return nullptr;
    }

    void try_reclaim(Node* old_head) {
        assert(old_head != nullptr);
        if (threads_in_pop_ == 1) {
            Node* nodes_to_delete = nodes_to_delete_.exchange(nullptr);
            if (--threads_in_pop_) {
                delete_nodes(nodes_to_delete);
            } else if (nodes_to_delete) {
                chain_pending_nodes(nodes_to_delete);
            }
            delete old_head;
        } else {
            old_head->next = nullptr;
            chain_pending_nodes(old_head);
            threads_in_pop_--;
        }
    }

    void delete_nodes(Node* head) {
        while (head) {
            auto next = head->next;
            delete head;
            head = next;
        }
    }

    void chain_pending_nodes(Node* first) {
        if (first == nullptr) {
            return;
        }
        Node* last = first;
        while (last->next) {
            last = last->next;
        }
        last->next = nodes_to_delete_;
        while (!nodes_to_delete_.compare_exchange_weak(last->next, first)) {
            std::this_thread::yield();
        }
    }
};
