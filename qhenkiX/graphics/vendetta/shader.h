#pragma once
#include <cstdint>
#include <smartpointer.h>

namespace vendetta
{
	enum ShaderType : uint8_t
	{
		VERTEX,
		PIXEL,
		COMPUTE,
	};
	struct Shader
	{
		ShaderType type;
		sPtr<void> internal_state;
	};
}
