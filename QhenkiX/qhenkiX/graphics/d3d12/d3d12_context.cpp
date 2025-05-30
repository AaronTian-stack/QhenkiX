#include "d3d12_context.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx12.h"

#include <d3d12shader.h>
#include <d3dcompiler.h>

#include "d3d12_pipeline.h"
#include "d3d12_shader_compiler.h"
#include "graphics/d3d11/d3d11_shader.h"
#include <application.h>

#include "d3d12_descriptor_heap.h"
#include "d3d12_fence.h"
#include "d3d12_texture.h"

#include "graphics/shared/d3d_helper.h"
#include "graphics/shared/math_helper.h"

using namespace qhenki::gfx;

static D3D12DescriptorHeap* to_internal(const DescriptorHeap& ext)
{
	auto d3d12_heap = static_cast<D3D12DescriptorHeap*>(ext.internal_state.get());
	assert(d3d12_heap);
	return d3d12_heap;
}

static D3D12Pipeline* to_internal(const GraphicsPipeline& ext)
{
	auto d3d12_pipeline = static_cast<D3D12Pipeline*>(ext.internal_state.get());
	assert(d3d12_pipeline);
	return d3d12_pipeline;
}

static ComPtr<ID3D12GraphicsCommandList7>* to_internal(const CommandList& ext)
{
	auto d3d12_cmd_list = static_cast<ComPtr<ID3D12GraphicsCommandList7>*>(ext.internal_state.get());
	assert(d3d12_cmd_list);
	return d3d12_cmd_list;
}

static ComPtr<ID3D12CommandQueue>* to_internal(const Queue& ext)
{
	auto d3d12_queue = static_cast<ComPtr<ID3D12CommandQueue>*>(ext.internal_state.get());
	assert(d3d12_queue);
	return d3d12_queue;
}

static D3D12Fence* to_internal(const Fence& ext)
{
	auto d3d12_fence = static_cast<D3D12Fence*>(ext.internal_state.get());
	assert(d3d12_fence);
	return d3d12_fence;
}

static ComPtr<ID3D12CommandAllocator>* to_internal(const CommandPool& ext)
{
	auto d3d12_cmd_pool = static_cast<ComPtr<ID3D12CommandAllocator>*>(ext.internal_state.get());
	assert(d3d12_cmd_pool);
	return d3d12_cmd_pool;
}

static ComPtr<D3D12MA::Allocation>* to_internal(const Buffer& ext)
{
	auto alloc = static_cast<ComPtr<D3D12MA::Allocation>*>(ext.internal_state.get());
	assert(alloc);
	return alloc;
}

static ComPtr<ID3D12RootSignature>* to_internal(const PipelineLayout& ext)
{
	auto root_sig = static_cast<ComPtr<ID3D12RootSignature>*>(ext.internal_state.get());
	assert(root_sig);
	return root_sig;
}

static D3D12Texture* to_internal(const Texture& ext)
{
	auto text = static_cast<D3D12Texture*>(ext.internal_state.get());
	assert(text);
	return text;
}

static D3D12_RESOURCE_STATES convert_state(BufferUsage usage)
{
	D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON;
    if (usage & (BufferUsage::VERTEX | BufferUsage::UNIFORM))  
    {  
       initial_state |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;  
    }  
    if (usage & BufferUsage::INDEX)  
    {  
       initial_state |= D3D12_RESOURCE_STATE_INDEX_BUFFER;  
    }  
    if (usage & BufferUsage::STORAGE)  
    {  
       initial_state |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;  
    }  
    if (usage & BufferUsage::INDIRECT)  
    {  
       initial_state |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;  
    }  
    if (usage & BufferUsage::COPY_SRC)  
    {  
       initial_state |= D3D12_RESOURCE_STATE_COPY_SOURCE;  
    }  
    if (usage & BufferUsage::COPY_DST)  
    {  
       initial_state |= D3D12_RESOURCE_STATE_COPY_DEST;  
    }
	return initial_state;
}

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
	if (FAILED(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(m_dxgi_factory_.ReleaseAndGetAddressOf()))))
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

	if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_device_.ReleaseAndGetAddressOf()))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create device");
		throw std::runtime_error("D3D12: Failed to create device");
	}

	if (FAILED(m_device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &m_options12_, sizeof(m_options12_))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to query D3D12 options 12\n");
	}

	if (!m_options12_.EnhancedBarriersSupported)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Enhanced barriers are not supported\n");
		throw std::runtime_error("D3D12: Enhanced barriers are not supported");
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
    allocatorDesc.Flags = static_cast<D3D12MA::ALLOCATOR_FLAGS>(
       D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED |
       D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

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

	create_fence(&m_fence_wait_all_, 0);
}

