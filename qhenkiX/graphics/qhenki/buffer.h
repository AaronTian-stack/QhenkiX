#pragma once
#include <cstdint>
#include <smartpointer.h>

namespace qhenki
{
	enum BufferUsage : uint8_t
	{
		VERTEX = 1 << 0,
		INDEX = 1 << 1,
		UNIFORM = 1 << 2,
		STORAGE = 1 << 3,
		INDIRECT = 1 << 4,
		TRANSFER_SRC = 1 << 5,
		TRANSFER_DST = 1 << 6,
	};

	enum BufferVisibility : uint8_t
	{
		GPU_ONLY = 0,
		CPU_SEQUENTIAL = 1 << 0,
		CPU_RANDOM = 1 << 1,
	};

	struct BufferDesc
	{
		uint64_t size = 0;
		BufferUsage usage;
		BufferVisibility visibility;
	};

	struct Buffer
	{
		BufferDesc desc;
		sPtr<void> internal_state;
	};
}
