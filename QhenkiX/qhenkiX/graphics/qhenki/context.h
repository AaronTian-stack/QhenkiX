#pragma once
#include <stdexcept>

#include "shader.h"
#include "swapchain.h"
#include <graphics/displaywindow.h>

#include "barrier.h"
#include "command_list.h"
#include "command_pool.h"
#include "pipeline.h"
#include "queue.h"
#include "render_target.h"
#include "shader_compiler.h"
#include "descriptor_heap.h"
#include "descriptor_table.h"
#include "submission.h"
#include "texture.h"

namespace qhenki::gfx
{
	// TODO: replace all D3D types with qhenki::gfx types
	class Context
	{
	protected:
		uPtr<ShaderCompiler> shader_compiler;

	public:
		virtual void create() = 0; // TODO: return error string for potential dialog box
		virtual bool is_compatability() = 0;

		// Creates swapchain based off specified description
		virtual bool create_swapchain(DisplayWindow& window, const SwapchainDesc& swapchain_desc, Swapchain& swapchain,
		                              Queue& direct_queue, unsigned& frame_index) = 0;
		virtual bool resize_swapchain(Swapchain& swapchain, int width, int height, DescriptorHeap& rtv_heap, unsigned& frame_index) = 0;
		virtual bool create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap& rtv_heap) = 0;
		virtual bool present(Swapchain& swapchain, unsigned fence_count, Fence* wait_fences, unsigned swapchain_index) = 0;

		virtual uPtr<ShaderCompiler> create_shader_compiler() = 0;
		// Dynamic shader compilation
        /**
         * @brief Dynamically creates a shader using the specified compiler and input.
         * 
         * This function compiles a shader at runtime using the provided ShaderCompiler instance.
         * The compiled shader is stored in the provided Shader object.
         * 
         * @param compiler Pointer to the ShaderCompiler instance used for compilation. If null default compiler with context is used (NOT thread safe).
         * @param shader Reference to the Shader object where the compiled shader will be stored.
         * @param input Reference to the CompilerInput structure containing the shader compilation parameters.
         * @return true if the shader was successfully compiled and created, false otherwise.
         */
        virtual bool create_shader_dynamic(ShaderCompiler* compiler, Shader* shader, const CompilerInput& input) = 0;
		virtual bool create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline* pipeline, const Shader& vertex_shader, const Shader& pixel_shader,
		                             PipelineLayout* in_layout, PipelineLayout* out_layout, wchar_t const* debug_name = nullptr) = 0;
		virtual bool bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline) = 0;

		virtual bool create_pipeline_layout(PipelineLayoutDesc& desc, PipelineLayout* layout) = 0;
		virtual void bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout) = 0;

		virtual bool create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap& heap) = 0;
		virtual void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap) = 0;

		virtual void set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor) = 0;
		virtual bool copy_descriptors(unsigned count, const Descriptor* src, const Descriptor* dst) = 0;
		virtual bool get_descriptor(unsigned count, DescriptorHeap& heap, Descriptor* descriptor) = 0;

		virtual bool create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, wchar_t const* debug_name = nullptr) = 0;
		virtual bool create_descriptor(const Buffer& buffer, DescriptorHeap& heap, Descriptor* descriptor) = 0;

		virtual void copy_buffer(CommandList* cmd_list, const Buffer& src, UINT64 src_offset, Buffer* dst, UINT64 dst_offset, UINT64 bytes) = 0;

		virtual bool create_texture(const TextureDesc& desc, Texture* texture, wchar_t const* debug_name = nullptr) = 0;
		virtual bool create_descriptor(const Texture& texture, DescriptorHeap& heap, Descriptor* descriptor) = 0;

		// Assumes that you are copying to first subresource only. Texture should be CPU visible TODO: add subresource index argument?
		virtual bool copy_to_texture(CommandList& cmd_list, const void* data, Buffer& staging, Texture& texture) = 0;

		// Write only
		virtual void* map_buffer(const Buffer& buffer) = 0;
		virtual void unmap_buffer(const Buffer& buffer) = 0;

		virtual void bind_vertex_buffers(CommandList* cmd_list, unsigned start_slot, unsigned buffer_count, const Buffer* buffers, const UINT* strides, const unsigned* offsets) = 0;
		virtual void bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format, unsigned offset) = 0;
		// TODO: bind compute pipeline

		virtual bool create_queue(QueueType type, Queue* queue) = 0;
		virtual bool create_command_pool(CommandPool* command_pool, const Queue& queue) = 0;
		// Begins in OPEN state
		virtual bool create_command_list(CommandList* cmd_list, const CommandPool& command_pool) = 0;

		virtual bool close_command_list(CommandList* cmd_list) = 0;

		virtual bool reset_command_pool(CommandPool* command_pool) = 0;

		// TODO: optional clear
		virtual void start_render_pass(CommandList* cmd_list, Swapchain& swapchain, const RenderTarget* depth_stencil, UINT frame_index) = 0;
		virtual void start_render_pass(CommandList* cmd_list, unsigned int rt_count, const RenderTarget* rts, const RenderTarget* depth_stencil) = 0;

		virtual void set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport) = 0;
		virtual void set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect) = 0;

		virtual void draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) = 0;
		virtual void draw_indexed(CommandList* cmd_list, uint32_t index_count, uint32_t start_index_offset, int32_t base_vertex_offset) = 0;
		// TODO: draw indirect
		// TODO: draw indirect count

		virtual void submit_command_lists(const SubmitInfo& submit_info, Queue* queue) = 0;

		virtual bool create_fence(Fence* fence, uint64_t initial_value) = 0;
		// If submission is pending value may be out of date
		virtual uint64_t get_fence_value(const Fence& fence) = 0;
		// Waits for fences on CPU
		virtual bool wait_fences(const WaitInfo& info) = 0;

		// Sets ImageBarrier resource to swapchain resource
		virtual void set_barrier_resource(unsigned count, ImageBarrier* barriers, const Swapchain& swapchain, unsigned frame_index) = 0;
		virtual void set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target) = 0;
		virtual void issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers) = 0;

		virtual void compatibility_set_constant_buffers(unsigned slot, unsigned count, Buffer* buffers, PipelineStage stage) = 0;

		// Wait for device to idle, should only be used on program exit
		virtual void wait_idle(Queue& queue) = 0;
		virtual ~Context() = default;
	};
}

#define THROW_IF_FAILED(result) \
if (!(result)) \
{ \
   throw std::runtime_error("Something went wrong!\n"); \
}

#define THROW_IF_TRUE(result) \
if ((result)) \
{ \
   throw std::runtime_error("Something went wrong!\n"); \
}