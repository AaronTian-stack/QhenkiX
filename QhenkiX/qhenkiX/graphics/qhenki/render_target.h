#pragma once

#include <smartpointer.h>
#include <array>

namespace qhenki::gfx
{
	// TODO: render target format: color, depth, stencil
	struct RenderTargetDesc
	{
		bool clear_color = true;
		std::array<float, 4> clear_color_value = { 0.0f, 0.0f, 0.0f, 1.0f };
	};
	struct RenderTarget
	{
		RenderTargetDesc desc{};
		sPtr<void> internal_state;
	};
}