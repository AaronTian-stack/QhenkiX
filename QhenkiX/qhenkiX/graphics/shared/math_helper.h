#pragma once
#include <cstdint>

class MathHelper
{
public:
	static inline uint32_t align_u32(uint32_t size, uint32_t alignment)
	{
		return (size + alignment - 1) & ~(alignment - 1);
	}
};
