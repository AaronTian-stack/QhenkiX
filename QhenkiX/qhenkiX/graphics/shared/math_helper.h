﻿#pragma once
#include <cassert>
#include <cstdint>

#define CONSTANT_BUFFER_ALIGNMENT 256

class MathHelper
{
public:

	static bool is_power_of_two(uint32_t value)
	{
		return (value & (value - 1)) == 0;
	}

	// Alignment should be power of 2
	static uint32_t align_u32(uint32_t size, uint32_t alignment)
	{
		assert(is_power_of_two(alignment) && "Alignment must be a power of two.");
		return (size + alignment - 1) & ~(alignment - 1);
	}
};
