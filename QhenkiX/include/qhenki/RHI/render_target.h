#pragma once

#include <array>

#include "descriptor_table.h"

namespace qhenki::gfx
{
	struct RenderTarget
	{
		union ClearParams
		{
			std::array<float, 4> clear_color_value = { 0.0f, 0.0f, 0.0f, 1.0f };
			struct DSVClearParams
			{
				float clear_depth_value = 1.0f;
				uint8_t clear_stencil_value = 0;
			} dsv_clear_params;
		} clear_params;
		enum ClearType : uint8_t
		{
			None = 0,
			Color = 1 << 1,
			Depth = 1 << 2,
			Stencil = 1 << 3,
		} clear_type; // Can be combined
		Descriptor descriptor;
	};
}
