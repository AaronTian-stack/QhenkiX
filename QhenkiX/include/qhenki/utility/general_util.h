#pragma once

#include <array>
#include <stdexcept>

namespace qhenki::util
{
template<typename T> auto ptr_array(T& v)
{
    return std::array<T*, 1>{&v};
}

template<typename T, typename... Ts> auto ptr_array(T& v, Ts&... vs)
{
    return std::array<T*, sizeof...(Ts) + 1>{&v, &vs...};
}
} // namespace qhenki::util

#define THROW_IF_FALSE(result)                                   \
    do                                                           \
    {                                                            \
        if (!result)                                             \
        {                                                        \
            throw std::runtime_error("Something went wrong!\n"); \
        }                                                        \
    } while (0)

#define THROW_IF_TRUE(result)                                    \
    do                                                           \
    {                                                            \
        if ((result))                                            \
        {                                                        \
            throw std::runtime_error("Something went wrong!\n"); \
        }                                                        \
    } while (0)
