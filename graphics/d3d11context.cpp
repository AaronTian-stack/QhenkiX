#include "d3d11context.h"

#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>

#include "shader.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")

void D3D11Context::create(DisplayWindow& window)
{
    // create factory
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory))))
    {
		throw std::runtime_error("Failed to create DXGI Factory");
    }

    // Targets features supported by Direct3D 11.1, including shader model 5 and logical blend operations.
    // This feature level requires a display driver that is at least implemented to WDDM for Windows 8 (WDDM 1.2).
    constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_1;

	// create device
    // adapter is controlled by driver. must change in windows settings
    if (FAILED(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        &deviceFeatureLevel,
        1,
        D3D11_SDK_VERSION,
        &device,
        nullptr,
        &device_context)))
    {
		throw std::runtime_error("Failed to create D3D11 Device");
    }

	this->window = &window;
    const auto info = window.get_display_info();

	// create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
	swapChainDescriptor.Width = info.width;
	swapChainDescriptor.Height = info.height;
    swapChainDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDescriptor.SampleDesc.Count = 1; // MSAA
    swapChainDescriptor.SampleDesc.Quality = 0; 
    swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescriptor.BufferCount = 2;
    swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDescriptor.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
    swapChainDescriptor.Flags = {};

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
		throw std::runtime_error("Failed to create Swap Chain");
    }

    // create swap chain render target
	create_swapchain_resources();
}

void D3D11Context::create_swapchain_resources()
{
    ComPtr<ID3D11Texture2D> backBuffer = nullptr;
    if (FAILED(swapchain->GetBuffer(
        0,
        IID_PPV_ARGS(&backBuffer))))
    {
		throw std::runtime_error("Failed to get Back Buffer from Swap Chain");
    }

    if (FAILED(device->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        &sc_render_target)))
    {
		throw std::runtime_error("Failed to create Render Target View");
    }
    // don't need to keep the back buffer reference only needed it to create RTV
}

void D3D11Context::destroy_swapchain_resources()
{
    sc_render_target.Reset();
}

void D3D11Context::resize_swapchain(int width, int height)
{
    device_context->Flush();

    destroy_swapchain_resources();

    if (FAILED(swapchain->ResizeBuffers(
        0,
        width,
        height,
        DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
        0)))
    {
		throw std::runtime_error("Failed to resize Swap Chain buffers");
    }

    create_swapchain_resources();
}

void D3D11Context::destroy()
{
    device_context->Flush();
    destroy_swapchain_resources();
    swapchain.Reset();
    dxgi_factory.Reset();
    device_context.Reset();
    device.Reset();
}

ComPtr<ID3D11VertexShader> D3D11Context::create_vertex_shader(const std::wstring& fileName,
	ComPtr<ID3DBlob>& vertexShaderBlob, const D3D_SHADER_MACRO* macros)
{
	return Shader::vertex_shader(device, fileName, vertexShaderBlob, macros);
}

ComPtr<ID3D11PixelShader> D3D11Context::create_pixel_shader(const std::wstring& fileName,
	const D3D_SHADER_MACRO* macros)
{
	return Shader::pixel_shader(device, fileName, macros);
}

void D3D11Context::clear(const ComPtr<ID3D11RenderTargetView>& render_target, const float r, const float g, const float b, const float a)
{
	const float clearColor[] = { r, g, b, a };
	device_context->ClearRenderTargetView(
        render_target.Get(),
		clearColor);
}

void D3D11Context::clear(const ComPtr<ID3D11RenderTargetView>& render_target, const XMFLOAT4 color)
{
	clear(render_target, color.x, color.y, color.z, color.w);
}

void D3D11Context::present(unsigned int blanks)
{
	assert(blanks <= 4);
    // nth blank (1 = double buffer)
	swapchain->Present(blanks, 0);
}

void D3D11Context::debug_clear()
{
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
	auto s = window->get_display_size();
    viewport.Width = static_cast<float>(s.x);
    viewport.Height = static_cast<float>(s.y);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    clear(sc_render_target, { 0.0f, 0.0f, 0.0f, 1.0f });

    // multiple viewports is only for geometry shader
    device_context->RSSetViewports(
        1,
        &viewport);
    device_context->OMSetRenderTargets(
        1,
        sc_render_target.GetAddressOf(),
        nullptr);
}