bool D3D12Context::create_swapchain(const DisplayWindow& window, const SwapchainDesc& swapchain_desc,
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
	const auto queue = to_internal(direct_queue);

	m_swapchain_queue_ = &direct_queue; // For resizing

	ComPtr<IDXGISwapChain1> swapchain1;

	if (FAILED(m_dxgi_factory_->CreateSwapChainForHwnd(
		queue->Get(),        // SwapChain needs the queue so that it can force a flush on it.
		window.get_window_handle(),
		&swap_chain_descriptor,
		&swap_chain_fullscreen_descriptor,
		nullptr,
		swapchain1.ReleaseAndGetAddressOf()
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

	for (unsigned i = 0; i < swapchain_desc.buffer_count; i++)
	{
		if (FAILED(m_swapchain_->GetBuffer(i, IID_PPV_ARGS(m_swapchain_buffers_[i].ReleaseAndGetAddressOf()))))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get back buffer from swap chain\n");
			return false;
		}
	}

	// Get RTV descriptors in separate step
	return true;
}

bool D3D12Context::resize_swapchain(Swapchain& swapchain, int width, int height, DescriptorHeap& rtv_heap, unsigned& frame_index)
{
	// Update description
	swapchain.desc.width = width;
	swapchain.desc.height = height;

	// Stall entire pipeline
	wait_idle(*m_swapchain_queue_);

	// Remove direct references to back buffer resources
	for (auto& buffer : m_swapchain_buffers_)
	{
		buffer->Release();
	}

	// Resize buffers
	if (FAILED(m_swapchain_->ResizeBuffers(
		swapchain.desc.buffer_count,
		width,
		height,
		swapchain.desc.format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	)))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to resize swap chain buffers\n");
		return false;
	}

	// Recreate descriptors
	for (unsigned i = 0; i < swapchain.desc.buffer_count; i++)
	{
		if (FAILED(m_swapchain_->GetBuffer(i, IID_PPV_ARGS(m_swapchain_buffers_[i].GetAddressOf()))))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get back buffer from swap chain\n");
			return false;
		}
	}
	auto d3d12_heap = to_internal(rtv_heap);

	THROW_IF_TRUE(swapchain.desc.buffer_count > m_swapchain_buffers_.size());

	for (unsigned i = 0; i < swapchain.desc.buffer_count; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_cpu_handle;
		if (!d3d12_heap->get_CPU_descriptor(&rtv_cpu_handle, m_swapchain_descriptors_[i].offset, 0))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get CPU start descriptor for swapchain RTV table\n");
			return false;
		}
		// Create an RTV for the i-th buffer
		m_device_->CreateRenderTargetView(m_swapchain_buffers_[i].Get(), nullptr, rtv_cpu_handle);
	}

	frame_index = m_swapchain_->GetCurrentBackBufferIndex();

	return true;
}

bool D3D12Context::create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap& rtv_heap)
{
	const auto d3d12_heap = to_internal(rtv_heap);
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

	THROW_IF_TRUE(swapchain.desc.buffer_count > m_swapchain_buffers_.size());

	for (unsigned i = 0; i < swapchain.desc.buffer_count; i++)
	{
		if (d3d12_heap->allocate(&m_swapchain_descriptors_[i].offset))
		{
			m_swapchain_descriptors_[i].heap = &rtv_heap;
		}
		else
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to allocate descriptor for swapchain RTV\n");
			return false;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_cpu_handle;
		if (!d3d12_heap->get_CPU_descriptor(&rtv_cpu_handle, m_swapchain_descriptors_[i].offset, 0))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get CPU start descriptor for swapchain RTV\n");
			return false;
		}
		// Create an RTV for the i-th buffer
		m_device_->CreateRenderTargetView(m_swapchain_buffers_[i].Get(), nullptr, rtv_cpu_handle);
	}
	
	return true;
}

bool D3D12Context::present(Swapchain& swapchain, unsigned fence_count, Fence* wait_fences, unsigned swapchain_index)
{
	// Vulkan version will use the queue swapchain was created with

	// D3D12 does not actually have to wait on anything
	const auto result = m_swapchain_->Present(1, 0);
	return result == S_OK;
}

std::unique_ptr<ShaderCompiler> D3D12Context::create_shader_compiler()
{
	return mkU<D3D12ShaderCompiler>();
}

bool D3D12Context::create_shader_dynamic(ShaderCompiler* compiler, Shader* shader, const CompilerInput& input)
{
	if (compiler == nullptr)
	{
		compiler = shader_compiler.get();
	}
	CompilerOutput output = {};
	if (!compiler->compile(input, output))
	{
		OutputDebugStringA(output.error_message.c_str());
		return false;
	}

	*shader =
	{
		.type = input.shader_type,
		.shader_model = input.min_shader_model,
		.internal_state = output.internal_state, // IDxcBlob
	};

	return true;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> D3D12Context::shader_reflection(ID3D12ShaderReflection* shader_reflection,
                                                                      const D3D12_SHADER_DESC& shader_desc, const bool increment_slot) const
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
			if (increment_slot) slot++;
		}
	}

	return input_element_desc;
}

