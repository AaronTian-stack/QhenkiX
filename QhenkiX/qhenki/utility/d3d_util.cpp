#include "d3d_util.h"

#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include "qhenki/RHI/shader.h"

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

#define PRIMITIVE_TOPOLOGY_MAP(X)                         \
    X(POINT_LIST, D3D_PRIMITIVE_TOPOLOGY_POINTLIST)       \
    X(LINE_LIST, D3D_PRIMITIVE_TOPOLOGY_LINELIST)         \
    X(LINE_STRIP, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP)       \
    X(TRIANGLE_LIST, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) \
    X(TRIANGLE_STRIP, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP)

#define ADDRESS_MODE_MAP(X)                      \
    X(WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP)     \
    X(MIRROR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR) \
    X(CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP)   \
    X(BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER)

#define COMPARISON_FUNC_MAP(X)                               \
    X(NONE, D3D12_COMPARISON_FUNC_NONE)                      \
    X(NEVER, D3D12_COMPARISON_FUNC_NEVER)                    \
    X(LESS, D3D12_COMPARISON_FUNC_LESS)                      \
    X(EQUAL, D3D12_COMPARISON_FUNC_EQUAL)                    \
    X(LESS_OR_EQUAL, D3D12_COMPARISON_FUNC_LESS_EQUAL)       \
    X(GREATER, D3D12_COMPARISON_FUNC_GREATER)                \
    X(NOT_EQUAL, D3D12_COMPARISON_FUNC_NOT_EQUAL)            \
    X(GREATER_OR_EQUAL, D3D12_COMPARISON_FUNC_GREATER_EQUAL) \
    X(ALWAYS, D3D12_COMPARISON_FUNC_ALWAYS)

std::wstring get_shader_model_wchar(const ShaderType type, const ShaderModel model)
{
    auto sm = magic_enum::enum_name(model);
    assert(sm.size() == 6);

    if (type == LIBRARY_SHADER)
    {
        // "SM_6_6" -> L"lib_6_6"
        const auto underscore = sm.find('_');
        assert(underscore != std::string_view::npos);
        std::wstring smc = L"lib";
        smc.push_back(L'_');
        smc.append(std::wstring(sm.begin() + underscore + 1, sm.end()));
        return smc;
    }

    std::wstring smc(sm.begin(), sm.end());
    smc[1] = 's';

    switch (type)
    {
    case VERTEX_SHADER:
        smc[0] = 'v';
        break;
    case PIXEL_SHADER:
        smc[0] = 'p';
        break;
    case COMPUTE_SHADER:
        smc[0] = 'c';
        break;
    case LIBRARY_SHADER:
        break;
    }

    return smc;
}

std::string get_shader_model_char(const ShaderType type, const ShaderModel model)
{
    auto sm = magic_enum::enum_name(model);
    assert(sm.size() == 6);

    if (type == LIBRARY_SHADER)
    {
        // "SM_6_6" -> "lib_6_6"
        const auto underscore = sm.find('_');
        assert(underscore != std::string_view::npos);
        std::string smc = "lib";
        smc.push_back('_');
        smc.append(std::string(sm.begin() + underscore + 1, sm.end()));
        return smc;
    }

    auto smc = std::string(sm); // Should not cause heap allocation (6 chars)
    smc[1] = 's';

    switch (type)
    {
    case VERTEX_SHADER:
        smc[0] = 'v';
        break;
    case PIXEL_SHADER:
        smc[0] = 'p';
        break;
    case COMPUTE_SHADER:
        smc[0] = 'c';
        break;
    case LIBRARY_SHADER:
        break;
    }

    return smc;
}

DXGI_FORMAT get_dxgi_format(const IndexType format)
{
    if (format == IndexType::UINT16)
    {
        return DXGI_FORMAT_R16_UINT;
    }
    return DXGI_FORMAT_R32_UINT;
}

D3D12_PRIMITIVE_TOPOLOGY get_primitive_topology(const PrimitiveTopology topology)
{
#define MAP_TOPOLOGY(our, d3d)   \
    case PrimitiveTopology::our: \
        return d3d;

    switch (topology)
    {
        PRIMITIVE_TOPOLOGY_MAP(MAP_TOPOLOGY)
    default:
        assert(false);
        return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }

#undef MAP_TOPOLOGY
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
    const Filter min, const Filter mag, const Filter mip, const ComparisonFunc func, const UINT max_anisotropy)
{
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_filter
    // Assemble the bitmask ourselves
    UINT filter = 0;
    // If linear then set bit to 1 in MIN MAG MIP
    // Why does Microsoft leave a 0 in between each bitmask bit???
    if (max_anisotropy == 0)
    {
        if (mip == Filter::LINEAR)
        {
            filter |= D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        }
        if (min == Filter::LINEAR)
        {
            filter |= D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        }
        if (mag == Filter::LINEAR)
        {
            filter |= D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        }
        if (func != ComparisonFunc::NONE)
        {
            filter |= D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        }
    }
    else
    {
        if (func == ComparisonFunc::NONE)
        {
            return D3D12_FILTER_ANISOTROPIC;
        }
        return D3D12_FILTER_COMPARISON_ANISOTROPIC;
    }
    return static_cast<D3D12_FILTER>(filter);
}

D3D12_TEXTURE_ADDRESS_MODE texture_address_mode(const AddressMode mode)
{
#define MAP_ADDRESS(our, d3d) \
    case AddressMode::our:    \
        return d3d;

    switch (mode)
    {
        ADDRESS_MODE_MAP(MAP_ADDRESS)
    }

    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;

#undef MAP_ADDRESS
}

D3D12_COMPARISON_FUNC comparison_func(const ComparisonFunc func)
{
#define MAP_COMPARISON(our, d3d) \
    case ComparisonFunc::our:    \
        return d3d;

    switch (func)
    {
        COMPARISON_FUNC_MAP(MAP_COMPARISON)
    }

    return D3D12_COMPARISON_FUNC_NONE;

#undef MAP_COMPARISON
}

bool is_depth_stencil_format(const DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return true;
    default:
        return false;
    }
}

} // namespace qhenki::gfx
#undef SYNC_STAGE_MAP
#undef ACCESS_FLAGS_MAP
#undef LAYOUT_MAP
#undef PRIMITIVE_TOPOLOGY_MAP
#undef ADDRESS_MODE_MAP
#undef COMPARISON_FUNC_MAP
