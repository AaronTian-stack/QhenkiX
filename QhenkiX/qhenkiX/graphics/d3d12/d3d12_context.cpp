#include "d3d12_context.h"

#include <d3d12shader.h>
#include <d3dcompiler.h>

#include "d3d12_pipeline.h"
#include "d3d12_shader_compiler.h"
#include "graphics/d3d11/d3d11_shader.h"
#include <application.h>
#include <iostream>

#include "d3d12_descriptor_heap.h"
#include "graphics/shared/d3d_helper.h"

using namespace qhenki::gfx;

void D3D12Context::create()
{
	UINT dxgi_factory_flags = 0;
#ifdef _DEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug_))))
	{
		m_debug_->EnableDebugLayer();
		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	// Create the DXGI factory
	if (FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&m_dxgi_factory_))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create DXGI factory\n");
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
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to find discrete GPU. Defaulting to 0th adapter\n");
		if (FAILED(m_dxgi_factory_->EnumAdapters1(0, &adapter)))
		{
			throw std::runtime_error("D3D12: Failed to find a adapter");
		}
	}

	DXGI_ADAPTER_DESC1 desc;
	HRESULT hr = adapter->GetDesc1(&desc);
	if (FAILED(hr))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get adapter description");
	}
	else
	{
		OutputDebugString((L"D3D12: Selected adapter: " + std::wstring(desc.Description) + L"\n").c_str());
	}

	if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device_))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create device");
		throw std::runtime_error("D3D12: Failed to create device");
	}

	// Find highest supported shader model
    const std::vector shader_models = {
        D3D_SHADER_MODEL_6_6,
        D3D_SHADER_MODEL_6_5,
        D3D_SHADER_MODEL_6_4,
        D3D_SHADER_MODEL_6_2,
        D3D_SHADER_MODEL_6_1,
        D3D_SHADER_MODEL_6_0,
		D3D_SHADER_MODEL_5_1,
    };

    for (const auto& model : shader_models) 
	{
        m_shader_model_.HighestShaderModel = model;
        hr = m_device_->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &m_shader_model_, sizeof(m_shader_model_));
        if (SUCCEEDED(hr)) 
		{
            break;
        }
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
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create memory allocator");
		throw std::runtime_error("D3D12: Failed to create memory allocator");
	}

#ifdef _DEBUG
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_dxgi_debug_))))
	{
		m_dxgi_debug_->EnableLeakTrackingForThread();
	}
#endif

	shader_compiler = mkU<D3D12ShaderCompiler>();

	if (FAILED(m_device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &m_options_, sizeof(m_options_))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to query feature data\n");
	}
}

bool D3D12Context::create_swapchain(DisplayWindow& window, const SwapchainDesc& swapchain_desc, 
	Swapchain& swapchain, Queue& direct_queue, unsigned& frame_index)
{
	swapchain.desc = swapchain_desc;

	DXGI_SWAP_CHAIN_DESC1 swap_chain_descriptor =
	{
		.Width = swapchain_desc.width,
		.Height = swapchain_desc.height,
		.Format = swapchain_desc.format,
		.SampleDesc =
		{
			.Count = 1, // MSAA Count
			.Quality = 0
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = swapchain_desc.buffer_count,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags = {},
	};

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_descriptor{};
	swap_chain_fullscreen_descriptor.Windowed = true;

	assert(direct_queue.type == QueueType::GRAPHICS);
	const auto queue = static_cast<ComPtr<ID3D12CommandQueue>*>(direct_queue.internal_state.get());
	assert(queue);

	ComPtr<IDXGISwapChain1> swapchain1;

	if (FAILED(m_dxgi_factory_->CreateSwapChainForHwnd(
		queue->Get(),        // SwapChain needs the queue so that it can force a flush on it.
		window.get_window_handle(),
		&swap_chain_descriptor,
		nullptr,
		nullptr,
		swapchain1.GetAddressOf()
	)))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create Swapchain");
		return false;
	}
	if (FAILED(swapchain1.As(&this->m_swapchain_)))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get IDXGISwapChain3 from IDXGISwapChain1");
		return false;
	}
	frame_index = this->m_swapchain_->GetCurrentBackBufferIndex();

	for (int i = 0; i < swapchain_desc.buffer_count; i++)
	{
		if (FAILED(m_swapchain_->GetBuffer(i, IID_PPV_ARGS(&m_swapchain_buffers_[i]))))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get back buffer from swap chain\n");
			return false;
		}
	}

	// Get RTV descriptors in separate step
	return true;
}

