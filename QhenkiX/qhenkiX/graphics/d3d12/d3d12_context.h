#pragma once
#include <d3d12shader.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <boost/pool/object_pool.hpp>

#include "D3D12MemAlloc.h"
#include "d3d12_heap.h"
#include "d3d12_pipeline.h"
#include "d3d12_root_hasher.h"
#include "graphics/d3d11/d3d11_shader_compiler.h"
#include "graphics/qhenki/context.h"

using Microsoft::WRL::ComPtr;

class D3D12Context : public qhenki::gfx::Context
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

	D3D11ShaderCompiler m_d3d11_shader_compiler_; // Needed for SM < 6.0

	D3D12RootHasher m_root_reflection_;

	std::vector<D3D12_INPUT_ELEMENT_DESC> shader_reflection(ID3D12ShaderReflection* shader_reflection, const D3D12_SHADER_DESC& shader_desc, bool interleaved) const;
	void root_signature_reflection(ID3D12ShaderReflection* shader_reflection, const D3D12_SHADER_DESC& shader_desc);

public:
	void create() override;
	bool create_swapchain(DisplayWindow& window, const qhenki::gfx::SwapchainDesc& swapchain_desc,
	                      qhenki::gfx::Swapchain& swapchain, qhenki::gfx::Queue& direct_queue, unsigned buffer_count, unsigned&
	                      frame_index) override;
	bool resize_swapchain(qhenki::gfx::Swapchain& swapchain, int width, int height) override;
	bool present(qhenki::gfx::Swapchain& swapchain) override;

	uPtr<ShaderCompiler> create_shader_compiler() override;
	bool create_shader_dynamic(ShaderCompiler* compiler, qhenki::gfx::Shader& shader, const CompilerInput& input) override;

	bool create_pipeline(const qhenki::gfx::GraphicsPipelineDesc& desc, qhenki::gfx::GraphicsPipeline& pipeline,
		qhenki::gfx::Shader& vertex_shader, qhenki::gfx::Shader& pixel_shader, 
		qhenki::gfx::PipelineLayout* in_layout, qhenki::gfx::PipelineLayout* out_layout, wchar_t const* debug_name) override;

	bool bind_pipeline(qhenki::gfx::CommandList& cmd_list, qhenki::gfx::GraphicsPipeline& pipeline) override;

	bool create_buffer(const qhenki::gfx::BufferDesc& desc, const void* data, qhenki::gfx::Buffer& buffer, wchar_t const* debug_name) override;

	void* map_buffer(const qhenki::gfx::Buffer& buffer) override;
	void unmap_buffer(const qhenki::gfx::Buffer& buffer) override;

	void bind_vertex_buffers(qhenki::gfx::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
		const qhenki::gfx::Buffer* buffers, const unsigned* offsets) override;

	void bind_index_buffer(qhenki::gfx::CommandList& cmd_list, const qhenki::gfx::Buffer& buffer, DXGI_FORMAT format,
		unsigned offset) override;

	bool create_queue(qhenki::gfx::QueueType type, qhenki::gfx::Queue& queue) override;
	bool create_command_pool(qhenki::gfx::CommandPool& command_pool, const qhenki::gfx::Queue& queue) override;
	bool create_command_list(qhenki::gfx::CommandList& cmd_list, const qhenki::gfx::CommandPool& command_pool) override;

	void start_render_pass(qhenki::gfx::CommandList& cmd_list, qhenki::gfx::Swapchain& swapchain,
		const qhenki::gfx::RenderTarget* depth_stencil) override;

	void start_render_pass(qhenki::gfx::CommandList& cmd_list, unsigned rt_count, const qhenki::gfx::RenderTarget* rts,
		const qhenki::gfx::RenderTarget* depth_stencil) override;

	void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) override;

	void draw(qhenki::gfx::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
	void draw_indexed(qhenki::gfx::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
		int32_t base_vertex_offset) override;

	void wait_all() override;

	~D3D12Context() override;
};
