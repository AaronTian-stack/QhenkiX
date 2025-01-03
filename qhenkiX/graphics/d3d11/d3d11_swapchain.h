#pragma once
#define NOMINMAX
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include "graphics/displaywindow.h"
#include "graphics/vendetta/swapchain.h"

using Microsoft::WRL::ComPtr;

struct D3D11Swapchain
{
	ComPtr<IDXGISwapChain1> swapchain;
	ComPtr<ID3D11RenderTargetView> sc_render_target;
	bool create(vendetta::SwapchainDesc desc, DisplayWindow& window,
	            IDXGIFactory2* const dxgi_factory, ID3D11Device* const device);
	bool create_swapchain_resources(ID3D11Device* const device);
	bool resize(ID3D11Device* const device, ID3D11DeviceContext* const device_context, int width, int height);
	~D3D11Swapchain();
};
