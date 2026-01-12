#include "d3d11_swapchain.h"
#include "d3d11_context.h"

using namespace qhenki::gfx;

bool D3D11Swapchain::create(const SwapchainDesc& desc,
                            const DisplayWindow& window,
                            IDXGIFactory2* const dxgi_factory,
                            ID3D11Device* const device,
                            unsigned& frame_index)
{
    frame_index = 0;
    const DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
        .Width = static_cast<UINT>(desc.width),
        .Height = static_cast<UINT>(desc.height),
        .Format = desc.format,
        .SampleDesc = {.Count = 1, // MSAA Count
                       .Quality = 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = desc.buffer_count,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = {},
    };

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = {};
    fullscreen_desc.Windowed = true;

    const auto hwnd = window.get_hwnd();
    if (!hwnd ||
        FAILED(
            dxgi_factory->CreateSwapChainForHwnd(device, hwnd, &swapchain_desc, &fullscreen_desc, nullptr, &swapchain)))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Swapchain\n");
        return false;
    }

    // create swap chain render target
    return create_swapchain_resources(device);
}

bool D3D11Swapchain::create_swapchain_resources(ID3D11Device* const device)
{
    ComPtr<ID3D11Texture2D> back_buffer = nullptr;
    if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer))))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to get Back Buffer from Swapchain\n");
        return false;
    }

    if (FAILED(device->CreateRenderTargetView(back_buffer.Get(), nullptr, &sc_render_target)))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to create Render Target View\n");
        return false;
    }
    set_debug_name(sc_render_target.Get(), "Swapchain Render Target");
    // Don't need to keep the back buffer reference only needed it to create RTV
    // D3D11 auto swaps the back buffer, uses the same pointer
    return true;
}

bool D3D11Swapchain::resize(ID3D11Device* const device,
                            ID3D11DeviceContext* const device_context,
                            const int width,
                            const int height)
{
    device_context->Flush();

    sc_render_target.Reset();

    if (FAILED(swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM, 0)))
    {
        OutputDebugStringA("Qhenki D3D11 ERROR: Failed to resize Swapchain buffers\n");
        return false;
    }

    return create_swapchain_resources(device);
}
