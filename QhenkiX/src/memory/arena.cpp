#include "qhenki/memory/arena.h"

#include <cassert>

#include "qhenki/utility/math_util.h"
#include "smartpointer.h"

using namespace qhenki::memory;

Arena::Arena(const size_t init_block_size)
    : m_block_size(init_block_size ? init_block_size : alignof(std::max_align_t))
{
    assert(init_block_size);
    m_blocks.emplace_back(mkU<uint8_t[]>(m_block_size), m_block_size, 0);
}

void Arena::reset()
{
    for (auto& block : m_blocks)
    {
        block.offset = 0;
    }
    m_block_index = 0;
}

void* Arena::alloc(const size_t size, const size_t alignment)
{
    assert(size);
    assert(alignment && util::is_power_of_two(alignment));

    auto& current_block = m_blocks[m_block_index];

    const auto current_address = reinterpret_cast<size_t>(current_block.memory.get()) + current_block.offset;
    const auto aligned_address = util::align_u(current_address, alignment);
    const auto padding = aligned_address - current_address;
    const auto total_size = size + padding;
    if (current_block.offset + total_size > current_block.capacity)
    {
        while (total_size > m_block_size)
        {
            m_block_size = util::align_u(total_size, m_block_size);
        }

        if (++m_block_index >= m_blocks.size())
        {
            m_blocks.emplace_back(mkU<uint8_t[]>(m_block_size), m_block_size, 0);
        }

        return alloc(size, alignment);
    }
    current_block.offset += padding;
    void* ptr = current_block.memory.get() + current_block.offset;
    current_block.offset += size;

    return ptr;
}
