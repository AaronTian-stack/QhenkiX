#pragma once
#include <cstdint>

namespace qhenki::gfx
{
	enum class IndexType : uint8_t
	{
		UINT16,
		UINT32,
	};
	enum class PrimitiveTopology : uint8_t
	{
		POINT_LIST,
		TRIANGLE_LIST,
		TRIANGLE_STRIP,
		LINE_LIST,
		LINE_STRIP,
	};

	enum class PipelineStage : uint8_t
	{
		VERTEX,
		PIXEL,
		COMPUTE,
	};
}
