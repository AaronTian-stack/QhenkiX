#include "d3d11_context.h"

#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>

#include "d3d11_shader.h"
#include "d3d11_swapchain.h"

void D3D11Context::create()
{
    // create factory
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory))))
    {
        throw std::runtime_error("D3D11: Failed to create DXGI Factory");
    }
#ifdef _DEBUG
    constexpr char factoryName[] = "dxgi_factory";
    dxgi_factory->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(factoryName), factoryName);
#endif

    // Targets features supported by Direct3D 11.1, including shader model 5 and logical blend operations.
    // This feature level requires a display driver that is at least implemented to WDDM for Windows 8 (WDDM 1.2).
    constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_1;

    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// pick discrete GPU (just find one labeled NVIDIA or AMD)
    ComPtr<IDXGIAdapter> adapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgi_factory->EnumAdapters(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC adapterDesc;
        adapter->GetDesc(&adapterDesc);

        // Check if the adapter is a discrete GPU
        if (adapterDesc.VendorId == 0x10DE || adapterDesc.VendorId == 0x1002) // NVIDIA or AMD
        {
            // Found a discrete GPU
            break;
        }
        adapter.Reset();
    }

    // if no discrete GPU is found, use the first adapter
    if (!adapter)
    {
        if (FAILED(dxgi_factory->EnumAdapters(0, &adapter)))
        {
            throw std::runtime_error("D3D11: Failed to enumerate adapters");
        }
    }

    // create device
    if (FAILED(D3D11CreateDevice(
        adapter.Get(),
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        creationFlags,
        &deviceFeatureLevel,
        1,
        D3D11_SDK_VERSION,
        &device,
        nullptr,
        &device_context)))
    {
        throw std::runtime_error("D3D11: Failed to create D3D11 Device");
    }
	
#ifdef _DEBUG
    constexpr char deviceName[] = "d3d11_device";
    device->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(deviceName), deviceName);
    if (FAILED(device.As(&debug)))
    {
        throw std::runtime_error("D3D11: Failed to get the debug layer from the device");
    }
#endif
}

bool D3D11Context::create_swapchain(DisplayWindow& window, vendetta::Swapchain& swapchain)
{
	swapchain.internal_state = mkS<D3D11Swapchain>();
	auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	return swap_d3d11->create(swapchain.desc, window, dxgi_factory, device);
}

bool D3D11Context::resize_swapchain(vendetta::Swapchain& swapchain, int width, int height)
{
	wait_all();
    auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	assert(swap_d3d11);
	assert(swap_d3d11->swapchain);
    return swap_d3d11->resize(device, width, height);
}

bool D3D11Context::create_shader(vendetta::Shader& shader, const std::wstring& path, vendetta::ShaderType type,
                                 std::vector<D3D_SHADER_MACRO> macros)
{
	macros.emplace_back(nullptr, nullptr);
	auto shader_d3d11 = static_cast<D3D11Shader*>(shader.internal_state.get());
    switch(type)
    {
	    case vendetta::ShaderType::VERTEX:
	    {
			shader_d3d11->vertex = D3D11Shader::vertex_shader(device, path, shader_d3d11->vertex_blob, macros.data());
            break;
	    }
		case vendetta::ShaderType::PIXEL:
		{
			shader_d3d11->pixel = D3D11Shader::pixel_shader(device, path, macros.data());
			break;
		}
		case vendetta::ShaderType::COMPUTE:
		{
			throw std::runtime_error("D3D11: Compute Shader not implemented");
		}
    }
    return true;
}

bool D3D11Context::create_pipeline(vendetta::Pipeline& pipeline, vendetta::Shader& shader)
{
	// d3d11 does not have concept of pipelines. d3d11 pipeline is just shader + state + input layout
    return false;
}

void D3D11Context::wait_all()
{
    device_context->Flush();
}

bool D3D11Context::present(vendetta::Swapchain& swapchain)
{
	auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
    // TODO: remove clear color it is for testing!
    constexpr float clearColor[] = { 1.0f, 0.1f, 0.1f, 1.0f };
	device_context->ClearRenderTargetView(swap_d3d11->sc_render_target.Get(), clearColor);
	auto result = swap_d3d11->swapchain->Present(1, 0);
    return result == S_OK;
}

D3D11Context::~D3D11Context()
{
    device_context->ClearState();
    device_context->Flush();
    device_context.Reset();
    dxgi_factory.Reset();
#if _DEBUG
    debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
    debug.Reset();
#endif
    device.Reset();
}