void D3D12Context::root_signature_reflection(ID3D12ShaderReflection* shader_reflection, const D3D12_SHADER_DESC& shader_desc)
{
	// TODO: finish this
	assert(false);
	throw std::runtime_error("D3D12: Root signature reflection not implemented");
	//D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
	//for (UINT i = 0; i < shader_desc.BoundResources; i++)
	//{
	//	D3D12_SHADER_INPUT_BIND_DESC bind_desc = {};
	//	shader_reflection->GetResourceBindingDesc(i, &bind_desc);
	//}
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

bool D3D12Context::create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline* pipeline,
                                   const Shader& vertex_shader, const Shader& pixel_shader,
                                   PipelineLayout* in_layout, PipelineLayout* out_layout, wchar_t const* debug_name)
{
	pipeline->internal_state = mkS<D3D12Pipeline>();

	const auto d3d12_pipeline = to_internal(*pipeline);

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
		d3d12_pipeline->input_layout_desc = this->shader_reflection(shader_reflection.Get(), shader_desc, desc.increment_slot);

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

		if (const auto hr = d3d12_shader_compiler->m_library_->CreateReflection(&vs_reflection_dxc_buffer, 
			IID_PPV_ARGS(&shader_reflection)); FAILED(hr))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to reflect vertex shader\n");
			return false;
		}
		// Input reflection (VS)
		const auto hr_d = shader_reflection->GetDesc(&shader_desc);
		assert(SUCCEEDED(hr_d));
		d3d12_pipeline->input_layout_desc = this->shader_reflection(shader_reflection.Get(), shader_desc, desc.increment_slot);

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

	assert((vs12 == nullptr) ^ (vs11 == nullptr));
	if ((vs12 && vs12->root_signature_blob) || (vs11 && vs11->root_signature_blob)) // Root signature is contained in the shader
	{
		assert(!in_layout); // Should not have both
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

	auto make_d3d12_rasterizer_desc = [](const RasterizerDesc& r)
	{
		return D3D12_RASTERIZER_DESC
		{
			r.fill_mode,
			r.cull_mode,
			r.front_counter_clockwise,
			r.depth_bias,
			r.depth_bias_clamp,
			r.slope_scaled_depth_bias,
			r.depth_clip_enable,
			FALSE, // MultisampleEnable
			FALSE, // AntialiasedLineEnable
			0,     // ForcedSampleCount
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};
	};

	pso_desc->RasterizerState = make_d3d12_rasterizer_desc(desc.rasterizer_state.value_or(RasterizerDesc{}));

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
		if (const auto hr = m_device_->CreateGraphicsPipelineState(pso_desc, 
			IID_PPV_ARGS(&d3d12_pipeline->pipeline_state)); FAILED(hr))
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

bool D3D12Context::bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline)
{
	const auto d3d12_pipeline = to_internal(pipeline);
	if (d3d12_pipeline->deferred)
	{
		// Set current render target info and create the pipeline
		assert(false);
		// Issue a warning that the pipeline was deferred
		OutputDebugString(L"Qhenki D3D12 WARNING: Deferred pipeline compilation\n");
		assert(d3d12_pipeline->input_layout_desc.empty());
	}

	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	cmd_list_d3d12->Get()->IASetPrimitiveTopology(d3d12_pipeline->primitive_topology);
	cmd_list_d3d12->Get()->SetPipelineState(d3d12_pipeline->pipeline_state.Get());

	return true;
}

bool D3D12Context::create_pipeline_layout(PipelineLayoutDesc& desc, PipelineLayout* layout)
{
	auto count_non_empty = [](const std::array<std::vector<LayoutBinding>, MAX_SPACES>& spaces)
		{
		unsigned count = 0;
		for (const auto& space : spaces)
		{
			if (!space.empty()) count++;
		}
		return count;
		};
	const auto spaces = count_non_empty(desc.spaces);

	const UINT param_count = desc.push_ranges.size() + spaces;
	
	std::array<D3D12_ROOT_PARAMETER, 10> params; // TODO: replace with small vector
	assert(param_count <= params.size());

	for (unsigned i = 0; i < desc.push_ranges.size(); i++)
	{
		assert(false);
		const auto& range = desc.push_ranges[i];
		params[i] =
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			.Constants =
			{
				.ShaderRegister = range.binding,
				.RegisterSpace = 5, // TODO: change this
				.Num32BitValues = range.size,
			},
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
		};
	}

	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> ranges;
	ranges.reserve(desc.spaces.size());
	for (unsigned i = 0; i < desc.spaces.size(); i++)
	{
		auto& space = desc.spaces[i];
		if (space.empty()) continue;
		// Sort vector of LayoutBindings by binding register
		std::ranges::sort(space,
	      [](const LayoutBinding& a, const LayoutBinding& b)
	      {
	          return a.binding < b.binding;
	      });
		// Assemble ranges dynamically
		std::vector<D3D12_DESCRIPTOR_RANGE> l_ranges; // TODO: Replace with small vector
		l_ranges.reserve(space.size());
		unsigned offset = 0;
		for (unsigned j = 0; j < space.size(); j++)
		{
			const auto& binding = space[j];
			// Check that this is not the last binding and not infinite register count
			assert(j == space.size() - 1 || binding.count != INFINITE_DESCRIPTORS);
			D3D12_DESCRIPTOR_RANGE range
			{
				.RangeType = binding.type,
				.NumDescriptors = binding.count, // TODO: test infinite and bindless
				.BaseShaderRegister = binding.binding,
				.RegisterSpace = i,
				.OffsetInDescriptorsFromTableStart = offset,
			};
			offset += binding.count;
			l_ranges.emplace_back(range);
		}
		params[i] =
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable =
			{
				.NumDescriptorRanges = static_cast<UINT>(l_ranges.size()),
				.pDescriptorRanges = l_ranges.data(),
			},
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
		};
		ranges.emplace_back(std::move(l_ranges));
	}

	D3D12_ROOT_SIGNATURE_DESC root_sig_desc
	{
		// Default range flags
		.NumParameters = param_count,
		.pParameters = params.data(),
		.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
	};

	ComPtr<ID3DBlob> root_sig_blob, error_blob;
	HRESULT hr = D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		&root_sig_blob, &error_blob);
	if (FAILED(hr))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to serialize root signature\n");
		// Output error blob
		if (error_blob)
		{
			OutputDebugStringA(static_cast<char*>(error_blob->GetBufferPointer()));
		}
		return false;
	}
	const void* root_sig_data = root_sig_blob->GetBufferPointer();
	size_t root_sig_data_size = root_sig_blob->GetBufferSize();

	layout->internal_state = mkS<ComPtr<ID3D12RootSignature>>();
	auto& root_signature = *static_cast<ComPtr<ID3D12RootSignature>*>(layout->internal_state.get());
	if(FAILED(m_device_->CreateRootSignature(0, root_sig_data, root_sig_data_size,
		IID_PPV_ARGS(root_signature.ReleaseAndGetAddressOf()))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create root signature\n");
		return false;
	}

	return true;
}

