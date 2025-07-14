#pragma once
#include <cstdint>
#include <vector>

class StackAllocator
{
	std::vector<uint8_t> m_start; // Can resize
	size_t m_offset = 0;
public:
	StackAllocator(size_t size) : m_start(size) {}
	void* allocate(size_t size);
	void deallocate(void* ptr, size_t size);
};
