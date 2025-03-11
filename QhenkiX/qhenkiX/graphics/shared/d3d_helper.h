#pragma once

// >= SM 5.1 required for spaces
// SM 6.6 is needed for Mesh shaders

#define NOMINMAX
#include <wrl/client.h>
#include "graphics/qhenki/shader.h"
#include <dxcapi.h>

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	class D3DHelper
	{
	public:
		static const wchar_t* get_shader_model_wchar(const qhenki::gfx::ShaderType type, const qhenki::gfx::ShaderModel model);
		static const char* get_shader_model_char(const qhenki::gfx::ShaderType type, const qhenki::gfx::ShaderModel model);
	};
}