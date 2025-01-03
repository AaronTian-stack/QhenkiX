#include "d3d11_context.h"

#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>

#include "d3d11_shader.h"
#include "d3d11_swapchain.h"
#include "d3d11_pipeline.h"
#include "d3d11_render_target.h"

void D3D11Context::create()
{
    // Create factory
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory))))
    {
		std::cerr << "D3D11: Failed to create DXGI Factory" << std::endl;
        throw std::runtime_error("D3D11: Failed to create DXGI Factory");
    }
#ifdef _DEBUG
    constexpr char factoryName[] = "DXGI Factory";
    dxgi_factory->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(factoryName), factoryName);
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
    else std::wcout << L"D3D11: Selected adapter: " << desc.Description << L"\n";

	constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

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

bool D3D11Context::create_swapchain(DisplayWindow& window, const vendetta::SwapchainDesc& swapchain_desc, vendetta::Swapchain& swapchain)
{
	swapchain.desc = swapchain_desc;
	swapchain.internal_state = mkS<D3D11Swapchain>();
	auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	return swap_d3d11->create(swapchain_desc, window, dxgi_factory.Get(), device_.Get());
}

bool D3D11Context::resize_swapchain(vendetta::Swapchain& swapchain, int width, int height)
{
	wait_all();
    auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	assert(swap_d3d11);
	assert(swap_d3d11->swapchain);
    return swap_d3d11->resize(device_.Get(), device_context_.Get(), width, height);
}

