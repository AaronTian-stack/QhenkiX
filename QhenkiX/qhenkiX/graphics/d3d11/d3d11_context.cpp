#include "d3d11_context.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_dx11.h"

#include <DirectXMath.h>
#include <d3dcompiler.h>

#include "d3d11_shader.h"
#include "d3d11_swapchain.h"
#include "d3d11_pipeline.h"
#include "d3d11_shader_compiler.h"
#include "d3d11_texture.h"
#include "graphics/shared/d3d_helper.h"

using namespace qhenki::gfx;

static ComPtr<ID3D11Buffer>* to_internal(const Buffer& ext)
{
	auto d3d11_buffer = static_cast<ComPtr<ID3D11Buffer>*>(ext.internal_state.get());
	assert(d3d11_buffer);
	return d3d11_buffer;
}

static D3D11Swapchain* to_internal(const Swapchain& ext)
{
	auto d3d11_swapchain = static_cast<D3D11Swapchain*>(ext.internal_state.get());
	assert(d3d11_swapchain);
	return d3d11_swapchain;
}

static D3D11Shader* to_internal(const Shader& ext)
{
	auto d3d11_shader = static_cast<D3D11Shader*>(ext.internal_state.get());
	assert(d3d11_shader);
	return d3d11_shader;
}

static D3D11GraphicsPipeline* to_internal(const GraphicsPipeline& ext)
{
	auto d3d11_pipeline = static_cast<D3D11GraphicsPipeline*>(ext.internal_state.get());
	assert(d3d11_pipeline);
	return d3d11_pipeline;
}

static ComPtr<ID3D11SamplerState>* to_internal(const Sampler& ext)
{
	auto d3d11_sampler = static_cast<ComPtr<ID3D11SamplerState>*>(ext.internal_state.get());
	assert(d3d11_sampler);
	return d3d11_sampler;
}

static D3D11Texture* to_internal(const Texture& ext)
{
	auto d3d11_texture = static_cast<D3D11Texture*>(ext.internal_state.get());
	assert(d3d11_texture);
	return d3d11_texture;
}

ID3D11Resource* get_texture_resource(D3D11Texture& tex)
{
	if (std::holds_alternative<ComPtr<ID3D11Texture1D>>(tex.texture))
	{
		return std::get<ComPtr<ID3D11Texture1D>>(tex.texture).Get();
	}
	if (std::holds_alternative<ComPtr<ID3D11Texture2D>>(tex.texture))
	{
		return std::get<ComPtr<ID3D11Texture2D>>(tex.texture).Get();
	}
	if (std::holds_alternative<ComPtr<ID3D11Texture3D>>(tex.texture))
	{
		return std::get<ComPtr<ID3D11Texture3D>>(tex.texture).Get();
	}
	return nullptr;
}

void D3D11Context::create()
{
    // Create factory
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgi_factory_))))
    {
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create DXGI Factory\n");
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
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to find discrete GPU. Defaulting to 0th adapter\n");
        if (FAILED(m_dxgi_factory_->EnumAdapters1(0, &adapter)))
        {
			OutputDebugString(L"Qhenki D3D11 ERROR: Failed to find a adapter\n");
			throw std::runtime_error("D3D11: Failed to find a adapter");
        }
    }

    DXGI_ADAPTER_DESC1 desc;
    HRESULT hr = adapter->GetDesc1(&desc);
	if (FAILED(hr))
	{
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to get adapter description\n");
	}
	else
	{
        std::wstring adapterDescription = desc.Description;
        OutputDebugStringW((L"D3D11: Selected adapter: " + adapterDescription + L"\n").c_str());
	}

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
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to get the debug layer from the device");
        throw std::runtime_error("D3D11: Failed to get the debug layer from the device");
    }
#endif

	shader_compiler = mkU<D3D11ShaderCompiler>();
}

bool D3D11Context::create_swapchain(const DisplayWindow& window, const SwapchainDesc& swapchain_desc, Swapchain& swapchain,
                                    Queue& direct_queue, unsigned& frame_index)
{
	swapchain.desc = swapchain_desc;
	swapchain.internal_state = mkS<D3D11Swapchain>();
	const auto swap_d3d11 = static_cast<D3D11Swapchain*>(swapchain.internal_state.get());
	return swap_d3d11->create(swapchain_desc, window, m_dxgi_factory_.Get(), m_device_.Get(), frame_index);
}

