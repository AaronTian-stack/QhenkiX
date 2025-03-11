#pragma once
#include <d3d12shader.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <boost/pool/object_pool.hpp>

#include <D3D12MemAlloc.h>
#include "d3d12_descriptor_heap.h"
#include "d3d12_pipeline.h"
#include "d3d12_root_hasher.h"
#include "graphics/d3d11/d3d11_shader_compiler.h"
#include "graphics/qhenki/context.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	class D3D12Context : public Context
	{
		D3D12_FEATURE_DATA_SHADER_MODEL m_shader_model_ = {};

		D3D12_FEATURE_DATA_D3D12_OPTIONS m_options_ = {};

		ComPtr<IDXGIFactory6> m_dxgi_factory_ = nullptr;
#ifdef _DEBUG
		ComPtr<ID3D12Debug3> m_debug_;
		ComPtr<IDXGIDebug1> m_dxgi_debug_;
#endif
		ComPtr<ID3D12Device> m_device_;
		ComPtr<D3D12MA::Allocator> m_allocator_;

		ComPtr<IDXGISwapChain3> m_swapchain_;
		std::array<ComPtr<ID3D12Resource>, 2> m_swapchain_buffers_;

		std::mutex m_pipeline_desc_mutex_;
		boost::object_pool<D3D12_GRAPHICS_PIPELINE_STATE_DESC> m_pipeline_desc_pool_;

		ComPtr<IDxcLibrary> m_library_;
		ComPtr<IDxcCompiler> m_compiler_;

		D3D11ShaderCompiler m_d3d11_shader_compiler_; // Needed for SM < 6.0

		D3D12RootHasher m_root_reflection_;

		std::vector<D3D12_INPUT_ELEMENT_DESC> shader_reflection(ID3D12ShaderReflection* shader_reflection, const D3D12_SHADER_DESC& shader_desc, bool interleaved) const;
		void root_signature_reflection(ID3D12ShaderReflection* shader_reflection, const D3D12_SHADER_DESC& shader_desc);

		UINT GetMaxDescriptorsForHeapType(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type) const;

	public:
		void create() override;
		bool is_compatability() override { return false; }
		bool create_swapchain(DisplayWindow& window, const SwapchainDesc& swapchain_desc, Swapchain& swapchain, 
			Queue& direct_queue, unsigned& frame_index) override;
		bool resize_swapchain(Swapchain& swapchain, int width, int height) override;
		bool create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap& rtv_heap, DescriptorTable& table) override;
		bool present(Swapchain& swapchain) override;

		uPtr<ShaderCompiler> create_shader_compiler() override;
		bool create_shader_dynamic(ShaderCompiler* compiler, Shader& shader, const CompilerInput& input) override;

		bool create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline& pipeline,
			Shader& vertex_shader, Shader& pixel_shader,
			PipelineLayout* in_layout, PipelineLayout* out_layout, wchar_t const* debug_name) override;

		bool bind_pipeline(CommandList& cmd_list, GraphicsPipeline& pipeline) override;

		bool create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap& heap) override;

		bool create_buffer(const BufferDesc& desc, const void* data, Buffer& buffer, wchar_t const* debug_name) override;

		void* map_buffer(const Buffer& buffer) override;
		void unmap_buffer(const Buffer& buffer) override;

		void bind_vertex_buffers(CommandList& cmd_list, unsigned start_slot, unsigned buffer_count,
			const Buffer* buffers, const UINT* strides, const unsigned* offsets) override;

		void bind_index_buffer(CommandList& cmd_list, const Buffer& buffer, DXGI_FORMAT format,
			unsigned offset) override;

		bool create_queue(QueueType type, Queue& queue) override;
		bool create_command_pool(CommandPool& command_pool, const Queue& queue) override;
		bool create_command_list(CommandList& cmd_list, const CommandPool& command_pool) override;

		void start_render_pass(CommandList& cmd_list, Swapchain& swapchain,
			const RenderTarget* depth_stencil) override;

		void start_render_pass(CommandList& cmd_list, unsigned rt_count, const RenderTarget* rts, const RenderTarget* depth_stencil) override;

		void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) override;

		void draw(CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
		void draw_indexed(CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset, int32_t base_vertex_offset) override;

		void wait_all() override;

		~D3D12Context() override;
	};
}
