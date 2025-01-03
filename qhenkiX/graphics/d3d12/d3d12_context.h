#pragma once
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "D3D12MemAlloc.h"
#include "graphics/vendetta/context.h"

using Microsoft::WRL::ComPtr;

class D3D12Context : public vendetta::Context
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
	bool create_swapchain(DisplayWindow& window, const vendetta::SwapchainDesc& swapchain_desc,
		vendetta::Swapchain& swapchain) override;
	bool resize_swapchain(vendetta::Swapchain& swapchain, int width, int height) override;
	bool present(vendetta::Swapchain& swapchain) override;

	bool create_shader(vendetta::Shader& shader, const std::wstring& path, vendetta::ShaderType type,
		std::vector<D3D_SHADER_MACRO> macros) override;
	bool create_pipeline(vendetta::GraphicsPipeline& pipeline, vendetta::Shader& vertex_shader,
		vendetta::Shader& pixel_shader) override;
	bool bind_pipeline(vendetta::CommandList& cmd_list, vendetta::GraphicsPipeline& pipeline) override;

	bool create_buffer(const vendetta::BufferDesc& desc, const void* data, vendetta::Buffer& buffer) override;
	void bind_vertex_buffers(vendetta::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
		const vendetta::Buffer* buffers, const unsigned* offsets) override;

	void start_render_pass(vendetta::CommandList& cmd_list, vendetta::Swapchain& swapchain,
		const vendetta::RenderTarget* depth_stencil) override;
	void start_render_pass(vendetta::CommandList& cmd_list, unsigned rt_count, const vendetta::RenderTarget* rts,
		const vendetta::RenderTarget* depth_stencil) override;

	void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) override;

	void draw(vendetta::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
	void draw_indexed(vendetta::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
		int32_t base_vertex_offset) override;

	void wait_all() override;
};
