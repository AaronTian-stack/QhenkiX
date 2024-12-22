#include "d3d11_context.h"

#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>

#include "d3d11_shader.h"
#include "d3d11_swapchain.h"
#include "d3d11_pipeline.h"

void D3D11Context::create()
{
    // Create factory
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

	// Pick discrete GPU
    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(dxgi_factory->EnumAdapterByGpuPreference(
        0, // Adapter index
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        __uuidof(IDXGIAdapter1),
        reinterpret_cast<void**>(adapter.GetAddressOf())
    )))
    {
		std::cerr << "D3D11: Failed to find discrete GPU. Defaulting to 0th adapter" << std::endl;
        if (FAILED(dxgi_factory->EnumAdapters1(0, &adapter)))
        {
			throw std::runtime_error("D3D11: Failed to find a adapter");
        }
    }

    DXGI_ADAPTER_DESC1 desc;
    HRESULT hr = adapter->GetDesc1(&desc);
	if (FAILED(hr)) std::cerr << "D3D11: Failed to get adapter description" << std::endl;
    else std::wcout << L"Selected D3D11 Adapter: " << desc.Description << L"\n";

    // Create device
    if (FAILED(D3D11CreateDevice(
        adapter.Get(),
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        creationFlags,
        &deviceFeatureLevel,
        1,
        D3D11_SDK_VERSION,
        &device_,
        nullptr,
        &device_context_)))
    {
        throw std::runtime_error("D3D11: Failed to create D3D11 Device");
    }
	
#ifdef _DEBUG
    constexpr char deviceName[] = "d3d11_device";
    device_->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(deviceName), deviceName);
    if (FAILED(device_.As(&debug_)))
    {
        throw std::runtime_error("D3D11: Failed to get the debug layer from the device");
    }
#endif
}

bool D3D11Context::create_swapchain(DisplayWindow& window, vendetta::Swapchain& swapchain)
{
	swapchain.internal_state = mkS<D3D11Swapchain>();
	auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	return swap_d3d11->create(swapchain.desc, window, dxgi_factory, device_);
}

bool D3D11Context::resize_swapchain(vendetta::Swapchain& swapchain, int width, int height)
{
	wait_all();
    auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	assert(swap_d3d11);
	assert(swap_d3d11->swapchain);
    return swap_d3d11->resize(device_, width, height);
}

bool D3D11Context::create_shader(vendetta::Shader& shader, const std::wstring& path, vendetta::ShaderType type,
                                 std::vector<D3D_SHADER_MACRO> macros)
{
	shader.internal_state = mkS<D3D11Shader>();

	macros.emplace_back(nullptr, nullptr);
	auto shader_d3d11 = static_cast<D3D11Shader*>(shader.internal_state.get());
    switch(type)
    {
	    case vendetta::ShaderType::VERTEX:
	    {
			shader_d3d11->vertex = D3D11Shader::vertex_shader(device_, path, shader_d3d11->vertex_blob, macros.data());
            break;
	    }
		case vendetta::ShaderType::PIXEL:
		{
			shader_d3d11->pixel = D3D11Shader::pixel_shader(device_, path, macros.data());
			break;
		}
		case vendetta::ShaderType::COMPUTE:
		{
			throw std::runtime_error("D3D11: Compute Shader not implemented");
		}
    }
    return true;
}