bool D3D11Context::resize_swapchain(Swapchain& swapchain, int width, int height, DescriptorHeap& rtv_heap, unsigned& frame_index)
{
	m_device_context_->Flush();
	const auto swap_d3d11 = to_internal(swapchain);
    return swap_d3d11->resize(m_device_.Get(), m_device_context_.Get(), width, height);
}

bool D3D11Context::create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap& rtv_heap)
{
	return true; // D3D11 does not have descriptors
}

bool D3D11Context::create_shader_dynamic(ShaderCompiler* compiler, Shader* shader, const CompilerInput& input)
{
	if (compiler == nullptr)
	{
		compiler = shader_compiler.get();
	}

	CompilerOutput output = {};
	// ID3DBlob
	if (!compiler->compile(input, output))
	{
		OutputDebugStringA(output.error_message.c_str());
		return false;
	}

	shader->type = input.shader_type;
	shader->shader_model = input.min_shader_model;
	bool result = true;
	// Calls CreateXShader(). Thread safe since it only uses the device
	shader->internal_state = mkS<D3D11Shader>(m_device_.Get(), input.shader_type, input.path, output, result);

    return result;
}

bool D3D11Context::create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline* pipeline,
                                   const Shader& vertex_shader, const Shader& pixel_shader,
                                   PipelineLayout* in_layout, PipelineLayout* out_layout, wchar_t const* debug_name)
{
	// D3D11 does not have concept of pipelines. D3D11 "pipeline" is just shader + state + input layout
	pipeline->internal_state = mkS<D3D11GraphicsPipeline>();
	const auto d3d11_pipeline = static_cast<D3D11GraphicsPipeline*>(pipeline->internal_state.get());
	const auto d3d11_vertex_shader = to_internal(vertex_shader);
	const auto d3d11_pixel_shader = to_internal(pixel_shader);
	assert(d3d11_pipeline);

	d3d11_pipeline->vertex_shader_ = vertex_shader.internal_state.get();
	d3d11_pipeline->pixel_shader_ = pixel_shader.internal_state.get();

	const auto true_vs = std::get_if<D3D11VertexShader>(&d3d11_vertex_shader->m_shader_);
	assert(true_vs);

	ID3D11InputLayout* input_layout_ = m_layout_assembler_.create_input_layout_reflection(m_device_.Get(),
		true_vs->vertex_shader_blob.Get(), desc.increment_slot);
	d3d11_pipeline->input_layout_ = input_layout_;

	bool succeeded = input_layout_ != nullptr;

	// Create Rasterizer state object
	const RasterizerDesc rs = desc.rasterizer_state.value_or(RasterizerDesc{});
	D3D11_RASTERIZER_DESC rasterizer_desc =
	{
		.FillMode = static_cast<D3D11_FILL_MODE>(rs.fill_mode),
		.CullMode = static_cast<D3D11_CULL_MODE>(rs.cull_mode),
		.FrontCounterClockwise = rs.front_counter_clockwise,
		.DepthBias = rs.depth_bias,
		.DepthBiasClamp = rs.depth_bias_clamp,
		.SlopeScaledDepthBias = rs.slope_scaled_depth_bias,
		.DepthClipEnable = rs.depth_clip_enable,
		.ScissorEnable = FALSE, // Scissor enable not included (TODO: add later?)
		.MultisampleEnable = FALSE, // Multisample enable not included (TODO: add later?)
		.AntialiasedLineEnable = FALSE, // Antialiased line not included (TODO: add later?)
	};
	if (FAILED(m_device_->CreateRasterizerState(&rasterizer_desc, d3d11_pipeline->rasterizer_state_.ReleaseAndGetAddressOf())))
	{
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create Rasterizer State");
		succeeded = false;
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
			OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create Blend State\n");
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

		if (FAILED(m_device_->CreateDepthStencilState(&depth_stencil_desc, d3d11_pipeline->depth_stencil_state_.ReleaseAndGetAddressOf())))
		{
			OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create Depth Stencil State\n");
			succeeded = false;
		}
	}

    return succeeded;
}

