/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <atomic>
#include <memory>

/**
 * @brief Lock-free multi-producer multi-consumer queue using split reference
 * counting (Williams, "C++ Concurrency in Action" Ch.7).
 *
 * @tparam T Value type stored in the queue.
 *
 * head_ and tail_ each hold an ExCountPtr (pointer + external_count). Each
 * Node carries an internal_count and num_external_counters so that a Node
 * is reclaimed only after every external slot is freed AND internal_count
 * reaches zero.
 */
template <typename T>
class MPMCQueue {
public:
    /**
     * @brief Construct an empty queue with a dummy sentinel node.
     *
     * The dummy node ensures head_ and tail_ are never null, avoiding a
     * special case when the queue is empty.
     */
    MPMCQueue() {
        auto dummy_node = new Node;
        ExCountPtr dummy_ptr;
        dummy_ptr.ptr = dummy_node;
        head_.store(dummy_ptr);
        tail_.store(dummy_ptr);
    }
    /**
     * @brief Enqueue a value at the tail of the queue.
     *
     * @param val Value to copy into the newly allocated node.
     *
     * Allocates a new node with a data pointer, then races to install it at
     * the tail via CAS on old_tail.ptr->data. The winner publishes the new
     * node via an exchange on tail_ (not a store — other producers can bump
     * external_count between the data-CAS win and the publish), then folds
     * the remaining external claims into internal_count.
     */
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

    /**
     * @brief Dequeue the front value from the queue.
     *
     * @return Unique pointer to the dequeued value, or nullptr if the queue
     * is empty.
     *
     * Bumps the external count on head_ to register interest, then races to
     * swing head_ with CAS. The winner takes ownership of the node's data and
     * folds the remaining external claims into internal_count for reclamation;
     * losers release their claim and retry. An empty queue is detected when
     * head_ and tail_ point to the same sentinel node.
     */
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

    /**
     * @brief Pointer to a Node bundled with an external reference count.
     *
     * Stored atomically as head_/tail_ so that bumping the external count
     * and swinging the pointer can be performed by a single CAS.
     */
    struct ExCountPtr {
        Node* ptr = nullptr;
        int external_count = 1;  ///< 1 == the slot's own implicit reference
    };

    /**
     * @brief Atomic refcount embedded in each Node.
     *
     * internal_count tracks outstanding internal references.
     * num_external_counters tracks how many external slots still point at
     * this node; every Node lives behind exactly two external slots in its
     * lifetime (head_ + tail_ for the dummy sentinel; tail_ + previous-node's
     * next for a freshly published node). Each free_external_counter drops one.
     */
    struct NodeCounter {
        unsigned int internal_count : 30;
        unsigned int num_external_counters : 2;
        NodeCounter() : internal_count(0), num_external_counters(2) {}
    };

    /**
     * @brief Singly linked queue node.
     *
     * data is allocated separately so that the atomic data store can also
     * serve as the synchronization point between producer and consumer.
     * The release_ref operation decrements internal_count and deletes the
     * node when both counters reach zero.
     */
    struct Node {
        std::atomic<T*> data = nullptr;
        std::atomic<NodeCounter> counter;
        ExCountPtr next;

        /**
         * @brief Decrement internal_count and delete this Node if it is no
         * longer referenced from either side.
         */
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

    /**
     * @brief Atomically bump the external_count on ptr by one.
     *
     * @param ptr  Atomic container of the pointer + counter pair.
     * @param old_ptr In/out: on entry the caller's view of the value; on
     * return reflects the value at the moment the bump was registered, with
     * external_count already incremented.
     */
    static void increase_external_count(std::atomic<ExCountPtr>& ptr, ExCountPtr& old_ptr) {
        ExCountPtr new_ptr = old_ptr;
        do {
            new_ptr.ptr = old_ptr.ptr;
            new_ptr.external_count = old_ptr.external_count + 1;
        } while (!ptr.compare_exchange_strong(old_ptr, new_ptr));
        old_ptr.external_count = new_ptr.external_count;
    }

    /**
     * @brief Release one external slot claim on ex_ptr.ptr, then fold the
     * remaining in-flight external references into internal_count.
     *
     * @param ex_ptr The external-count pointer being freed.
     *
     * external_count = 1 (implicit slot ref) + N in-flight increase_external_count
     * callers. The thread running free_external_counter is one of them, so
     * (external_count - 2) refs are still outstanding and will each call
     * release_ref later — fold that many into internal_count to balance.
     */
    static void free_external_counter(ExCountPtr ex_ptr) {
        auto old_node = ex_ptr.ptr;
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