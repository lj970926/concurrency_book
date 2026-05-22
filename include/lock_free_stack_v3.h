/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <atomic>
#include <memory>
#include <thread>

/**
 * @brief Lock-free Treiber stack that reclaims removed nodes with split
 * reference counting.
 *
 * @tparam T Value type stored in the stack.
 *
 * Each node carries an internal atomic counter, while the head pointer carries
 * an external counter packed alongside the raw pointer. A popper first bumps
 * the external counter on the head to register interest, then races to swing
 * the head with compare-exchange. Losers release their claim through the
 * internal counter; the winner folds the remaining external claims into the
 * internal counter and deletes the node once both sides agree no other thread
 * still references it.
 */
template <typename T>
class LockFreeStackV3 {
public:
    /**
     * @brief Push a value onto the stack.
     *
     * @param val Value to copy into the newly allocated node.
     *
     * The new node is published with an external_count of 1, representing the
     * head reference itself. Concurrent pushers retry through compare-exchange
     * on the head.
     */
    void push(const T& val) {
        RefcountPtr new_head;
        new_head.external_count = 1;
        new_head.ptr = new Node(val);
        new_head.ptr->next = head_.load(std::memory_order_relaxed);
        while (!head_.compare_exchange_weak(new_head.ptr->next, new_head, std::memory_order_release,
                                            std::memory_order_relaxed));
    }

    /**
     * @brief Pop the current top value from the stack.
     *
     * @return Shared pointer to the popped value, or nullptr if the stack is
     * empty.
     *
     * The loop registers a claim on the observed head via increase_head_count
     * before each compare-exchange attempt. Losing attempts release that claim
     * through the node's internal counter, which also performs deferred
     * reclamation when the last reference disappears. The winner folds the
     * remaining external claims (external_count - 2) into the internal
     * counter; the minus two removes the head reference itself and this
     * thread's own claim.
     */
    std::shared_ptr<T> pop() {
        auto old_head = head_.load(std::memory_order_relaxed);
        std::shared_ptr<T> res = nullptr;
        // 1. pin the head node.
        increase_head_count(old_head);
        Node* old_ptr = old_head.ptr;
        // 2. try to take ownership of the head node.
        while (old_ptr && !head_.compare_exchange_strong(old_head, old_head.ptr->next,
                                                         std::memory_order_relaxed,
                                                         std::memory_order_relaxed)) {
            // 3. The origin node has been taken by another thread,
            // try if we are the last one and delete it.
            // The old_head has been modified by the CAS,
            // we need to use the old_ptr recorded before.
            if (old_ptr->internal_count.fetch_sub(1, std::memory_order_relaxed) == 1) {
                old_ptr->internal_count.load(std::memory_order_acquire);
                delete old_ptr;
            }
            // pin the new head again.
            increase_head_count(old_head);
            old_ptr = old_head.ptr;
        }
        if (old_head.ptr == nullptr) {
            // It does not matter to have ghost pointer.
            // Because it will be updated when new node pushed.
            return res;
        }
        res.swap(old_head.ptr->data);
        int release_cnt = old_head.external_count - 2;
        if (old_head.ptr->internal_count.fetch_add(release_cnt, std::memory_order_release) ==
            -release_cnt) {
            delete old_head.ptr;
        }
        return res;
    }

private:
    struct Node;

    /**
     * @brief Pointer to a Node bundled with an external reference count.
     *
     * Stored atomically as the stack head so that bumping the external count
     * and swinging the pointer can be performed by a single CAS.
     */
    struct RefcountPtr {
        Node* ptr = nullptr;
        int external_count = 0;
    };

    /**
     * @brief Singly linked stack node owned by the stack.
     *
     * internal_count starts at zero and accumulates the net delta from losing
     * poppers (each subtracts one) and the winning popper (adds the remaining
     * external claims). The node is deleted once the two sides cancel out.
     */
    struct Node {
        std::shared_ptr<T> data;
        RefcountPtr next;
        std::atomic<int> internal_count = 0;
        Node(const T& d) : data(std::make_shared<T>(d)) {}
    };

    /**
     * @brief Register interest in the current head by bumping its external
     * counter.
     *
     * @param old_head In/out parameter holding the caller's view of the head.
     * On entry it is the head value the caller wishes to claim; on return it
     * reflects the head value at the moment the claim was registered, with
     * external_count already incremented.
     */
    void increase_head_count(RefcountPtr& old_head) {
        RefcountPtr new_head;
        do {
            new_head = old_head;
            new_head.external_count++;
        } while (!head_.compare_exchange_strong(old_head, new_head, std::memory_order_acquire,
                                                std::memory_order_relaxed));
        old_head.external_count++;
    }

    std::atomic<RefcountPtr> head_{};
};
