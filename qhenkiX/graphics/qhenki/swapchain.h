#pragma once
#include <dxgiformat.h>
#include <smartpointer.h>

#include "render_target.h"

namespace qhenki
{
	struct SwapchainDesc
	{
		int width;
		int height;
		DXGI_FORMAT format;
	};
	struct Swapchain
	{
		SwapchainDesc desc;
		RenderTarget render_target;
		sPtr<void> internal_state;
	};

}