void D3D12Context::bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout)
{
	auto cmd_list_d3d12 = to_internal(*cmd_list);
	auto layout_d3d12 = to_internal(layout);
	cmd_list_d3d12->Get()->SetGraphicsRootSignature(layout_d3d12->Get());
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

	const auto d3d12_heap = to_internal(heap);
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

void D3D12Context::set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	const auto heap_d3d12 = to_internal(heap);
	if (heap.desc.type == DescriptorHeapDesc::Type::CBV_SRV_UAV || heap.desc.type == DescriptorHeapDesc::Type::SAMPLER)
	{
		cmd_list_d3d12->Get()->SetDescriptorHeaps(1, heap_d3d12->Get().GetAddressOf());
	}
	else
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Invalid descriptor heap type\n");
	}
}

void D3D12Context::set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap,
	const DescriptorHeap& sampler_heap)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	const auto heap_d3d12 = to_internal(heap);
	const auto sampler_heap_d3d12 = to_internal(sampler_heap);
	if (heap.desc.type == DescriptorHeapDesc::Type::CBV_SRV_UAV)
	{
		ID3D12DescriptorHeap* heaps[] = { heap_d3d12->Get().Get(), sampler_heap_d3d12->Get().Get() };
		cmd_list_d3d12->Get()->SetDescriptorHeaps(2, heaps);
	}
	else
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Invalid descriptor heap type\n");
	}
}

void D3D12Context::set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	assert(gpu_descriptor.heap);

	const auto heap_d3d12 = to_internal(*gpu_descriptor.heap);
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
	if (!heap_d3d12->get_GPU_descriptor(&gpu_handle, gpu_descriptor.offset, 0))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get GPU descriptor handle\n");
		return;
	}

	cmd_list_d3d12->Get()->SetGraphicsRootDescriptorTable(index, gpu_handle);
}

bool D3D12Context::copy_descriptors(unsigned count, const Descriptor& src, const Descriptor& dst)
{
	const auto src_heap_d3d12 = to_internal(*src.heap);
	const auto dst_heap_d3d12 = to_internal(*dst.heap);

	if (src_heap_d3d12->desc.Type != dst_heap_d3d12->desc.Type)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Source and destination descriptor heaps must be of the same type\n");
		return false;
	}

	if (src_heap_d3d12->desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Source heap cannot be shader visible\n");
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE src_cpu_handle;
	if (!src_heap_d3d12->get_CPU_descriptor(&src_cpu_handle, src.offset, 0))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get CPU descriptor handle\n");
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE dst_cpu_handle;
	if (!dst_heap_d3d12->get_CPU_descriptor(&dst_cpu_handle, dst.offset, 0))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get CPU descriptor handle\n");
		return false;
	}

	m_device_->CopyDescriptorsSimple(count, dst_cpu_handle, src_cpu_handle, src_heap_d3d12->desc.Type);

	return true;
}

bool D3D12Context::get_descriptor(unsigned descriptor_count_offset, DescriptorHeap& heap, Descriptor* descriptor)
{
	const auto heap_d3d12 = to_internal(heap);
	assert(descriptor);
	*descriptor =
	{
		.heap = &heap,
		.offset = heap_d3d12->descriptor_count_to_bytes(descriptor_count_offset),
	};
	return true;
}

bool D3D12Context::create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer,
	wchar_t const* debug_name)
{
	buffer->desc = desc;
	buffer->internal_state = mkS<ComPtr<D3D12MA::Allocation>>();
	const auto buffer_d3d12 = to_internal(*buffer);

	D3D12_RESOURCE_DESC1 resource_desc =
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
		.Flags = D3D12_RESOURCE_FLAG_NONE, // TODO: allow unordered access
		// Don't care about SamplerFeedbackMipRegion
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

	// Initial state is not used in D3D12

	// Barrier layout is undefined for CreateResource3
	if (FAILED(m_allocator_->CreateResource3(
		&allocation_desc,
		&resource_desc,
		D3D12_BARRIER_LAYOUT_UNDEFINED,
		nullptr,
		0, nullptr,
		buffer_d3d12->ReleaseAndGetAddressOf(),
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

#ifdef _DEBUG
	buffer_d3d12->Get()->SetName(debug_name);
#endif

	// Need to also create the associated view
	// This is determined by the buffer usage, some views will need a heap to be created (CBV, SRV, UAV)
	// This is done in a separate step exposed to the developer

	return true;
}

bool D3D12Context::create_descriptor(const Buffer& buffer, DescriptorHeap& cpu_heap, Descriptor* descriptor, BufferDescriptorType type)
{
	const auto buffer_d3d12 = to_internal(buffer);
	const auto heap_d3d12 = to_internal(cpu_heap);
	if (cpu_heap.desc.type != DescriptorHeapDesc::Type::CBV_SRV_UAV)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Invalid descriptor heap type\n");
		return false;
	}
	if (!heap_d3d12->allocate(&descriptor->offset))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to allocate descriptor for buffer\n");
		return false;
	}
	descriptor->heap = &cpu_heap;

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
	if (!heap_d3d12->get_CPU_descriptor(&cpu_handle, descriptor->offset, 0))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get CPU descriptor handle\n");
		return false;
	}

	if (type == BufferDescriptorType::CBV)
	{
		if (buffer.desc.size != MathHelper::align_u32(buffer.desc.size, 256))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Constant buffer size is not aligned to 256 bytes\n");
			return false;
		}
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc
		{
			.BufferLocation = buffer_d3d12->Get()->GetResource()->GetGPUVirtualAddress(),
			.SizeInBytes = static_cast<UINT>(buffer.desc.size),
		};
		m_device_->CreateConstantBufferView(&desc, cpu_handle);
	}
	else if (type == BufferDescriptorType::UAV)
	{
		assert(false);
		// TODO: need pDesc
		m_device_->CreateUnorderedAccessView(buffer_d3d12->Get()->GetResource(), nullptr, nullptr, cpu_handle);
	}

	return true;
}

