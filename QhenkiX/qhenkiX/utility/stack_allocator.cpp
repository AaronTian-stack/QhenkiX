#include "qhenkiX/utility/stack_allocator.h"
#include <bit>
#include <cassert>

void* StackAllocator::allocate(size_t size)
{
	assert(size > 0);
	m_offset += size;
	if (m_offset > m_start.size())
	{
		m_start.resize(std::bit_ceil(m_offset));
	}
	return &m_start[m_offset - size];
}

void StackAllocator::deallocate(void* ptr, size_t size)
{
	assert(ptr >= m_start.data() && ptr < m_start.data() + m_start.size());
	assert(size > 0);
	m_offset -= size;
	if (m_offset < 0)
	{
		m_offset = 0; // Reset to zero if underflow occurs
	}
}