bool D3D12Context::resize_swapchain(Swapchain& swapchain, int width, int height)
{
	assert(false);
	return false;
}

bool D3D12Context::create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap& rtv_heap)
{
	const auto d3d12_heap = static_cast<D3D12DescriptorHeap*>(rtv_heap.internal_state.get());
	assert(d3d12_heap);
	if (d3d12_heap->desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: RTV heap must not be shader visible\n");
		return false;
	}
	if (d3d12_heap->desc.Type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: RTV heap must be of type RTV\n");
		return false;
	}

	m_swapchain_descriptors_.desc =
	{
		.descriptor_count = swapchain.desc.buffer_count,
		.heap = &rtv_heap,
	};

	m_swapchain_descriptors_.internal_state = mkS<D3D12MA::VirtualAllocation>();
	auto& allocation = *static_cast<D3D12MA::VirtualAllocation*>(m_swapchain_descriptors_.internal_state.get());

	// This will set the offset of the table in the heap
	// Synchronized internally
	const bool result = d3d12_heap->allocate(allocation, m_swapchain_descriptors_.desc.offset, swapchain.desc.buffer_count);
	if (!result)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to allocate RTV descriptors\n");
		return false;
	}

	// Virtual allocation made. Now create RTVs in the table position
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_cpu_handle;
	if (!d3d12_heap->get_CPU_descriptor(rtv_cpu_handle, m_swapchain_descriptors_.desc.offset, 0))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get CPU start descriptor for swapchain RTV table\n");
		return false;
	}
	for (int i = 0; i < swapchain.desc.buffer_count; i++)
	{
		// Create an RTV for the i-th buffer
		m_device_->CreateRenderTargetView(m_swapchain_buffers_[i].Get(), nullptr, rtv_cpu_handle);
		rtv_cpu_handle.ptr += d3d12_heap->descriptor_size;
	}
	
	return true;
}

bool D3D12Context::present(Swapchain& swapchain)
{
	const auto result = m_swapchain_->Present(1, 0);
	return result == S_OK;
}

std::unique_ptr<ShaderCompiler> D3D12Context::create_shader_compiler()
{
	return mkU<D3D12ShaderCompiler>();
}

