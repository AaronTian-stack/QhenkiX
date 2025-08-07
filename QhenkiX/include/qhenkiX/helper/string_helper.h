#pragma once
#include <array>
#include <string>
#include <utf8.h>

namespace qhenki::util
{
	template <size_t StackCapacity = 512>
	class Utf8To16Scoped
	{
		union
		{
			std::array<wchar_t, StackCapacity + 1> buffer{};
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

	template <size_t StackCapacity>
	Utf8To16Scoped<StackCapacity>::Utf8To16Scoped(std::string_view input)
	{
		if (input.size() <= StackCapacity)
		{
			utf8::utf8to16(input.begin(), input.end(), buffer.begin());
			buffer[input.size()] = L'\0'; // Null-terminate the string
			m_stack_buffer = true;
		}
		else
		{
			heap.resize(input.size() + 1);
			utf8::utf8to16(input.begin(), input.end(), heap.begin());
			heap[input.size()] = L'\0'; // Null-terminate the string
			m_stack_buffer = false;
		}
	}
}
