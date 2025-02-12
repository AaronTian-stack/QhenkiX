#pragma once
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <boost/pool/object_pool.hpp>

#include "D3D12MemAlloc.h"
#include "d3d12_heap.h"
#include "graphics/qhenki/context.h"

using Microsoft::WRL::ComPtr;

class D3D12Context : public qhenki::graphics::Context
{
	D3D12_FEATURE_DATA_SHADER_MODEL m_shader_model_ = {};

	ComPtr<IDXGIFactory6> m_dxgi_factory_ = nullptr;
#ifdef _DEBUG
	ComPtr<ID3D12Debug3> m_debug_;
	ComPtr<IDXGIDebug1> m_dxgi_debug_;
#endif
	ComPtr<ID3D12Device> m_device_;
	ComPtr<D3D12MA::Allocator> m_allocator_;

	ComPtr<IDXGISwapChain3> m_swapchain_;

	D3D12Heap m_rtv_heap_;

	std::mutex m_pipeline_desc_mutex_;
	boost::object_pool<D3D12_GRAPHICS_PIPELINE_STATE_DESC> m_pipeline_desc_pool_;

	ComPtr<IDxcLibrary> m_library_;
	ComPtr<IDxcCompiler> m_compiler_;

public:
	void create() override;
	bool create_swapchain(DisplayWindow& window, const qhenki::graphics::SwapchainDesc& swapchain_desc,
	                      qhenki::graphics::Swapchain& swapchain, qhenki::graphics::Queue& direct_queue) override;
	bool resize_swapchain(qhenki::graphics::Swapchain& swapchain, int width, int height) override;
	bool present(qhenki::graphics::Swapchain& swapchain) override;

	bool create_shader_dynamic(qhenki::graphics::Shader& shader, const CompilerInput& input) override;
	bool create_pipeline(const qhenki::graphics::GraphicsPipelineDesc& desc, qhenki::graphics::GraphicsPipeline& pipeline,
	                     qhenki::graphics::Shader& vertex_shader, qhenki::graphics::Shader& pixel_shader, wchar_t const* debug_name) override;
	bool bind_pipeline(qhenki::graphics::CommandList& cmd_list, qhenki::graphics::GraphicsPipeline& pipeline) override;

	bool create_buffer(const qhenki::graphics::BufferDesc& desc, const void* data, qhenki::graphics::Buffer& buffer, wchar_t const* debug_name) override;

	void* map_buffer(const qhenki::graphics::Buffer& buffer) override;
	void unmap_buffer(const qhenki::graphics::Buffer& buffer) override;

	void bind_vertex_buffers(qhenki::graphics::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
		const qhenki::graphics::Buffer* buffers, const unsigned* offsets) override;
	void bind_index_buffer(qhenki::graphics::CommandList& cmd_list, const qhenki::graphics::Buffer& buffer, DXGI_FORMAT format,
		unsigned offset) override;

	bool create_queue(const qhenki::graphics::QueueType type, qhenki::graphics::Queue& queue) override;
	bool create_command_pool(qhenki::graphics::CommandPool& command_pool, const qhenki::graphics::Queue& queue) override;

	void start_render_pass(qhenki::graphics::CommandList& cmd_list, qhenki::graphics::Swapchain& swapchain,
		const qhenki::graphics::RenderTarget* depth_stencil) override;
	void start_render_pass(qhenki::graphics::CommandList& cmd_list, unsigned rt_count, const qhenki::graphics::RenderTarget* rts,
		const qhenki::graphics::RenderTarget* depth_stencil) override;

	void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) override;

	void draw(qhenki::graphics::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
	void draw_indexed(qhenki::graphics::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
		int32_t base_vertex_offset) override;

	void wait_all() override;

	~D3D12Context() override;
};
