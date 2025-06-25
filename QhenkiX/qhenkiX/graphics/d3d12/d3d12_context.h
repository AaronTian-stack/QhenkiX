#pragma once
#include <d3d12shader.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <dxgidebug.h>
#include <boost/pool/object_pool.hpp>

#include <D3D12MemAlloc.h>
#include "d3d12_descriptor_heap.h"
#include "d3d12_pipeline.h"
#include "d3d12_root_hasher.h"
#include "../d3d11/d3d11_shader_compiler.h"
#include "qhenkiX/RHI/context.h"
#include "qhenkiX/RHI/descriptor_table.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
	class D3D12Context : public Context
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS12 m_options12 = {}; // Enhanced barriers
		D3D12_FEATURE_DATA_SHADER_MODEL m_shader_model = {};

		D3D12_FEATURE_DATA_D3D12_OPTIONS m_options = {};

		ComPtr<IDXGIFactory6> m_dxgi_factory = nullptr;
#ifdef _DEBUG
		ComPtr<ID3D12Debug3> m_debug;
		ComPtr<IDXGIDebug1> m_dxgi_debug;
#endif
		ComPtr<ID3D12Device> m_device;
		ComPtr<D3D12MA::Allocator> m_allocator;

		ComPtr<IDXGISwapChain3> m_swapchain;
		std::array<ComPtr<ID3D12Resource>, 2> m_swapchain_buffers; // 2 is upper limit
		std::array<Descriptor, 2> m_swapchain_descriptors{}; // 2 is upper limit

		D3D12DescriptorHeap m_imgui_heap{}; // ImGUI only
		std::array<Descriptor, 2> m_imgui_descriptors{}; // ImGUI only

		Queue* m_swapchain_queue = nullptr;

		std::mutex m_pipeline_desc_mutex;
		boost::object_pool<D3D12_GRAPHICS_PIPELINE_STATE_DESC> m_pipeline_desc_pool;

		D3D11ShaderCompiler m_d3d11_shader_compiler; // Needed for SM < 6.0

		D3D12RootHasher m_root_reflection;

		Fence m_fence_wait_all{}; // For stalling queues

		std::vector<D3D12_INPUT_ELEMENT_DESC> shader_reflection(ID3D12ShaderReflection* shader_reflection, const D3D12_SHADER_DESC& shader_desc, bool increment_slot) const;
		void root_signature_reflection(ID3D12ShaderReflection* shader_reflection, const D3D12_SHADER_DESC& shader_desc);

		UINT GetMaxDescriptorsForHeapType(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type) const;

	public:
		void create() override;
		bool is_compatibility() override { return false; }
		bool create_swapchain(const DisplayWindow& window, const SwapchainDesc& swapchain_desc, Swapchain& swapchain,
		                      Queue& direct_queue, unsigned& frame_index) override;
		bool resize_swapchain(Swapchain& swapchain, int width, int height, DescriptorHeap& rtv_heap, unsigned& frame_index) override;
		bool create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap& rtv_heap) override;
		bool present(Swapchain& swapchain, unsigned fence_count, Fence* wait_fences, unsigned swapchain_index) override;

		uPtr<ShaderCompiler> create_shader_compiler() override;
		bool create_shader_dynamic(ShaderCompiler* compiler, Shader* shader, const CompilerInput& input) override;

		bool create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline* pipeline,
		                     const Shader& vertex_shader, const Shader& pixel_shader,
		                     PipelineLayout* in_layout, PipelineLayout* out_layout, wchar_t const* debug_name) override;

		bool bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline) override;

		bool create_pipeline_layout(PipelineLayoutDesc& desc, PipelineLayout* layout) override;
		void bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout) override;

		bool set_pipeline_constant(CommandList* cmd_list, UINT param, UINT32 offset, UINT size, void* data) override;

		bool create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap& heap, wchar_t const* debug_name = nullptr) override;
		void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap) override;
		void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap, const DescriptorHeap& sampler_heap) override;

		void set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor) override;
		bool copy_descriptors(unsigned count, const Descriptor& src, const Descriptor& dst) override;
		bool get_descriptor(unsigned descriptor_count_offset, DescriptorHeap& heap, Descriptor* descriptor) override;
		bool free_descriptor(Descriptor* descriptor) override;

		bool create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, wchar_t const* debug_name) override;
		bool create_descriptor(const Buffer& buffer, DescriptorHeap& heap, Descriptor* descriptor, BufferDescriptorType type) override;

		void copy_buffer(CommandList* cmd_list, const Buffer& src, UINT64 src_offset, Buffer* dst, UINT64 dst_offset, UINT64 bytes) override;

		bool create_texture(const TextureDesc& desc, Texture* texture, wchar_t const* debug_name) override;
		bool create_descriptor_texture_view(const Texture& texture, DescriptorHeap& heap, Descriptor* descriptor) override;
		bool create_descriptor_depth_stencil(const Texture& texture, DescriptorHeap& heap, Descriptor* descriptor) override;

		bool copy_to_texture(CommandList* cmd_list, const void* data, Buffer* staging, Texture* texture) override;

		bool create_sampler(const SamplerDesc& desc, Sampler* sampler) override;
		bool create_descriptor(const Sampler& sampler, DescriptorHeap& heap, Descriptor* descriptor) override;

		void* map_buffer(const Buffer& buffer) override;
		void unmap_buffer(const Buffer& buffer) override;

		void bind_vertex_buffers(CommandList* cmd_list, unsigned start_slot, unsigned buffer_count,
		                         const Buffer* const* buffers, const unsigned* strides, const unsigned* offsets) override;

		void bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format,
		                       unsigned offset) override;

		bool create_queue(QueueType type, Queue* queue) override;
		bool create_command_pool(CommandPool* command_pool, const Queue& queue) override;
		bool create_command_list(CommandList* cmd_list, const CommandPool& command_pool) override;

		bool close_command_list(CommandList* cmd_list) override;

		bool reset_command_pool(CommandPool* command_pool) override;

		void start_render_pass(CommandList* cmd_list, Swapchain& swapchain, const float* clear_color_values, const RenderTarget* depth_stencil, UINT frame_index) override;

		void start_render_pass(CommandList* cmd_list, unsigned rt_count, const RenderTarget* const* rts, const RenderTarget* depth_stencil) override;

		void set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport) override;
		void set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect) override;

		void draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) override;
		void draw_indexed(CommandList* cmd_list, uint32_t index_count, uint32_t start_index_offset, int32_t base_vertex_offset) override;

		void submit_command_lists(const SubmitInfo& submit_info, Queue* queue) override;

		bool create_fence(Fence* fence, uint64_t initial_value) override;
		uint64_t get_fence_value(const Fence& fence) override;
		bool wait_fences(const WaitInfo& info) override;

		void set_barrier_resource(unsigned count, ImageBarrier* barriers, const Swapchain& swapchain, unsigned frame_index) override;
		void set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target) override;

		void issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers) override;

		void init_imgui(const DisplayWindow& window, const Swapchain& swapchain) override;
		void start_imgui_frame() override;
		void render_imgui_draw_data(CommandList* cmd_list) override;
		void destroy_imgui() override;

		// D3D12 does not implement compability functions
		void compatibility_set_constant_buffers(unsigned slot, unsigned count, Buffer* const buffers, PipelineStage stage) override {}
		void compatibility_set_textures(unsigned slot, unsigned count, Descriptor* const descriptors, AccessFlags flag, PipelineStage stage) override {}
		void compatibility_set_samplers(unsigned slot, unsigned count, Sampler* const samplers, PipelineStage stage) override {}

		void wait_idle(Queue& queue) override;

		~D3D12Context() override;
	};
}