void D3D12Context::copy_buffer(CommandList* cmd_list, const Buffer& src, UINT64 src_offset, Buffer* dst, UINT64 dst_offset, UINT64 bytes)
{
	assert(src_offset + bytes <= src.desc.size);
	assert(dst_offset + bytes <= dst->desc.size);
	const auto src_allocation = to_internal(src);
	const auto dst_allocation = to_internal(*dst);
	const auto src_resource = src_allocation->Get()->GetResource();
	const auto dst_resource = dst_allocation->Get()->GetResource();

	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	cmd_list_d3d12->Get()->CopyBufferRegion(dst_resource, dst_offset, src_resource, src_offset, bytes);
}

bool D3D12Context::create_texture(const TextureDesc& desc, Texture* texture,
                                  wchar_t const* debug_name)
{
	if (desc.height > 1 && desc.dimension == TextureDimension::TEXTURE_1D)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Tried to initialize 1D texture with height > 1\n");
		return false;
	}

	texture->desc = desc;
	texture->internal_state = mkS<D3D12Texture>();
	const auto texture_d3d12 = to_internal(*texture);
	D3D12_RESOURCE_DESC1 resource_desc =
	{
		.Alignment = 0,
		.Width = desc.width,
		.Height = desc.height,
		.DepthOrArraySize = desc.depth_or_array_size,
		.MipLevels = desc.mip_levels,
		.Format = desc.format,
		.SampleDesc = // TODO: options for RT usage
		{
			.Count = 1,
			.Quality = 0,
		},
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = D3D12_RESOURCE_FLAG_NONE, // TODO: need to set flags for RT and UAV
		//.SamplerFeedbackMipRegion // TODO: sampler feedback mip region?
	};

	switch (desc.dimension)
	{
	case TextureDimension::TEXTURE_1D:
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		break;
	case TextureDimension::TEXTURE_2D:
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		break;
	case TextureDimension::TEXTURE_3D:
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		break;
	}

	D3D12MA::ALLOCATION_DESC allocation_desc
	{
		.HeapType = D3D12_HEAP_TYPE_DEFAULT,
	};

	//D3D12_CLEAR_VALUE clear
	//{
	//	.Format = desc.format,
	//};
	//// If it is a depth format, use depth clear value instead of color
	//if (desc.format == DXGI_FORMAT_D32_FLOAT || desc.format == DXGI_FORMAT_D24_UNORM_S8_UINT)
	//{
	//	clear.DepthStencil = { .Depth= 1.0f, .Stencil= 0};
	//}
	//else
	//{
	//	clear.Color[0] = clear.Color[1] = clear.Color[2] = clear.Color[3] = 0.0f;
	//}

	if (FAILED(m_allocator_->CreateResource3(
		&allocation_desc,
		&resource_desc,
		D3DHelper::layout_D3D(desc.initial_layout), // Common use should be as copy destination (need to transition later)
		nullptr, // TODO: need to set clear value for RT
		0, nullptr, // probably not going to cast 
		texture_d3d12->allocation.ReleaseAndGetAddressOf(),
		IID_NULL, NULL)))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create texture\n");
		return false;
	}

#ifdef _DEBUG
	texture_d3d12->allocation.Get()->SetName(debug_name);
#endif

	return true;
}

bool D3D12Context::create_descriptor_texture_view(const Texture& texture, DescriptorHeap& heap, Descriptor* descriptor)
{
	const auto texture_d3d12 = to_internal(texture);
	const auto heap_d3d12 = to_internal(heap);

	if (!heap_d3d12->allocate(&descriptor->offset))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to allocate descriptor for texture\n");
		return false;
	}
	descriptor->heap = &heap;

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
	if (!heap_d3d12->get_CPU_descriptor(&cpu_handle, descriptor->offset, 0))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get CPU descriptor handle\n");
		return false;
	}
	
	// TODO: description
	m_device_->CreateShaderResourceView(texture_d3d12->allocation.Get()->GetResource(), nullptr, cpu_handle);

	return true;
}

bool D3D12Context::copy_to_texture(CommandList& cmd_list, const void* data, Buffer& staging, Texture& texture)
{
	const uint32_t num_subresources = texture.desc.mip_levels * texture.desc.depth_or_array_size;
	// Get info to know how big staging needs to be
	const auto texture_allocation = to_internal(texture);
	const auto desc = texture_allocation->allocation.Get()->GetResource()->GetDesc();

	UINT64 size;
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(num_subresources); // TODO: replace with small vector
	std::vector<UINT> row_sizes(num_subresources); // For subresource height TODO: replace with small vector
	m_device_->GetCopyableFootprints(&desc, 0, num_subresources, 0,
		layouts.data(), row_sizes.data(), nullptr, &size);

	BufferDesc staging_desc
	{
		.size = size,
		.usage = BufferUsage::COPY_SRC,
		.visibility = CPU_SEQUENTIAL,
	};
	if (!create_buffer(staging_desc, data, &staging, nullptr))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create staging buffer for texture copy\n");
		return false;
	}

	uint8_t* upload_memory = static_cast<uint8_t*>(map_buffer(staging));
	// memcpy from data to staging buffer based off footprint data

	constexpr auto subresource_index = 0;

	const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& sub_resource_layout = layouts[subresource_index];
	uint8_t* destination_sub_resource_memory = upload_memory + sub_resource_layout.Offset;
	const uint64_t sub_resource_pitch = MathHelper::align_u32(sub_resource_layout.Footprint.RowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

	const auto bytes_row = texture.desc.width * D3DHelper::bytes_per_pixel(texture.desc.format);
	for (uint32_t y = 0; y < texture.desc.height; ++y)
	{
		auto row_ptr = static_cast<const uint8_t*>(data);
		memcpy(destination_sub_resource_memory, &row_ptr[y * bytes_row],
			std::min(sub_resource_pitch, bytes_row));
		destination_sub_resource_memory += sub_resource_pitch;
	}
	
	// https://alextardif.com/D3D11To12P3.html for entire copy details

	// Record commands to copy subresources from staging to texture
	
	D3D12_TEXTURE_COPY_LOCATION destination
	{
		.pResource = texture_allocation->allocation.Get()->GetResource(),
		.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		.SubresourceIndex = 0,
	};

	const auto staging_internal = to_internal(staging);

	D3D12_TEXTURE_COPY_LOCATION source
	{
		.pResource = staging_internal->Get()->GetResource(),
		.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
		.PlacedFootprint = layouts[subresource_index],
		// PlacedFootprint offset is 0 since handled by allocator
	};

	// Copy to texture
	const auto cmd_list_d3d12 = to_internal(cmd_list);
	cmd_list_d3d12->Get()->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

	return true;
}

