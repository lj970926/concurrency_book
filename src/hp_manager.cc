/*
 * SPDX-FileCopyrightText: 2026 lj970926
 *
 * SPDX-License-Identifier: MIT
 */

#include "hp_manager.h"

#include <stdexcept>

#define MAX_THREAD_NUM 100

/**
 * @brief One hazard pointer slot and the thread id that owns it.
 */
struct HazardPointer {
    std::atomic<std::thread::id> id = std::thread::id();
    std::atomic<void*> pointer = nullptr;
};

/**
 * @brief Fixed-size process-wide hazard pointer table.
 */
class HPTable {
public:
    /**
     * @brief Return the singleton hazard pointer table.
     */
    static HPTable* instance() {
        static HPTable table;
        return &table;
    }
    HPTable(const HPTable&) = delete;
    HPTable& operator=(const HPTable&) = delete;

    /**
     * @brief Claim an unused hazard pointer slot for the current thread.
     */
    HazardPointer* get_hazard_poiner() {
        for (int i = 0; i < MAX_THREAD_NUM; ++i) {
            std::thread::id old_tid;
            if (pointer_table_[i].id.compare_exchange_strong(old_tid, std::this_thread::get_id())) {
                return &pointer_table_[i];
            }
        }
        throw std::runtime_error("No hazard pointer available");
    }

    /**
     * @brief Check whether an address is absent from all hazard pointer slots.
     */
    bool can_delete_safe(void* p) {
        for (int i = 0; i < MAX_THREAD_NUM; ++i) {
            if (pointer_table_[i].pointer == p) {
                return false;
            }
        }
        return true;
    }

private:
    HPTable() = default;
    HazardPointer pointer_table_[MAX_THREAD_NUM];
};

/**
 * @brief Lock-free pending list for nodes that cannot yet be reclaimed.
 */
class PendingList {
public:
    /**
     * @brief Return the singleton pending reclamation list.
     */
    static PendingList* instance() {
        static PendingList pending_list;
        return &pending_list;
    }

    PendingList(const PendingList& list) = delete;
    PendingList& operator=(const PendingList& list) = delete;

    /**
     * @brief Add an address and its deleter to the pending reclamation list.
     */
    void add_to_pending_list(void* p, std::function<void(void*)> deleter) {
        auto new_node = new PendingNode(p, deleter);
        add_to_pending_list(new_node);
    }

    /**
     * @brief Delete pending entries that are not referenced by hazard pointers.
     */
    void delete_unused_pending_pointers() {
        PendingNode* nodes_to_delete = pending_list_.exchange(nullptr);
        while (nodes_to_delete) {
            auto next = nodes_to_delete->next;
            if (hp_can_delete(nodes_to_delete->data)) {
                nodes_to_delete->deleter(nodes_to_delete->data);
                delete nodes_to_delete;
            } else {
                add_to_pending_list(nodes_to_delete);
            }
            nodes_to_delete = next;
        }
    }
private:
    PendingList() = default;

    /**
     * @brief Pending reclamation entry.
     */
    struct PendingNode {
        void* data;
        PendingNode* next;
        std::function<void(void*)> deleter;
        PendingNode(void* d, std::function<void(void*)> del): data(d), next(nullptr), deleter(del) {}
    };

    /**
     * @brief Push an existing pending node back onto the pending list.
     */
    void add_to_pending_list(PendingNode* node) {
        node->next = pending_list_;
        while (!pending_list_.compare_exchange_weak(node->next, node)) {
            std::this_thread::yield();
        }
    }

    std::atomic<PendingNode*> pending_list_ = nullptr;
};

HPManager::HPManager() {
    hp_ = HPTable::instance()->get_hazard_poiner();
}

HPManager::~HPManager() {
    hp_->pointer = nullptr;
    hp_->id.store(std::thread::id());
}

std::atomic<void*>& HPManager::pointer() {
    return hp_->pointer;
}

bool hp_can_delete(void* p) {
    return HPTable::instance()->can_delete_safe(p);
}

void hp_add_to_pending_list(void* p, std::function<void(void*)> deleter) {
    PendingList::instance()->add_to_pending_list(p, deleter);
}

void hp_delete_unused_pending_pointers() {
    PendingList::instance()->delete_unused_pending_pointers();
}
