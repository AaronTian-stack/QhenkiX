#pragma once
#include <dxgiformat.h>

namespace vendetta
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
		sPtr<void> internal_state;
	};

}
