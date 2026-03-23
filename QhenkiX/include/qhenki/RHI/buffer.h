#pragma once

#include <qhenki/utility/math_util.h>
#include <smartpointer.h>

namespace qhenki::gfx
{
enum class BufferUsage : uint8_t
{
    VERTEX = BIT(0),
    INDEX = BIT(1),
    CONSTANT = BIT(2),
    UAV = BIT(3),
    INDIRECT = BIT(4),
    COPY_SRC = BIT(5),
    COPY_DST = BIT(6),
};

constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs)
{
    using Underlying = std::underlying_type_t<BufferUsage>;
    return static_cast<BufferUsage>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
}

constexpr bool operator&(BufferUsage lhs, BufferUsage rhs)
{
    using Underlying = std::underlying_type_t<BufferUsage>;
    return (static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs)) != 0;
}

enum BufferVisibility : uint8_t
{
    // Device local memory. Can be &ed with other visibilities to try to get BAR memory
    GPU = BIT(0),
    // Host Visible: Written to by the CPU preferably sequentially
    CPU_SEQUENTIAL = BIT(1),
};

struct BufferDesc
{
    uint64_t size = 0;
    uint64_t stride = 0;
    BufferUsage usage;
    BufferVisibility visibility = GPU;
};

struct Buffer
{
    BufferDesc desc;
    sPtr<void> internal_state;
};
} // namespace qhenki::gfx
