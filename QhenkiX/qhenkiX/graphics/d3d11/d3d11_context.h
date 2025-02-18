#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <boost/pool/object_pool.hpp>

#include "d3d11_layout_assembler.h"
#include "graphics/qhenki/context.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// SM 5.1
class D3D11Context : public qhenki::gfx::Context
{
	ComPtr<IDXGIFactory6> m_dxgi_factory_;
#ifdef _DEBUG
	ComPtr<ID3D11Debug> m_debug_;
#endif
	ComPtr<ID3D11Device> m_device_;
	ComPtr<ID3D11DeviceContext> m_device_context_;

	D3D11LayoutAssembler m_layout_assembler_;

	std::array<D3D11_VIEWPORT, 16> m_viewports_;

public:
	// Will not work with things that don't derive from ID3D11DeviceChild
	template<UINT TDebugNameLength>
	static void set_debug_name(_In_ ID3D11DeviceChild* device_resource, _In_z_ const char(&debug_name)[TDebugNameLength]);

	void create() override;
	bool create_swapchain(DisplayWindow& window, const qhenki::gfx::SwapchainDesc& swapchain_desc, qhenki::gfx::Swapchain& swapchain, qhenki::gfx::Queue
	                      & direct_queue, unsigned buffer_count, unsigned& frame_index) override;
	bool resize_swapchain(qhenki::gfx::Swapchain& swapchain, int width, int height) override;
	bool present(qhenki::gfx::Swapchain& swapchain) override;

	uPtr<ShaderCompiler> create_shader_compiler() override;
	// thread safe
	bool create_shader_dynamic(ShaderCompiler* compiler, qhenki::gfx::Shader& shader, const CompilerInput& input) override;
	// thread safe
	bool create_pipeline(const qhenki::gfx::GraphicsPipelineDesc& desc, qhenki::gfx::GraphicsPipeline& pipeline, 
		qhenki::gfx::Shader& vertex_shader, qhenki::gfx::Shader& pixel_shader, qhenki::gfx::PipelineLayout* layout, wchar_t const* debug_name) override;
	bool bind_pipeline(qhenki::gfx::CommandList& cmd_list, qhenki::gfx::GraphicsPipeline& pipeline) override;

	bool create_buffer(const qhenki::gfx::BufferDesc& desc, const void* data, qhenki::gfx::Buffer& buffer, wchar_t const* debug_name = nullptr) override;

	void* map_buffer(const qhenki::gfx::Buffer& buffer) override;
	void unmap_buffer(const qhenki::gfx::Buffer& buffer) override;

	void bind_vertex_buffers(qhenki::gfx::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count, const qhenki::gfx::Buffer* buffers, const
	                        unsigned* offsets) override;
	void bind_index_buffer(qhenki::gfx::CommandList& cmd_list, const qhenki::gfx::Buffer& buffer, DXGI_FORMAT format,
		unsigned offset) override;

	bool create_queue(const qhenki::gfx::QueueType type, qhenki::gfx::Queue& queue) override;
	bool create_command_pool(qhenki::gfx::CommandPool& command_pool, const qhenki::gfx::Queue& queue) override;
	bool create_command_list(qhenki::gfx::CommandList& cmd_list, const qhenki::gfx::CommandPool& command_pool) override;

	// Recording commands is not thread safe in D3D11. TODO: runtime check that this is called from the same thread

	void start_render_pass(qhenki::gfx::CommandList& cmd_list, qhenki::gfx::Swapchain& swapchain,
		const qhenki::gfx::RenderTarget* depth_stencil) override;
	void start_render_pass(qhenki::gfx::CommandList& cmd_list, unsigned rt_count,
	                       const qhenki::gfx::RenderTarget* rts, const qhenki::gfx::RenderTarget* depth_stencil) override;

	void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) override;

	void draw(qhenki::gfx::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
	void draw_indexed(qhenki::gfx::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
		int32_t base_vertex_offset) override;

	void wait_all() override;
	~D3D11Context() override;

	friend class D3D11GraphicsPipeline;
};

template <UINT TDebugNameLength>
void D3D11Context::set_debug_name(ID3D11DeviceChild* device_resource, const char(& debug_name)[TDebugNameLength])
{
	device_resource->SetPrivateData(WKPDID_D3DDebugObjectName, TDebugNameLength - 1, debug_name);
}