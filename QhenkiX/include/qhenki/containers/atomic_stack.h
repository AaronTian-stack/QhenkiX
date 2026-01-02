#pragma once

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <vector>

namespace qhenki::containers
{
// Lock free stack using linked list and atomics. Do not try to allocate lots of objects with this.
template<typename T> class AtomicStack
{
    struct Node
    {
        T* data;
        Node* next;
    };
    std::atomic<Node*> m_head{nullptr};

    std::vector<std::unique_ptr<T>> m_owned_objects;

    // Function to create new objects when stack is empty
    std::function<std::unique_ptr<T>()> m_factory;

public:
    explicit AtomicStack(std::function<std::unique_ptr<T>()> factory)
        : m_factory{std::move(factory)}
    {
    }

    ~AtomicStack()
    {
        auto current = m_head.load(std::memory_order_relaxed);
        while (current != nullptr)
        {
            auto next = current->next;
            delete current;
            current = next;
        }
    }

    AtomicStack(const AtomicStack&) = delete;
    AtomicStack& operator=(const AtomicStack&) = delete;
    AtomicStack(AtomicStack&&) = delete;
    AtomicStack& operator=(AtomicStack&&) = delete;

    T* pop()
    {
        auto head = m_head.load(std::memory_order_acquire);

        while (head != nullptr)
        {
            auto next = head->next;
            // If another thread has not changed the head, update it to next
            if (m_head.compare_exchange_weak(head, next, std::memory_order_release, std::memory_order_acquire))
            {
                auto data = head->data;
                delete head;
                return data;
            }
        }

        return create_object();
    }

    void push(T* data)
    {
        assert(data);

        auto const new_node = new Node{data, nullptr};

        new_node->next = m_head.load(std::memory_order_acquire);
        while (!m_head.compare_exchange_weak(
            new_node->next, new_node, std::memory_order_release, std::memory_order_acquire))
        {
            // CAS failed retry with new_node->next updated to current heads
        }
    }

    // Apply a function to all objects regardless of ownership
    template<typename Func>
    void for_each(Func&& func)
    {
        for (auto& obj : m_owned_objects)
        {
            func(obj.get());
        }
    }

private:
    T* create_object()
    {
        auto obj = m_factory();
        auto obj_ptr = obj.get();
        m_owned_objects.push_back(std::move(obj));
        return obj_ptr;
    }
};
} // namespace qhenki::containers
