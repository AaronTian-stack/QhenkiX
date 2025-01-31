#pragma once
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <dxgidebug.h>

#include "D3D12MemAlloc.h"
#include "d3d12_heap.h"
#include "graphics/qhenki/context.h"

using Microsoft::WRL::ComPtr;

class D3D12Context : public qhenki::Context
{
	ComPtr<IDXGIFactory6> m_dxgi_factory_ = nullptr;
#ifdef _DEBUG
	ComPtr<ID3D12Debug3> m_debug_;
	ComPtr<IDXGIDebug1> m_dxgi_debug_;
#endif
	ComPtr<ID3D12Device> m_device_;
	ComPtr<D3D12MA::Allocator> m_allocator_;

	ComPtr<IDXGISwapChain3> m_swapchain_;

	D3D12Heap m_rtv_heap_;

public:
	void create() override;
	bool create_swapchain(DisplayWindow& window, const qhenki::SwapchainDesc& swapchain_desc,
	                      qhenki::Swapchain& swapchain, qhenki::Queue& direct_queue) override;
	bool resize_swapchain(qhenki::Swapchain& swapchain, int width, int height) override;
	bool present(qhenki::Swapchain& swapchain) override;

	bool create_shader(qhenki::Shader& shader, const std::wstring& path, qhenki::ShaderType type,
		std::vector<D3D_SHADER_MACRO> macros) override;
	bool create_pipeline(const qhenki::GraphicsPipelineDesc& desc, qhenki::GraphicsPipeline& pipeline,
	                     qhenki::Shader& vertex_shader, qhenki::Shader& pixel_shader, wchar_t const* debug_name) override;
	bool bind_pipeline(qhenki::CommandList& cmd_list, qhenki::GraphicsPipeline& pipeline) override;

	bool create_buffer(const qhenki::BufferDesc& desc, const void* data, qhenki::Buffer& buffer, wchar_t const* debug_name) override;

	void* map_buffer(const qhenki::Buffer& buffer) override;
	void unmap_buffer(const qhenki::Buffer& buffer) override;

	void bind_vertex_buffers(qhenki::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
		const qhenki::Buffer* buffers, const unsigned* offsets) override;
	void bind_index_buffer(qhenki::CommandList& cmd_list, const qhenki::Buffer& buffer, DXGI_FORMAT format,
		unsigned offset) override;

	bool create_queue(const qhenki::QueueType type, qhenki::Queue& queue) override;
	bool create_command_pool(qhenki::CommandPool& command_pool, const qhenki::Queue& queue) override;

	void start_render_pass(qhenki::CommandList& cmd_list, qhenki::Swapchain& swapchain,
		const qhenki::RenderTarget* depth_stencil) override;
	void start_render_pass(qhenki::CommandList& cmd_list, unsigned rt_count, const qhenki::RenderTarget* rts,
		const qhenki::RenderTarget* depth_stencil) override;

	void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) override;

	void draw(qhenki::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
	void draw_indexed(qhenki::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
		int32_t base_vertex_offset) override;

	void wait_all() override;

	~D3D12Context() override;
};
