#include "d3d11_context.h"

#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <iostream>

#include "d3d11_shader.h"
#include "d3d11_swapchain.h"
#include "d3d11_pipeline.h"
#include "d3d11_shader_compiler.h"

void D3D11Context::create()
{
    // Create factory
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgi_factory_))))
    {
		std::cerr << "D3D11: Failed to create DXGI Factory" << std::endl;
        throw std::runtime_error("D3D11: Failed to create DXGI Factory");
    }
#ifdef _DEBUG
    constexpr char factoryName[] = "DXGI Factory";
    m_dxgi_factory_->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(factoryName), factoryName);
#endif

	// Pick discrete GPU
    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(m_dxgi_factory_->EnumAdapterByGpuPreference(
        0, // Adapter index
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
        __uuidof(IDXGIAdapter1),
        reinterpret_cast<void**>(adapter.GetAddressOf())
    )))
    {
		std::cerr << "D3D11: Failed to find discrete GPU. Defaulting to 0th adapter" << std::endl;
        if (FAILED(m_dxgi_factory_->EnumAdapters1(0, &adapter)))
        {
			throw std::runtime_error("D3D11: Failed to find a adapter");
        }
    }

    DXGI_ADAPTER_DESC1 desc;
    HRESULT hr = adapter->GetDesc1(&desc);
	if (FAILED(hr)) std::cerr << "D3D11: Failed to get adapter description" << std::endl;
    else std::wcout << L"D3D11: Selected adapter: " << desc.Description << L"\n";

	constexpr D3D_FEATURE_LEVEL device_feature_level = D3D_FEATURE_LEVEL_11_0;

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
        &device_feature_level,
        1,
        D3D11_SDK_VERSION,
        &m_device_,
        nullptr,
        &m_device_context_)))
    {
        throw std::runtime_error("D3D11: Failed to create D3D11 Device");
    }
	
#ifdef _DEBUG
    constexpr char deviceName[] = "d3d11_device";
    m_device_->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(deviceName), deviceName);
    if (FAILED(m_device_.As(&m_debug_)))
    {
        throw std::runtime_error("D3D11: Failed to get the debug layer from the device");
    }
#endif

	shader_compiler = mkU<D3D11ShaderCompiler>();
}

bool D3D11Context::create_swapchain(DisplayWindow& window, const qhenki::graphics::SwapchainDesc& swapchain_desc, qhenki::graphics::Swapchain& swapchain, qhenki::graphics::Queue
                                    & direct_queue)
{
	swapchain.desc = swapchain_desc;
	swapchain.internal_state = mkS<D3D11Swapchain>();
	auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	return swap_d3d11->create(swapchain_desc, window, m_dxgi_factory_.Get(), m_device_.Get());
}

bool D3D11Context::resize_swapchain(qhenki::graphics::Swapchain& swapchain, int width, int height)
{
	wait_all();
    auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	assert(swap_d3d11);
	assert(swap_d3d11->swapchain);
    return swap_d3d11->resize(m_device_.Get(), m_device_context_.Get(), width, height);
}

bool D3D11Context::create_shader_dynamic(ShaderCompiler* compiler, qhenki::graphics::Shader& shader, const CompilerInput& input)
{
	if (compiler == nullptr)
	{
		compiler = shader_compiler.get();
	}

	CompilerOutput output = {};
	// ID3DBlob
	if (!compiler->compile(input, output))
	{
		return false;
	}

	shader.type = input.shader_type;
	shader.shader_model = input.min_shader_model;
	bool result = true;
	// Calls CreateXShader(). Thread safe since it only uses the device
	shader.internal_state = mkS<D3D11Shader>(m_device_.Get(), input.shader_type, input.path, output, result);

    return result;
}

