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
}

void* Arena::alloc(const size_t size, const size_t alignment)
{
    assert(size);
    assert(alignment && util::is_power_of_two(alignment));

    auto place_in_block = [&](Block& block) -> void*
    {
        const auto current_address = reinterpret_cast<size_t>(block.memory.get()) + block.offset;
        const auto aligned_address = util::align_u(current_address, alignment);
        const auto padding = aligned_address - current_address;
        const auto total_size = size + padding;
        if (block.offset + total_size <= block.capacity)
        {
            void* ptr = block.memory.get() + block.offset + padding;
            block.offset += total_size;
            return ptr;
        }
        return nullptr;
    };

    for (auto& block : m_blocks)
    {
        if (const auto ptr = place_in_block(block))
        {
            return ptr;
        }
    }

    const auto required_capacity = size + alignment - 1;
    while (required_capacity > m_block_size)
    {
        m_block_size *= 2;
    }

    auto& new_block = m_blocks.emplace_back(mkU<uint8_t[]>(m_block_size), m_block_size, 0);

    const auto ptr = place_in_block(new_block);
    assert(ptr);
    return ptr;
}
