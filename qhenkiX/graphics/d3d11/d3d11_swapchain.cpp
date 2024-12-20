//#include "d3d11_swapchain.h"
//
//#include <stdexcept>
//
//void D3D11Swapchain::create(vendetta::SwapchainDesc desc, DisplayWindow& window, 
//    const ComPtr<IDXGIFactory2>& dxgi_factory, const ComPtr<ID3D11Device>& device)
//{
//    DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor =
//    {
//        .Width = static_cast<UINT>(desc.width),
//        .Height = static_cast<UINT>(desc.height),
//        .Format = desc.format,
//        .SampleDesc =
//        {
//            .Count = 1, // MSAA Count
//            .Quality = 0
//        },
//        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
//        .BufferCount = 2,
//        .Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH,
//        .SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD,
//        .Flags = {},
//    };
//
//    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor = {};
//    swapChainFullscreenDescriptor.Windowed = true;
//
//    if (FAILED(dxgi_factory->CreateSwapChainForHwnd(
//        device.Get(),
//        window.get_window_handle(),
//        &swapChainDescriptor,
//        &swapChainFullscreenDescriptor,
//        nullptr,
//        &swapchain)))
//    {
//        throw std::runtime_error("D3D11: Failed to create Swapchain");
//    }
//
//    // create swap chain render target
//    create_swapchain_resources(device);
//}
//
//void D3D11Swapchain::create_swapchain_resources(const ComPtr<ID3D11Device>& device)
//{
//    ComPtr<ID3D11Texture2D> backBuffer = nullptr;
//    if (FAILED(swapchain->GetBuffer(
//        0,
//        IID_PPV_ARGS(&backBuffer))))
//    {
//        throw std::runtime_error("D3D11: Failed to get Back Buffer from Swapchain");
//    }
//
//    if (FAILED(device->CreateRenderTargetView(
//        backBuffer.Get(),
//        nullptr,
//        &sc_render_target)))
//    {
//        throw std::runtime_error("D3D11: Failed to create Render Target View");
//    }
//    // don't need to keep the back buffer reference only needed it to create RTV
//    // d3d11 auto swaps the back buffer, uses the same pointer
//}
//
//D3D11Swapchain::~D3D11Swapchain()
//{
//	if (swapchain)
//		swapchain.Reset();
//	if (sc_render_target)
//		sc_render_target.Reset();
//}
