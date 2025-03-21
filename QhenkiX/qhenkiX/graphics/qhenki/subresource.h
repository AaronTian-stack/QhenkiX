#pragma once
#include <cstdint>

namespace qhenki::gfx
{
	struct ImageSubresourceRange
	{
		uint32_t base_mip_level = 0;
		uint32_t mip_level_count = 1;
		uint32_t base_array_layer = 0;
		uint32_t array_layer_count = 1;
	};
}
 