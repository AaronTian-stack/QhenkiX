#pragma once

#include <cstdint>

#include <qhenki/utility/math_util.h>
#include "subresource.h"

namespace qhenki::gfx
{
// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#synchronization
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineStageFlagBits.html
enum SyncStage : uint32_t
{
    SYNC_NONE = 0,
    SYNC_ALL = BIT(0),
    SYNC_DRAW = BIT(1),
    SYNC_INDEX_INPUT = BIT(2),
    SYNC_VERTEX_SHADING = BIT(3),
    SYNC_PIXEL_SHADING = BIT(4),
    SYNC_DEPTH_STENCIL = BIT(5),
    SYNC_RENDER_TARGET = BIT(6),
    SYNC_COMPUTE_SHADING = BIT(7),
    SYNC_RAYTRACING = BIT(8),
    SYNC_COPY = BIT(9),
    SYNC_RESOLVE = BIT(10),
    SYNC_EXECUTE_INDIRECT = BIT(11),
    SYNC_PREDICATION = BIT(12),
    SYNC_ALL_SHADING = BIT(13),
    SYNC_NON_PIXEL_SHADING = BIT(14),
    SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO = BIT(15),
    SYNC_CLEAR_UNORDERED_ACCESS_VIEW = BIT(16),
    SYNC_VIDEO_DECODE = BIT(17),
    SYNC_VIDEO_PROCESS = BIT(18),
    SYNC_VIDEO_ENCODE = BIT(19),
    SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE = BIT(20),
    SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE = BIT(21),
};
// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#access-transitions
// https://registry.khronos.org/vulkan/specs/latest/man/html/VkAccessFlagBits2.html
enum AccessFlags : uint32_t
{
    ACCESS_COMMON = BIT(0),
    ACCESS_VERTEX_BUFFER = BIT(1),
    ACCESS_UNIFORM_BUFFER = BIT(2),
    ACCESS_INDEX_BUFFER = BIT(3),
    ACCESS_RENDER_TARGET = BIT(4),
    ACCESS_STORAGE_ACCESS = BIT(5),
    ACCESS_DEPTH_STENCIL_WRITE = BIT(6),
    ACCESS_DEPTH_STENCIL_READ = BIT(7),
    ACCESS_SHADER_RESOURCE = BIT(8),
    ACCESS_INDIRECT_ARGUMENT = BIT(10),
    ACCESS_COPY_DEST = BIT(12),
    ACCESS_COPY_SOURCE = BIT(13),
    ACCESS_RESOLVE_DEST = BIT(14),
    ACCESS_RESOLVE_SOURCE = BIT(15),
    ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ = BIT(16),
    ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE = BIT(17),
    ACCESS_SHADING_RATE_SOURCE = BIT(18),
    ACCESS_VIDEO_DECODE_READ = BIT(19),
    ACCESS_VIDEO_DECODE_WRITE = BIT(20),
    ACCESS_VIDEO_PROCESS_READ = BIT(21),
    ACCESS_VIDEO_PROCESS_WRITE = BIT(22),
    ACCESS_VIDEO_ENCODE_READ = BIT(23),
    ACCESS_VIDEO_ENCODE_WRITE = BIT(24),
    NO_ACCESS = BIT(25)
};
// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#equivalent-d3d12_barrier_layout-for-each-d3d12_resource_states-bit
enum class Layout
{
    UNDEFINED,
    COMMON,
    PRESENT,
    LAYOUT_GENERIC_READ,
    RENDER_TARGET,
    UNORDERED_ACCESS,
    DEPTH_STENCIL_WRITE,
    DEPTH_STENCIL_READ,
    SHADER_RESOURCE,
    COPY_SOURCE,
    COPY_DEST,
    RESOLVE_SOURCE,
    RESOLVE_DEST,
    SHADING_RATE_SOURCE,
    VIDEO_DECODE_READ,
    VIDEO_DECODE_WRITE,
    VIDEO_PROCESS_READ,
    VIDEO_PROCESS_WRITE,
    VIDEO_ENCODE_READ,
    VIDEO_ENCODE_WRITE,
    DIRECT_QUEUE_COMMON,
    DIRECT_QUEUE_GENERIC_READ,
    DIRECT_QUEUE_UNORDERED_ACCESS,
    DIRECT_QUEUE_SHADER_RESOURCE,
    DIRECT_QUEUE_COPY_SOURCE,
    DIRECT_QUEUE_COPY_DEST,
    COMPUTE_QUEUE_COMMON,
    COMPUTE_QUEUE_GENERIC_READ,
    COMPUTE_QUEUE_UNORDERED_ACCESS,
    COMPUTE_QUEUE_SHADER_RESOURCE,
    COMPUTE_QUEUE_COPY_SOURCE,
    COMPUTE_QUEUE_COPY_DEST,
    COUNT,
};

struct LayoutSet
{
    uint64_t mask = 0;

    constexpr bool contains(const Layout layout) const
    {
        return (mask & bit(layout)) != 0;
    }

    constexpr LayoutSet& operator|=(const Layout layout)
    {
        mask |= bit(layout);
        return *this;
    }
    constexpr LayoutSet& operator|=(const LayoutSet rhs)
    {
        mask |= rhs.mask;
        return *this;
    }

    friend constexpr LayoutSet operator|(LayoutSet lhs, const LayoutSet rhs)
    {
        lhs |= rhs;
        return lhs;
    }
    friend constexpr LayoutSet operator|(LayoutSet lhs, const Layout rhs)
    {
        lhs |= rhs;
        return lhs;
    }
    friend constexpr LayoutSet operator|(const Layout lhs, const Layout rhs)
    {
        LayoutSet s{};
        s |= lhs;
        s |= rhs;
        return s;
    }

private:
    static constexpr uint64_t bit(Layout layout)
    {
        return 1ull << static_cast<uint32_t>(layout);
    }
};

static_assert(static_cast<uint32_t>(Layout::COUNT) <= 64);

struct ImageBarrier
{
    void* resource = nullptr;
    bool discard = false;
    // Sync Flags
    SyncStage src_stage = SYNC_NONE;
    SyncStage dst_stage = SYNC_NONE;
    // Access Flags
    AccessFlags src_access = ACCESS_COMMON;
    AccessFlags dst_access = ACCESS_COMMON;
    // Layout
    Layout src_layout = Layout::COMMON;
    Layout dst_layout = Layout::COMMON;
    // Subresource range
    ImageSubresourceRange subresource_range;
};

// struct MemoryBarrier
//{

//};
// struct BufferBarrier
//{
//
//};
} // namespace qhenki::gfx