bool D3D11Context::create_pipeline(const qhenki::graphics::GraphicsPipelineDesc& desc, qhenki::graphics::GraphicsPipeline& pipeline, qhenki::graphics::Shader& vertex_shader, qhenki::graphics::Shader& pixel_shader, wchar_t
                                   const* debug_name)
{
	// D3D11 does not have concept of pipelines. D3D11 "pipeline" is just shader + state + input layout
	pipeline.internal_state = mkS<D3D11GraphicsPipeline>();
	const auto d3d11_pipeline = static_cast<D3D11GraphicsPipeline*>(pipeline.internal_state.get());
	const auto d3d11_vertex_shader = static_cast<D3D11Shader*>(vertex_shader.internal_state.get());
	const auto d3d11_pixel_shader = static_cast<D3D11Shader*>(pixel_shader.internal_state.get());
	assert(d3d11_pipeline);
	assert(d3d11_vertex_shader);
	assert(d3d11_pixel_shader);

	d3d11_pipeline->vertex_shader_ = vertex_shader.internal_state.get();
	d3d11_pipeline->pixel_shader_ = pixel_shader.internal_state.get();

	const auto true_vs = std::get_if<D3D11VertexShader>(&d3d11_vertex_shader->m_shader_);
	assert(true_vs);

	ID3D11InputLayout* input_layout_ = m_layout_assembler_.create_input_layout_reflection(m_device_.Get(),
		true_vs->vertex_shader_blob.Get(), desc.interleaved);
	d3d11_pipeline->input_layout_ = input_layout_;

	bool succeeded = input_layout_ != nullptr;

	// Create Rasterizer state object
	if (const auto& rs = desc.rasterizer_state; rs.has_value())
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
		if (FAILED(m_device_->CreateRasterizerState(&rasterizer_desc, &d3d11_pipeline->rasterizer_state_)))
		{
			std::cerr << "D3D11: Failed to create Rasterizer State" << std::endl;
			succeeded = false;
		}
	}

	// Create Blend state
	if (const auto& blend = desc.blend_desc; blend.has_value())
	{
		D3D11_BLEND_DESC blend_desc
		{
			.AlphaToCoverageEnable = blend->AlphaToCoverageEnable,
			.IndependentBlendEnable = blend->IndependentBlendEnable,
		};
		for (int i = 0; i < 8; i++)
		{
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
		if (FAILED(m_device_->CreateBlendState(&blend_desc, &d3d11_pipeline->blend_state_)))
		{
			std::cerr << "D3D11: Failed to create Blend State" << std::endl;
			succeeded = false;
		}
	}

	// Create Depth Stencil state
	if (const auto& ds = desc.depth_stencil_state; ds.has_value())
	{
		D3D11_DEPTH_STENCIL_DESC depth_stencil_desc =
		{
			.DepthEnable = static_cast<BOOL>(ds->depth_enable),
			.DepthWriteMask = static_cast<D3D11_DEPTH_WRITE_MASK>(ds->depth_write_mask),
			.DepthFunc = static_cast<D3D11_COMPARISON_FUNC>(ds->depth_func),
			.StencilEnable = ds->stencil_enable,
			.StencilReadMask = ds->stencil_read_mask,
			.StencilWriteMask = ds->stencil_write_mask,
			.FrontFace =
			{
				.StencilFailOp = static_cast<D3D11_STENCIL_OP>(ds->front_face.StencilFailOp),
				.StencilDepthFailOp = static_cast<D3D11_STENCIL_OP>(ds->front_face.StencilDepthFailOp),
				.StencilPassOp = static_cast<D3D11_STENCIL_OP>(ds->front_face.StencilPassOp),
				.StencilFunc = static_cast<D3D11_COMPARISON_FUNC>(ds->front_face.StencilFunc)
			},
			.BackFace =
			{
				.StencilFailOp = static_cast<D3D11_STENCIL_OP>(ds->back_face.StencilFailOp),
				.StencilDepthFailOp = static_cast<D3D11_STENCIL_OP>(ds->back_face.StencilDepthFailOp),
				.StencilPassOp = static_cast<D3D11_STENCIL_OP>(ds->back_face.StencilPassOp),
				.StencilFunc = static_cast<D3D11_COMPARISON_FUNC>(ds->back_face.StencilFunc)
			},
		};

		if (FAILED(m_device_->CreateDepthStencilState(&depth_stencil_desc, &d3d11_pipeline->depth_stencil_state_)))
		{
			std::cerr << "D3D11: Failed to create Depth Stencil State" << std::endl;
			succeeded = false;
		}
	}

    return succeeded;
}

bool D3D11Context::bind_pipeline(qhenki::graphics::CommandList& cmd_list, qhenki::graphics::GraphicsPipeline& pipeline)
{
	const auto d3d11_pipeline = static_cast<D3D11GraphicsPipeline*>(pipeline.internal_state.get());
	assert(d3d11_pipeline);
	d3d11_pipeline->bind(m_device_context_.Get());
	return true;
}