bool D3D11Context::create_shader(vendetta::Shader& shader, const std::wstring& path, vendetta::ShaderType type,
                                 std::vector<D3D_SHADER_MACRO> macros)
{
	shader.type = type;
	shader.internal_state = mkS<D3D11Shader>();

	macros.emplace_back(nullptr, nullptr);
	auto shader_d3d11 = static_cast<D3D11Shader*>(shader.internal_state.get());
    switch(type)
    {
	    case vendetta::ShaderType::VERTEX_SHADER:
	    {
			shader_d3d11->vertex = D3D11Shader::vertex_shader(device_.Get(), path, shader_d3d11->vertex_blob, macros.data());
            break;
	    }
		case vendetta::ShaderType::PIXEL_SHADER:
		{
			shader_d3d11->pixel = D3D11Shader::pixel_shader(device_.Get(), path, macros.data());
			break;
		}
		case vendetta::ShaderType::COMPUTE_SHADER:
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
	ID3D11InputLayout* input_layout_ = nullptr;
	input_layout_ = layout_assembler_.create_input_layout_reflection(device_.Get(), d3d11_vertex_shader->vertex_blob.Get(), pipeline.interleaved);
	d3d11_pipeline->input_layout_ = input_layout_;

	bool succeeded = input_layout_ != nullptr;

	if (const auto desc = pipeline.desc)
	{
		// Create Rasterizer state object
		if (const auto rs = desc->rasterizer_state)
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

		// Create Blend state
		if (const auto blend = desc->blend_desc)
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

		// Create Depth Stencil state
		if (const auto ds = desc->depth_stencil_state)
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
	}

    return succeeded;
}

bool D3D11Context::bind_pipeline(vendetta::CommandList& cmd_list, vendetta::GraphicsPipeline& pipeline)
{
	const auto d3d11_pipeline = static_cast<D3D11GraphicsPipeline*>(pipeline.internal_state.get());
	assert(d3d11_pipeline);
	d3d11_pipeline->bind(device_context_);
	return true;
}

bool D3D11Context::create_buffer(const vendetta::BufferDesc& desc, const void* data, vendetta::Buffer& buffer)
{
	buffer.desc = desc;
	buffer.internal_state = mkS<ComPtr<ID3D11Buffer>>();
	const auto buffer_d3d11 = static_cast<ComPtr<ID3D11Buffer>*>(buffer.internal_state.get());

	D3D11_BUFFER_DESC bufferInfo{};
	bufferInfo.ByteWidth = desc.size;
	switch(desc.usage)
	{
		case vendetta::BufferUsage::VERTEX:
			bufferInfo.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			break;
		case vendetta::BufferUsage::INDEX:
			bufferInfo.BindFlags = D3D11_BIND_INDEX_BUFFER;
			break;
		case vendetta::BufferUsage::UNIFORM:
			bufferInfo.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			break;
		case vendetta::BufferUsage::STORAGE:
			bufferInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			break;
		case vendetta::BufferUsage::INDIRECT:
			bufferInfo.BindFlags = D3D11_BIND_UNORDERED_ACCESS; // TODO: check this
			break;
		case vendetta::BufferUsage::TRANSFER_SRC:
		case vendetta::BufferUsage::TRANSFER_DST:
			bufferInfo.BindFlags = 0; // TODO: check this
			break;
		default:
			throw std::runtime_error("D3D11: buffer type not implemented");
	}
	if ((desc.visibility & vendetta::BufferVisibility::CPU_SEQUENTIAL) 
		|| (desc.visibility & vendetta::BufferVisibility::CPU_RANDOM))
	{
		bufferInfo.Usage = D3D11_USAGE_DYNAMIC;
		bufferInfo.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		bufferInfo.Usage = D3D11_USAGE_DEFAULT;
		bufferInfo.CPUAccessFlags = 0;
	}
	// misc flags
	//TODO bufferInfo.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA resourceData;
	resourceData.pSysMem = data;
	if (FAILED(device_->CreateBuffer(
		&bufferInfo,
		&resourceData,
		&*buffer_d3d11)))
	{
		std::cerr << "D3D11: Failed to create triangle vertex buffer" << std::endl;
		return false;
	}
	return true;
}

void D3D11Context::bind_vertex_buffers(vendetta::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count, const vendetta::Buffer* buffers, const
                                      unsigned* offsets)
{
	assert(buffer_count <= 16);
	std::array<ID3D11Buffer**, 16> buffer_d3d11{};
	for (unsigned int i = 0; i < buffer_count; i++)
	{
		buffer_d3d11[i] = static_cast<ComPtr<ID3D11Buffer>*>(buffers[i].internal_state.get())->GetAddressOf();
	}
	// strides must be fetched from current input layout
	ComPtr<ID3D11InputLayout> input_layout = nullptr;
	device_context_->IAGetInputLayout(&input_layout);
	assert(input_layout && "No Input Layout bound"); // if null then no input layout is bound

	auto layout = layout_assembler_.find_layout(input_layout.Get());
	assert(layout);

	// linear search through the list of attributes and find the stride (size of input, float1/2/3/4) of the matching slot number
	std::array<UINT, 16> strides{};
	const auto& desc_vec = layout->desc;
	for (unsigned int i = 0; i < buffer_count; i++)
	{
		const auto slot = start_slot + i;
		for (const auto& desc : desc_vec)
		{
			bool found = false;
			if (desc.InputSlot == slot)
			{
				switch (desc.Format)
				{
				case DXGI_FORMAT_R32G32B32A32_FLOAT:
					strides[i] = 4 * sizeof(float);
					break;
				case DXGI_FORMAT_R32G32B32_FLOAT:
					strides[i] = 3 * sizeof(float);
					break;
				case DXGI_FORMAT_R32G32_FLOAT:
					strides[i] = 2 * sizeof(float);
					break;
				case DXGI_FORMAT_R32_FLOAT:
					strides[i] = sizeof(float);
					break;
				default:
					std::cerr << "D3D11: Unknown format in input layout" << std::endl;
					throw std::runtime_error("D3D11: Unknown format in input layout");
				}
				found = true;
				break;
			}
			assert(found && "D3D11: Input slot not found in input layout");
		}
	}
	device_context_->IASetVertexBuffers(start_slot, buffer_count, buffer_d3d11[0], strides.data(), offsets);
}

void D3D11Context::start_render_pass(vendetta::CommandList& cmd_list, vendetta::Swapchain& swapchain,
                                     const vendetta::RenderTarget* depth_stencil)
{
	auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	auto rtv = swap_d3d11->sc_render_target.Get();
	const float clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	device_context_->ClearRenderTargetView(rtv, clear_color);
	device_context_->OMSetRenderTargets(1, &rtv, nullptr);
}

void D3D11Context::start_render_pass(vendetta::CommandList& cmd_list, unsigned rt_count,
                                     const vendetta::RenderTarget* rts, const vendetta::RenderTarget* depth_stencil)
{
    std::array<ID3D11RenderTargetView**, 8> rtvs{};
    // Clear render target views (if applicable)
	for (unsigned int i = 0; i < rt_count; i++)
	{
		auto& desc = rts[i].desc;
		auto d3d11_rtv = static_cast<D3D11RenderTarget*>(rts[i].internal_state.get());
		if (desc.clear_color)
		{
			device_context_->ClearRenderTargetView(d3d11_rtv->rtv.Get(), desc.clear_color_value.data());
		}
		rtvs[i] = d3d11_rtv->rtv.GetAddressOf();
	}
    // D3D11 does not have concept of render pass
    // Set render target views
    // TODO: depth stencil views
	ID3D11DepthStencilView* ds = nullptr;
	if (depth_stencil)
	{
		assert(false && "D3D11: Depth Stencil not implemented");
		//device_context_->ClearDepthStencilView(dsi->rtv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
    device_context_->OMSetRenderTargets(rt_count, rtvs[0], ds);
}

void D3D11Context::set_viewports(unsigned count, const D3D12_VIEWPORT* viewport)
{
    for (int i = 0; i < count; i++)
    {
		viewports[i] =
		{
			.TopLeftX = viewport[i].TopLeftX,
			.TopLeftY = viewport[i].TopLeftY,
			.Width = viewport[i].Width,
			.Height = viewport[i].Height,
			.MinDepth = viewport[i].MinDepth,
			.MaxDepth = viewport[i].MaxDepth,
		};

    }
	device_context_->RSSetViewports(count, viewports.data());
}

void D3D11Context::draw(vendetta::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset)
{
	device_context_->Draw(vertex_count, start_vertex_offset);
}

void D3D11Context::draw_indexed(vendetta::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
	int32_t base_vertex_offset)
{
	device_context_->DrawIndexed(index_count, start_index_offset, base_vertex_offset);
}

void D3D11Context::wait_all()
{
    device_context_->Flush();
}

bool D3D11Context::present(vendetta::Swapchain& swapchain)
{
	auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	auto result = swap_d3d11->swapchain->Present(1, 0);
    return result == S_OK;
}

D3D11Context::~D3D11Context()
{
	layout_assembler_.dispose();
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
