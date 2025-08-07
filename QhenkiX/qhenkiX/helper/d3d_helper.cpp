#include "qhenkiX/helper/d3d_helper.h"

#include <stdexcept>
#include "qhenkiX/RHI/shader.h"
#include <magic_enum/magic_enum.hpp>

using namespace qhenki::gfx;

std::wstring D3DHelper::get_shader_model_wchar(const ShaderType type, const ShaderModel model)
{
    auto sm = magic_enum::enum_name(model);
    assert(sm.size() == 6);

    std::wstring smc(sm.begin(), sm.end());
    smc[1] = 's';

    switch (type)
    {
        case VERTEX_SHADER: smc[0] = 'v'; break;
        case PIXEL_SHADER:  smc[0] = 'p'; break;
        case COMPUTE_SHADER:smc[0] = 'c'; break;
    }

    return smc;
}

std::string D3DHelper::get_shader_model_char(const ShaderType type, const ShaderModel model)
{
    auto sm = magic_enum::enum_name(model);
    assert(sm.size() == 6);

	auto smc = std::string(sm); // Should not cause heap allocation (6 chars)
    smc[1] = 's';

    switch (type)
    {
        case VERTEX_SHADER: smc[0] = 'v'; break;
        case PIXEL_SHADER:  smc[0] = 'p'; break;
        case COMPUTE_SHADER:smc[0] = 'c'; break;
    }

    return smc;
}

DXGI_FORMAT D3DHelper::get_dxgi_format(const IndexType format)
{
	if (format == IndexType::UINT16)
	{
		return DXGI_FORMAT_R16_UINT;
	}
	return DXGI_FORMAT_R32_UINT;
}

