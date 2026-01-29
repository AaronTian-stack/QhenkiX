#pragma once

#include <cassert>
#include <concepts>

namespace qhenki::util
{
constexpr uint64_t CONSTANT_BUFFER_ALIGNMENT = 256;
constexpr uint64_t KILOBYTE = 1024;
constexpr uint64_t MEGABYTE = KILOBYTE * 1024;

#define BIT(x) (1 << (x))

inline bool is_power_of_two(const uint32_t value)
{
    return (value & (value - 1)) == 0;
}

template<typename T>
    requires std::unsigned_integral<T>
constexpr T align_u(const T size, const T alignment)
{
    assert(is_power_of_two(alignment));
    return (size + alignment - 1) & ~(alignment - 1);
}

template<typename T>
    requires std::unsigned_integral<T>
constexpr T ceil_div(const T value, const T divisor)
{
    return (value + divisor - 1) / divisor;
}
}; // namespace qhenki::util
