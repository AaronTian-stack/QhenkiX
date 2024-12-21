#include "d3d11_swapchain.h"

#include <iostream>

bool D3D11Swapchain::create(vendetta::SwapchainDesc desc, DisplayWindow& window,
                            const ComPtr<IDXGIFactory2>& dxgi_factory, const ComPtr<ID3D11Device>& device)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor =
    {
        .Width = static_cast<UINT>(desc.width),
        .Height = static_cast<UINT>(desc.height),
        .Format = desc.format,
        .SampleDesc =
        {
            .Count = 1, // MSAA Count
            .Quality = 0
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = {},
    };

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor = {};
    swapChainFullscreenDescriptor.Windowed = true;

    if (FAILED(dxgi_factory->CreateSwapChainForHwnd(
        device.Get(),
        window.get_window_handle(),
        &swapChainDescriptor,
        &swapChainFullscreenDescriptor,
        nullptr,
        &swapchain)))
    {
		std::cerr << "D3D11: Failed to create Swapchain" << std::endl;
        return false;
    }

    // create swap chain render target
    return create_swapchain_resources(device);
}

bool D3D11Swapchain::create_swapchain_resources(const ComPtr<ID3D11Device>& device)
{
    ComPtr<ID3D11Texture2D> backBuffer = nullptr;
    if (FAILED(swapchain->GetBuffer(
        0,
        IID_PPV_ARGS(&backBuffer))))
    {
		std::cerr << "D3D11: Failed to get Back Buffer from Swapchain" << std::endl;
		return false;
    }

    if (FAILED(device->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        &sc_render_target)))
    {
		std::cerr << "D3D11: Failed to create Render Target View" << std::endl;
		return false;
    }
    // don't need to keep the back buffer reference only needed it to create RTV
    // d3d11 auto swaps the back buffer, uses the same pointer
    return true;
}

bool D3D11Swapchain::resize(const ComPtr<ID3D11Device>& device, int width, int height)
{
    sc_render_target.Reset();

    if (FAILED(swapchain->ResizeBuffers(
        0,
        width,
        height,
        DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
        0)))
    {
		std::cerr << "D3D11: Failed to resize Swapchain buffers" << std::endl;
    }

    return create_swapchain_resources(device);
}

D3D11Swapchain::~D3D11Swapchain()
{
	swapchain.Reset();
	sc_render_target.Reset();
}
