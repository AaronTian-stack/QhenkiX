#include "d3d_helper.h"

#include <stdexcept>

using namespace qhenki::gfx;

const wchar_t* D3DHelper::get_shader_model_wchar(const ShaderType type, const ShaderModel model)
{
	switch (type)
	{
	case VERTEX_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return L"vs_5_0";
		case ShaderModel::SM_5_1:
			return L"vs_5_1";
		case ShaderModel::SM_6_0:
			return L"vs_6_0";
		case ShaderModel::SM_6_1:
			return L"vs_6_1";
		case ShaderModel::SM_6_2:
			return L"vs_6_2";
		case ShaderModel::SM_6_3:
			return L"vs_6_3";
		case ShaderModel::SM_6_4:
			return L"vs_6_4";
		case ShaderModel::SM_6_5:
			return L"vs_6_5";
		case ShaderModel::SM_6_6:
			return L"vs_6_6";
		}
		break;
	case PIXEL_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return L"ps_5_0";
		case ShaderModel::SM_5_1:
			return L"ps_5_1";
		case ShaderModel::SM_6_0:
			return L"ps_6_0";
		case ShaderModel::SM_6_1:
			return L"ps_6_1";
		case ShaderModel::SM_6_2:
			return L"ps_6_2";
		case ShaderModel::SM_6_3:
			return L"ps_6_3";
		case ShaderModel::SM_6_4:
			return L"ps_6_4";
		case ShaderModel::SM_6_5:
			return L"ps_6_5";
		case ShaderModel::SM_6_6:
			return L"ps_6_6";
		}
		break;
	case COMPUTE_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return L"cs_5_0";
		case ShaderModel::SM_5_1:
			return L"cs_5_1";
		case ShaderModel::SM_6_0:
			return L"cs_6_0";
		case ShaderModel::SM_6_1:
			return L"cs_6_1";
		case ShaderModel::SM_6_2:
			return L"cs_6_2";
		case ShaderModel::SM_6_3:
			return L"cs_6_3";
		case ShaderModel::SM_6_4:
			return L"cs_6_4";
		case ShaderModel::SM_6_5:
			return L"cs_6_5";
		case ShaderModel::SM_6_6:
			return L"cs_6_6";
		}
		break;
	default:
		throw std::runtime_error("D3DHelper: not implemented");
	}
	return nullptr;
}

const char* D3DHelper::get_shader_model_char(const ShaderType type,
	const ShaderModel model)
{
	switch (type)
	{
	case VERTEX_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return "vs_5_0";
		case ShaderModel::SM_5_1:
			return "vs_5_1";
		case ShaderModel::SM_6_0:
			return "vs_6_0";
		case ShaderModel::SM_6_1:
			return "vs_6_1";
		case ShaderModel::SM_6_2:
			return "vs_6_2";
		case ShaderModel::SM_6_3:
			return "vs_6_3";
		case ShaderModel::SM_6_4:
			return "vs_6_4";
		case ShaderModel::SM_6_5:
			return "vs_6_5";
		case ShaderModel::SM_6_6:
			return "vs_6_6";
		}
		break;
	case PIXEL_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return "ps_5_0";
		case ShaderModel::SM_5_1:
			return "ps_5_1";
		case ShaderModel::SM_6_0:
			return "ps_6_0";
		case ShaderModel::SM_6_1:
			return "ps_6_1";
		case ShaderModel::SM_6_2:
			return "ps_6_2";
		case ShaderModel::SM_6_3:
			return "ps_6_3";
		case ShaderModel::SM_6_4:
			return "ps_6_4";
		case ShaderModel::SM_6_5:
			return "ps_6_5";
		case ShaderModel::SM_6_6:
			return "ps_6_6";
		}
		break;
	case COMPUTE_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_1:
			return "cs_5_1";
		case ShaderModel::SM_6_0:
			return "cs_6_0";
		case ShaderModel::SM_6_1:
			return "cs_6_1";
		case ShaderModel::SM_6_2:
			return "cs_6_2";
		case ShaderModel::SM_6_3:
			return "cs_6_3";
		case ShaderModel::SM_6_4:
			return "cs_6_4";
		case ShaderModel::SM_6_5:
			return "cs_6_5";
		case ShaderModel::SM_6_6:
			return "cs_6_6";
		}
		break;
	default:
		throw std::runtime_error("D3DHelper: not implemented");
	}
	return nullptr;
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
	default:
		throw std::runtime_error("D3DHelper: Invalid primitive topology");
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

D3D12_FILTER D3DHelper::filter(SamplerDesc::Filter min, SamplerDesc::Filter mag, SamplerDesc::Filter mip, SamplerDesc::ComparisonFunc func, UINT
                               max_anisotropy)
{
	// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_filter
	// Assemble the bitmask ourselves
	UINT filter = 0;
	// If linear then set bit to 1 in MIN MAG MIP
	// Why does Microsoft leave a 0 in between each bitmask bit???
	if (max_anisotropy == 0)
	{
		if (mip == SamplerDesc::Filter::LINEAR)
		{
			filter |= D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		}
		if (min == SamplerDesc::Filter::LINEAR)
		{
			filter |= D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		}
		if (mag == SamplerDesc::Filter::LINEAR)
		{
			filter |= D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		}
		if (func != SamplerDesc::ComparisonFunc::NONE)
		{
			filter |= D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		}
	}
	else
	{
		if (func == SamplerDesc::ComparisonFunc::NONE)
		{
			return D3D12_FILTER_ANISOTROPIC;
		}
		return D3D12_FILTER_COMPARISON_ANISOTROPIC;
	}
	return static_cast<D3D12_FILTER>(filter);
}
