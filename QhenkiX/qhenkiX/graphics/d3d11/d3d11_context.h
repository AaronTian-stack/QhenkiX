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

		std::mutex m_context_mutex_; // For anything that uses the device context. Do not call Context methods from each other to prevent deadlock

	public:
		// Will not work with things that don't derive from ID3D11DeviceChild
		template<UINT TDebugNameLength>
		static void set_debug_name(_In_ ID3D11DeviceChild* device_resource, _In_z_ const char(&debug_name)[TDebugNameLength]);

		void create() override;
		bool is_compatibility() override { return true; }
		bool create_swapchain(const DisplayWindow& window, const SwapchainDesc& swapchain_desc, Swapchain& swapchain,
		                      Queue& direct_queue, unsigned& frame_index) override;
		bool resize_swapchain(Swapchain& swapchain, int width, int height, DescriptorHeap& rtv_heap, unsigned& frame_index) override;
		bool create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap& rtv_heap) override;
		bool present(Swapchain& swapchain, unsigned fence_count, Fence* wait_fences, unsigned swapchain_index) override;

		uPtr<ShaderCompiler> create_shader_compiler() override;
		// thread safe
		bool create_shader_dynamic(ShaderCompiler* compiler, Shader* shader, const CompilerInput& input) override;
		// thread safe
		bool create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline* pipeline,
		                     const Shader& vertex_shader, const Shader& pixel_shader,
		                     PipelineLayout* in_layout, PipelineLayout* out_layout, wchar_t const* debug_name) override;
		bool bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline) override;

		// D3D11 does not have root signatures
		bool create_pipeline_layout(PipelineLayoutDesc& desc, PipelineLayout* layout) override { return true; }
		void bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout) override {}

		bool set_pipeline_constant(CommandList* cmd_list, UINT param, UINT32 offset, UINT size, void* data) override { return !is_compatibility(); }

		bool create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap& heap) override;
		// Heaps only store views in D3D11
		void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap) override {}
		void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap, const DescriptorHeap& sampler_heap) override {}

		void set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor) override {}
		bool copy_descriptors(unsigned count, const Descriptor& src, const Descriptor& dst) override { return true; }
		bool get_descriptor(unsigned descriptor_count_offset, DescriptorHeap& heap, Descriptor* descriptor) override { return true; }

		bool create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, wchar_t const* debug_name = nullptr) override;
		bool create_descriptor(const Buffer& buffer, DescriptorHeap& cpu_heap, Descriptor* descriptor, BufferDescriptorType type) override { return true; }

		void copy_buffer(CommandList* cmd_list, const Buffer& src, UINT64 src_offset, Buffer* dst, UINT64 dst_offset, UINT64 bytes) override;

		bool create_texture(const TextureDesc& desc, Texture* texture, wchar_t const* debug_name = nullptr) override;
		bool create_descriptor_texture_view(const Texture& texture, DescriptorHeap& heap, Descriptor* descriptor) override;
		bool create_descriptor_depth_stencil(const Texture& texture, DescriptorHeap& heap, Descriptor* descriptor) override;

		bool copy_to_texture(CommandList& cmd_list, const void* data, Buffer& staging, Texture& texture) override;

		bool create_sampler(const SamplerDesc& desc, Sampler* sampler) override;
		bool create_descriptor(const Sampler& sampler, DescriptorHeap& heap, Descriptor* descriptor) override { return true; }

		void* map_buffer(const Buffer& buffer) override;
		void unmap_buffer(const Buffer& buffer) override;

		void bind_vertex_buffers(CommandList* cmd_list, unsigned start_slot, unsigned buffer_count,
								 const Buffer* const* buffers, const unsigned* const strides, const unsigned* const offsets) override;
		void bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format,
		                       unsigned offset) override;

		bool create_queue(QueueType type, Queue* queue) override;
		bool create_command_pool(CommandPool* command_pool, const Queue& queue) override;
		bool create_command_list(CommandList* cmd_list, const CommandPool& command_pool) override;

		bool close_command_list(CommandList* cmd_list) override;

		bool reset_command_pool(CommandPool* command_pool) override;

		// Recording commands is NOT thread safe in D3D11

		void start_render_pass(CommandList* cmd_list, Swapchain& swapchain,
		                       const float* clear_color_values, const RenderTarget* depth_stencil, UINT frame_index) override;
		void start_render_pass(CommandList* cmd_list, unsigned rt_count,
		                       const RenderTarget* const* rts, const RenderTarget* depth_stencil) override;

		void set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport) override;
		void set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect) override;

		void draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
		void draw_indexed(CommandList* cmd_list, uint32_t index_count, uint32_t start_index_offset,
		                  int32_t base_vertex_offset) override;

		void submit_command_lists(const SubmitInfo& submit_info, Queue* queue) override {}

		bool create_fence(Fence* fence, uint64_t initial_value) override { return true; }
		uint64_t get_fence_value(const Fence& fence) override { return 0; }
		bool wait_fences(const WaitInfo& info) override { return true; }

		void set_barrier_resource(unsigned count, ImageBarrier* const* barriers, const Swapchain& swapchain, unsigned frame_index) override {}
		void set_barrier_resource(unsigned count, ImageBarrier* const* barriers, const Texture& render_target) override {}

		void issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* const* barriers) override {}

		void init_imgui(const DisplayWindow& window, const Swapchain& swapchain) override;
		void start_imgui_frame() override;
		void render_imgui_draw_data(CommandList* cmd_list) override;
		void destroy_imgui() override;

		void compatibility_set_constant_buffers(unsigned slot, unsigned count, Buffer* const* buffers, PipelineStage stage) override;
		void compatibility_set_textures(unsigned slot, unsigned count, Descriptor* const* descriptors, AccessFlags flag, PipelineStage stage) override;
		void compatibility_set_samplers(unsigned slot, unsigned count, Sampler* const* samplers, PipelineStage stage) override;

		void wait_idle(Queue& queue) override;
		~D3D11Context() override;

		friend struct D3D11GraphicsPipeline;
	};

	template <UINT TDebugNameLength>
	void D3D11Context::set_debug_name(ID3D11DeviceChild* device_resource, const char(&debug_name)[TDebugNameLength])
	{
		device_resource->SetPrivateData(WKPDID_D3DDebugObjectName, TDebugNameLength - 1, debug_name);
	}
}