bool D3D11Context::create_pipeline(vendetta::GraphicsPipeline& pipeline, vendetta::Shader& vertex_shader, vendetta::Shader& pixel_shader)
{
	// D3D11 does not have concept of pipelines. D3D11 pipeline is just shader + state + input layout
	pipeline.internal_state = mkS<D3D11GraphicsPipeline>();
	const auto d3d11_pipeline = static_cast<D3D11GraphicsPipeline*>(pipeline.internal_state.get());
	const auto d3d11_vertex_shader = static_cast<D3D11Shader*>(vertex_shader.internal_state.get());
	const auto d3d11_pixel_shader = static_cast<D3D11Shader*>(pixel_shader.internal_state.get());
    assert(d3d11_pipeline);
	assert(d3d11_vertex_shader);
    assert(d3d11_pixel_shader);

    d3d11_pipeline->vertex_shader_ = vertex_shader.internal_state.get();
	d3d11_pipeline->pixel_shader_ = pixel_shader.internal_state.get();

    // Create and store input layout
	const auto input_layout_ = layout_assembler_.create_input_layout_reflection(device_, d3d11_vertex_shader->vertex_blob, pipeline.interleaved);
	d3d11_pipeline->input_layout_ = input_layout_;

    bool succeeded = false;

	// Create rasterizer state object (deduplicated)
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createrasterizerstate
	if (const auto rs = pipeline.desc.rasterizer_state)
    {
        D3D11_RASTERIZER_DESC rasterizer_desc = 
        {
            .FillMode = static_cast<D3D11_FILL_MODE>(rs->fill_mode),
			.CullMode = static_cast<D3D11_CULL_MODE>(rs->cull_mode),
			.FrontCounterClockwise = rs->front_counter_clockwise,
			.DepthBias = rs->depth_bias,
			.DepthBiasClamp = rs->depth_bias_clamp,
			.SlopeScaledDepthBias = rs->slope_scaled_depth_bias,
			.DepthClipEnable = rs->depth_clip_enable,
			.ScissorEnable = FALSE, // Scissor enable not included (TODO: add later?)
			.MultisampleEnable = FALSE, // Multisample enable not included (TODO: add later?)
			.AntialiasedLineEnable = FALSE, // Antialiased line not included (TODO: add later?)
        };
        if (FAILED(device_->CreateRasterizerState(&rasterizer_desc, &d3d11_pipeline->rasterizer_state_)))
        {
            std::cerr << "D3D11: Failed to create Rasterizer State" << std::endl;
			succeeded = false;
        }
    }

    // Create blend state
	if (const auto blend = pipeline.desc.blend_desc)
    {
        D3D11_BLEND_DESC blend_desc;
        blend_desc.AlphaToCoverageEnable = blend->AlphaToCoverageEnable;
        blend_desc.IndependentBlendEnable = blend->IndependentBlendEnable;
		for (int i = 0; i < 8; i++)
		{
            auto& brt = blend_desc.RenderTarget[i];
            // D3D11 does not have logic operations
			assert(!(blend->RenderTarget[i].BlendEnable && blend->RenderTarget[i].LogicOpEnable));
            blend_desc.RenderTarget[i] =
            {
	            .BlendEnable = blend->RenderTarget[i].BlendEnable,
	            .SrcBlend = static_cast<D3D11_BLEND>(blend->RenderTarget[i].SrcBlend),
	            .DestBlend = static_cast<D3D11_BLEND>(blend->RenderTarget[i].DestBlend),
	            .BlendOp = static_cast<D3D11_BLEND_OP>(blend->RenderTarget[i].BlendOp),
	            .SrcBlendAlpha = static_cast<D3D11_BLEND>(blend->RenderTarget[i].SrcBlendAlpha),
	            .DestBlendAlpha = static_cast<D3D11_BLEND>(blend->RenderTarget[i].DestBlendAlpha),
	            .BlendOpAlpha = static_cast<D3D11_BLEND_OP>(blend->RenderTarget[i].BlendOpAlpha),
	            .RenderTargetWriteMask = blend->RenderTarget[i].RenderTargetWriteMask,
            };
		}
        if (FAILED(device_->CreateBlendState(&blend_desc, &d3d11_pipeline->blend_state_)))
        {
            std::cerr << "D3D11: Failed to create Blend State" << std::endl;
			succeeded = false;
        }
    }

	// Create depth stencil state
    if (const auto ds = pipeline.desc.depth_stencil_state)
    {
        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc;
		depth_stencil_desc.DepthEnable = ds->depth_enable;
		depth_stencil_desc.DepthWriteMask = static_cast<D3D11_DEPTH_WRITE_MASK>(ds->depth_write_mask);
		depth_stencil_desc.DepthFunc = static_cast<D3D11_COMPARISON_FUNC>(ds->depth_func);
		depth_stencil_desc.StencilEnable = ds->stencil_enable;
		depth_stencil_desc.StencilReadMask = ds->stencil_read_mask;
		depth_stencil_desc.StencilWriteMask = ds->stencil_write_mask;
        depth_stencil_desc.FrontFace =
        {
            .StencilFailOp = static_cast<D3D11_STENCIL_OP>(ds->front_face.StencilFailOp),
            .StencilDepthFailOp = static_cast<D3D11_STENCIL_OP>(ds->front_face.StencilDepthFailOp),
            .StencilPassOp = static_cast<D3D11_STENCIL_OP>(ds->front_face.StencilPassOp),
            .StencilFunc = static_cast<D3D11_COMPARISON_FUNC>(ds->front_face.StencilFunc)
        };
		depth_stencil_desc.BackFace =
		{
			.StencilFailOp = static_cast<D3D11_STENCIL_OP>(ds->back_face.StencilFailOp),
			.StencilDepthFailOp = static_cast<D3D11_STENCIL_OP>(ds->back_face.StencilDepthFailOp),
			.StencilPassOp = static_cast<D3D11_STENCIL_OP>(ds->back_face.StencilPassOp),
			.StencilFunc = static_cast<D3D11_COMPARISON_FUNC>(ds->back_face.StencilFunc)
		};
		if (FAILED(device_->CreateDepthStencilState(&depth_stencil_desc, &d3d11_pipeline->depth_stencil_state_)))
		{
			std::cerr << "D3D11: Failed to create Depth Stencil State" << std::endl;
			succeeded = false;
		}
    }

    return succeeded;
}

void D3D11Context::wait_all()
{
    device_context_->Flush();
}

bool D3D11Context::present(vendetta::Swapchain& swapchain)
{
	auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
    // TODO: remove clear color it is for testing!
    constexpr float clearColor[] = { 1.0f, 0.1f, 0.1f, 1.0f };
	device_context_->ClearRenderTargetView(swap_d3d11->sc_render_target.Get(), clearColor);
	auto result = swap_d3d11->swapchain->Present(1, 0);
    return result == S_OK;
}

D3D11Context::~D3D11Context()
{
    device_context_->ClearState();
    device_context_->Flush();
    device_context_.Reset();
    dxgi_factory.Reset();
#if _DEBUG
    debug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
    debug_.Reset();
#endif
    device_.Reset();
}
