#pragma once
#include <dxgiformat.h>
#include <smartpointer.h>

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
