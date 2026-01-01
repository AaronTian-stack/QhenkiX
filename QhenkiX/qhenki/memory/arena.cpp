#include "qhenki/memory/arena.h"

#include <cassert>

#include "qhenki/utility/math_util.h"
#include "smartpointer.h"

using namespace qhenki::memory;

Arena::Arena(const size_t capacity)
    : m_memory{mkU<uint8_t[]>(capacity)},
      m_capacity{capacity},
      m_offset(0)
{
}

void Arena::reset()
{
    m_offset = 0;
}

void* Arena::alloc(const size_t size, const size_t alignment)
{
    assert(size);
    assert(alignment);

    const auto current_address = reinterpret_cast<size_t>(m_memory.get()) + m_offset;
    const auto aligned_address = util::align_u(current_address, alignment);
    const auto padding = aligned_address - current_address;
    const auto total_size = size + padding;
    if (m_offset + total_size > m_capacity)
    {
        return nullptr;
    }
    m_offset += padding;
    void* ptr = m_memory.get() + m_offset;
    m_offset += size;

    return ptr;
}
