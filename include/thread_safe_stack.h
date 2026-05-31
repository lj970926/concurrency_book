/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <memory>
#include <mutex>
#include <stack>

/**
 * @brief A thread-safe stack implementation using a mutex.
 *
 * This stack is safe for concurrent push and pop operations from
 * multiple threads. It uses std::shared_ptr to store elements,
 * ensuring that a popped value remains valid even if other threads
 * modify the stack concurrently.
 *
 * @tparam T The type of elements stored in the stack.
 */
template <typename T>
class ThreadSafeStack {
public:
    /**
     * @brief Pushes a value onto the stack.
     *
     * The value is copied and stored via std::shared_ptr.
     * This operation is thread-safe.
     *
     * @param val The value to push.
     */
    void push(const T& val) {
        std::lock_guard guard(mutex_);
        inner_stack_.push(std::make_shared<T>(val));
    }

    /**
     * @brief Pops the top element from the stack.
     *
     * Returns a shared_ptr to the popped value, or nullptr if the
     * stack is empty. Using shared_ptr avoids the race condition
     * between checking emptiness and using the popped value.
     *
     * @return std::shared_ptr<T> The popped value, or nullptr if empty.
     */
    std::shared_ptr<T> pop() {
        std::lock_guard guard(mutex_);
        if (inner_stack_.empty()) {
            return nullptr;
        }

        auto res = inner_stack_.top();
        inner_stack_.pop();
        return res;
    }

private:
    std::stack<std::shared_ptr<T>> inner_stack_;  ///< The underlying stack.
    std::mutex mutex_;                            ///< Mutex for thread safety.
};