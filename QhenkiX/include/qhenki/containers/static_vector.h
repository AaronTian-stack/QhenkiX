#pragma once

#include <cassert>
#include <type_traits>
#include <utility>

namespace qhenki::containers
{
/**
 * Fixed size vector.
 * @tparam T Type of elements in the vector.
 * @tparam MaxSize Maximum number of elements in the vector.
 */
template<typename T, size_t MaxSize> class StaticVector
{
    std::aligned_storage_t<sizeof(T), alignof(T)> m_storage[MaxSize];
    size_t m_size = 0;

public:
    StaticVector() = default;
    ~StaticVector()
    {
        for (size_t i = 0; i < m_size; i++)
        {
            reinterpret_cast<T*>(&m_storage[i])->~T();
        }
    }

    StaticVector(const StaticVector&) = delete;
    StaticVector& operator=(const StaticVector&) = delete;
    StaticVector(StaticVector&&) = delete;
    StaticVector& operator=(StaticVector&&) = delete;

    /**
     * Constructs an element in-place at the end of the vector.
     * @param args Arguments to forward to the constructor of T.
     * @return true if the element was successfully added, false if the vector is at capacity.
     */
    template<typename... Args> bool emplace_back(Args&&... args)
    {
        if (m_size >= MaxSize)
        {
            return false;
        }
        new (&m_storage[m_size]) T(std::forward<Args>(args)...);
        ++m_size;
        return true;
    }

    T& back()
    {
        assert(m_size > 0);
        return *reinterpret_cast<T*>(&m_storage[m_size - 1]);
    }

    const T& back() const
    {
        assert(m_size > 0);
        return *reinterpret_cast<const T*>(&m_storage[m_size - 1]);
    }

    T* begin()
    {
        return reinterpret_cast<T*>(m_storage);
    }
    T* end()
    {
        return reinterpret_cast<T*>(m_storage) + m_size;
    }
    const T* begin() const
    {
        return reinterpret_cast<const T*>(m_storage);
    }
    const T* end() const
    {
        return reinterpret_cast<const T*>(m_storage) + m_size;
    }
    const T* cbegin() const
    {
        return begin();
    }
    const T* cend() const
    {
        return end();
    }

    size_t size() const
    {
        return m_size;
    }
    bool empty() const
    {
        return m_size == 0;
    }
};
} // namespace qhenki::containers
