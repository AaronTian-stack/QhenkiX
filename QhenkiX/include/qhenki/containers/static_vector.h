#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace qhenki::containers
{
template<typename T, size_t MaxSize> class StaticVector
{
    static_assert(MaxSize > 0);
    alignas(T) std::array<std::byte, sizeof(T) * MaxSize> m_storage;
    size_t m_size = 0;

public:
    StaticVector() = default;

    StaticVector(const StaticVector& vector)
        : m_size(vector.m_size)
    {
        for (size_t i = 0; i < m_size; i++)
        {
            std::construct_at(begin() + i, vector[i]);
        }
    }

    StaticVector& operator=(const StaticVector& vector)
    {
        if (this != &vector)
        {
            for (size_t i = 0; i < m_size; i++)
            {
                std::destroy_at(begin() + i);
            }
            m_size = vector.m_size;
            for (size_t i = 0; i < m_size; i++)
            {
                std::construct_at(begin() + i, vector[i]);
            }
        }
        return *this;
    }

    StaticVector(StaticVector&& vector)
        : m_size(vector.m_size)
    {
        for (size_t i = 0; i < m_size; i++)
        {
            std::construct_at(begin() + i, std::move(vector[i]));
        }
        vector.m_size = 0;
    }

    ~StaticVector()
    {
        for (size_t i = 0; i < m_size; i++)
        {
            std::destroy_at(begin() + i);
        }
    }

    StaticVector& operator=(StaticVector&& vector)
    {
        if (this != &vector)
        {
            for (size_t i = 0; i < m_size; i++)
            {
                std::destroy_at(begin() + i);
            }
            m_size = vector.m_size;
            for (size_t i = 0; i < m_size; i++)
            {
                std::construct_at(begin() + i, std::move(vector[i]));
            }
            vector.m_size = 0;
        }
        return *this;
    }

    T& operator[](size_t index)
    {
        check_index(index);
        return reinterpret_cast<T*>(m_storage.data())[index];
    }

    const T& operator[](size_t index) const
    {
        check_index(index);
        return reinterpret_cast<const T*>(m_storage.data())[index];
    }

#define CHECK_SIZE()           \
    do                         \
    {                          \
        if (m_size >= MaxSize) \
        {                      \
            return false;      \
        }                      \
    } while (0)

    bool push_back(const T& value)
    {
        CHECK_SIZE();
        std::construct_at(begin() + m_size, value);
        ++m_size;
        return true;
    }

    bool push_back(T&& value)
    {
        CHECK_SIZE();
        std::construct_at(begin() + m_size, std::move(value));
        ++m_size;
        return true;
    }

    template<typename... Args> bool emplace_back(Args&&... args)
    {
        CHECK_SIZE();
        std::construct_at(begin() + m_size, std::forward<Args>(args)...);
        ++m_size;
        return true;
    }

#undef CHECK_SIZE

    T& back()
    {
#if HAS_EXCEPTIONS
        if (empty())
        {
            throw std::out_of_range("StaticVector::back(): empty vector");
        }
#else
        assert(m_size > 0);
#endif
        return *(begin() + m_size - 1);
    }

    const T& back() const
    {
#if HAS_EXCEPTIONS
        if (empty())
        {
            throw std::out_of_range("StaticVector::back(): empty vector");
        }
#else
        assert(m_size > 0);
#endif
        return *(begin() + m_size - 1);
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
        return reinterpret_cast<const T*>(m_storage.data()) + m_size;
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

private:
    void check_index(const size_t index) const
    {
#if HAS_EXCEPTIONS
        if (index >= m_size)
        {
            throw std::out_of_range("StaticVector::operator[]: index out of range");
        }
#else
        assert(index < m_size);
#endif
    }
};
} // namespace qhenki::containers
