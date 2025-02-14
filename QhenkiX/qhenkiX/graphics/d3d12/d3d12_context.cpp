#include "d3d12_context.h"

#include <d3d12shader.h>
#include <iostream>

#include "d3d12_pipeline.h"
#include "d3d12_shader_compiler.h"
#include "graphics/d3d11/d3d11_shader.h"

void D3D12Context::create()
{
#ifdef _DEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug_))))
	{
		m_debug_->EnableDebugLayer();
	}
#endif

	// Create the DXGI factory
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgi_factory_))))
	{
		std::cerr << "D3D12: Failed to create DXGI factory" << std::endl;
		throw std::runtime_error("D3D12: Failed to create DXGI factory");
	}
#ifdef _DEBUG
	m_dxgi_factory_->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("DXGI Factory") - 1, "DXGI Factory");
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
		std::cerr << "D3D12: Failed to find discrete GPU. Defaulting to 0th adapter" << std::endl;
		if (FAILED(m_dxgi_factory_->EnumAdapters1(0, &adapter)))
		{
			throw std::runtime_error("D3D12: Failed to find a adapter");
		}
	}

	DXGI_ADAPTER_DESC1 desc;
	HRESULT hr = adapter->GetDesc1(&desc);
	if (FAILED(hr)) std::cerr << "D3D12: Failed to get adapter description" << std::endl;
	else std::wcout << L"D3D12: Selected adapter: " << desc.Description << L"\n";

	if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device_))))
	{
		std::cerr << "D3D12: Failed to create device" << std::endl;
		throw std::runtime_error("D3D12: Failed to create device");
	}

	// Find highest supported shader model
	#if defined(NTDDI_WIN10_VB) && (NTDDI_VERSION >= NTDDI_WIN10_VB)
		m_shader_model_.HighestShaderModel = D3D_SHADER_MODEL_6_6;
	#elif defined(NTDDI_WIN10_19H1) && (NTDDI_VERSION >= NTDDI_WIN10_19H1)
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
	#elif defined(NTDDI_WIN10_RS5) && (NTDDI_VERSION >= NTDDI_WIN10_RS5)
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_4;
	#elif defined(NTDDI_WIN10_RS4) && (NTDDI_VERSION >= NTDDI_WIN10_RS4)
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_2;
	#elif defined(NTDDI_WIN10_RS3) && (NTDDI_VERSION >= NTDDI_WIN10_RS3)
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_1;
	#else
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_0;
	#endif
	hr = m_device_->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &m_shader_model_, sizeof(m_shader_model_));
	while (hr == E_INVALIDARG && m_shader_model_.HighestShaderModel > D3D_SHADER_MODEL_6_0)
	{
		m_shader_model_.HighestShaderModel = static_cast<D3D_SHADER_MODEL>(static_cast<int>(m_shader_model_.HighestShaderModel) - 1);
		hr = m_device_->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &m_shader_model_, sizeof(m_shader_model_));
	}
	if (FAILED(hr))
	{
		m_shader_model_.HighestShaderModel = D3D_SHADER_MODEL_5_1;
	}

	D3D12MA::ALLOCATOR_DESC allocatorDesc = 
	{
		.pDevice = m_device_.Get(),
		.pAdapter = adapter.Get(),
	};
	// These flags are optional but recommended.
	allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED |
		D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;

	if (FAILED(CreateAllocator(&allocatorDesc, &m_allocator_)))
	{
		std::cerr << "D3D12: Failed to create memory allocator" << std::endl;
		throw std::runtime_error("D3D12: Failed to create memory allocator");
	}

	// Create RTV heap
	m_rtv_heap_.create(m_device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

#ifdef _DEBUG
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_dxgi_debug_))))
	{
		m_dxgi_debug_->EnableLeakTrackingForThread();
	}
#endif

	shader_compiler = std::make_unique<D3D12ShaderCompiler>();
}

