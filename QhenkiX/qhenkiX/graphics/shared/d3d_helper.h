#pragma once

// >= SM 5.1 required for spaces
// SM 6.6 is needed for Mesh shaders

#define NOMINMAX
#include <dxgiformat.h>
#include <wrl/client.h>
#include "graphics/qhenki/shader.h"
#include <graphics/qhenki/enums.h>

#include "D3D12MemAlloc.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	class D3DHelper
	{
	public:
		static const wchar_t* get_shader_model_wchar(const ShaderType type, const ShaderModel model);
		static const char* get_shader_model_char(const ShaderType type, const ShaderModel model);
		static DXGI_FORMAT get_dxgi_format(const IndexType format);
		static D3D12_PRIMITIVE_TOPOLOGY get_primitive_topology(const PrimitiveTopology topology);
	};
}