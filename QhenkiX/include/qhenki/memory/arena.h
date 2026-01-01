#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace qhenki::memory
{
class Arena
{
    std::unique_ptr<uint8_t[]> m_memory;
    size_t m_capacity;
    size_t m_offset;

public:
    Arena(size_t capacity);

    /**
     * Resets offset. Does not zero memory.
     */
    void reset();

    /**
     * Create uninitialized block of given size and alignment
     * @param size Number of bytes to allocate
     * @param alignment Alignment requirement for the allocation
     * @return Pointer to the allocated memory or null if arena is full
     */
    void* alloc(size_t size, size_t alignment = alignof(std::max_align_t));

    /**
     * Create default initialized array of given type
     * @tparam T Type of array elements
     * @param count Number of elements in array
     * @return Pointer to the allocated array or null if arena is full
     */
    template<typename T> T* alloc_array(const size_t count)
    {
        static_assert(std::is_default_constructible_v<T>);
        auto mem = alloc(count * sizeof(T), alignof(T));
        if (!mem)
        {
            return nullptr;
        }
        auto arr = static_cast<T*>(mem);
        for (size_t i = 0; i < count; i++)
        {
            new (&arr[i]) T(); // Invoke default constructor
        }
        return arr;
    }
};
} // namespace qhenki::memory
