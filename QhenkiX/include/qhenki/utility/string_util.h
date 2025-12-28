#pragma once

#include <utf8.h>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace qhenki::util
{
template<size_t StackCapacity = 256> class Utf8To16Scoped
{
    union
    {
        std::array<wchar_t, StackCapacity> buffer{};
        std::wstring heap;
    };
    const wchar_t* m_ptr = nullptr;
    bool m_stack_buffer;

public:
    explicit Utf8To16Scoped(std::string_view input);

    const wchar_t* c_str() const
    {
        if (m_stack_buffer)
        {
            return buffer.data();
        }
        return heap.c_str();
    }

    Utf8To16Scoped(const Utf8To16Scoped&) = delete;
    Utf8To16Scoped& operator=(const Utf8To16Scoped&) = delete;
    Utf8To16Scoped(Utf8To16Scoped&&) = delete;
    Utf8To16Scoped& operator=(Utf8To16Scoped&&) = delete;

    ~Utf8To16Scoped()
    {
        if (!m_stack_buffer)
        {
            heap.~basic_string();
        }
    }
};

template<size_t StackCapacity> Utf8To16Scoped<StackCapacity>::Utf8To16Scoped(std::string_view input)
{
    if (input.size() <= StackCapacity)
    {
        utf8::utf8to16(input.begin(), input.end(), buffer.begin());
        buffer[input.size()] = L'\0';
        m_stack_buffer = true;
    }
    else
    {
        heap.resize(input.size() + 1);
        utf8::utf8to16(input.begin(), input.end(), heap.begin());
        heap[input.size()] = L'\0';
        m_stack_buffer = false;
    }
}

template<size_t N> struct FormatResult
{
    std::array<char, N> buffer;
    bool truncated;
};

template<size_t N> struct FormatWResult
{
    std::array<wchar_t, N> buffer;
    bool truncated;
};

template<size_t N = 256> FormatResult<N> format_string(const char* fmt, ...)
{
    FormatResult<N> result{};
    va_list args;
    va_start(args, fmt);
    const int written = std::vsnprintf(result.buffer.data(), N, fmt, args);
    va_end(args);
    result.truncated = (written < 0) || (static_cast<size_t>(written) >= N);
    return result;
}

template<size_t N = 256> FormatWResult<N> format_wstring(const wchar_t* fmt, ...)
{
    FormatWResult<N> result{};
    va_list args;
    va_start(args, fmt);
    const int written = std::vswprintf(result.buffer.data(), N, fmt, args);
    va_end(args);
    result.truncated = (written < 0) || (static_cast<size_t>(written) >= N);
    return result;
}
} // namespace qhenki::util