bool D3D12Context::create_shader_dynamic(ShaderCompiler* compiler, Shader& shader, const CompilerInput& input)
{
	if (compiler == nullptr)
	{
		compiler = shader_compiler.get();
	}
	CompilerOutput output = {};
	const bool result = compiler->compile(input, output);

	shader =
	{
		.type = input.shader_type,
		.shader_model = input.min_shader_model,
		.internal_state = output.internal_state, // IDxcBlob
	};

	return result;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> D3D12Context::shader_reflection(ID3D12ShaderReflection* shader_reflection,
                                                                      const D3D12_SHADER_DESC& shader_desc, const bool interleaved) const
{
	assert(shader_reflection);

	std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_desc;
	input_element_desc.reserve(shader_desc.InputParameters);
	{
		UINT slot = 0;
		for (UINT parameter_index = 0; parameter_index < shader_desc.InputParameters; parameter_index++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC signature_parameter_desc{};
			const auto hr = shader_reflection->GetInputParameterDesc(parameter_index, &signature_parameter_desc);

			if (FAILED(hr))
			{
				continue;
			}

			input_element_desc.emplace_back(D3D12_INPUT_ELEMENT_DESC
				{
					.SemanticName = signature_parameter_desc.SemanticName,
					.SemanticIndex = signature_parameter_desc.SemanticIndex,
					.Format = D3D12ShaderCompiler::mask_to_format(signature_parameter_desc.Mask, signature_parameter_desc.ComponentType),
					.InputSlot = slot,
					.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
					.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
					.InstanceDataStepRate = 0u, // TODO: manual options for instancing
				});
			if (!interleaved) slot++;
		}
	}

	return input_element_desc;
}

void D3D12Context::root_signature_reflection(ID3D12ShaderReflection* shader_reflection, const D3D12_SHADER_DESC& shader_desc)
{
	// TODO: finish this
	assert(false);
	D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
	for (UINT i = 0; i < shader_desc.BoundResources; i++)
	{
		D3D12_SHADER_INPUT_BIND_DESC bind_desc = {};
		shader_reflection->GetResourceBindingDesc(i, &bind_desc);
	}
}

UINT D3D12Context::GetMaxDescriptorsForHeapType(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
    {
        OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get D3D12 options\n");
        return 0;
    }
    switch (type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1;
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE;
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		return 1 << 20; // 1M
    default:
        return 0;
    }
}

UINT D3D12Context::GetMaxDescriptorsForHeapType(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
    {
        OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get D3D12 options\n");
        return 0;
    }
    switch (type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1;
    case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
        return D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE;
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		return 1 << 20; // 1M
    default:
        return 0;
    }
}

bool D3D12Context::create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline& pipeline,
	Shader& vertex_shader, Shader& pixel_shader, 
	PipelineLayout* in_layout, PipelineLayout* out_layout, wchar_t const* debug_name)
{
	pipeline.internal_state = mkS<D3D12Pipeline>();

	const auto d3d12_pipeline = static_cast<D3D12Pipeline*>(pipeline.internal_state.get());
	assert(d3d12_pipeline);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC* pso_desc;
	{
		std::scoped_lock lock(m_pipeline_desc_mutex_);
		pso_desc = m_pipeline_desc_pool_.construct();
	}

	assert(vertex_shader.shader_model == pixel_shader.shader_model);

	D3D12ShaderOutput* vs12 = nullptr;
	D3D11ShaderOutput* vs11 = nullptr;

	ComPtr<ID3D12ShaderReflection> shader_reflection;
	D3D12_SHADER_DESC shader_desc{};
	if (vertex_shader.shader_model < ShaderModel::SM_6_0)
	{
		vs11 = static_cast<D3D11ShaderOutput*>(vertex_shader.internal_state.get());
		const auto ps11 = static_cast<D3D11ShaderOutput*>(pixel_shader.internal_state.get());
		assert(vs11);
		assert(ps11);
		if (const auto hr = D3DReflect(
			vs11->shader_blob->GetBufferPointer(),
			vs11->shader_blob->GetBufferSize(),
			IID_ID3D12ShaderReflection,
			&shader_reflection); FAILED(hr))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to reflect vertex shader\n");
			return false;
		}
		const auto hr_d = shader_reflection->GetDesc(&shader_desc);
		assert(SUCCEEDED(hr_d));
		// Input reflection (VS)
		d3d12_pipeline->input_layout_desc = this->shader_reflection(shader_reflection.Get(), shader_desc, desc.interleaved);

		pso_desc->VS =
		{
			.pShaderBytecode = vs11->shader_blob->GetBufferPointer(),
			.BytecodeLength = vs11->shader_blob->GetBufferSize()
		};
		pso_desc->PS =
		{
			.pShaderBytecode = ps11->shader_blob->GetBufferPointer(),
			.BytecodeLength = ps11->shader_blob->GetBufferSize()
		};
	}
	else // SM >= 6.0
	{
		vs12 = static_cast<D3D12ShaderOutput*>(vertex_shader.internal_state.get());
		const auto ps12 = static_cast<D3D12ShaderOutput*>(pixel_shader.internal_state.get());
		assert(vs12);
		assert(ps12);
		const auto& vs_reflection_buffer_12 = vs12->reflection_blob;

		const DxcBuffer vs_reflection_dxc_buffer =
		{
			vs_reflection_buffer_12->GetBufferPointer(),
			vs_reflection_buffer_12->GetBufferSize(),
			0
		};
		const auto d3d12_shader_compiler = static_cast<D3D12ShaderCompiler*>(shader_compiler.get());
		assert(d3d12_shader_compiler);

		if (const auto hr = d3d12_shader_compiler->m_library_->CreateReflection(&vs_reflection_dxc_buffer, IID_PPV_ARGS(shader_reflection.GetAddressOf())); FAILED(hr))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to reflect vertex shader\n");
			return false;
		}
		// Input reflection (VS)
		const auto hr_d = shader_reflection->GetDesc(&shader_desc);
		assert(SUCCEEDED(hr_d));
		d3d12_pipeline->input_layout_desc = this->shader_reflection(shader_reflection.Get(), shader_desc, desc.interleaved);

		pso_desc->VS =
		{
			.pShaderBytecode = vs12->shader_blob->GetBufferPointer(),
			.BytecodeLength = vs12->shader_blob->GetBufferSize()
		};
		pso_desc->PS =
		{
			.pShaderBytecode = ps12->shader_blob->GetBufferPointer(),
			.BytecodeLength = ps12->shader_blob->GetBufferSize()
		};
	}

	const auto& input_layout_desc = d3d12_pipeline->input_layout_desc;
	pso_desc->InputLayout =
	{
		.pInputElementDescs = input_layout_desc.data(),
		.NumElements = static_cast<uint32_t>(input_layout_desc.size())
	};

	assert(vs12 == nullptr ^ vs11 == nullptr);
	if ((vs12 && vs12->root_signature_blob) || (vs11 && vs11->root_signature_blob)) // Root signature is contained in the shader
	{
		void* blob_ptr;
		size_t blob_size;
		if (vs11)
		{
			blob_ptr = vs11->root_signature_blob->GetBufferPointer();
			blob_size = vs11->root_signature_blob->GetBufferSize();
		}
		else
		{
			blob_ptr = vs12->root_signature_blob->GetBufferPointer();
			blob_size = vs12->root_signature_blob->GetBufferSize();
		}
		// Root signatures are always created using blobs (they must be serialized)
		const auto root_result = m_root_reflection_.add_root_signature(m_device_.Get(), blob_ptr, blob_size);
		assert(root_result);
		pso_desc->pRootSignature = root_result;
	}
	else if (in_layout) // Check if premade root signature is provided
	{
		const auto rs = static_cast<ComPtr<ID3D12RootSignature>*>(in_layout->internal_state.get())->Get();
		assert(rs);
		pso_desc->pRootSignature = rs;
	}
	else // Create new root signature using reflection
	{
		root_signature_reflection(shader_reflection.Get(), shader_desc);
	}

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

	D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	d3d12_pipeline->primitive_topology = D3DHelper::get_primitive_topology(desc.topology);

	switch (desc.topology)
	{
	case PrimitiveTopology::POINT_LIST:
		topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case PrimitiveTopology::TRIANGLE_LIST:
	case PrimitiveTopology::TRIANGLE_STRIP:
		topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case PrimitiveTopology::LINE_LIST:
	case PrimitiveTopology::LINE_STRIP:
		topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	default:
		throw std::runtime_error("D3D12: Invalid primitive topology");
	}

	pso_desc->PrimitiveTopologyType = topology_type;

	pso_desc->NumRenderTargets = desc.num_render_targets;

	// TODO: MSAA support
	pso_desc->SampleDesc.Count = 1;

	if (desc.num_render_targets < 1)
	{
		OutputDebugString(L"Qhenki D3D12 WARNING: Pipeline creation deferred due to lack of targets\n");
		d3d12_pipeline->deferred = true;
	}
	else
	{
		for (int i = 0; i < desc.num_render_targets; i++)
		{
			pso_desc->RTVFormats[i] = desc.rtv_formats[i];
		}
		if (const auto hr = m_device_->CreateGraphicsPipelineState(pso_desc, IID_PPV_ARGS(&d3d12_pipeline->pipeline_state)); FAILED(hr))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create Graphics Pipeline State\n");
			return false;
		}
		d3d12_pipeline->input_layout_desc.clear();
		// Free description
		std::scoped_lock lock(m_pipeline_desc_mutex_);
		m_pipeline_desc_pool_.destroy(pso_desc);
	}

