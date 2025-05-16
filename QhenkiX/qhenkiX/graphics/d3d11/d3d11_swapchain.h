#pragma once
#define NOMINMAX
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include "graphics/display_window.h"
#include "graphics/qhenki/swapchain.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	struct D3D11Swapchain
	{
		ComPtr<IDXGISwapChain1> swapchain;
		ComPtr<ID3D11RenderTargetView> sc_render_target;
		bool create(const SwapchainDesc& desc, const DisplayWindow& window,
		            IDXGIFactory2* dxgi_factory, ID3D11Device* device, unsigned& frame_index);
		bool create_swapchain_resources(ID3D11Device* device);
		bool resize(ID3D11Device* device, ID3D11DeviceContext* device_context, int width, int height);
		~D3D11Swapchain();
	};
}