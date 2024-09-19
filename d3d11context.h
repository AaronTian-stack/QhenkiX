#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl.h>

#include "displaywindow.h"

using Microsoft::WRL::ComPtr;

class D3D11Context
{
private:
	ComPtr<IDXGIFactory2> dxgi_factory = nullptr;
	ComPtr<ID3D11Device> device = nullptr;
	ComPtr<ID3D11DeviceContext> device_context = nullptr;
	ComPtr<IDXGISwapChain1> swapchain = nullptr;
	ComPtr<ID3D11RenderTargetView> render_target = nullptr;

public:
	void create(DisplayWindow &window);
	void create_swapchain_resources();
	void destroy_swapchain_resources();
	void resize_swapchain(int width, int height);
	void destroy();

	void debug_clear();
};