bool D3D12Context::create_sampler(const SamplerDesc& desc, Sampler* sampler)
{
	// D3D12 samplers are descriptors and do not have a separate object
	sampler->desc = desc;
	return true;
}

bool D3D12Context::create_descriptor(const Sampler& sampler, DescriptorHeap& heap, Descriptor* descriptor)
{
	const auto heap_d3d12 = to_internal(heap);
	if (heap.desc.type != DescriptorHeapDesc::Type::SAMPLER)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Invalid descriptor heap type\n");
		return false;
	}
	if (!heap_d3d12->allocate(&descriptor->offset))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to allocate descriptor for sampler\n");
		return false;
	}
	descriptor->heap = &heap;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
	if (!heap_d3d12->get_CPU_descriptor(&cpu_handle, descriptor->offset, 0))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get CPU descriptor handle\n");
		return false;
	}
	const auto& desc = sampler.desc;
	D3D12_SAMPLER_DESC sampler_desc
	{
		.Filter = D3DHelper::filter(
			desc.min_filter, desc.mag_filter, desc.mip_filter, desc.comparison_func, desc.max_anisotropy),
		.AddressU = D3DHelper::texture_address_mode(desc.address_mode_u),
		.AddressV = D3DHelper::texture_address_mode(desc.address_mode_v),
		.AddressW = D3DHelper::texture_address_mode(desc.address_mode_w),
		.MipLODBias = desc.mip_lod_bias,
		.MaxAnisotropy = desc.max_anisotropy,
		.ComparisonFunc = D3DHelper::comparison_func(desc.comparison_func),
		.BorderColor = { desc.border_color[0], desc.border_color[1],desc.border_color[2] , desc.border_color[3] },
		.MinLOD = desc.min_lod,
		.MaxLOD = desc.max_lod,
	};
	m_device_->CreateSampler(&sampler_desc, cpu_handle);
	return true;
}

void* D3D12Context::map_buffer(const Buffer& buffer)
{
	// Check if buffer is CPU visible
	if ((buffer.desc.visibility & CPU_SEQUENTIAL)
		|| (buffer.desc.visibility & CPU_RANDOM))
	{
		const auto allocation = to_internal(buffer);
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
		const auto allocation = to_internal(buffer);
		const auto resource = allocation->Get()->GetResource();
		resource->Unmap(0, nullptr);
	}
	else
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Buffer is not CPU visible\n");
	}
}

void D3D12Context::bind_vertex_buffers(CommandList* cmd_list, unsigned start_slot, unsigned buffer_count,
	const Buffer* const* buffers, const unsigned* const strides, const unsigned* const offsets)
{
	assert(buffer_count <= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	const auto command_list = cmd_list_d3d12->Get();

	// Create views for each buffer
	std::array<D3D12_VERTEX_BUFFER_VIEW, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> vertex_buffer_views;
	for (unsigned i = 0; i < buffer_count; i++)
	{
		const auto allocation = to_internal(*buffers[i]);
		const auto resource = allocation->Get()->GetResource();

		vertex_buffer_views[i] =
		{
			.BufferLocation = resource->GetGPUVirtualAddress() + offsets[i],
			.SizeInBytes = static_cast<UINT>((*buffers[i]).desc.size - offsets[i]),
			.StrideInBytes = strides[i],
		};
	}

	command_list->IASetVertexBuffers(start_slot, buffer_count, vertex_buffer_views.data());
}

void D3D12Context::bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format,
                                     unsigned offset)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	const auto command_list = cmd_list_d3d12->Get();

	const auto allocation = to_internal(buffer);
	const auto resource = allocation->Get()->GetResource();
	D3D12_INDEX_BUFFER_VIEW view =
	{
		.BufferLocation = resource->GetGPUVirtualAddress() + offset,
		.SizeInBytes = static_cast<UINT>(buffer.desc.size - offset),
		.Format = D3DHelper::get_dxgi_format(format),
	};

	command_list->IASetIndexBuffer(&view);
}

bool D3D12Context::create_queue(const QueueType type, Queue* queue)
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
	queue->type = type;
	queue->internal_state = mkS<ComPtr<ID3D12CommandQueue>>();
	const auto queue_d3d12 = to_internal(*queue);
	if (FAILED(m_device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(queue_d3d12->ReleaseAndGetAddressOf()))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create command queue\n");
		return false;
	}
	return true;
}

bool D3D12Context::create_command_pool(CommandPool* command_pool, const Queue& queue)
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

	command_pool->queue = &queue;
	command_pool->internal_state = mkS<ComPtr<ID3D12CommandAllocator>>();
	const auto command_allocator = to_internal(*command_pool);

	if (FAILED(m_device_->CreateCommandAllocator(type, IID_PPV_ARGS(command_allocator->ReleaseAndGetAddressOf()))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create command allocator\n");
		return false;
	}
	return true;
}