bool D3D11Context::create_buffer(const qhenki::graphics::BufferDesc& desc, const void* data, qhenki::graphics::Buffer& buffer, wchar_t const* debug_name)
{
	buffer.desc = desc;
	buffer.internal_state = mkS<ComPtr<ID3D11Buffer>>();
	const auto buffer_d3d11 = static_cast<ComPtr<ID3D11Buffer>*>(buffer.internal_state.get());
	assert(buffer_d3d11);

	D3D11_BUFFER_DESC bufferInfo{};
	bufferInfo.ByteWidth = desc.size;
	switch(desc.usage)
	{
		case qhenki::graphics::BufferUsage::VERTEX:
			bufferInfo.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			break;
		case qhenki::graphics::BufferUsage::INDEX:
			bufferInfo.BindFlags = D3D11_BIND_INDEX_BUFFER;
			break;
		case qhenki::graphics::BufferUsage::UNIFORM:
			bufferInfo.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			break;
		case qhenki::graphics::BufferUsage::STORAGE:
			bufferInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			break;
		case qhenki::graphics::BufferUsage::INDIRECT:
			bufferInfo.BindFlags = D3D11_BIND_UNORDERED_ACCESS; // TODO: check this
			break;
		case qhenki::graphics::BufferUsage::TRANSFER_SRC:
		case qhenki::graphics::BufferUsage::TRANSFER_DST:
			bufferInfo.BindFlags = 0; // TODO: check this
			break;
		default:
			throw std::runtime_error("D3D11: buffer type not implemented");
	}
	if ((desc.visibility & qhenki::graphics::BufferVisibility::CPU_SEQUENTIAL) 
		|| (desc.visibility & qhenki::graphics::BufferVisibility::CPU_RANDOM))
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

	D3D11_SUBRESOURCE_DATA resource_data;
	resource_data.pSysMem = data;

	const auto resource_data_ptr = data ? &resource_data : nullptr;

	if (FAILED(m_device_->CreateBuffer(
		&bufferInfo,
		resource_data_ptr,
		&*buffer_d3d11)))
	{
		return false;
	}

#ifdef _DEBUG
    if (debug_name)
    {
		constexpr size_t max_length = 256;
        char debug_name_w[max_length] = {};
		size_t converted_chars = 0;
		wcstombs_s(&converted_chars, debug_name_w, debug_name, max_length - 1);
        set_debug_name(buffer_d3d11->Get(), debug_name_w);
    }
#endif

	return true;
}

void* D3D11Context::map_buffer(const qhenki::graphics::Buffer& buffer)
{
	D3D11_MAPPED_SUBRESOURCE mapped_resource;
	const auto buffer_d3d11 = static_cast<ComPtr<ID3D11Buffer>*>(buffer.internal_state.get());
	assert(buffer_d3d11);
	if (FAILED(m_device_context_->Map(
		buffer_d3d11->Get(),
		0, 
		D3D11_MAP_WRITE_DISCARD, 
		0, 
		&mapped_resource)))
	{
		std::cerr << "D3D11: Failed to map buffer" << std::endl;
		return nullptr;
	}
	return mapped_resource.pData;
}

void D3D11Context::unmap_buffer(const qhenki::graphics::Buffer& buffer)
{
	const auto buffer_d3d11 = static_cast<ComPtr<ID3D11Buffer>*>(buffer.internal_state.get());
	assert(buffer_d3d11);
	m_device_context_->Unmap(buffer_d3d11->Get(), 0);
}

void D3D11Context::bind_vertex_buffers(qhenki::graphics::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count, const qhenki::graphics::Buffer* buffers, const
                                       unsigned* offsets)
{
	assert(buffer_count <= 16);
	std::array<ID3D11Buffer**, 16> buffer_d3d11{};
	for (unsigned int i = 0; i < buffer_count; i++)
	{
		auto buffer = static_cast<ComPtr<ID3D11Buffer>*>(buffers[i].internal_state.get());
		assert(buffer);
		buffer_d3d11[i] = buffer->GetAddressOf();
	}
	// strides must be fetched from current input layout
	ComPtr<ID3D11InputLayout> input_layout;
	m_device_context_->IAGetInputLayout(&input_layout);
	assert(input_layout && "No Input Layout bound"); // if null then no input layout is bound

	auto layout = m_layout_assembler_.find_layout(input_layout.Get());
	assert(layout);

	// linear search through the list of attributes and find the stride (size of input, float1/2/3/4) of the matching slot number
	std::array<UINT, 16> strides{};
	const auto& desc_vec = layout->desc;
	for (unsigned int i = 0; i < buffer_count; i++)
	{
		const auto slot = start_slot + i; // Assumes 1 buffer per slot
		for (const auto& desc : desc_vec)
		{
			bool found = false;
			if (desc.InputSlot == slot)
			{
				switch (desc.Format)
				{
				case DXGI_FORMAT_R32G32B32A32_FLOAT:
					strides[i] += 4 * sizeof(float);
					break;
				case DXGI_FORMAT_R32G32B32_FLOAT:
					strides[i] += 3 * sizeof(float);
					break;
				case DXGI_FORMAT_R32G32_FLOAT:
					strides[i] += 2 * sizeof(float);
					break;
				case DXGI_FORMAT_R32_FLOAT:
					strides[i] += sizeof(float);
					break;
				default:
					std::cerr << "D3D11: Unknown format in input layout" << std::endl;
					throw std::runtime_error("D3D11: Unknown format in input layout");
				}
				found = true;
			}
			assert(found && "D3D11: Input slot not found in input layout");
		}
	}
	m_device_context_->IASetVertexBuffers(start_slot, buffer_count, buffer_d3d11[0], strides.data(), offsets);
}

