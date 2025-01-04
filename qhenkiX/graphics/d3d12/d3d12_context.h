#pragma once
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "D3D12MemAlloc.h"
#include "graphics/qhenki/context.h"

using Microsoft::WRL::ComPtr;

class D3D12Context : public qhenki::Context
{
	ComPtr<IDXGIFactory6> dxgi_factory = nullptr;
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debug_;
#endif
	ComPtr<ID3D12Device> device_;
	ComPtr<D3D12MA::Allocator> allocator_;

	UINT frame_index = 0;

	ComPtr<IDXGISwapChain3> swapchain;

	UINT rtv_descriptor_size = 0;
	const UINT rtv_descriptor_count = 1000;
	ComPtr<ID3D12DescriptorHeap> rtv_heap;

public:
	void create() override;
	bool create_swapchain(DisplayWindow& window, const qhenki::SwapchainDesc& swapchain_desc,
		qhenki::Swapchain& swapchain) override;
	bool resize_swapchain(qhenki::Swapchain& swapchain, int width, int height) override;
	bool present(qhenki::Swapchain& swapchain) override;

	bool create_shader(qhenki::Shader& shader, const std::wstring& path, qhenki::ShaderType type,
		std::vector<D3D_SHADER_MACRO> macros) override;
	bool create_pipeline(const qhenki::GraphicsPipelineDesc& desc, qhenki::GraphicsPipeline& pipeline,
	                     qhenki::Shader& vertex_shader, qhenki::Shader& pixel_shader) override;
	bool bind_pipeline(qhenki::CommandList& cmd_list, qhenki::GraphicsPipeline& pipeline) override;

	bool create_buffer(const qhenki::BufferDesc& desc, const void* data, qhenki::Buffer& buffer, wchar_t const* debug_name) override;
	void bind_vertex_buffers(qhenki::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
		const qhenki::Buffer* buffers, const unsigned* offsets) override;

	void start_render_pass(qhenki::CommandList& cmd_list, qhenki::Swapchain& swapchain,
		const qhenki::RenderTarget* depth_stencil) override;
	void start_render_pass(qhenki::CommandList& cmd_list, unsigned rt_count, const qhenki::RenderTarget* rts,
		const qhenki::RenderTarget* depth_stencil) override;

	void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) override;

	void draw(qhenki::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
	void draw_indexed(qhenki::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
		int32_t base_vertex_offset) override;

	void wait_all() override;
};