#ifdef _DEBUG
	d3d12_pipeline->pipeline_state->SetName(debug_name);
#endif

	return true;
}

bool D3D12Context::bind_pipeline(CommandList& cmd_list, GraphicsPipeline& pipeline)
{
	const auto d3d12_pipeline = static_cast<D3D12Pipeline*>(pipeline.internal_state.get());
	assert(d3d12_pipeline);
	if (d3d12_pipeline->deferred)
	{
		// Set current render target info and create the pipeline
		assert(false);
		// Issue a warning that the pipeline was deferred
		OutputDebugString(L"Qhenki D3D12 WARNING: Deferred pipeline compilation\n");
		assert(d3d12_pipeline->input_layout_desc.empty());
	}

	const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(cmd_list.internal_state.get());
	assert(cmd_list_d3d12);
	cmd_list_d3d12->Get()->IASetPrimitiveTopology(d3d12_pipeline->primitive_topology);
	cmd_list_d3d12->Get()->SetPipelineState(d3d12_pipeline->pipeline_state.Get());

	return true;
}

bool D3D12Context::create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap& heap)
{
	// Check that you are not trying to make GPU heap of RTVs, this is not valid
	if (desc.visibility == DescriptorHeapDesc::Visibility::GPU && desc.type == DescriptorHeapDesc::Type::RTV)
	{
		OutputDebugString(L"Qhenki D3D12: Cannot create GPU visible RTV heap\n");
		return false;
	}

	heap.desc = desc;
	heap.internal_state = mkS<D3D12DescriptorHeap>();

	const auto d3d12_heap = static_cast<D3D12DescriptorHeap*>(heap.internal_state.get());
	assert(d3d12_heap);
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};

	switch (desc.type)
	{
	case DescriptorHeapDesc::Type::CBV_SRV_UAV:
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		break;
	case DescriptorHeapDesc::Type::SAMPLER:
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		break;
	case DescriptorHeapDesc::Type::RTV:
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		break;
	case DescriptorHeapDesc::Type::DSV:
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		break;
	default:
		OutputDebugString(L"Qhenki D3D12: Invalid descriptor heap type\n");
		return false;
	}

	assert(desc.descriptor_count <= GetMaxDescriptorsForHeapType(m_device_.Get(), heap_desc.Type));

	heap_desc.NumDescriptors = desc.descriptor_count;

	if (desc.visibility == DescriptorHeapDesc::Visibility::CPU)
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	else if (desc.visibility == DescriptorHeapDesc::Visibility::GPU)
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	d3d12_heap->create(m_device_.Get(), heap_desc);

	return true;
}

