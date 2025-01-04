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
	ComPtr<IDXGIFactory6> dxgi_factory;
#ifdef _DEBUG
	ComPtr<ID3D11Debug> debug_;
#endif
	ComPtr<ID3D11Device> device_;
	ComPtr<ID3D11DeviceContext> device_context_;

	D3D11LayoutAssembler layout_assembler_;

	std::array<D3D11_VIEWPORT, 16> viewports;

	std::mutex pipeline_mutex;
	boost::object_pool<qhenki::GraphicsPipelineDesc> pool;

public:
	// Will not work with things that don't derive from ID3D11DeviceChild
	template<UINT TDebugNameLength>
	static void set_debug_name(_In_ ID3D11DeviceChild* device_resource, _In_z_ const char(&debug_name)[TDebugNameLength]);

	void create() override;
	bool create_swapchain(DisplayWindow& window, const qhenki::SwapchainDesc& swapchain_desc, qhenki::Swapchain& swapchain) override;
	bool resize_swapchain(qhenki::Swapchain& swapchain, int width, int height) override;
	bool present(qhenki::Swapchain& swapchain) override;

	// thread safe
	bool create_shader(qhenki::Shader& shader, const std::wstring& path, qhenki::ShaderType type,
	                   std::vector<D3D_SHADER_MACRO> macros) override;
	// thread safe
	bool create_pipeline(const qhenki::GraphicsPipelineDesc& desc, qhenki::GraphicsPipeline& pipeline, qhenki::Shader& vertex_shader, qhenki::Shader& pixel_shader) override;
	bool bind_pipeline(qhenki::CommandList& cmd_list, qhenki::GraphicsPipeline& pipeline) override;

	bool create_buffer(const qhenki::BufferDesc& desc, const void* data, qhenki::Buffer& buffer, wchar_t const* debug_name = nullptr) override;

	void bind_vertex_buffers(qhenki::CommandList& cmd_list, unsigned start_slot, unsigned buffer_count, const qhenki::Buffer* buffers, const
	                        unsigned* offsets) override;

	// recording commands is not thread safe

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