bool D3D12Context::create_command_list(CommandList* cmd_list, const CommandPool& command_pool)
{
	const auto command_allocator = to_internal(command_pool);
	// create the appropriate command list type
	switch (command_pool.queue->type)
	{
	case GRAPHICS:
		cmd_list->internal_state = mkS<ComPtr<ID3D12GraphicsCommandList7>>();
		break;
	case COMPUTE:
		//break;
	case COPY: 
		//break;
	default:
		throw std::runtime_error("D3D12: Not implemented command list type");
	}
	const auto d3d12_cmd_list = to_internal(*cmd_list);
	if FAILED(m_device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator->Get(),
		nullptr, IID_PPV_ARGS(d3d12_cmd_list->ReleaseAndGetAddressOf())))
	{
		return false;
	}
	return true;
}

bool D3D12Context::close_command_list(CommandList* cmd_list)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	assert(cmd_list_d3d12);
	if (FAILED(cmd_list_d3d12->Get()->Close()))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to close command list\n");
		return false;
	}
	return true;
}

bool D3D12Context::reset_command_pool(CommandPool* command_pool)
{
	const auto command_allocator = to_internal(*command_pool);
	if (FAILED(command_allocator->Get()->Reset()))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to reset command allocator\n");
		return false;
	}
	return true;
}

void D3D12Context::start_render_pass(CommandList* cmd_list, Swapchain& swapchain,
                                     const RenderTarget* depth_stencil, UINT frame_index)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	const auto command_list = cmd_list_d3d12->Get();

	// Get RTV descriptor
	assert(m_swapchain_descriptors_[0].heap && (m_swapchain_descriptors_[0].heap == m_swapchain_descriptors_[1].heap));
	const auto rtv_heap = to_internal(*m_swapchain_descriptors_[0].heap);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle;
	rtv_heap->get_CPU_descriptor(&rtv_handle, m_swapchain_descriptors_[frame_index].offset, 0);

	// TODO: depth stencil
	command_list->OMSetRenderTargets(1, &rtv_handle, 
		FALSE, nullptr);

	// TODO: optional clear values
	const float clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
}

void D3D12Context::start_render_pass(CommandList* cmd_list, unsigned rt_count,
                                     const RenderTarget* rts, const RenderTarget* depth_stencil)
{
	assert(false);
}

void D3D12Context::set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport)
{
	const auto cmd_list_d3d12 = to_internal(*list);
	const auto command_list = cmd_list_d3d12->Get();
	command_list->RSSetViewports(count, viewport);
}

void D3D12Context::set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect)
{
	const auto cmd_list_d3d12 = to_internal(*list);
	const auto command_list = cmd_list_d3d12->Get();
	command_list->RSSetScissorRects(count, scissor_rect);
}

void D3D12Context::draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	const auto command_list = cmd_list_d3d12->Get();
	command_list->DrawInstanced(vertex_count, 1, start_vertex_offset, 0);
}

void D3D12Context::draw_indexed(CommandList* cmd_list, uint32_t index_count, uint32_t start_index_offset,
                                int32_t base_vertex_offset)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	const auto command_list = cmd_list_d3d12->Get();
	command_list->DrawIndexedInstanced(index_count, 1, 
		start_index_offset, base_vertex_offset, 0);
}

void D3D12Context::submit_command_lists(const SubmitInfo& submit_info, Queue* queue)
{
	const auto queue_d3d12 = to_internal(*queue);

	assert(submit_info.command_list_count < 16);
	std::array<ID3D12CommandList*, 16> cmd_list_ptrs;
	for (unsigned i = 0; i < submit_info.command_list_count; i++)
	{
		const auto cmd_list_d3d12 = to_internal(submit_info.command_lists[i]);
		cmd_list_ptrs[i] = cmd_list_d3d12->Get();
	}
	queue_d3d12->Get()->ExecuteCommandLists(submit_info.command_list_count, cmd_list_ptrs.data());

	// Signal the fences
	for (unsigned i = 0; i < submit_info.signal_fence_count; i++)
	{
		const auto fence = to_internal(submit_info.signal_fences[i]);
		const auto result = queue_d3d12->Get()->Signal(fence->fence.Get(), submit_info.signal_values[i]);
		if (FAILED(result))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to signal fence\n");
		}
	}
}

bool D3D12Context::create_fence(Fence* fence, uint64_t initial_value)
{
	fence->internal_state = mkS<D3D12Fence>();
	const auto fence_d3d12 = to_internal(*fence);
	if (FAILED(m_device_->CreateFence(initial_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_d3d12->fence.ReleaseAndGetAddressOf()))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create fence\n");
		return false;
	}
	// Give each fence a handle event as well
	fence_d3d12->event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	return true;
}

uint64_t D3D12Context::get_fence_value(const Fence& fence)
{
	const auto fence_d3d12 = to_internal(fence);
	return fence_d3d12->fence->GetCompletedValue();
}

bool D3D12Context::wait_fences(const WaitInfo& info)
{
	assert(info.count < 16);
	std::array<HANDLE, 16> wait_handles{};
	for (unsigned i = 0; i < info.count; i++)
	{
		const auto d3d12_fence = to_internal(info.fences[i]);
		if (FAILED(d3d12_fence->fence->SetEventOnCompletion(info.values[i], d3d12_fence->event)))
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Failed to set event on fence\n");
			return false;
		}
		wait_handles[i] = d3d12_fence->event;
	}
	WaitForMultipleObjectsEx(info.count, wait_handles.data(), info.wait_all, info.timeout, FALSE);
	return true;
}