bool D3D12Context::create_buffer(const BufferDesc& desc, const void* data, Buffer& buffer, wchar_t const* debug_name)
{
	buffer.desc = desc;
	buffer.internal_state = mkS<ComPtr<D3D12MA::Allocation>>();
	const auto buffer_d3d12 = static_cast<ComPtr<D3D12MA::Allocation>*>(buffer.internal_state.get());

	D3D12_RESOURCE_DESC resource_desc =
	{
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment = 0,
		.Width = desc.size,
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.Format = DXGI_FORMAT_UNKNOWN,
		.SampleDesc =
		{
			.Count = 1,
			.Quality = 0,
		},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags = D3D12_RESOURCE_FLAG_NONE,
	};

	const auto is_cpu_visible = (desc.visibility & CPU_SEQUENTIAL)
		|| (desc.visibility & CPU_RANDOM);

	D3D12MA::ALLOCATION_DESC allocation_desc{};
	if (is_cpu_visible)
	{
		allocation_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	}
	else
	{
		allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	}

	D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON;
	switch (desc.usage)
	{
	case VERTEX:
	case UNIFORM:
		initial_state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		break;
	case INDEX:
		initial_state = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		break;
	case STORAGE:
		initial_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		break;
	case INDIRECT:
		initial_state = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		break;
	case TRANSFER_SRC:
		initial_state = D3D12_RESOURCE_STATE_COPY_SOURCE;
		break;
	case TRANSFER_DST:
		initial_state = D3D12_RESOURCE_STATE_COPY_DEST;
		break;
	}

	if (FAILED(m_allocator_->CreateResource(
		&allocation_desc,
		&resource_desc,
		initial_state,
		nullptr,
		buffer_d3d12->GetAddressOf(),
		IID_NULL, NULL)))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create buffer\n");
		return false;
	}
	const auto resource = buffer_d3d12->Get()->GetResource();
	if (data)
	{
		// If this is a CPU visible buffer memcpy the data
		if (is_cpu_visible)
		{
			D3D12_RANGE range(0, 0);
			void* mapped_ptr;
			if (FAILED(resource->Map(0, &range, &mapped_ptr)))
			{
				OutputDebugString(L"Qhenki D3D12 ERROR: Failed to map buffer\n");
				return false;
			}
			memcpy(mapped_ptr, data, desc.size);
			resource->Unmap(0, nullptr);
		}
		else
		{
			OutputDebugString(L"Qhenki D3D12 WARNING: Tried to initialize non CPU visible buffer with data\n");
		}
	}

	// Need to also create the associated view
	// This is determined by the buffer usage, some views will need a heap to be created (CBV, SRV, UAV)
	// This is done in a separate step exposed to the developer

	return true;
}

