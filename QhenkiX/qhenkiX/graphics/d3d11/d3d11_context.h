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

namespace qhenki::gfx
{
	// SM 5.1
	class D3D11Context : public Context
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
		bool is_compatability() override { return true; }
		bool create_swapchain(DisplayWindow& window, const SwapchainDesc& swapchain_desc, Swapchain& swapchain, 
			Queue& direct_queue, unsigned& frame_index) override;
		bool resize_swapchain(Swapchain& swapchain, int width, int height) override;
		bool create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap& rtv_heap, DescriptorTable& table) override;
		bool present(Swapchain& swapchain) override;

		uPtr<ShaderCompiler> create_shader_compiler() override;
		// thread safe
		bool create_shader_dynamic(ShaderCompiler* compiler, Shader& shader, const CompilerInput& input) override;
		// thread safe
		bool create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline& pipeline,
			Shader& vertex_shader, Shader& pixel_shader,
			PipelineLayout* in_layout, PipelineLayout* out_layout, wchar_t const* debug_name) override;
		bool bind_pipeline(CommandList& cmd_list, GraphicsPipeline& pipeline) override;

		bool create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap& heap) override;

		bool create_buffer(const BufferDesc& desc, const void* data, Buffer& buffer, wchar_t const* debug_name = nullptr) override;

		void* map_buffer(const Buffer& buffer) override;
		void unmap_buffer(const Buffer& buffer) override;

		void bind_vertex_buffers(CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
			const Buffer* buffers, const UINT* strides, const unsigned* offsets) override;
		void bind_index_buffer(CommandList& cmd_list, const Buffer& buffer, DXGI_FORMAT format,
			unsigned offset) override;

		bool create_queue(QueueType type, Queue& queue) override;
		bool create_command_pool(CommandPool& command_pool, const Queue& queue) override;
		bool create_command_list(CommandList& cmd_list, const CommandPool& command_pool) override;

		// Recording commands is not thread safe in D3D11. TODO: runtime check that this is called from the same thread

		void start_render_pass(CommandList& cmd_list, Swapchain& swapchain,
			const RenderTarget* depth_stencil) override;
		void start_render_pass(CommandList& cmd_list, unsigned rt_count,
			const RenderTarget* rts, const RenderTarget* depth_stencil) override;

		void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) override;

		void draw(CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
		void draw_indexed(CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset,
			int32_t base_vertex_offset) override;

		void wait_all() override;
		~D3D11Context() override;

		friend struct D3D11GraphicsPipeline;
	};

	template <UINT TDebugNameLength>
	void D3D11Context::set_debug_name(ID3D11DeviceChild* device_resource, const char(&debug_name)[TDebugNameLength])
	{
		device_resource->SetPrivateData(WKPDID_D3DDebugObjectName, TDebugNameLength - 1, debug_name);
	}
}