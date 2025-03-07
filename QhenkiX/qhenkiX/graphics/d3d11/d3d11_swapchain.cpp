#include "d3d11_swapchain.h"
#include "d3d11_context.h"

bool D3D11Swapchain::create(qhenki::gfx::SwapchainDesc desc, DisplayWindow& window,
                            IDXGIFactory2* const dxgi_factory, ID3D11Device* const device, unsigned buffer_count, unsigned& frame_index)
{
    frame_index = 0;
    DXGI_SWAP_CHAIN_DESC1 swap_chain_descriptor =
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
        .BufferCount = buffer_count,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = {},
    };

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_descriptor = {};
    swap_chain_fullscreen_descriptor.Windowed = true;

    if (FAILED(dxgi_factory->CreateSwapChainForHwnd(
        device,
        window.get_window_handle(),
        &swap_chain_descriptor,
        &swap_chain_fullscreen_descriptor,
        nullptr,
        &swapchain)))
    {
		OutputDebugString(L"Qhenki D3D11: Failed to create Swapchain\n");
        return false;
    }

    // create swap chain render target
    return create_swapchain_resources(device);
}

bool D3D11Swapchain::create_swapchain_resources(ID3D11Device* const device)
{
    ComPtr<ID3D11Texture2D> backBuffer = nullptr;
    if (FAILED(swapchain->GetBuffer(
        0,
        IID_PPV_ARGS(&backBuffer))))
    {
		OutputDebugString(L"Qhenki D3D11: Failed to get Back Buffer from Swapchain\n");
		return false;
    }

    if (FAILED(device->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        &sc_render_target)))
    {
		OutputDebugString(L"Qhenki D3D11: Failed to create Render Target View\n");
		return false;
    }
#ifdef _DEBUG
	D3D11Context::set_debug_name(sc_render_target.Get(), "Swapchain Render Target");
#endif
    // Don't need to keep the back buffer reference only needed it to create RTV
    // D3D11 auto swaps the back buffer, uses the same pointer
    return true;
}

bool D3D11Swapchain::resize(ID3D11Device* const device, ID3D11DeviceContext* const device_context, int width, int height)
{
    device_context->Flush();

    sc_render_target.Reset();

    if (FAILED(swapchain->ResizeBuffers(
        0,
        width,
        height,
        DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
        0)))
    {
		OutputDebugString(L"Qhenki D3D11: Failed to resize Swapchain buffers\n");
        return false;
    }

    return create_swapchain_resources(device);
}

D3D11Swapchain::~D3D11Swapchain()
{
	swapchain.Reset();
	sc_render_target.Reset();
}