void D3D12Context::set_barrier_resource(unsigned count, ImageBarrier* barriers, const Swapchain& swapchain, unsigned frame_index)
{
	for (unsigned i = 0; i < count; i++)
	{
		assert(frame_index == m_swapchain_->GetCurrentBackBufferIndex());
		barriers[i].resource = static_cast<void*>(m_swapchain_buffers_[frame_index].Get());
	}
}

void D3D12Context::set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target)
{
	for (unsigned i = 0; i < count; i++)
	{
		barriers[i].resource = static_cast<void*>(to_internal(render_target)->allocation.Get()->GetResource());
	}
}

void D3D12Context::issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	const auto command_list = cmd_list_d3d12->Get();

	assert(count < 16);
	std::array<D3D12_TEXTURE_BARRIER, 16> d3d12_barriers;
	for (unsigned i = 0; i < count; i++)
	{
		const auto& barrier = barriers[i];

		if (!barrier.resource)
		{
			OutputDebugString(L"Qhenki D3D12 ERROR: Barrier resource is null\n");
			return;
		}

		auto& d3d12_barrier = d3d12_barriers[i];
		d3d12_barrier =
		{
			.SyncBefore = D3DHelper::sync_stage_D3D(barrier.src_stage),
			.SyncAfter = D3DHelper::sync_stage_D3D(barrier.dst_stage),
			.AccessBefore = D3DHelper::access_flags_D3D(barrier.src_access),
			.AccessAfter = D3DHelper::access_flags_D3D(barrier.dst_access),
			.LayoutBefore = D3DHelper::layout_D3D(barrier.src_layout),
			.LayoutAfter = D3DHelper::layout_D3D(barrier.dst_layout),
			.pResource = static_cast<ID3D12Resource*>(barrier.resource),
			.Subresources =
			{
				.IndexOrFirstMipLevel = barrier.subresource_range.base_mip_level,
				.NumMipLevels = barrier.subresource_range.mip_level_count,
				.FirstArraySlice = barrier.subresource_range.base_array_layer,
				.NumArraySlices = barrier.subresource_range.array_layer_count,
				.FirstPlane = 0,
				.NumPlanes = 1,
			},
			.Flags = barrier.discard ? D3D12_TEXTURE_BARRIER_FLAG_DISCARD : D3D12_TEXTURE_BARRIER_FLAG_NONE,
		};
	}

	D3D12_BARRIER_GROUP barrier_group =
	{
		.Type = D3D12_BARRIER_TYPE_TEXTURE,
		.NumBarriers = count,
		.pTextureBarriers = d3d12_barriers.data(),
	};

	command_list->Barrier(count, &barrier_group);
}

void D3D12Context::init_imgui(const DisplayWindow& window, const Swapchain& swapchain)
{
	// Create dedicated heap for ImGUI
	D3D12_DESCRIPTOR_HEAP_DESC desc
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = swapchain.desc.buffer_count,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		.NodeMask = 0,
	};
	m_imgui_heap.create(m_device_.Get(), desc);

	const auto queue_d3d12 = to_internal(*m_swapchain_queue_);

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = m_device_.Get();
	init_info.CommandQueue = queue_d3d12->Get();
	init_info.NumFramesInFlight = swapchain.desc.buffer_count;
	init_info.RTVFormat = swapchain.desc.format;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	init_info.SrvDescriptorHeap = m_imgui_heap.Get().Get();

    struct qinfo
    {
	    D3D12DescriptorHeap* heap;
	    std::array<Descriptor, 2>* descriptors;
    } info
	{
		.heap = &m_imgui_heap,
		.descriptors = &m_imgui_descriptors_,
	};

	init_info.UserData = &info;

	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
	{
		const auto qin = static_cast<qinfo*>(info->UserData);
		static UINT index = 0;

		auto& array = *qin->descriptors;

		qin->heap->allocate(&array[index].offset);

		qin->heap->get_CPU_descriptor(out_cpu_handle, array[index].offset, 0);
		qin->heap->get_GPU_descriptor(out_gpu_handle, array[index].offset, 0);

		index++;
	};
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
	{
		// TODO
			OutputDebugString(L"WARNING: ImGui descriptors not freed\n");
	};
	ImGui_ImplSDL3_InitForD3D(window.get_window());
	ImGui_ImplDX12_Init(&init_info);
}

void D3D12Context::start_imgui_frame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void D3D12Context::render_imgui_draw_data(CommandList* cmd_list)
{
	const auto cmd_list_d3d12 = to_internal(*cmd_list);
	ID3D12DescriptorHeap* heaps[] = { m_imgui_heap.Get().Get() };
	cmd_list_d3d12->Get()->SetDescriptorHeaps(1, heaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list_d3d12->Get());
}

void D3D12Context::destroy_imgui()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void D3D12Context::wait_idle(Queue& queue)
{
	auto value = get_fence_value(m_fence_wait_all_) + 1;

	// Signal
	const auto queue_d3d12 = to_internal(queue);
	auto fence = to_internal(m_fence_wait_all_);
	queue_d3d12->Get()->Signal(fence->fence.Get(), value);

	const WaitInfo wait_info
	{
		.wait_all = true,
		.count = 1,
		.fences = &m_fence_wait_all_,
		.values = &value,
		.timeout = INFINITE
	};
	wait_fences(wait_info);
}

D3D12Context::~D3D12Context()
{
    m_allocator_.Reset();
    m_swapchain_.Reset();
	m_dxgi_factory_.Reset();
#ifdef _DEBUG
	m_dxgi_debug_->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	m_dxgi_debug_.Reset();
	m_debug_.Reset();
#endif
	m_allocator_.Reset();
	m_device_.Reset();
}
