#pragma once
#include <cstdint>
#include <smartpointer.h>

namespace qhenki::graphics
{
	enum ShaderType : uint8_t
	{
		VERTEX_SHADER,
		PIXEL_SHADER,
		COMPUTE_SHADER,
	};
	struct Shader
	{
		ShaderType type;
		sPtr<void> internal_state;
	};
	enum class ShaderModel
	{
		SM_5_1,
		SM_6_0,
		SM_6_1,
		SM_6_2,
		SM_6_3,
		SM_6_4,
		SM_6_5,
		SM_6_6,
	};
}
