#pragma once

#include <smartpointer.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace qhenki::memory
{
class Arena
{
    struct Block
    {
        uPtr<uint8_t[]> memory;
        size_t capacity = 0;
        size_t offset = 0;
    };

    size_t m_block_size;

    std::vector<Block> m_blocks;
    size_t m_block_index = 0;

public:
    Arena() = delete;
    Arena(size_t init_block_size);
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = default;
    Arena& operator=(Arena&&) = default;
    /**
     * Caller is responsible for destroying contained objects before arena is destroyed.
     */
    ~Arena() = default;

    /**
     * Resets offset. Does not zero memory. This function does not check if you have destructors to call.
     */
    void reset();

    /**
     * Create uninitialized block of given size and alignment.
     * @param size Number of bytes to allocate.
     * @param alignment Alignment requirement for the allocation.
     * @return Pointer to the allocated memory or null on allocation failure.
     */
    void* alloc(size_t size, size_t alignment = alignof(std::max_align_t));

    /**
     * Create default initialized array of given POD type.
     * @tparam T Type of array elements.
     * @param count Number of elements in array.
     * @return Pointer to the allocated array or null if arena is full.
     */
    template<typename T> T* alloc_array(const size_t count)
    {
        static_assert(std::is_default_constructible_v<T>);
        // We don't do any bookkeeping of objects created here so we can't call destructors, hence POD only.
        static_assert(std::is_trivial_v<T> && std::is_standard_layout_v<T>);
        auto mem = alloc(count * sizeof(T), alignof(T));
        if (!mem)
        {
            return nullptr;
        }
        auto arr = static_cast<T*>(mem);
        for (size_t i = 0; i < count; i++)
        {
            std::construct_at(arr + i); // Invoke default constructor
        }
        return arr;
    }

    /**
     * Create default initialized array of given type.
     *
     * Any unique pointer created from this function must be reset before the arena is reset.
     *
     * @tparam T Type of array elements.
     * @param count Number of elements in array.
     * @return Unique pointer to the allocated array (calls destructors) or null if arena is full.
     */
    template<typename T> uPtr<T[], std::function<void(T*)>> alloc_array_managed(const size_t count)
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
            std::construct_at(arr + i); // Invoke default constructor
        }
        return uPtr<T[], std::function<void(T*)>>(arr,
                                                  [count](T* ptr)
                                                  {
                                                      for (size_t i = 0; i < count; i++)
                                                      {
                                                          std::destroy_at(ptr + i);
                                                      }
                                                  });
    }
};
} // namespace qhenki::memory
