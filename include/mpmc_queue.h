/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <atomic>
#include <memory>

// Lock-free MPMC queue (Williams, "C++ Concurrency in Action" Ch.7) using
// split reference counting: head_/tail_ hold an ExCountPtr (Node* +
// external_count); each Node also carries internal_count + num_external_counters.
// A Node is reclaimed only after every external slot is freed AND
// internal_count == 0.
template <typename T>
class MPMCQueue {
public:
    MPMCQueue() {
        auto dummy_node = new Node;
        ExCountPtr dummy_ptr;
        dummy_ptr.ptr = dummy_node;
        head_.store(dummy_ptr);
        tail_.store(dummy_ptr);
    }
    void push(const T& val) {
        auto new_node = new Node;
        auto new_data = new T(val);
        ExCountPtr old_tail;
        while (true) {
            ExCountPtr old_tail = tail_.load();
            increase_external_count(tail_, old_tail);
            T* old_data = nullptr;
            if (old_tail.ptr->data.compare_exchange_strong(old_data, new_data)) {
                ExCountPtr new_tail;
                new_tail.ptr = new_node;
                old_tail.ptr->next = new_tail;
                // Must use exchange, not store: other producers can still bump
                // tail_.external_count between our data-CAS win and the publish.
                // Exchange atomically publishes new_tail AND snapshots the final
                // external_count so free_external_counter can transfer the right
                // number of in-flight refs into internal_count.
                old_tail = tail_.exchange(new_tail);
                free_external_counter(old_tail);
                break;
            }
            old_tail.ptr->release_ref();
        }
    }

    std::unique_ptr<T> pop() {
        auto old_head = head_.load();
        while (true) {
            increase_external_count(head_, old_head);
            if (old_head.ptr == tail_.load().ptr) {
                old_head.ptr->release_ref();
                return nullptr;
            }
            auto new_head = old_head.ptr->next;
            if (head_.compare_exchange_strong(old_head, new_head)) {
                std::unique_ptr<T> res(old_head.ptr->data);
                free_external_counter(old_head);
                return res;
            }
            old_head.ptr->release_ref();
        }
    }

private:
    struct Node;
    struct ExCountPtr {
        Node* ptr = nullptr;
        int external_count = 1;  // 1 = the slot's own implicit reference
    };

    struct NodeCounter {
        unsigned int internal_count : 30;
        unsigned int num_external_counters : 2;
        // Every Node lives behind exactly two external slots in its lifetime
        // (head_+tail_ for the initial dummy; tail_+previous-node's next for a
        // freshly published node). Each free_external_counter drops one.
        NodeCounter() : internal_count(0), num_external_counters(2) {}
    };
    struct Node {
        std::atomic<T*> data = nullptr;
        std::atomic<NodeCounter> counter;
        ExCountPtr next;

        void release_ref() {
            NodeCounter new_counter;
            NodeCounter old_counter = counter.load();
            do {
                new_counter.internal_count = old_counter.internal_count - 1;
                new_counter.num_external_counters = old_counter.num_external_counters;
            } while (!counter.compare_exchange_strong(old_counter, new_counter));

            if (new_counter.internal_count == 0 && new_counter.num_external_counters == 0) {
                delete this;
            }
        }
    };

    static void increase_external_count(std::atomic<ExCountPtr>& ptr, ExCountPtr& old_ptr) {
        ExCountPtr new_ptr = old_ptr;
        do {
            new_ptr.ptr = old_ptr.ptr;
            new_ptr.external_count = old_ptr.external_count + 1;
        } while (!ptr.compare_exchange_strong(old_ptr, new_ptr));
        old_ptr.external_count = new_ptr.external_count;
    }

    static void free_external_counter(ExCountPtr ex_ptr) {
        auto old_node = ex_ptr.ptr;
        // external_count = 1 (implicit slot ref) + N in-flight increase_external_count
        // callers. The thread running free_external_counter is one of them, so
        // (external_count - 2) refs are still outstanding and will each call
        // release_ref later -- fold that many into internal_count to balance.
        int count_to_inc = ex_ptr.external_count - 2;
        NodeCounter old_counter = old_node->counter.load();
        NodeCounter new_counter;
        do {
            new_counter.internal_count = old_counter.internal_count + count_to_inc;
            new_counter.num_external_counters = old_counter.num_external_counters - 1;
        } while (!old_node->counter.compare_exchange_strong(old_counter, new_counter));

        if (new_counter.internal_count == 0 && new_counter.num_external_counters == 0) {
            delete old_node;
        }
    }

    std::atomic<ExCountPtr> head_;
    std::atomic<ExCountPtr> tail_;
};