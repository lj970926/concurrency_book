// SPDX-FileCopyrightText: 2026 lj970926
//
// SPDX-License-Identifier: MIT

#include <atomic>
#include <thread>

/**
 * @brief Lock-free single-producer single-consumer queue.
 *
 * @tparam T Element type.
 *
 * @note push() must be called from exactly one producer thread and pop() from
 *       exactly one consumer thread. Concurrent calls on the same side are
 *       undefined behavior.
 *
 * Uses a sentinel (dummy) node so that tail_ always points to an empty node.
 * Data lives in the node immediately before tail_. This avoids a
 * producer–consumer race on the tail node itself.
 */
template<typename T>
class SPSCQueue {
public:
    SPSCQueue(): head_(new Node), tail_(head_.load()) {};
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    ~SPSCQueue() {
        while (auto cur_head = head_.load()) {
            head_.store(cur_head->next);
            delete cur_head;
        }
    }

    /**
     * @brief Enqueues a value. Called only from the producer thread.
     *
     * Writes @p val into the current sentinel node, appends a new sentinel,
     * then advances tail_. The tail_.store() synchronizes-with the
     * tail_.load() in pop(), making the written data visible to the consumer.
     *
     * @param val Value to enqueue.
     */
    void push(const T& val) {
        auto new_data = std::make_shared<T>(val);
        auto new_node = new Node;
        auto old_tail = tail_.load();
        old_tail->data.swap(new_data);
        old_tail->next = new_node;
        tail_.store(new_node);
    }

    /**
     * @brief Dequeues the front value. Called only from the consumer thread.
     *
     * Loads tail_ to detect whether the producer has published a new node.
     * Returns nullptr if the queue is empty.
     *
     * @return Shared pointer to the dequeued value, or nullptr if empty.
     */
    std::shared_ptr<T> pop() {
        auto cur_head = head_.load();
        auto cur_tail = tail_.load();
        if (cur_head == cur_tail) {
            return nullptr;
        }
        std::shared_ptr<T> res = cur_head->data;
        head_.store(cur_head->next);
        delete cur_head;
        return res;
    }

private:
    struct Node {
        std::shared_ptr<T> data = nullptr;
        Node* next = nullptr;
        Node(const T& val): data(std::make_shared<T>(val)), next(nullptr) {}
        Node() = default;
    };
    std::atomic<Node*> head_; ///< Owned by the consumer thread.
    std::atomic<Node*> tail_; ///< Owned by the producer thread.
};