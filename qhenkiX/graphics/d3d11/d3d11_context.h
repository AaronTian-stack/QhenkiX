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

class D3D11Context : public qhenki::Context
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
	bool create_swapchain(DisplayWindow& window, const qhenki::SwapchainDesc& swapchain_desc, qhenki::Swapchain& swapchain, qhenki::Queue
	                      & direct_queue) override;
	bool resize_swapchain(qhenki::Swapchain& swapchain, int width, int height) override;
	bool present(qhenki::Swapchain& swapchain) override;

	// thread safe
	bool create_shader(qhenki::Shader& shader, const std::wstring& path, qhenki::ShaderType type,
	                   std::vector<D3D_SHADER_MACRO> macros) override;
	// thread safe
	bool create_pipeline(const qhenki::GraphicsPipelineDesc& desc, qhenki::GraphicsPipeline& pipeline, qhenki::Shader& vertex_shader, qhenki::Shader& pixel_shader, wchar_t
	                     const* debug_name) override;
	bool bind_pipeline(qhenki::CommandList& cmd_list, qhenki::GraphicsPipeline& pipeline) override;

	bool create_buffer(const qhenki::BufferDesc& desc, const void* data, qhenki::Buffer& buffer, wchar_t const* debug_name = nullptr) override;

	void* map_buffer(const qhenki::Buffer& buffer) override;
	void unmap_buffer(const qhenki::Buffer& buffer) override;

	void bind_vertex_buffers(qhenki::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count, const qhenki::Buffer* buffers, const
	                        unsigned* offsets) override;
	void bind_index_buffer(qhenki::CommandList& cmd_list, const qhenki::Buffer& buffer, DXGI_FORMAT format,
		unsigned offset) override;

	bool create_queue(const qhenki::QueueType type, qhenki::Queue& queue) override;
	bool create_command_pool(qhenki::CommandPool& command_pool, const qhenki::Queue& queue) override;

	// Recording commands is not thread safe in D3D11. TODO: runtime check that this is called from the same thread

	void start_render_pass(qhenki::CommandList& cmd_list, qhenki::Swapchain& swapchain,
		const qhenki::RenderTarget* depth_stencil) override;
	void start_render_pass(qhenki::CommandList& cmd_list, unsigned rt_count,
	                       const qhenki::RenderTarget* rts, const qhenki::RenderTarget* depth_stencil) override;

	void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) override;

	void draw(qhenki::CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
	void draw_indexed(qhenki::CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
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