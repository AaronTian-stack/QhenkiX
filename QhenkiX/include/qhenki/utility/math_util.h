#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>

namespace qhenki::util
{
constexpr uint64_t KILOBYTE = 1024;
constexpr uint64_t MEGABYTE = KILOBYTE * 1024;

#define BIT(x) (1 << (x))

template<typename T>
    requires std::unsigned_integral<T>
bool is_power_of_two(const T value)
{
    return (value & (value - 1)) == 0;
}

template<typename T>
    requires std::unsigned_integral<T>
constexpr T align_up(const T value, const T alignment)
{
    if (alignment == 0)
    {
        return value;
    }
    assert(is_power_of_two(alignment));
    return (value + alignment - 1) & ~(alignment - 1);
}

template<typename T>
    requires std::unsigned_integral<T>
constexpr T align_up_non_power_of_two(const T value, const T alignment)
{
    if (alignment == 0)
    {
        return value;
    }
    return (value + alignment - 1) / alignment * alignment;
}

template<typename T>
    requires std::unsigned_integral<T>
constexpr T ceil_div(const T value, const T divisor)
{
    return (value + divisor - 1) / divisor;
}
}; // namespace qhenki::util