void* D3D12Context::map_buffer(const Buffer& buffer)
{
	// Check if buffer is CPU visible
	if ((buffer.desc.visibility & CPU_SEQUENTIAL)
		|| (buffer.desc.visibility & CPU_RANDOM))
	{
		const auto allocation = static_cast<ComPtr<D3D12MA::Allocation>*>(buffer.internal_state.get());
		assert(allocation);
		const auto resource = allocation->Get()->GetResource();
		D3D12_RANGE range(0, 0);
		void* mapped_ptr;
		if (FAILED(resource->Map(0, &range, &mapped_ptr)))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to map buffer\n");
			return nullptr;
		}
		return mapped_ptr;
	}
	OutputDebugString(L"Qhenki D3D12 ERROR: Buffer is not CPU visible\n");

	return nullptr;
}

void D3D12Context::unmap_buffer(const Buffer& buffer)
{
	// Check if buffer is CPU visible
	if ((buffer.desc.visibility & CPU_SEQUENTIAL)
		|| (buffer.desc.visibility & CPU_RANDOM))
	{
		const auto allocation = static_cast<ComPtr<D3D12MA::Allocation>*>(buffer.internal_state.get());
		assert(allocation);
		const auto resource = allocation->Get()->GetResource();
		resource->Unmap(0, nullptr);
	}
	else
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Buffer is not CPU visible\n");
	}
}

