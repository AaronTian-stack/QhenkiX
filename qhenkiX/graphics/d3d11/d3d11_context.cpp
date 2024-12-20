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
        throw std::runtime_error("Failed to create DXGI Factory");
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

    // create device
    // adapter is controlled by driver. must change in windows settings
    if (FAILED(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        &deviceFeatureLevel,
        1,
        D3D11_SDK_VERSION,
        &device,
        nullptr,
        &device_context)))
    {
        throw std::runtime_error("Failed to create D3D11 Device");
    }

#ifdef _DEBUG
    constexpr char deviceName[] = "d3d11_device";
    //device->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(deviceName), deviceName);
    if (FAILED(device.As(&debug)))
    {
        throw std::runtime_error("D3D11: Failed to get the debug layer from the device");
    }
#endif

    debug->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
    OutputDebugString(L"fuck\n");
}

void D3D11Context::destroy()
{
	device_context->ClearState();
    device_context->Flush();
    dxgi_factory.Reset();
    device_context.Reset();
#if _DEBUG
    debug->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
    debug.Reset();
#endif
    device.Reset();
}

void D3D11Context::create_swapchain(DisplayWindow& window, vendetta::Swapchain& swapchain)
{
 //   swapchain.internal_state = mkS<D3D11Swapchain>();
	//auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	//swap_d3d11->create(swapchain.desc, window, dxgi_factory, device);
}

bool D3D11Context::resize_swapchain(vendetta::Swapchain& swapchain, int width, int height)
{
    return true;
}

bool D3D11Context::create_shader(vendetta::Shader* shader, const char* path, vendetta::ShaderType type,
	std::vector<D3D_SHADER_MACRO> macros)
{
    return true;
}

void D3D11Context::wait_all()
{
	
}

//D3D11Context::~D3D11Context()
//{
//    //destroy_swapchain_resources();
//    //swapchain.Reset();
//    dxgi_factory.Reset();
//    device_context->ClearState();
//    device_context->Flush();
//    device_context.Reset();
//#if _DEBUG
//    debug->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
//    debug.Reset();
//#endif
//    device.Reset();
//}
