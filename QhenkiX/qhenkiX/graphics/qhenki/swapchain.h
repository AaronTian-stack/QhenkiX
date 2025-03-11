#pragma once
#include <dxgiformat.h>
#include <smartpointer.h>

#include "render_target.h"

namespace qhenki::gfx
{
	struct SwapchainDesc
	{
		unsigned int width;
		unsigned int height;
		DXGI_FORMAT format;
		unsigned int buffer_count;
	};
	struct Swapchain
	{
		SwapchainDesc desc;
		RenderTarget render_target;
		sPtr<void> internal_state;
	};

}