bool D3D11Context::bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline)
{
	std::scoped_lock lock(m_context_mutex_);
	const auto d3d11_pipeline = to_internal(pipeline);
	d3d11_pipeline->bind(m_device_context_.Get());
	return true;
}

bool D3D11Context::create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, wchar_t const* debug_name)
{
	buffer->desc = desc;
	buffer->internal_state = mkS<ComPtr<ID3D11Buffer>>();
	const auto buffer_d3d11 = to_internal(*buffer);

	D3D11_BUFFER_DESC buffer_info{};
	buffer_info.ByteWidth = desc.size;
    if (desc.usage & BufferUsage::VERTEX)
    {
		buffer_info.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
    }
    if (desc.usage & BufferUsage::INDEX)
    {
		buffer_info.BindFlags = D3D11_BIND_INDEX_BUFFER;
    }
    if (desc.usage & BufferUsage::UNIFORM)
    {
		buffer_info.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    }
    if (desc.usage & BufferUsage::STORAGE)
    {
		buffer_info.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    }
    if (desc.usage & BufferUsage::INDIRECT)
    {
		buffer_info.BindFlags = D3D11_BIND_UNORDERED_ACCESS; // TODO: check this
    }
	if ((desc.visibility & CPU_SEQUENTIAL) 
		|| (desc.visibility & CPU_RANDOM))
	{
		buffer_info.Usage = D3D11_USAGE_DYNAMIC;
		buffer_info.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		buffer_info.Usage = D3D11_USAGE_DEFAULT;
		buffer_info.CPUAccessFlags = 0;
	}
	// misc flags
	//TODO bufferInfo.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA resource_data;
	resource_data.pSysMem = data;

	// Only prefill if explicitly CPU visible, you will need to copy to device local memory like D3D12
	// TODO: get rid of this feature to make it consistent with D3D12?
	const auto resource_data_ptr = data && buffer_info.CPUAccessFlags == D3D11_CPU_ACCESS_WRITE ? &resource_data : nullptr;

	if (FAILED(m_device_->CreateBuffer(
		&buffer_info,
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

bool D3D11Context::create_descriptor(const Buffer& buffer, DescriptorHeap& cpu_heap, Descriptor* descriptor, BufferDescriptorType type)
{
	// D3D11 does not have descriptors
	return true;
}

void D3D11Context::copy_buffer(CommandList* cmd_list, const Buffer& src, UINT64 src_offset, Buffer* dst, UINT64 dst_offset, UINT64 bytes)
{
	assert(src_offset + bytes <= src.desc.size);
	assert(dst_offset + bytes <= dst->desc.size);

	const auto src_d3d11 = to_internal(src);
	const auto dst_d3d11 = to_internal(*dst);

	// Assume 1D for now
	const auto box = CD3D11_BOX(static_cast<long>(src_offset), 0, 0, static_cast<long>(src_offset + bytes), 1, 1);
	
	// Copy entire buffer for now
	// TODO: per subresource
	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->CopySubresourceRegion(
		dst_d3d11->Get(),
		0, // Dst subresource
		static_cast<long>(dst_offset), 0, 0,
		src_d3d11->Get(),
		0, // Src subresource
		&box);
}

bool D3D11Context::create_texture(const TextureDesc& desc, Texture* texture, wchar_t const* debug_name)
{
	texture->desc = desc;
	texture->internal_state = mkS<D3D11Texture>();
	auto texture_d3d11 = static_cast<D3D11Texture*>(texture->internal_state.get());

	// TODO: RT, UAV BindFlags
	UINT bind_flags = D3D11_BIND_SHADER_RESOURCE;
	if (D3DHelper::is_depth_stencil_format(desc.format))
	{
		bind_flags |= D3D11_BIND_DEPTH_STENCIL;
	}

	if (desc.dimension == TextureDimension::TEXTURE_1D)
	{
		D3D11_TEXTURE1D_DESC texture_desc
		{
			.Width = static_cast<UINT>(desc.width),
			.MipLevels = desc.mip_levels,
			.ArraySize = desc.depth_or_array_size,
			.Format = desc.format,
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = bind_flags,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};

		texture_d3d11->texture = ComPtr<ID3D11Texture1D>();
		if (FAILED(m_device_->CreateTexture1D(&texture_desc, nullptr, 
			std::get<ComPtr<ID3D11Texture1D>>(texture_d3d11->texture).ReleaseAndGetAddressOf())))
		{
			OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create 1D texture\n");
			return false;
		}
	}
	else if (desc.dimension == TextureDimension::TEXTURE_2D)
	{
		D3D11_TEXTURE2D_DESC texture_desc
		{
			.Width = static_cast<UINT>(desc.width),
			.Height = static_cast<UINT>(desc.height),
			.MipLevels = desc.mip_levels,
			.ArraySize = desc.depth_or_array_size,
			.Format = desc.format,
			.SampleDesc = { 1, 0 }, // TODO: sample count
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = bind_flags,
			.CPUAccessFlags = 0,
			.MiscFlags = 0, // TODO: cubemaps?
		};

		texture_d3d11->texture = ComPtr<ID3D11Texture2D>();
		if (FAILED(m_device_->CreateTexture2D(&texture_desc, nullptr,
			std::get<ComPtr<ID3D11Texture2D>>(texture_d3d11->texture).ReleaseAndGetAddressOf())))
		{
			OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create 2D texture\n");
			return false;
		}
	}
	else if (desc.dimension == TextureDimension::TEXTURE_3D)
	{
		D3D11_TEXTURE3D_DESC texture_desc
		{
			.Width = static_cast<UINT>(desc.width),
			.Height = static_cast<UINT>(desc.height),
			.Depth = static_cast<UINT>(desc.depth_or_array_size),
			.MipLevels = desc.mip_levels,
			.Format = desc.format,
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = bind_flags,
			.CPUAccessFlags = 0,
			.MiscFlags = 0,
		};

		texture_d3d11->texture = ComPtr<ID3D11Texture3D>();
		if (FAILED(m_device_->CreateTexture3D(&texture_desc, nullptr,
			std::get<ComPtr<ID3D11Texture3D>>(texture_d3d11->texture).ReleaseAndGetAddressOf())))
		{
			OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create 3D texture\n");
			return false;
		}
	}

	return true;
}

bool D3D11Context::create_descriptor_texture_view(const Texture& texture, DescriptorHeap& heap, Descriptor* descriptor)
{
	auto texture_d3d11 = to_internal(texture);

	descriptor->heap = &heap;
	descriptor->offset = texture_d3d11->shader_resource_views.size();

	texture_d3d11->shader_resource_views.push_back({});

	auto resource = get_texture_resource(*texture_d3d11);

	// TODO: description
	if (FAILED(m_device_->CreateShaderResourceView(resource, nullptr, 
		texture_d3d11->shader_resource_views.back().ReleaseAndGetAddressOf())))
	{
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create texture SRV\n");
		return false;
	}
	return true;
}

bool D3D11Context::copy_to_texture(CommandList& cmd_list, const void* data, Buffer& staging, Texture& texture)
{
	D3D11_BOX box
	{
		.left = 0,
		.top = 0, // TODO: 3D texture
		.front = 0, // TODO: 3D texture
		.right = static_cast<UINT>(texture.desc.width),
		.bottom = texture.desc.height,
		.back = 1, // TODO: which subresource
	};
	
	auto texture_d3d11 = to_internal(texture);
	ID3D11Resource* resource = get_texture_resource(*texture_d3d11);
	if (!resource)
	{
		OutputDebugString(L"Qhenki D3D11 ERROR: copy_to_texture Failed to get texture resource\n");
		return false;
	}

	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->UpdateSubresource(
		resource,
		0, // TODO: which subresource
		&box,
		data,
		texture.desc.width * D3DHelper::bytes_per_pixel(texture.desc.format), // TODO: which subresource
		0);

	return true;
}

bool D3D11Context::create_sampler(const SamplerDesc& desc, Sampler* sampler)
{
	assert(sampler);
	sampler->desc = desc;
	sampler->internal_state = mkS<ComPtr<ID3D11SamplerState>>();
	auto sampler_d3d11 = static_cast<ComPtr<ID3D11SamplerState>*>(sampler->internal_state.get());

	D3D11_SAMPLER_DESC sampler_desc
	{
		.Filter = static_cast<D3D11_FILTER>(D3DHelper::filter(
			desc.min_filter, desc.mag_filter, desc.mip_filter, desc.comparison_func, desc.max_anisotropy)), // Shared type values D3D12
		.AddressU = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(D3DHelper::texture_address_mode(desc.address_mode_u)), // Same in D3D11, D3D12
		.AddressV = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(D3DHelper::texture_address_mode(desc.address_mode_v)),
		.AddressW = static_cast<D3D11_TEXTURE_ADDRESS_MODE>(D3DHelper::texture_address_mode(desc.address_mode_w)),
		.MipLODBias = desc.mip_lod_bias,
		.MaxAnisotropy = desc.max_anisotropy,
		.ComparisonFunc = desc.comparison_func == ComparisonFunc::NONE ? D3D11_COMPARISON_NEVER :
			static_cast<D3D11_COMPARISON_FUNC>(D3DHelper::comparison_func(desc.comparison_func)), // D3D11 doesn't have NONE
		.BorderColor = { desc.border_color[0], desc.border_color[1],desc.border_color[2] , desc.border_color[3] },
		.MinLOD = desc.min_lod,
		.MaxLOD = desc.max_lod,
	};

	auto result = m_device_->CreateSamplerState(&sampler_desc, sampler_d3d11->ReleaseAndGetAddressOf());

	if (FAILED(result))
	{
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to create sampler state\n");
		return false;
	}

	return true;
}

void* D3D11Context::map_buffer(const Buffer& buffer)
{
	D3D11_MAPPED_SUBRESOURCE mapped_resource;
	const auto buffer_d3d11 = to_internal(buffer);
	std::scoped_lock lock(m_context_mutex_);
	if (FAILED(m_device_context_->Map(
		buffer_d3d11->Get(),
		0, 
		D3D11_MAP_WRITE_DISCARD, 
		0, 
		&mapped_resource)))
	{
		OutputDebugString(L"Qhenki D3D11 ERROR: Failed to map buffer\n");
		return nullptr;
	}
	return mapped_resource.pData;
}

void D3D11Context::unmap_buffer(const Buffer& buffer)
{
	const auto buffer_d3d11 = to_internal(buffer);
	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->Unmap(buffer_d3d11->Get(), 0);
}

void D3D11Context::bind_vertex_buffers(CommandList* cmd_list, unsigned start_slot, unsigned buffer_count, 
	const Buffer* const* buffers, const unsigned* const strides, const unsigned* const offsets)
{
	assert(buffer_count <= D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
	std::array<ID3D11Buffer**, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> buffer_d3d11{};
	for (unsigned int i = 0; i < buffer_count; i++)
	{
		auto buffer = to_internal(*buffers[i]);
		buffer_d3d11[i] = buffer->GetAddressOf();
	}
	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->IASetVertexBuffers(start_slot, buffer_count, buffer_d3d11[0], strides, offsets);
}

void D3D11Context::bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format,
                                     unsigned offset)
{
	const auto buffer_d3d11 = to_internal(buffer);
	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->IASetIndexBuffer(buffer_d3d11->Get(), D3DHelper::get_dxgi_format(format), offset);
}

bool D3D11Context::create_queue(const QueueType type, Queue* queue)
{
	return true; // D3D11 does not have queues
}

bool D3D11Context::create_command_pool(CommandPool* command_pool, const Queue& queue)
{
	return true; // D3D11 does not have queues
}

bool D3D11Context::create_command_list(CommandList* cmd_list,
                                       const CommandPool& command_pool)
{
	return true; // D3D11 does not have command lists
}

bool D3D11Context::close_command_list(CommandList* cmd_list)
{
	return true; // D3D11 does not have command lists
}

bool D3D11Context::reset_command_pool(CommandPool* command_pool)
{
	return true; // D3D11 does not have command pools
}

void D3D11Context::start_render_pass(CommandList* cmd_list, Swapchain& swapchain,
                                     const RenderTarget* depth_stencil, UINT frame_index)
{
	const auto swap_d3d11 = to_internal(swapchain);
	const auto rtv = swap_d3d11->sc_render_target.Get();
	const float clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	// TODO: optional clear, user set values
	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->ClearRenderTargetView(rtv, clear_color);
	m_device_context_->OMSetRenderTargets(1, &rtv, nullptr);
}

void D3D11Context::start_render_pass(CommandList* cmd_list, unsigned rt_count,
                                     const RenderTarget* rts, const RenderTarget* depth_stencil)
{
	std::scoped_lock lock(m_context_mutex_);
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

void D3D11Context::set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport)
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
	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->RSSetViewports(count, m_viewports_.data());
}

void D3D11Context::set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect)
{
	// D3D12_RECT = D3D11_RECT = RECT
	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->RSSetScissorRects(count, scissor_rect);
}

void D3D11Context::draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset)
{
	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->Draw(vertex_count, start_vertex_offset);
}

void D3D11Context::draw_indexed(CommandList* cmd_list, uint32_t index_count, uint32_t start_index_offset,
                                int32_t base_vertex_offset)
{
	std::scoped_lock lock(m_context_mutex_);
	m_device_context_->DrawIndexed(index_count, start_index_offset, base_vertex_offset);
}

void D3D11Context::submit_command_lists(const SubmitInfo& submit_info, Queue* queue)
{
	// D3D11 does not have command lists
}

bool D3D11Context::create_fence(Fence* fence, uint64_t initial_value)
{
	// D3D11 has fences but these are only for interop with D3D12
	return true;
}

uint64_t D3D11Context::get_fence_value(const Fence& fence)
{
	return 0;
}

bool D3D11Context::wait_fences(const WaitInfo& info)
{
	// D3D11 does not have fences
	return true;
}

void D3D11Context::set_barrier_resource(unsigned count, ImageBarrier* barriers, const Swapchain& swapchain, unsigned frame_index)
{
	// D3D11 does not have barriers
}

void D3D11Context::set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target)
{
	// D3D11 does not have barriers
}

