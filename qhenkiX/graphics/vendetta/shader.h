#pragma once
#include <cstdint>
#include <smartpointer.h>

namespace vendetta
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
}
