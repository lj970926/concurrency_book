/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <atomic>
#include <functional>
#include <thread>

/**
 * @brief Entry in the global hazard pointer table.
 */
struct HazardPointer;

/**
 * @brief RAII owner for one hazard pointer slot assigned to the current thread.
 */
class HPManager {
private:
    HazardPointer* hp_;

public:
    /**
     * @brief Claim an unused hazard pointer slot for the current thread.
     */
    HPManager();

    /**
     * @brief Clear and release the claimed hazard pointer slot.
     */
    ~HPManager();

    /**
     * @brief Return the atomic pointer published by this manager.
     */
    std::atomic<void*>& pointer();
};

/**
 * @brief Return the hazard pointer assigned to the current thread.
 */
std::atomic<void*>& get_harzard_pointer_for_current_thread() {
    thread_local HPManager hp_manager;
    return hp_manager.pointer();
}

/**
 * @brief Check whether no active hazard pointer references the address.
 *
 * @param p Address being considered for deletion.
 * @return true if no thread currently protects p.
 */
bool hp_can_delete(void* p);

/**
 * @brief Defer deletion of an address until it is no longer hazard-protected.
 *
 * @param p Address to reclaim later.
 * @param deleter Callback that destroys the object stored at p.
 */
void hp_add_to_pending_list(void* p, std::function<void(void*)> deleter);

/**
 * @brief Scan the pending list and delete entries that are no longer protected.
 */
void hp_delete_unused_pending_pointers();
