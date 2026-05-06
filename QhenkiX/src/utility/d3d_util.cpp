#include "d3d_util.h"

#include <magic_enum/magic_enum.hpp>
#include "qhenki/rhi/shader.h"

namespace qhenki::gfx
{
#define SYNC_STAGE_MAP(X)                                                                                       \
    X(SYNC_NONE, D3D12_BARRIER_SYNC_NONE)                                                                       \
    X(SYNC_ALL, D3D12_BARRIER_SYNC_ALL)                                                                         \
    X(SYNC_DRAW, D3D12_BARRIER_SYNC_DRAW)                                                                       \
    X(SYNC_INDEX_INPUT, D3D12_BARRIER_SYNC_INDEX_INPUT)                                                         \
    X(SYNC_VERTEX_SHADING, D3D12_BARRIER_SYNC_VERTEX_SHADING)                                                   \
    X(SYNC_PIXEL_SHADING, D3D12_BARRIER_SYNC_PIXEL_SHADING)                                                     \
    X(SYNC_DEPTH_STENCIL, D3D12_BARRIER_SYNC_DEPTH_STENCIL)                                                     \
    X(SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_RENDER_TARGET)                                                     \
    X(SYNC_COMPUTE_SHADING, D3D12_BARRIER_SYNC_COMPUTE_SHADING)                                                 \
    X(SYNC_RAYTRACING, D3D12_BARRIER_SYNC_RAYTRACING)                                                           \
    X(SYNC_COPY, D3D12_BARRIER_SYNC_COPY)                                                                       \
    X(SYNC_RESOLVE, D3D12_BARRIER_SYNC_RESOLVE)                                                                 \
    X(SYNC_EXECUTE_INDIRECT, D3D12_BARRIER_SYNC_EXECUTE_INDIRECT)                                               \
    X(SYNC_PREDICATION, D3D12_BARRIER_SYNC_PREDICATION)                                                         \
    X(SYNC_ALL_SHADING, D3D12_BARRIER_SYNC_ALL_SHADING)                                                         \
    X(SYNC_NON_PIXEL_SHADING, D3D12_BARRIER_SYNC_NON_PIXEL_SHADING)                                             \
    X(SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO,                                               \
      D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO)                                 \
    X(SYNC_CLEAR_UNORDERED_ACCESS_VIEW, D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW)                         \
    X(SYNC_VIDEO_DECODE, D3D12_BARRIER_SYNC_VIDEO_DECODE)                                                       \
    X(SYNC_VIDEO_PROCESS, D3D12_BARRIER_SYNC_VIDEO_PROCESS)                                                     \
    X(SYNC_VIDEO_ENCODE, D3D12_BARRIER_SYNC_VIDEO_ENCODE)                                                       \
    X(SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE) \
    X(SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE)

#define ACCESS_FLAGS_MAP(X)                                                                                         \
    X(ACCESS_COMMON, D3D12_BARRIER_ACCESS_COMMON)                                                                   \
    X(ACCESS_VERTEX_BUFFER, D3D12_BARRIER_ACCESS_VERTEX_BUFFER)                                                     \
    X(ACCESS_UNIFORM_BUFFER, D3D12_BARRIER_ACCESS_CONSTANT_BUFFER)                                                  \
    X(ACCESS_INDEX_BUFFER, D3D12_BARRIER_ACCESS_INDEX_BUFFER)                                                       \
    X(ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_RENDER_TARGET)                                                     \
    X(ACCESS_STORAGE_ACCESS, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS)                                                 \
    X(ACCESS_DEPTH_STENCIL_WRITE, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE)                                         \
    X(ACCESS_DEPTH_STENCIL_READ, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ)                                           \
    X(ACCESS_SHADER_RESOURCE, D3D12_BARRIER_ACCESS_SHADER_RESOURCE)                                                 \
    X(ACCESS_INDIRECT_ARGUMENT, D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT)                                             \
    X(ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_COPY_DEST)                                                             \
    X(ACCESS_COPY_SOURCE, D3D12_BARRIER_ACCESS_COPY_SOURCE)                                                         \
    X(ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ, D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ)   \
    X(ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE, D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE) \
    X(ACCESS_SHADING_RATE_SOURCE, D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE)                                         \
    X(ACCESS_VIDEO_DECODE_READ, D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ)                                             \
    X(ACCESS_VIDEO_DECODE_WRITE, D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE)                                           \
    X(ACCESS_VIDEO_PROCESS_READ, D3D12_BARRIER_ACCESS_VIDEO_PROCESS_READ)                                           \
    X(ACCESS_VIDEO_PROCESS_WRITE, D3D12_BARRIER_ACCESS_VIDEO_PROCESS_WRITE)                                         \
    X(ACCESS_VIDEO_ENCODE_READ, D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ)                                             \
    X(ACCESS_VIDEO_ENCODE_WRITE, D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE)                                           \
    X(NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS)

#define LAYOUT_MAP(X)                                                                      \
    X(UNDEFINED, D3D12_BARRIER_LAYOUT_UNDEFINED)                                           \
    X(COMMON, D3D12_BARRIER_LAYOUT_COMMON)                                                 \
    X(PRESENT, D3D12_BARRIER_LAYOUT_PRESENT)                                               \
    X(LAYOUT_GENERIC_READ, D3D12_BARRIER_LAYOUT_GENERIC_READ)                              \
    X(RENDER_TARGET, D3D12_BARRIER_LAYOUT_RENDER_TARGET)                                   \
    X(UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS)                             \
    X(DEPTH_STENCIL_WRITE, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE)                       \
    X(DEPTH_STENCIL_READ, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ)                         \
    X(SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE)                               \
    X(COPY_SOURCE, D3D12_BARRIER_LAYOUT_COPY_SOURCE)                                       \
    X(COPY_DEST, D3D12_BARRIER_LAYOUT_COPY_DEST)                                           \
    X(RESOLVE_SOURCE, D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE)                                 \
    X(RESOLVE_DEST, D3D12_BARRIER_LAYOUT_RESOLVE_DEST)                                     \
    X(SHADING_RATE_SOURCE, D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE)                       \
    X(VIDEO_DECODE_READ, D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ)                           \
    X(VIDEO_DECODE_WRITE, D3D12_BARRIER_LAYOUT_VIDEO_DECODE_WRITE)                         \
    X(VIDEO_PROCESS_READ, D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_READ)                         \
    X(VIDEO_PROCESS_WRITE, D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_WRITE)                       \
    X(VIDEO_ENCODE_READ, D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_READ)                           \
    X(VIDEO_ENCODE_WRITE, D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_WRITE)                         \
    X(DIRECT_QUEUE_COMMON, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON)                       \
    X(DIRECT_QUEUE_GENERIC_READ, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ)           \
    X(DIRECT_QUEUE_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS)   \
    X(DIRECT_QUEUE_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE)     \
    X(DIRECT_QUEUE_COPY_SOURCE, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE)             \
    X(DIRECT_QUEUE_COPY_DEST, D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST)                 \
    X(COMPUTE_QUEUE_COMMON, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COMMON)                     \
    X(COMPUTE_QUEUE_GENERIC_READ, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_GENERIC_READ)         \
    X(COMPUTE_QUEUE_UNORDERED_ACCESS, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_UNORDERED_ACCESS) \
    X(COMPUTE_QUEUE_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE)   \
    X(COMPUTE_QUEUE_COPY_SOURCE, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_SOURCE)           \
    X(COMPUTE_QUEUE_COPY_DEST, D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_DEST)

#define BLEND_MAP(X)                                 \
    X(ZERO, D3D12_BLEND_ZERO)                        \
    X(ONE, D3D12_BLEND_ONE)                          \
    X(SRC_COLOR, D3D12_BLEND_SRC_COLOR)              \
    X(INVERT_SRC_COLOR, D3D12_BLEND_INV_SRC_COLOR)   \
    X(SRC_ALPHA, D3D12_BLEND_SRC_ALPHA)              \
    X(INV_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA)      \
    X(DEST_ALPHA, D3D12_BLEND_DEST_ALPHA)            \
    X(INVERT_DEST_ALPHA, D3D12_BLEND_INV_DEST_ALPHA) \
    X(DEST_COLOR, D3D12_BLEND_DEST_COLOR)            \
    X(INVERT_DEST_COLOR, D3D12_BLEND_INV_DEST_COLOR) \
    X(SRC_ALPHA_CLAMP, D3D12_BLEND_SRC_ALPHA_SAT)    \
    X(SRC1_COLOR, D3D12_BLEND_SRC1_COLOR)            \
    X(INVERT_SRC1_COLOR, D3D12_BLEND_INV_SRC1_COLOR) \
    X(SRC1_ALPHA, D3D12_BLEND_SRC1_ALPHA)            \
    X(INVERT_SRC1_ALPHA, D3D12_BLEND_INV_SRC1_ALPHA) \
    // TODO: Add support for blend factor
/*
X(CONSTANT_COLOR, D3D12_BLEND_BLEND_FACTOR)            \
X(INVERT_CONSTANT_COLOR, D3D12_BLEND_INV_BLEND_FACTOR) \
*/

#define LOGIC_OP_MAP(X)                            \
    X(CLEAR, D3D12_LOGIC_OP_CLEAR)                 \
    X(SET, D3D12_LOGIC_OP_SET)                     \
    X(COPY, D3D12_LOGIC_OP_COPY)                   \
    X(COPY_INVERTED, D3D12_LOGIC_OP_COPY_INVERTED) \
    X(NOOP, D3D12_LOGIC_OP_NOOP)                   \
    X(INVERT, D3D12_LOGIC_OP_INVERT)               \
    X(AND, D3D12_LOGIC_OP_AND)                     \
    X(NAND, D3D12_LOGIC_OP_NAND)                   \
    X(OR, D3D12_LOGIC_OP_OR)                       \
    X(NOR, D3D12_LOGIC_OP_NOR)                     \
    X(XOR, D3D12_LOGIC_OP_XOR)                     \
    X(EQUIV, D3D12_LOGIC_OP_EQUIV)                 \
    X(AND_REVERSE, D3D12_LOGIC_OP_AND_REVERSE)     \
    X(AND_INVERTED, D3D12_LOGIC_OP_AND_INVERTED)   \
    X(OR_REVERSE, D3D12_LOGIC_OP_OR_REVERSE)       \
    X(OR_INVERTED, D3D12_LOGIC_OP_OR_INVERTED)

DXGI_FORMAT dxgi_format(const IndexType format)
{
    if (format == IndexType::UINT16)
    {
        return DXGI_FORMAT_R16_UINT;
    }
    return DXGI_FORMAT_R32_UINT;
}

D3D12_BARRIER_SYNC sync_stage(const SyncStage stage)
{
#define MAP_SYNC(our, d3d) \
    if (stage & (our))     \
        sync |= (d3d);

    D3D12_BARRIER_SYNC sync{};
    SYNC_STAGE_MAP(MAP_SYNC)
    return sync;

#undef MAP_SYNC
}

D3D12_BARRIER_ACCESS access_flags(const AccessFlags access)
{
#define MAP_ACCESS(our, d3d) \
    if (access & (our))      \
        flags |= (d3d);

    D3D12_BARRIER_ACCESS flags = {};
    ACCESS_FLAGS_MAP(MAP_ACCESS)
    return flags;

#undef MAP_ACCESS
}

D3D12_BARRIER_LAYOUT layout(const Layout layout)
{
#define MAP_LAYOUT(our, d3d) \
    case Layout::our:        \
        return d3d;

    switch (layout)
    {
        LAYOUT_MAP(MAP_LAYOUT)
    }
    return D3D12_BARRIER_LAYOUT_UNDEFINED;

#undef MAP_LAYOUT
}

D3D12_FILTER filter(
    const Filter min, const Filter mag, const Filter mip, const bool comparison_enable, const UINT max_anisotropy)
{
    const D3D12_FILTER_REDUCTION_TYPE reduction_type = comparison_enable ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON
                                                                         : D3D12_FILTER_REDUCTION_TYPE_STANDARD;

    if (max_anisotropy > 1)
    {
        return D3D12_ENCODE_ANISOTROPIC_FILTER(reduction_type);
    }

    return D3D12_ENCODE_BASIC_FILTER(min, mag, mip, reduction_type);
}

D3D12_BLEND blend(const Blend blend)
{
#define MAP_BLEND(our, d3d) \
    case Blend::our:        \
        return d3d;

    switch (blend)
    {
        BLEND_MAP(MAP_BLEND)
    }
    assert(false); // This should be exhaustive
    return D3D12_BLEND_ZERO;

#undef MAP_BLEND
}

D3D12_LOGIC_OP logic_op(const LogicOp logic_op)
{
#define MAP_LOGIC_OP(our, d3d) \
    case LogicOp::our:         \
        return d3d;

    switch (logic_op)
    {
        LOGIC_OP_MAP(MAP_LOGIC_OP)
    }
    assert(false); // This should be exhaustive
    return D3D12_LOGIC_OP_NOOP;

#undef MAP_LOGIC_OP
}

} // namespace qhenki::gfx
#undef SYNC_STAGE_MAP
#undef ACCESS_FLAGS_MAP
#undef LAYOUT_MAP
#undef BLEND_MAP
#undef LOGIC_OP_MAP
#undef PRIMITIVE_TOPOLOGY_MAP
#undef ADDRESS_MODE_MAP
#undef COMPARISON_FUNC_MAP
