#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <memory>

#include "static_vector.h"

namespace qhenki::containers
{
/**
 * Lock free stack implemented with linked lists and atomics.
 * @tparam T Type of object stored in the stack.
 * @tparam Capacity Maximum number of objects that can be stored in the stack.
 */
template<typename T, size_t Capacity> class AtomicStack
{
    StaticVector<T, Capacity> m_owned_objects;

    struct Node
    {
        T* data;
        Node* next;
    };
    std::array<Node, Capacity> m_nodes{};

    std::atomic<Node*> m_head{nullptr};

    std::atomic<Node*> m_free_nodes{nullptr};

    // Function to create new objects when stack is empty
    std::function<T()> m_factory;

public:
    explicit AtomicStack(std::function<T()> factory)
        : m_factory{std::move(factory)}
    {
        for (size_t i = 0; i < Capacity; i++)
        {
            m_nodes[i].next = i + 1 < Capacity ? &m_nodes[i + 1] : nullptr;
        }
        m_free_nodes.store(&m_nodes[0], std::memory_order_release);
    }

    ~AtomicStack() = default;

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
                // Return node to free list
                return_node_to_free_list(head);
                return data;
            }
        }

        m_owned_objects.emplace_back(m_factory());
        return &m_owned_objects.back();
    }

    /**
     * Return an object to the stack. If no free nodes are available (stack exceeds capacity), the push fails.
     * @param data Pointer to the object to push.
     * @return Whether the push succeeded.
     */
    bool push(T* data)
    {
        if (!data)
        {
            return false;
        }

        auto new_node = acquire_node_from_free_list();
        if (new_node == nullptr)
        {
            // No free nodes available, cannot push
            return false;
        }

        new_node->data = data;
        new_node->next = m_head.load(std::memory_order_acquire);
        while (!m_head.compare_exchange_weak(
            new_node->next, new_node, std::memory_order_release, std::memory_order_acquire))
        {
            // CAS failed retry with new_node->next updated to current head
        }
        return true;
    }

    // Apply a function to all objects regardless of ownership
    template<typename Func> void for_each(Func&& func)
    {
        for (auto& obj : m_owned_objects)
        {
            func(&obj);
        }
    }

private:
    Node* acquire_node_from_free_list()
    {
        auto free_head = m_free_nodes.load(std::memory_order_acquire);
        while (free_head != nullptr)
        {
            auto next = free_head->next;
            if (m_free_nodes.compare_exchange_weak(
                    free_head, next, std::memory_order_release, std::memory_order_acquire))
            {
                return free_head;
            }
        }
        return nullptr;
    }

    void return_node_to_free_list(Node* node)
    {
        node->data = nullptr;
        node->next = m_free_nodes.load(std::memory_order_acquire);
        while (
            !m_free_nodes.compare_exchange_weak(node->next, node, std::memory_order_release, std::memory_order_acquire))
        {
            // CAS failed retry with node->next updated to current free head
        }
    }
};
} // namespace qhenki::containers
