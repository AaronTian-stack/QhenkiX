#include "d3d12_context.h"

#include <iostream>

void D3D12Context::create()
{
	// Create the DXGI factory
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory))))
	{
		std::cerr << "D3D12: Failed to create DXGI factory" << std::endl;
		throw std::runtime_error("D3D12: Failed to create DXGI factory");
	}
#ifdef _DEBUG
	dxgi_factory->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof("DXGI Factory") - 1, "DXGI Factory");
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
		std::cerr << "D3D12: Failed to find discrete GPU. Defaulting to 0th adapter" << std::endl;
		if (FAILED(dxgi_factory->EnumAdapters1(0, &adapter)))
		{
			throw std::runtime_error("D3D12: Failed to find a adapter");
		}
	}

	DXGI_ADAPTER_DESC1 desc;
	HRESULT hr = adapter->GetDesc1(&desc);
	if (FAILED(hr)) std::cerr << "D3D12: Failed to get adapter description" << std::endl;
	else std::wcout << L"D3D12: Selected adapter: " << desc.Description << L"\n";

	if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_))))
	{
		std::cerr << "D3D12: Failed to create device" << std::endl;
		throw std::runtime_error("D3D12: Failed to create device");
	}

	D3D12MA::ALLOCATOR_DESC allocatorDesc = 
	{
		.pDevice = device_.Get(),
		.pAdapter = adapter.Get(),
	};
	// These flags are optional but recommended.
	allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED |
		D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;

	if (FAILED( CreateAllocator(&allocatorDesc, &allocator_)))
	{
		std::cerr << "D3D12: Failed to create memory allocator" << std::endl;
		throw std::runtime_error("D3D12: Failed to create memory allocator");
	}

	// Create RTV heap
	rtv_heap.create(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

#ifdef _DEBUG
	// Enable the D3D12 debug layer
	ComPtr<ID3D12Debug> debug_controller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
	{
		debug_controller->EnableDebugLayer();
	}
#endif
}

bool D3D12Context::create_swapchain(DisplayWindow& window, const qhenki::SwapchainDesc& swapchain_desc,
	qhenki::Swapchain& swapchain)
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
		.BufferCount = 2,
		.Scaling = DXGI_SCALING_STRETCH,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags = {},
	};

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_descriptor = {};
	swap_chain_fullscreen_descriptor.Windowed = true;

	ComPtr<IDXGISwapChain1> swapchain1;
	if (FAILED(dxgi_factory->CreateSwapChainForHwnd(
		nullptr,        // TODO: Swap chain needs the queue so that it can force a flush on it.
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
	if (FAILED(swapchain1.As(&this->swapchain)))
	{
		std::cerr << "D3D12: Failed to get IDXGISwapChain3 from IDXGISwapChain1" << std::endl;
		return false;
	}
	frame_index = this->swapchain->GetCurrentBackBufferIndex();
	return true;
}

bool D3D12Context::resize_swapchain(qhenki::Swapchain& swapchain, int width, int height)
{
	return false;
}

bool D3D12Context::present(qhenki::Swapchain& swapchain)
{
	return false;
}

bool D3D12Context::create_shader(qhenki::Shader& shader, const std::wstring& path, qhenki::ShaderType type,
	std::vector<D3D_SHADER_MACRO> macros)
{
	return false;
}

bool D3D12Context::create_pipeline(const qhenki::GraphicsPipelineDesc& desc, qhenki::GraphicsPipeline& pipeline,
                                   qhenki::Shader& vertex_shader, qhenki::Shader& pixel_shader)
{
	return false;
}

bool D3D12Context::bind_pipeline(qhenki::CommandList& cmd_list, qhenki::GraphicsPipeline& pipeline)
{
	return false;
}

bool D3D12Context::create_buffer(const qhenki::BufferDesc& desc, const void* data, qhenki::Buffer& buffer, wchar_t const* debug_name)
{
	return false;
}

void* D3D12Context::map_buffer(const qhenki::Buffer& buffer)
{
	return nullptr;
}

void D3D12Context::unmap_buffer(const qhenki::Buffer& buffer)
{
}

void D3D12Context::bind_vertex_buffers(qhenki::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
                                       const qhenki::Buffer* buffers, const unsigned* offsets)
{
}

void D3D12Context::bind_index_buffer(qhenki::CommandList& cmd_list, const qhenki::Buffer& buffer, DXGI_FORMAT format,
	unsigned offset)
{
	assert(false && "D3D12: Index Buffer not implemented");
}

void D3D12Context::start_render_pass(qhenki::CommandList& cmd_list, qhenki::Swapchain& swapchain,
                                     const qhenki::RenderTarget* depth_stencil)
{
}

void D3D12Context::start_render_pass(qhenki::CommandList& cmd_list, unsigned rt_count,
	const qhenki::RenderTarget* rts, const qhenki::RenderTarget* depth_stencil)
{
}

void D3D12Context::set_viewports(unsigned count, const D3D12_VIEWPORT* viewport)
{
}

void D3D12Context::draw(qhenki::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset)
{
}

void D3D12Context::draw_indexed(qhenki::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
	int32_t base_vertex_offset)
{
}

void D3D12Context::wait_all()
{
}
