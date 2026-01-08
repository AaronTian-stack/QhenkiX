#pragma once

#include <cassert>
#include <cstdint>
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
    alignas(T) std::array<uint8_t, sizeof(T) * MaxSize> m_storage;
    size_t m_size = 0;

public:
    StaticVector() = default;

    StaticVector(const StaticVector& vector)
    {
        for (size_t i = 0; i < m_size; i++)
        {
            new (&m_storage[i]) T(*reinterpret_cast<const T*>(&vector.m_storage[i]));
        }
    }

    StaticVector& operator=(const StaticVector& vector)
    {
        if (this != &vector)
        {
            for (size_t i = 0; i < m_size; i++)
            {
                new (&m_storage[i]) T(*reinterpret_cast<const T*>(&vector.m_storage[i]));
            }
        }
        return *this;
    }

    StaticVector(StaticVector&& vector) noexcept
    {
        for (size_t i = 0; i < m_size; i++)
        {
            new (&m_storage[i]) T(std::move(*reinterpret_cast<T*>(&vector.m_storage[i])));
        }
    }

    ~StaticVector()
    {
        for (size_t i = 0; i < m_size; i++)
        {
            reinterpret_cast<T*>(&m_storage[i])->~T();
        }
    }

    StaticVector& operator=(StaticVector&& vector) noexcept
    {
        if (this != &vector)
        {
            for (size_t i = 0; i < m_size; i++)
            {
                new (&m_storage[i]) T(std::move(*reinterpret_cast<T*>(&vector.m_storage[i])));
            }
        }
        vector.m_size = 0;
        return *this;
    }

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
        return reinterpret_cast<T*>(m_storage.data());
    }
    T* end()
    {
        return reinterpret_cast<T*>(m_storage.data()) + m_size;
    }
    const T* begin() const
    {
        return reinterpret_cast<const T*>(m_storage.data());
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