D3D12_PRIMITIVE_TOPOLOGY D3DHelper::get_primitive_topology(const PrimitiveTopology topology)
{
	switch (topology)
	{
	case PrimitiveTopology::POINT_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case PrimitiveTopology::LINE_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case PrimitiveTopology::LINE_STRIP:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case PrimitiveTopology::TRIANGLE_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case PrimitiveTopology::TRIANGLE_STRIP:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}
	return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

D3D12_BARRIER_SYNC D3DHelper::sync_stage_D3D(SyncStage stage)
{
    D3D12_BARRIER_SYNC sync = {};

    if (stage & SYNC_NONE)
    {
        sync |= D3D12_BARRIER_SYNC_NONE;
    }
    if (stage & SYNC_ALL)
    {
        sync |= D3D12_BARRIER_SYNC_ALL;
    }
    if (stage & SYNC_DRAW)
    {
        sync |= D3D12_BARRIER_SYNC_DRAW;
    }
    if (stage & SYNC_INDEX_INPUT)
    {
        sync |= D3D12_BARRIER_SYNC_INDEX_INPUT;
    }
    if (stage & SYNC_VERTEX_SHADING)
    {
        sync |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
    }
    if (stage & SYNC_PIXEL_SHADING)
    {
        sync |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
    }
    if (stage & SYNC_DEPTH_STENCIL)
    {
        sync |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
    }
    if (stage & SYNC_RENDER_TARGET)
    {
        sync |= D3D12_BARRIER_SYNC_RENDER_TARGET;
    }
    if (stage & SYNC_COMPUTE_SHADING)
    {
        sync |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
    }
    if (stage & SYNC_RAYTRACING)
    {
        sync |= D3D12_BARRIER_SYNC_RAYTRACING;
    }
    if (stage & SYNC_COPY)
    {
        sync |= D3D12_BARRIER_SYNC_COPY;
    }
    if (stage & SYNC_RESOLVE)
    {
        sync |= D3D12_BARRIER_SYNC_RESOLVE;
    }
    if (stage & SYNC_EXECUTE_INDIRECT)
    {
        sync |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
    }
    if (stage & SYNC_PREDICATION)
    {
        sync |= D3D12_BARRIER_SYNC_PREDICATION;
    }
    if (stage & SYNC_ALL_SHADING)
    {
        sync |= D3D12_BARRIER_SYNC_ALL_SHADING;
    }
    if (stage & SYNC_NON_PIXEL_SHADING)
    {
        sync |= D3D12_BARRIER_SYNC_NON_PIXEL_SHADING;
    }
    if (stage & SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO)
    {
        sync |= D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
    }
    if (stage & SYNC_CLEAR_UNORDERED_ACCESS_VIEW)
    {
        sync |= D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW;
    }
    if (stage & SYNC_VIDEO_DECODE)
    {
        sync |= D3D12_BARRIER_SYNC_VIDEO_DECODE;
    }
    if (stage & SYNC_VIDEO_PROCESS)
    {
        sync |= D3D12_BARRIER_SYNC_VIDEO_PROCESS;
    }
    if (stage & SYNC_VIDEO_ENCODE)
    {
        sync |= D3D12_BARRIER_SYNC_VIDEO_ENCODE;
    }
    if (stage & SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE)
    {
        sync |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
    }
    if (stage & SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE)
    {
        sync |= D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE;
    }

    return sync;
}

D3D12_BARRIER_ACCESS D3DHelper::access_flags_D3D(AccessFlags access)
{
	D3D12_BARRIER_ACCESS flags = {};

	if (access & ACCESS_COMMON)
	{
		flags |= D3D12_BARRIER_ACCESS_COMMON;
	}
	if (access & ACCESS_VERTEX_BUFFER)
	{
		flags |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
	}
	if (access & ACCESS_UNIFORM_BUFFER)
	{
		flags |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;
	}
	if (access & ACCESS_INDEX_BUFFER)
	{
		flags |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;
	}
	if (access & ACCESS_RENDER_TARGET)
	{
		flags |= D3D12_BARRIER_ACCESS_RENDER_TARGET;
	}
	if (access & ACCESS_STORAGE_ACCESS)
	{
		flags |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
	}
	if (access & ACCESS_DEPTH_STENCIL_WRITE)
	{
		flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
	}
	if (access & ACCESS_DEPTH_STENCIL_READ)
	{
		flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
	}
	if (access & ACCESS_SHADER_RESOURCE)
	{
		flags |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
	}
	if (access & ACCESS_STREAM_OUTPUT)
	{
		flags |= D3D12_BARRIER_ACCESS_STREAM_OUTPUT;
	}
	if (access & ACCESS_INDIRECT_ARGUMENT)
	{
		flags |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
	}
	if (access & ACCESS_COPY_DEST)
	{
		flags |= D3D12_BARRIER_ACCESS_COPY_DEST;
	}
	if (access & ACCESS_COPY_SOURCE)
	{
		flags |= D3D12_BARRIER_ACCESS_COPY_SOURCE;
	}
	if (access & ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ)
	{
		flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
	}
	if (access & ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE)
	{
		flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
	}
	if (access & ACCESS_SHADING_RATE_SOURCE)
	{
		flags |= D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE;
	}
	if (access & ACCESS_VIDEO_DECODE_READ)
	{
		flags |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ;
	}
	if (access & ACCESS_VIDEO_DECODE_WRITE)
	{
		flags |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE;
	}
	if (access & ACCESS_VIDEO_PROCESS_READ)
	{
		flags |= D3D12_BARRIER_ACCESS_VIDEO_PROCESS_READ;
	}
	if (access & ACCESS_VIDEO_PROCESS_WRITE)
	{
		flags |= D3D12_BARRIER_ACCESS_VIDEO_PROCESS_WRITE;
	}
	if (access & ACCESS_VIDEO_ENCODE_READ)
	{
		flags |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ;
	}
	if (access & ACCESS_VIDEO_ENCODE_WRITE)
	{
		flags |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE;
	}
	if (access & NO_ACCESS)
	{
		flags |= D3D12_BARRIER_ACCESS_NO_ACCESS;
	}

	return flags;
}

D3D12_BARRIER_LAYOUT D3DHelper::layout_D3D(Layout layout)
{
	D3D12_BARRIER_LAYOUT state = {};
    switch (layout)
    {
    case Layout::UNDEFINED:
        state = D3D12_BARRIER_LAYOUT_UNDEFINED;
        break;
    case Layout::COMMON:
        state = D3D12_BARRIER_LAYOUT_COMMON;
        break;
    case Layout::PRESENT:
        state = D3D12_BARRIER_LAYOUT_PRESENT;
        break;
    case Layout::LAYOUT_GENERIC_READ:
        state = D3D12_BARRIER_LAYOUT_GENERIC_READ;
        break;
    case Layout::RENDER_TARGET:
        state = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
        break;
    case Layout::UNORDERED_ACCESS:
        state = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
        break;
    case Layout::DEPTH_STENCIL_WRITE:
        state = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
        break;
    case Layout::DEPTH_STENCIL_READ:
        state = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
        break;
    case Layout::SHADER_RESOURCE:
        state = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
        break;
    case Layout::COPY_SOURCE:
        state = D3D12_BARRIER_LAYOUT_COPY_SOURCE;
        break;
    case Layout::COPY_DEST:
        state = D3D12_BARRIER_LAYOUT_COPY_DEST;
        break;
    case Layout::RESOLVE_SOURCE:
        state = D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
        break;
    case Layout::RESOLVE_DEST:
        state = D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
        break;
    case Layout::SHADING_RATE_SOURCE:
        state = D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE;
        break;
    case Layout::VIDEO_DECODE_READ:
        state = D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ;
        break;
    case Layout::VIDEO_DECODE_WRITE:
        state = D3D12_BARRIER_LAYOUT_VIDEO_DECODE_WRITE;
        break;
    case Layout::VIDEO_PROCESS_READ:
        state = D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_READ;
        break;
    case Layout::VIDEO_PROCESS_WRITE:
        state = D3D12_BARRIER_LAYOUT_VIDEO_PROCESS_WRITE;
        break;
    case Layout::VIDEO_ENCODE_READ:
        state = D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_READ;
        break;
    case Layout::VIDEO_ENCODE_WRITE:
        state = D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_WRITE;
        break;
    case Layout::DIRECT_QUEUE_COMMON:
        state = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON;
        break;
    case Layout::DIRECT_QUEUE_GENERIC_READ:
        state = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ;
        break;
    case Layout::DIRECT_QUEUE_UNORDERED_ACCESS:
        state = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS;
        break;
    case Layout::DIRECT_QUEUE_SHADER_RESOURCE:
        state = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE;
        break;
    case Layout::DIRECT_QUEUE_COPY_SOURCE:
        state = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE;
        break;
    case Layout::DIRECT_QUEUE_COPY_DEST:
        state = D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST;
        break;
    case Layout::COMPUTE_QUEUE_COMMON:
        state = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COMMON;
        break;
    case Layout::COMPUTE_QUEUE_GENERIC_READ:
        state = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_GENERIC_READ;
        break;
    case Layout::COMPUTE_QUEUE_UNORDERED_ACCESS:
        state = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_UNORDERED_ACCESS;
        break;
    case Layout::COMPUTE_QUEUE_SHADER_RESOURCE:
        state = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE;
        break;
    case Layout::COMPUTE_QUEUE_COPY_SOURCE:
        state = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_SOURCE;
        break;
    case Layout::COMPUTE_QUEUE_COPY_DEST:
        state = D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_DEST;
        break;
    case Layout::VIDEO_QUEUE_COMMON:
        state = D3D12_BARRIER_LAYOUT_VIDEO_QUEUE_COMMON;
        break;
    default:
        throw std::runtime_error("D3DHelper: Invalid layout");
    }
    return state;
}

D3D12_FILTER D3DHelper::filter(Filter min, Filter mag, Filter mip, ComparisonFunc func, UINT max_anisotropy)
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

D3D12_TEXTURE_ADDRESS_MODE D3DHelper::texture_address_mode(AddressMode mode)
{
	switch (mode)
	{
	case AddressMode::WRAP:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case AddressMode::MIRROR:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case AddressMode::CLAMP:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case AddressMode::BORDER:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	default:
		throw std::runtime_error("D3DHelper: Invalid texture address mode");
	}
}

D3D12_COMPARISON_FUNC D3DHelper::comparison_func(ComparisonFunc func)
{
	switch (func)
	{
	case ComparisonFunc::NONE:
		return D3D12_COMPARISON_FUNC_NONE;
	case ComparisonFunc::NEVER:
		return D3D12_COMPARISON_FUNC_NEVER;
	case ComparisonFunc::LESS:
		return D3D12_COMPARISON_FUNC_LESS;
	case ComparisonFunc::EQUAL:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case ComparisonFunc::LESS_OR_EQUAL:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case ComparisonFunc::GREATER:
		return D3D12_COMPARISON_FUNC_GREATER;
	case ComparisonFunc::NOT_EQUAL:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case ComparisonFunc::GREATER_OR_EQUAL:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case ComparisonFunc::ALWAYS:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	default:
		throw std::runtime_error("D3DHelper: Invalid comparison function");
	}
}

bool D3DHelper::is_depth_stencil_format(DXGI_FORMAT format)
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
