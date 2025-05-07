#pragma once

// >= SM 5.1 required for spaces
// SM 6.6 is needed for Mesh shaders

#define NOMINMAX
#include <dxgiformat.h>
#include <wrl/client.h>
#include "graphics/qhenki/shader.h"
#include <graphics/qhenki/enums.h>

#include "D3D12MemAlloc.h"
#include "graphics/qhenki/barrier.h"
#include "graphics/qhenki/sampler.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	class D3DHelper
	{
	public:
		static const wchar_t* get_shader_model_wchar(ShaderType type, ShaderModel model);
		static const char* get_shader_model_char(ShaderType type, ShaderModel model);
		static DXGI_FORMAT get_dxgi_format(IndexType format);
		static D3D12_PRIMITIVE_TOPOLOGY get_primitive_topology(PrimitiveTopology topology);

		static D3D12_BARRIER_SYNC sync_stage_D3D(SyncStage stage);
		static D3D12_BARRIER_ACCESS access_flags_D3D(AccessFlags access);
		static D3D12_BARRIER_LAYOUT layout_D3D(Layout layout);

		// Only returns the filter types shared with D3D11
		static D3D12_FILTER filter(Filter min, Filter mag, Filter mip, ComparisonFunc func, UINT max_anisotropy);
		static D3D12_TEXTURE_ADDRESS_MODE texture_address_mode(AddressMode mode);
		static D3D12_COMPARISON_FUNC comparison_func(ComparisonFunc func);

		static UINT bytes_per_pixel(DXGI_FORMAT format);
		static bool is_depth_stencil_format(DXGI_FORMAT format);
	};
}