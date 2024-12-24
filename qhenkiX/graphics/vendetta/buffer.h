#pragma once
#include <cstdint>
#include <smartpointer.h>

namespace vendetta
{
	enum BufferUsage : uint8_t
	{
		Vertex = 1 << 1,
		Index = 1 << 2,
		Uniform = 1 << 3,
		Storage = 1 << 4,
		Indirect = 1 << 5,
		TransferSrc = 1 << 6,
		TransferDst = 1 << 7,
	};

	enum BufferVisibility : uint8_t
	{
		CPU_WRITE = 1 << 1, // Host visible
		CPU_WRITE_PERSISTENT = 1 << 2, // Not supported in D3D11
		GPU_READ = 1 << 3,
		GPU_WRITE = 1 << 4,
	};

	// High level description of a buffer
	struct BufferDesc
	{
		uint64_t size = 0;
		BufferUsage usage;
		BufferVisibility visibility;
	};

	struct Buffer
	{
		BufferDesc desc;
		sPtr<void> state;
	};
}