void D3D11Context::issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers)
{
	// D3D11 does not have barriers
}

void D3D11Context::init_imgui(const DisplayWindow& window, const Swapchain& swapchain)
{
	ImGui_ImplSDL3_InitForD3D(window.get_window());
	ImGui_ImplDX11_Init(m_device_.Get(), m_device_context_.Get());
}

void D3D11Context::start_imgui_frame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void D3D11Context::render_imgui_draw_data(CommandList* cmd_list)
{
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void D3D11Context::destroy_imgui()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void D3D11Context::compatibility_set_constant_buffers(unsigned slot, unsigned count, Buffer* buffers, PipelineStage stage)
{
	std::array<ID3D11Buffer**, 15> buffer_d3d11{};
	assert(count <= buffer_d3d11.size());
	for (unsigned i = 0; i < count; i++)
	{
		buffer_d3d11[i] = to_internal(buffers[i])->GetAddressOf();
	}
	switch (stage)
	{
	case PipelineStage::VERTEX:
		m_device_context_->VSSetConstantBuffers(slot, count, buffer_d3d11[0]);
		break;
	case PipelineStage::PIXEL:
		m_device_context_->PSSetConstantBuffers(slot, count, buffer_d3d11[0]);
		break;
	case PipelineStage::COMPUTE:
		m_device_context_->CSSetConstantBuffers(slot, count, buffer_d3d11[0]);
		break;
	default:
		throw std::runtime_error("D3D11: Invalid pipeline stage");
	}
}

void D3D11Context::compatibility_set_textures(unsigned slot, unsigned count, Texture* textures, Descriptor* descriptors, AccessFlags flag, PipelineStage stage)
{
	// Read or write (as UAV not RT) access

    union ResourceViews {
           std::array<ID3D11UnorderedAccessView**, 15> unordered_access_views;
           std::array<ID3D11ShaderResourceView**, 15> shader_resource_views;
       } resource_views;

	switch (flag)
	{
	case ACCESS_STORAGE_ACCESS:
		resource_views.unordered_access_views = std::array<ID3D11UnorderedAccessView**, 15>{};
		break;
	case ACCESS_SHADER_RESOURCE:
		resource_views.shader_resource_views = std::array<ID3D11ShaderResourceView**, 15>{};
		break;
	default:
		OutputDebugString(L"Qhenki D3D11 ERROR: Invalid access flag for texture\n");
		return;
	}

	for (unsigned i = 0; i < count; i++)
	{
		auto texture_d3d11 = to_internal(textures[i]);
		// The descriptor offset is used as index into vector
		switch (flag)
		{
		case ACCESS_STORAGE_ACCESS:
			resource_views.unordered_access_views[i] = texture_d3d11->unordered_access_views[descriptors[i].offset].GetAddressOf();
			break;
		case ACCESS_SHADER_RESOURCE:
			resource_views.shader_resource_views[i] = texture_d3d11->shader_resource_views[descriptors[i].offset].GetAddressOf();
			break;
		default:
			OutputDebugString(L"Qhenki D3D11 ERROR: Invalid access flag for texture\n");
			return;
		}
	}
	const UINT n1 = -1;

	switch (flag)
	{
	//case ACCESS_RENDER_TARGET:
	case ACCESS_STORAGE_ACCESS:
		switch (stage)
		{
			// TODO: need a better way of doing this
			// D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL and D3D11_KEEP_UNORDERED_ACCESS_VIEWS ?
		case PipelineStage::VERTEX:
			m_device_context_->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
				slot, count, resource_views.unordered_access_views[0], &n1);
			break;
		case PipelineStage::PIXEL:
			m_device_context_->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
				slot, count, resource_views.unordered_access_views[0], &n1);
			break;
		case PipelineStage::COMPUTE:
			m_device_context_->CSSetUnorderedAccessViews(slot, count, resource_views.unordered_access_views[0], nullptr);
			break;
		default:
			OutputDebugString(L"Qhenki D3D11 ERROR: Invalid pipeline stage for storage access\n");
		}
		break;
	case ACCESS_SHADER_RESOURCE:
		switch (stage)
		{
		case PipelineStage::VERTEX:
			m_device_context_->VSSetShaderResources(slot, count, resource_views.shader_resource_views[0]);
			break;
		case PipelineStage::PIXEL:
			m_device_context_->PSSetShaderResources(slot, count, resource_views.shader_resource_views[0]);
			break;
		case PipelineStage::COMPUTE:
			m_device_context_->CSSetShaderResources(slot, count, resource_views.shader_resource_views[0]);
			break;
		}
		break;
	default:
		OutputDebugString(L"Qhenki D3D11 ERROR: Invalid access flag for texture\n");
		break;
	}
}

