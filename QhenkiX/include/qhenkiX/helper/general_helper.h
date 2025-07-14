#pragma once

#include <array>

namespace qhenki::util
{
    template <typename T>
    auto ptr_array(T& v)
    {
        return std::array<T*, 1>{ &v };
    }

    template <typename T, typename... Ts>
    auto ptr_array(T& v, Ts&... vs)
    {
        return std::array<T*, sizeof...(Ts) + 1>{ &v, & vs... };
    }
}
