//#pragma once
//#include <d3d11.h>
//#include <dxgi1_2.h>
//#include <wrl/client.h>
//
//#include "graphics/displaywindow.h"
//#include "graphics/vendetta/swapchain.h"
//
//using Microsoft::WRL::ComPtr;
//
//struct D3D11Swapchain
//{
//	ComPtr<IDXGISwapChain1> swapchain = nullptr;
//	ComPtr<ID3D11RenderTargetView> sc_render_target = nullptr;
//	void create(vendetta::SwapchainDesc desc, DisplayWindow& window, 
//		const ComPtr<IDXGIFactory2>& dxgi_factory, const ComPtr<ID3D11Device>& device);
//	void create_swapchain_resources(const ComPtr<ID3D11Device>& device);
//	~D3D11Swapchain();
//};