void D3D12Context::bind_vertex_buffers(CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
                                       const Buffer* buffers, const UINT* strides, const UINT* strides, const unsigned* offsets)
{
	assert(buffer_count <= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

	const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(cmd_list.internal_state.get());
	assert(cmd_list_d3d12);
	const auto command_list = cmd_list_d3d12->Get();

	// Create views for each buffer
	std::array<D3D12_VERTEX_BUFFER_VIEW, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> vertex_buffer_views;
	for (unsigned i = 0; i < buffer_count; i++)
	{
		const auto allocation = static_cast<ComPtr<D3D12MA::Allocation>*>(buffers[i].internal_state.get());
		assert(allocation);
		const auto resource = allocation->Get()->GetResource();

		vertex_buffer_views[i] =
		{
			.BufferLocation = resource->GetGPUVirtualAddress() + offsets[i],
			.SizeInBytes = static_cast<UINT>(buffers[i].desc.size - offsets[i]),
			.StrideInBytes = strides[i],
		};
	}

	// TODO: is this scope enough?
	command_list->IASetVertexBuffers(start_slot, buffer_count, vertex_buffer_views.data());
}

void D3D12Context::bind_index_buffer(CommandList& cmd_list, const Buffer& buffer, IndexType format,
                                     unsigned offset)
{
	const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(cmd_list.internal_state.get());
	assert(cmd_list_d3d12);
	const auto command_list = cmd_list_d3d12->Get();

	const auto allocation = static_cast<ComPtr<D3D12MA::Allocation>*>(buffer.internal_state.get());
	assert(allocation);
	const auto resource = allocation->Get()->GetResource();
	D3D12_INDEX_BUFFER_VIEW view =
	{
		.BufferLocation = resource->GetGPUVirtualAddress() + offset,
		.SizeInBytes = static_cast<UINT>(buffer.desc.size - offset),
		.Format = D3DHelper::get_dxgi_format(format),
	};

	command_list->IASetIndexBuffer(&view);
}

bool D3D12Context::create_queue(const QueueType type, Queue& queue)
{
	D3D12_COMMAND_QUEUE_DESC queue_desc{};
	switch (type)
	{
	case GRAPHICS:
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case COMPUTE:
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		break;
	case COPY:
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	}
	queue.type = type;
	queue.internal_state = mkS<ComPtr<ID3D12CommandQueue>>();
	const auto queue_d3d12 = static_cast<ComPtr<ID3D12CommandQueue>*>(queue.internal_state.get());
	assert(queue_d3d12);
	if (FAILED(m_device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(queue_d3d12->GetAddressOf()))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create command queue\n");
		return false;
	}
	return true;
}

bool D3D12Context::create_command_pool(CommandPool& command_pool, const Queue& queue)
{
	// Unlike Vulkan, command allocator creation does not require the queue object.
	D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	switch (queue.type)
	{
	case GRAPHICS:
		type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case COMPUTE:
		type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		break;
	case COPY:
		type = D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	}

	command_pool.queue = &queue;
	command_pool.internal_state = mkS<ComPtr<ID3D12CommandAllocator>>();
	auto command_allocator = static_cast<ComPtr<ID3D12CommandAllocator>*>(command_pool.internal_state.get())->GetAddressOf();

	if (FAILED(m_device_->CreateCommandAllocator(type, IID_PPV_ARGS(command_allocator))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create command allocator\n");
		return false;
	}

	return true;
}

bool D3D12Context::create_command_list(CommandList& cmd_list, const CommandPool& command_pool)
{
	const auto command_allocator = static_cast<ComPtr<ID3D12CommandAllocator>*>(command_pool.internal_state.get());
	assert(command_allocator);
	// create the appropriate command list type
	switch (command_pool.queue->type)
	{
	case GRAPHICS:
		cmd_list.internal_state = mkS<ComPtr<ID3D12GraphicsCommandList>>();
		break;
	case COMPUTE:
		//break;
	case COPY: 
		//break;
	default:
		throw std::runtime_error("D3D12: Not implemented command list type");
	}
	const auto d3d12_cmd_list = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(cmd_list.internal_state.get());
	if FAILED(m_device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator->Get(),
		nullptr, IID_PPV_ARGS(d3d12_cmd_list->GetAddressOf())))
	{
		return false;
	}
	// print out address of the command list for debug
	std::cerr << "Command list address: " << d3d12_cmd_list->Get() << std::endl;
	return true;
}

bool D3D12Context::close_command_list(CommandList& cmd_list)
{
	const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(cmd_list.internal_state.get());
	assert(cmd_list_d3d12);
	if (FAILED(cmd_list_d3d12->Get()->Close()))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to close command list\n");
		return false;
	}
	return true;
}

