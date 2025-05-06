#pragma once
#include <cstdint>

#define CONSTANT_BUFFER_ALIGNMENT 256

class MathHelper
{
public:
	// alignment should be power of 2
	static inline uint32_t align_u32(uint32_t size, uint32_t alignment)
	{
		return (size + alignment - 1) & ~(alignment - 1);
	}
};
