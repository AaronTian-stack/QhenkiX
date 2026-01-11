#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <memory>

namespace qhenki::containers
{
/**
 * Lock free stack implemented with linked lists and atomics.
 * @tparam T Type of object stored in the stack.
 * @tparam Capacity Maximum number of objects that can be stored in the stack.
 */
template<typename T, size_t Capacity> class AtomicStack
{
    struct Node
    {
        T* data;
        Node* next;
    };

    struct TaggedPtr
    {
        Node* ptr;
        // In case the same node gets returned but now with a different next node. Incremented when pushing.
        uintptr_t tag;
        // Example
        // Current state of m_head: A -> B -> C
        // Thread 1 is about to pop A (next B)
        // Thread 2 does pop() pop() and returns A
        // Current state of m_head: A -> C
        // Thread 1 A.next (next of head node) is outdated and would set head to B (resulting in B -> C)
        // But tags don't match (Thread 2 returned A with incremented tag) and this is prevented
        // Thread 1 can properly update it's local head node with CAS
        bool operator==(const TaggedPtr&) const = default;
    };

    alignas(T) std::array<std::byte, sizeof(T) * Capacity> m_storage;
    std::atomic<size_t> m_size{0};

    std::array<Node, Capacity> m_nodes{};

    // Available Node structs that can be associated with some T* to avoid heap allocating Nodes
    std::atomic<TaggedPtr> m_free_nodes{};
    // Nodes that can be popped
    std::atomic<TaggedPtr> m_head{};

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
        m_free_nodes.store(TaggedPtr{&m_nodes[0], 0}, std::memory_order_release);
    }

    ~AtomicStack()
    {
        T* objects = reinterpret_cast<T*>(m_storage.data());
        const auto count = m_size.load(std::memory_order_acquire);
        for (size_t i = 0; i < count; i++)
        {
            std::destroy_at(&objects[i]);
        }
    }

    AtomicStack(const AtomicStack&) = delete;
    AtomicStack& operator=(const AtomicStack&) = delete;
    AtomicStack(AtomicStack&&) = delete;
    AtomicStack& operator=(AtomicStack&&) = delete;

    /**
     * Removes and returns an object from the stack, or creates a new one if the stack is empty.
     * @return Pointer to the object, or nullptr if the stack is empty and capacity is reached.
     */
    T* pop()
    {
        // Popping either grabs node from m_head that already has a T constructed, or if m_head list empty return a
        // pointer to newly constructed T stored in m_storage. If it is from m_head, the node is then moved into
        // m_free_nodes so that it can be associated with a new T*.

        TaggedPtr head = m_head.load(std::memory_order_acquire);

        // Reuse T from head case
        while (head.ptr != nullptr)
        {
            auto head_node = head.ptr;
            TaggedPtr new_head{head_node->next, head.tag + 1};

            if (m_head.compare_exchange_weak(head, new_head, std::memory_order_release, std::memory_order_acquire))
            {
                T* data = head_node->data;
                return_node_to_free_list(head_node);
                return data;
            }
            // head is updated by compare_exchange_weak on failure
        }

        // Stack is empty, create a new object
        auto index = m_size.fetch_add(1, std::memory_order_acq_rel);
        if (index >= Capacity)
        {
            m_size.fetch_sub(1, std::memory_order_release);
            return nullptr;
        }

        T* objects = reinterpret_cast<T*>(m_storage.data());
        return std::construct_at(&objects[index], m_factory());
    }

    /**
     * Return an object to the stack. If no free nodes are available (stack exceeds capacity), the push fails.
     * @param data Pointer to the object to push.
     * @return Whether the push succeeded.
     */
    bool push(T* data)
    {
        // Get a node from m_free_nodes to associate with data and then push it into m_head for popping.

        if (!data)
        {
            return false;
        }

        auto new_node = acquire_node_from_free_list();
        if (new_node == nullptr)
        {
            // No free nodes available, cannot push
            // This should never happen assuming the user only pushes objects previously popped from this stack
#if HAS_EXCEPTIONS
            throw std::runtime_error("No free nodes available");
#else
            return false;
#endif
        }

        new_node->data = data;

        auto head = m_head.load(std::memory_order_acquire);
        while (true)
        {
            new_node->next = head.ptr;
            const TaggedPtr new_head{new_node, head.tag + 1};

            if (m_head.compare_exchange_weak(head, new_head, std::memory_order_release, std::memory_order_acquire))
            {
                return true;
            }
        }
    }

    // Apply a function to all objects regardless of ownership. Not thread safe
    template<typename Func> void for_each(Func&& func)
    {
        T* objects = reinterpret_cast<T*>(m_storage.data());
        const auto count = m_size.load(std::memory_order_acquire);
        for (size_t i = 0; i < count; i++)
        {
            func(&objects[i]);
        }
    }

private:
    Node* acquire_node_from_free_list()
    {
        TaggedPtr head = m_free_nodes.load(std::memory_order_acquire);

        while (head.ptr != nullptr)
        {
            auto head_node = head.ptr;
            const TaggedPtr new_head{head_node->next, head.tag + 1};

            if (m_free_nodes.compare_exchange_weak(
                    head, new_head, std::memory_order_release, std::memory_order_acquire))
            {
                return head_node;
            }
        }
        return nullptr;
    }

    void return_node_to_free_list(Node* node)
    {
        node->data = nullptr;

        auto head = m_free_nodes.load(std::memory_order_acquire);
        while (true)
        {
            node->next = head.ptr;
            const TaggedPtr new_head{node, head.tag + 1};

            if (m_free_nodes.compare_exchange_weak(
                    head, new_head, std::memory_order_release, std::memory_order_acquire))
            {
                return;
            }
        }
    }
};
} // namespace qhenki::containers