void D3D11Context::compatibility_set_samplers(unsigned slot, unsigned count, Sampler* samplers, PipelineStage stage)
{
	std::array<ID3D11SamplerState**, 15> sampler_d3d11{};
	assert(count <= sampler_d3d11.size());
	for (unsigned i = 0; i < count; i++)
	{
		sampler_d3d11[i] = to_internal(samplers[i])->GetAddressOf();
	}
	switch (stage)
	{
	case PipelineStage::VERTEX:
		m_device_context_->VSSetSamplers(slot, count, sampler_d3d11[0]);
		break;
	case PipelineStage::PIXEL:
		m_device_context_->PSSetSamplers(slot, count, sampler_d3d11[0]);
		break;
	case PipelineStage::COMPUTE:
		m_device_context_->CSSetSamplers(slot, count, sampler_d3d11[0]);
		break;
	default:
		throw std::runtime_error("D3D11: Invalid pipeline stage");
	}
}

void D3D11Context::wait_idle(Queue& queue)
{
    m_device_context_->Flush();
}

bool D3D11Context::present(Swapchain& swapchain, unsigned fence_count, Fence* wait_fences, unsigned swapchain_index)
{
	const auto swap_d3d11 = to_internal(swapchain);
	const auto result = swap_d3d11->swapchain->Present(1, 0);
    return result == S_OK;
}

std::unique_ptr<ShaderCompiler> D3D11Context::create_shader_compiler()
{
	return std::make_unique<D3D11ShaderCompiler>();
}

D3D11Context::~D3D11Context()
{
    m_device_context_->ClearState();
    m_device_context_->Flush();
    m_device_context_.Reset();
    m_dxgi_factory_.Reset();
	m_layout_assembler_.clear_maps();
#if _DEBUG
    m_debug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
    m_debug_.Reset();
#endif
    m_device_.Reset();
}