bool D3D12Context::create_swapchain(DisplayWindow& window, const qhenki::graphics::SwapchainDesc& swapchain_desc,
                                    qhenki::graphics::Swapchain& swapchain, qhenki::graphics::Queue& direct_queue)
{
	DXGI_SWAP_CHAIN_DESC1 swap_chain_descriptor =
	{
		.Width = static_cast<UINT>(swapchain_desc.width),
		.Height = static_cast<UINT>(swapchain_desc.height),
		.Format = swapchain_desc.format,
		.SampleDesc =
		{
			.Count = 1, // MSAA Count
			.Quality = 0
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = m_frames_in_flight,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags = {},
	};

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_descriptor = {};
	swap_chain_fullscreen_descriptor.Windowed = true;

	assert(direct_queue.type == qhenki::graphics::QueueType::GRAPHICS);
	const auto queue = static_cast<ComPtr<ID3D12CommandQueue>*>(direct_queue.internal_state.get());
	assert(queue);

	ComPtr<IDXGISwapChain1> swapchain1;
	if (FAILED(m_dxgi_factory_->CreateSwapChainForHwnd(
		queue->Get(),        // TODO: Swap chain needs the queue so that it can force a flush on it.
		window.get_window_handle(),
		&swap_chain_descriptor,
		&swap_chain_fullscreen_descriptor,
		nullptr,
		&swapchain1
	)))
	{
		std::cerr << "D3D12: Failed to create Swapchain" << std::endl;
		return false;
	}
	if (FAILED(swapchain1.As(&this->m_swapchain_)))
	{
		std::cerr << "D3D12: Failed to get IDXGISwapChain3 from IDXGISwapChain1" << std::endl;
		return false;
	}
	m_frame_index_ = this->m_swapchain_->GetCurrentBackBufferIndex();
	return true;
}

bool D3D12Context::resize_swapchain(qhenki::graphics::Swapchain& swapchain, int width, int height)
{
	assert(false);
	return false;
}

bool D3D12Context::present(qhenki::graphics::Swapchain& swapchain)
{
	assert(false);
	return false;
}

bool D3D12Context::create_shader_dynamic(qhenki::graphics::Shader& shader, const CompilerInput& input)
{
	CompilerOutput output = {};
	const bool result = shader_compiler->compile(input, output);

	shader.type = input.shader_type;
	// IDxcBlob
	shader.internal_state = output.internal_state;

	return result;
}

D3D12ReflectionData D3D12Context::shader_reflection(ID3D12ShaderReflection* shader_reflection, const bool interleaved) const
{
	D3D12_SHADER_DESC shader_desc{};

	std::vector<std::string> input_element_semantic_names;
	input_element_semantic_names.reserve(shader_desc.InputParameters);
	std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_desc;
	input_element_desc.reserve(shader_desc.InputParameters);
	shader_reflection->GetDesc(&shader_desc);
	{
		UINT slot = 0;
		for (UINT parameter_index = 0; parameter_index < shader_desc.InputParameters; parameter_index++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC signature_parameter_desc{};
			shader_reflection->GetInputParameterDesc(parameter_index, &signature_parameter_desc);

			input_element_semantic_names.emplace_back(signature_parameter_desc.SemanticName);

			input_element_desc.emplace_back(D3D12_INPUT_ELEMENT_DESC
				{
					.SemanticName = input_element_semantic_names.back().c_str(),
					.SemanticIndex = signature_parameter_desc.SemanticIndex,
					.Format = D3D12ShaderCompiler::mask_to_format(signature_parameter_desc.Mask),
					.InputSlot = slot,
					.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
					.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
					.InstanceDataStepRate = 0u, // TODO: manual options for instancing
				});
			if (!interleaved) slot++;
		}
	}

	return { input_element_desc, input_element_semantic_names };
}

bool D3D12Context::create_pipeline(const qhenki::graphics::GraphicsPipelineDesc& desc, qhenki::graphics::GraphicsPipeline& pipeline,
                                   qhenki::graphics::Shader& vertex_shader, qhenki::graphics::Shader& pixel_shader, wchar_t const* debug_name)
{
	pipeline.internal_state = mkS<D3D12Pipeline>();
	const auto d3d12_pipeline = static_cast<D3D12Pipeline*>(pipeline.internal_state.get());
	assert(d3d12_pipeline);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC* pso_desc;
	{
		std::scoped_lock lock(m_pipeline_desc_mutex_);
		pso_desc = m_pipeline_desc_pool_.construct();
	}

	const auto d3d12_vs = static_cast<D3D12ShaderOutput*>(vertex_shader.internal_state.get());
	const auto d3d12_ps = static_cast<D3D12ShaderOutput*>(pixel_shader.internal_state.get());
	assert(d3d12_vs);
	assert(d3d12_ps);

	const auto& vertex_shader_blob = d3d12_vs->shader_blob;
	const auto& pixel_shader_blob = d3d12_ps->shader_blob;

	const auto& vs_reflection_buffer = d3d12_vs->reflection_blob;

	const DxcBuffer vs_reflection_dxc_buffer =
	{
		vs_reflection_buffer->GetBufferPointer(),
		vs_reflection_buffer->GetBufferSize(),
		0
	};
	const auto d3d12_shader_compiler = static_cast<D3D12ShaderCompiler*>(shader_compiler.get());
	assert(d3d12_shader_compiler);

	ComPtr<ID3D12ShaderReflection> shader_reflection{};
	d3d12_shader_compiler->m_library_->CreateReflection(&vs_reflection_dxc_buffer, IID_PPV_ARGS(shader_reflection.GetAddressOf()));

	// Input reflection (VS)
	d3d12_pipeline->reflection_data = this->shader_reflection(shader_reflection.Get(), desc.interleaved);
	const auto& input_layout_desc = d3d12_pipeline->reflection_data.input_layout_desc;
	pso_desc->InputLayout = 
	{
		input_layout_desc.data(),
		static_cast<uint32_t>(input_layout_desc.size())
	};

	// TODO: handling root signatures?
	pso_desc->pRootSignature = {};

	// TODO: DS, HS, maybe GS

	pso_desc->VS =
	{
		.pShaderBytecode = vertex_shader_blob->GetBufferPointer(),
		.BytecodeLength = vertex_shader_blob->GetBufferSize()
	};
	pso_desc->PS =
	{
		.pShaderBytecode = pixel_shader_blob->GetBufferPointer(),
		.BytecodeLength = pixel_shader_blob->GetBufferSize()
	};

	if (desc.rasterizer_state.has_value())
	{
		pso_desc->RasterizerState =
		{
			.FillMode = desc.rasterizer_state->fill_mode,
			.CullMode = desc.rasterizer_state->cull_mode,
			.FrontCounterClockwise = desc.rasterizer_state->front_counter_clockwise,
			.DepthBias = desc.rasterizer_state->depth_bias,
			.DepthBiasClamp = desc.rasterizer_state->depth_bias_clamp,
			.SlopeScaledDepthBias = desc.rasterizer_state->slope_scaled_depth_bias,
			.DepthClipEnable = desc.rasterizer_state->depth_clip_enable,

			.MultisampleEnable = FALSE,
			.AntialiasedLineEnable = FALSE,
			.ForcedSampleCount = 0,
			.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};
	}
	else
	{
		pso_desc->RasterizerState =
		{
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_BACK,
			.FrontCounterClockwise = FALSE,
			.DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
			.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
			.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
			.DepthClipEnable = TRUE,
			.MultisampleEnable = FALSE,
			.AntialiasedLineEnable = FALSE,
			.ForcedSampleCount = 0,
			.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
		};
	}

	if (desc.blend_desc.has_value())
	{
		pso_desc->BlendState = *desc.blend_desc;
	}
	else
	{
		pso_desc->BlendState.AlphaToCoverageEnable = FALSE;
		pso_desc->BlendState.IndependentBlendEnable = FALSE;
		constexpr D3D12_RENDER_TARGET_BLEND_DESC default_render_target_blend_desc =
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (auto& i : pso_desc->BlendState.RenderTarget)
			i = default_render_target_blend_desc;
	}

	if (desc.depth_stencil_state.has_value())
	{
		auto& depth_stencil_state = desc.depth_stencil_state.value();
		pso_desc->DepthStencilState =
		{
			.DepthEnable = static_cast<INT>(depth_stencil_state.depth_enable),
			.DepthWriteMask = depth_stencil_state.depth_write_mask,
			.DepthFunc = depth_stencil_state.depth_func,
			.StencilEnable = depth_stencil_state.stencil_enable,
			.StencilReadMask = depth_stencil_state.stencil_read_mask,
			.StencilWriteMask = depth_stencil_state.stencil_write_mask,
			.FrontFace = depth_stencil_state.front_face,
			.BackFace = depth_stencil_state.back_face,
		};
	}
	else
	{
		pso_desc->DepthStencilState.DepthEnable = FALSE;
		pso_desc->DepthStencilState.StencilEnable = FALSE;
	}

	pso_desc->SampleMask = UINT_MAX;
	pso_desc->PrimitiveTopologyType = desc.primitive_topology_type;

	// TODO: If RenderTargets < 0, compilation must be deferred until bind time
	pso_desc->NumRenderTargets = desc.num_render_targets;
	for (int i = 0; i < desc.num_render_targets; i++)
	{
		pso_desc->RTVFormats[i] = desc.rtv_formats[i];
	}

	// TODO: MSAA support
	pso_desc->SampleDesc.Count = 1;

	if (desc.num_render_targets < 1)
	{
		std::cerr << "D3D12: Pipeline creation deferred due to lack of targets" << std::endl;
		d3d12_pipeline->deferred = true;
	}
	else
	{
		if (const auto hr = m_device_->CreateGraphicsPipelineState(pso_desc, IID_PPV_ARGS(&d3d12_pipeline->pipeline_state)); FAILED(hr))
		{
			std::cerr << "D3D12: Failed to create Graphics Pipeline State" << std::endl;
			return false;
		}
		d3d12_pipeline->reflection_data.clear();
	}

	return true;
}

bool D3D12Context::bind_pipeline(qhenki::graphics::CommandList& cmd_list, qhenki::graphics::GraphicsPipeline& pipeline)
{
	assert(false);


	// check if pipeline is deferred
	// if it is then set correct render target info

	return true;
}

bool D3D12Context::create_buffer(const qhenki::graphics::BufferDesc& desc, const void* data, qhenki::graphics::Buffer& buffer, wchar_t const* debug_name)
{
	assert(false);
	return true;
}

void* D3D12Context::map_buffer(const qhenki::graphics::Buffer& buffer)
{
	assert(false);
	return nullptr;
}

void D3D12Context::unmap_buffer(const qhenki::graphics::Buffer& buffer)
{
	assert(false);
}

void D3D12Context::bind_vertex_buffers(qhenki::graphics::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
                                       const qhenki::graphics::Buffer* buffers, const unsigned* offsets)
{
	assert(false);
}

void D3D12Context::bind_index_buffer(qhenki::graphics::CommandList& cmd_list, const qhenki::graphics::Buffer& buffer, DXGI_FORMAT format,
	unsigned offset)
{
	assert(false);
}

bool D3D12Context::create_queue(const qhenki::graphics::QueueType type, qhenki::graphics::Queue& queue)
{
	D3D12_COMMAND_QUEUE_DESC queue_desc{};
	switch (type)
	{
	case qhenki::graphics::QueueType::GRAPHICS:
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case qhenki::graphics::QueueType::COMPUTE:
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		break;
	case qhenki::graphics::QueueType::COPY:
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	}
	queue.type = type;
	queue.internal_state = mkS<ComPtr<ID3D12CommandQueue>>();
	auto queue_d3d12 = static_cast<ComPtr<ID3D12CommandQueue>*>(queue.internal_state.get());
	assert(queue_d3d12);
	if (FAILED(m_device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&*queue_d3d12))))
	{
		std::cerr << "D3D12: Failed to create command queue" << std::endl;
		return false;
	}
	return true;
}

