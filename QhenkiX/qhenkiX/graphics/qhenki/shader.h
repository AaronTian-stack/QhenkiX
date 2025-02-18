#pragma once
#include <cstdint>
#include <smartpointer.h>

namespace qhenki::gfx
{
	enum ShaderType : uint8_t
	{
		VERTEX_SHADER,
		PIXEL_SHADER,
		COMPUTE_SHADER,
	};
	enum class ShaderModel
	{
		SM_5_0,
		SM_5_1,
		SM_6_0,
		SM_6_1,
		SM_6_2,
		SM_6_3,
		SM_6_4,
		SM_6_5,
		SM_6_6,
	};
	struct Shader
	{
		ShaderType type;
		ShaderModel shader_model; // SM shader was compiled with
		sPtr<void> internal_state;
	};
}