bool D3D12Context::reset_command_pool(CommandPool& command_pool)
{
	const auto command_allocator = static_cast<ComPtr<ID3D12CommandAllocator>*>(command_pool.internal_state.get())->Get();
	assert(command_allocator);
	if (FAILED(command_allocator->Reset()))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to reset command allocator\n");
		return false;
	}
	return true;
}

void D3D12Context::start_render_pass(CommandList& cmd_list, Swapchain& swapchain,
                                     const RenderTarget* depth_stencil, UINT frame_index)
{
	const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(cmd_list.internal_state.get());
	assert(cmd_list_d3d12);
	auto command_list = cmd_list_d3d12->Get();

	// Get RTV descriptor
	assert(m_swapchain_descriptors_.desc.heap);
	const auto rtv_heap = static_cast<D3D12DescriptorHeap*>(m_swapchain_descriptors_.desc.heap->internal_state.get());
	assert(rtv_heap);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle;
	rtv_heap->get_CPU_descriptor(rtv_handle, m_swapchain_descriptors_.desc.offset, frame_index);

	// TODO: depth stencil
	command_list->OMSetRenderTargets(1, &rtv_handle, 
		FALSE, nullptr);

	// TODO: optional clear values
	const float clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
}

void D3D12Context::start_render_pass(CommandList& cmd_list, unsigned rt_count,
	const RenderTarget* rts, const RenderTarget* depth_stencil)
{
	assert(false);
}

void D3D12Context::set_viewports(CommandList& list, unsigned count, const D3D12_VIEWPORT* viewport)
{
	const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(list.internal_state.get());
	assert(cmd_list_d3d12);
	const auto command_list = cmd_list_d3d12->Get();
	command_list->RSSetViewports(count, viewport);
}

void D3D12Context::set_scissor_rects(CommandList& list, unsigned count, const D3D12_RECT* scissor_rect)
{
	const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(list.internal_state.get());
	assert(cmd_list_d3d12);
	const auto command_list = cmd_list_d3d12->Get();
	command_list->RSSetScissorRects(count, scissor_rect);
}

void D3D12Context::draw(CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset)
{
	const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(cmd_list.internal_state.get());
	assert(cmd_list_d3d12);
	const auto command_list = cmd_list_d3d12->Get();
	command_list->DrawInstanced(vertex_count, 1, start_vertex_offset, 0);
}

void D3D12Context::draw_indexed(CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
	int32_t base_vertex_offset)
{
	const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(cmd_list.internal_state.get());
	assert(cmd_list_d3d12);
	const auto command_list = cmd_list_d3d12->Get();
	command_list->DrawIndexedInstanced(index_count, 1, 
		start_index_offset, base_vertex_offset, 0);
}

void D3D12Context::submit_command_lists(unsigned count, CommandList* cmd_lists, Queue& queue)
{
	const auto queue_d3d12 = static_cast<ComPtr<ID3D12CommandQueue>*>(queue.internal_state.get());
	assert(queue_d3d12);
	assert(count < 16);
	std::array<ID3D12CommandList*, 16> cmd_list_ptrs;
	for (unsigned i = 0; i < count; i++)
	{
		const auto cmd_list_d3d12 = static_cast<ComPtr<ID3D12GraphicsCommandList>*>(cmd_lists[i].internal_state.get());
		assert(cmd_list_d3d12);
		cmd_list_ptrs[i] = cmd_list_d3d12->Get();
	}
	queue_d3d12->Get()->ExecuteCommandLists(count, cmd_list_ptrs.data());
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
	m_allocator_.Reset();
	m_device_.Reset();
}