bool D3D12Context::create_command_pool(qhenki::graphics::CommandPool& command_pool, const qhenki::graphics::Queue& queue)
{
	// Unlike Vulkan, command allocator creation does not require the queue object.
	D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	switch (queue.type)
	{
	case qhenki::graphics::QueueType::GRAPHICS:
		type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case qhenki::graphics::QueueType::COMPUTE:
		type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		break;
	case qhenki::graphics::QueueType::COPY:
		type = D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	}

	command_pool.internal_state = mkS<ComPtr<ID3D12CommandAllocator>>();
	auto command_allocator = static_cast<ComPtr<ID3D12CommandAllocator>*>(command_pool.internal_state.get())->Get();

	if (FAILED(m_device_->CreateCommandAllocator(type, IID_PPV_ARGS(&command_allocator))))
	{
		std::cerr << "D3D12: Failed to create command allocator" << std::endl;
		return false;
	}
	return true;
}

void D3D12Context::start_render_pass(qhenki::graphics::CommandList& cmd_list, qhenki::graphics::Swapchain& swapchain,
                                     const qhenki::graphics::RenderTarget* depth_stencil)
{
	assert(false);
}

void D3D12Context::start_render_pass(qhenki::graphics::CommandList& cmd_list, unsigned rt_count,
	const qhenki::graphics::RenderTarget* rts, const qhenki::graphics::RenderTarget* depth_stencil)
{
	assert(false);
}

void D3D12Context::set_viewports(unsigned count, const D3D12_VIEWPORT* viewport)
{
	assert(false);
}

void D3D12Context::draw(qhenki::graphics::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset)
{
	assert(false);
}

void D3D12Context::draw_indexed(qhenki::graphics::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
	int32_t base_vertex_offset)
{
	assert(false);
}

void D3D12Context::wait_all()
{
	assert(false);
}

D3D12Context::~D3D12Context()
{
    m_allocator_.Reset();
    m_swapchain_.Reset();
	m_dxgi_factory_.Reset();
#ifdef _DEBUG
	m_dxgi_debug_->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	m_dxgi_debug_.Reset();
	m_debug_.Reset();
#endif
	m_device_.Reset();
}
