#include "qhenki/memory/arena.h"

#include <cassert>

#include "qhenki/utility/math_util.h"
#include "smartpointer.h"

using namespace qhenki::memory;

Arena::Arena(const size_t capacity)
    : m_memory{mkU<uint8_t[]>(capacity)},
      m_capacity{capacity}
{
}

void Arena::reset()
{
    m_offset = 0;
}

bool Arena::init(const size_t capacity)
{
    assert(capacity);
    if (m_memory)
    {
        assert(false);
        return false;
    }
    m_memory = mkU<uint8_t[]>(capacity);
    m_capacity = capacity;
    m_offset = 0;
    return true;
}

void* Arena::alloc(const size_t size, const size_t alignment)
{
    assert(size);
    assert(alignment);
    assert(m_memory);

    const auto current_address = reinterpret_cast<size_t>(m_memory.get()) + m_offset;
    const auto aligned_address = util::align_u(current_address, alignment);
    const auto padding = aligned_address - current_address;
    const auto total_size = size + padding;
    if (m_offset + total_size > m_capacity)
    {
        // TODO: Need some growing strategy that supports contiguous memory
        // Since separate allocs are not probably not related easiest solution would probably be a linked list of
        // blocks. Then this failed alloc would just go into the next block.
        return nullptr;
    }
    m_offset += padding;
    void* ptr = m_memory.get() + m_offset;
    m_offset += size;

    return ptr;
}