void D3D11Context::bind_index_buffer(qhenki::graphics::CommandList& cmd_list, const qhenki::graphics::Buffer& buffer, DXGI_FORMAT format,
	unsigned offset)
{
	const auto buffer_d3d11 = static_cast<ComPtr<ID3D11Buffer>*>(buffer.internal_state.get());
	assert(buffer_d3d11);
	m_device_context_->IASetIndexBuffer(buffer_d3d11->Get(), format, offset);
}

bool D3D11Context::create_queue(const qhenki::graphics::QueueType type, qhenki::graphics::Queue& queue)
{
	return true; // D3D11 does not have queues
}

bool D3D11Context::create_command_pool(qhenki::graphics::CommandPool& command_pool, const qhenki::graphics::Queue& queue)
{
	return true; // D3D11 does not have queues
}

void D3D11Context::start_render_pass(qhenki::graphics::CommandList& cmd_list, qhenki::graphics::Swapchain& swapchain,
                                     const qhenki::graphics::RenderTarget* depth_stencil)
{
	const auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	const auto rtv = swap_d3d11->sc_render_target.Get();
	const float clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // TODO: clear color
	m_device_context_->ClearRenderTargetView(rtv, clear_color);
	m_device_context_->OMSetRenderTargets(1, &rtv, nullptr);
}

void D3D11Context::start_render_pass(qhenki::graphics::CommandList& cmd_list, unsigned rt_count,
                                     const qhenki::graphics::RenderTarget* rts, const qhenki::graphics::RenderTarget* depth_stencil)
{
    std::array<ID3D11RenderTargetView**, 8> rtvs{};
    // Clear render target views (if applicable)
	for (unsigned int i = 0; i < rt_count; i++)
	{
		const auto& [clear_color, clear_color_value] = rts[i].desc;
		const auto d3d11_rtv = static_cast<ComPtr<ID3D11RenderTargetView>*>(rts[i].internal_state.get());
		assert(d3d11_rtv);
		if (clear_color)
		{
			m_device_context_->ClearRenderTargetView(d3d11_rtv->Get(), clear_color_value.data());
		}
		rtvs[i] = d3d11_rtv->GetAddressOf();
	}
    // D3D11 does not have concept of render pass
    // Set render target views
    // TODO: depth stencil views
	ID3D11DepthStencilView* ds = nullptr;
	if (depth_stencil)
	{
		assert(false && "D3D11: Depth Stencil not implemented");
		m_device_context_->ClearDepthStencilView(ds, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
    m_device_context_->OMSetRenderTargets(rt_count, rtvs[0], ds);
}

void D3D11Context::set_viewports(unsigned count, const D3D12_VIEWPORT* viewport)
{
    for (unsigned int i = 0; i < count; i++)
    {
		m_viewports_[i] =
		{
			.TopLeftX = viewport[i].TopLeftX,
			.TopLeftY = viewport[i].TopLeftY,
			.Width = viewport[i].Width,
			.Height = viewport[i].Height,
			.MinDepth = viewport[i].MinDepth,
			.MaxDepth = viewport[i].MaxDepth,
		};

    }
	m_device_context_->RSSetViewports(count, m_viewports_.data());
}

void D3D11Context::draw(qhenki::graphics::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset)
{
	m_device_context_->Draw(vertex_count, start_vertex_offset);
}

void D3D11Context::draw_indexed(qhenki::graphics::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
	int32_t base_vertex_offset)
{
	m_device_context_->DrawIndexed(index_count, start_index_offset, base_vertex_offset);
}

void D3D11Context::wait_all()
{
    m_device_context_->Flush();
}

bool D3D11Context::present(qhenki::graphics::Swapchain& swapchain)
{
	const auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	const auto result = swap_d3d11->swapchain->Present(1, 0);
    return result == S_OK;
}

std::unique_ptr<ShaderCompiler> D3D11Context::create_shader_compiler()
{
	return std::make_unique<D3D11ShaderCompiler>();
}

D3D11Context::~D3D11Context()
{
	m_layout_assembler_.dispose();
    m_device_context_->ClearState();
    m_device_context_->Flush();
    m_device_context_.Reset();
    m_dxgi_factory_.Reset();
#if _DEBUG
    m_debug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
    m_debug_.Reset();
#endif
    m_device_.Reset();
}
