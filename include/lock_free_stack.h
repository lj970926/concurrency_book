#include <atomic>
#include <thread>

template<typename T>
class LockFreeStack {
private:
    struct Node {
        T data;
        Node* next = nullptr;
        Node(const T& d): data(d) {}
    };
    std::atomic<Node*> head_ = nullptr;
    std::atomic<int> size_ = 0;
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
};