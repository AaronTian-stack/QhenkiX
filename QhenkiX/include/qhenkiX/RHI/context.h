#pragma once
#include <stdexcept>

#include "shader.h"
#include "swapchain.h"
#include <qhenkiX/display_window.h>

#include "barrier.h"
#include "buffer.h"
#include "command_list.h"
#include "command_pool.h"
#include "pipeline.h"
#include "queue.h"
#include "render_target.h"
#include "shader_compiler.h"
#include "descriptor_heap.h"
#include "descriptor_table.h"
#include "sampler.h"
#include "submission.h"
#include "texture.h"

namespace qhenki::gfx
{
	// TODO: replace all D3D types with qhenki::gfx types
	class Context
	{
		virtual bool is_debug_layer_enabled() const = 0;

	protected:
		uPtr<ShaderCompiler> m_shader_compiler;

	public:
		virtual void create(bool enable_debug_layer) = 0; // TODO: return error string for potential dialog box
		virtual bool is_compatibility() const = 0;

		// Creates swapchain based off specified description
		virtual bool create_swapchain(const DisplayWindow& window, const SwapchainDesc& swapchain_desc, Swapchain* swapchain,
		                              Queue* direct_queue, unsigned* frame_index) = 0;
		virtual bool resize_swapchain(Swapchain* swapchain, int width, int height, DescriptorHeap* rtv_heap, unsigned& frame_index) = 0;
		virtual bool create_swapchain_descriptors(const Swapchain& swapchain, DescriptorHeap* rtv_heap) = 0;
		virtual bool present(Swapchain* swapchain, UINT fence_count, Fence* wait_fences, UINT swapchain_index) = 0;

        virtual bool create_shader_dynamic(ShaderCompiler* compiler, Shader* shader, const CompilerInput& input) = 0;
		virtual bool create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline* pipeline, const Shader& vertex_shader, const Shader& pixel_shader,
		                             PipelineLayout* in_layout, const char* debug_name = nullptr) = 0;
		virtual bool bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline) = 0;

		virtual bool create_pipeline_layout(PipelineLayoutDesc* desc, PipelineLayout* layout) = 0;
		virtual void bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout) = 0;

		virtual bool set_pipeline_constant(CommandList* cmd_list, UINT param, UINT32 offset, UINT size, void* data) = 0;

		virtual bool create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap* heap, const char* debug_name = nullptr) = 0;
		virtual void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap) = 0;
		virtual void set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap, const DescriptorHeap& sampler_heap) = 0;

		virtual void set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor) = 0;
		virtual bool copy_descriptors(unsigned count, const Descriptor& src, const Descriptor& dst) = 0;
		virtual bool get_descriptor(unsigned descriptor_count_offset, DescriptorHeap* heap, Descriptor* descriptor) = 0;
		virtual bool free_descriptor(Descriptor* descriptor) = 0;

		virtual bool create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, const char* debug_name = nullptr) = 0;
		virtual bool create_descriptor_constant_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor) = 0;
		virtual bool create_descriptor_shader_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor) = 0;

		virtual void copy_buffer(CommandList* cmd_list, const Buffer& src, UINT64 src_offset, Buffer* dst, UINT64 dst_offset, UINT64 bytes) = 0;

		virtual bool create_texture(const TextureDesc& desc, Texture* texture, const char* debug_name = nullptr) = 0;
		// TODO: add description
		virtual bool create_descriptor_shader_view(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor) = 0;
		// virtual bool create_descriptor_render_target(const Texture& texture, DescriptorHeap& heap, Descriptor* descriptor) = 0;
		virtual bool create_descriptor_depth_stencil(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor) = 0;

        /**
		 * @brief Creates staging buffer with data pointer and copies it to the texture.
         * 
         * @param cmd_list Pointer to the command list used to record the copy operation.
         * @param data Pointer to the data to be copied.
		 * @param staging Pointer to the uninitialized staging buffer.
         * @param texture Pointer to the destination texture where the data will be copied.
         * @return true if the copy operation was successful, false otherwise.
         */
        virtual bool copy_to_texture(CommandList* cmd_list, const void* data, Buffer* staging, Texture* texture) = 0;

		virtual bool create_sampler(const SamplerDesc& desc, Sampler* sampler) = 0;
		virtual bool create_descriptor(const Sampler& sampler, DescriptorHeap* heap, Descriptor* descriptor) = 0;

		// Write only
		virtual void* map_buffer(const Buffer& buffer) = 0;
		virtual void unmap_buffer(const Buffer& buffer) = 0;

		virtual void bind_vertex_buffers(CommandList* cmd_list, unsigned start_slot, unsigned buffer_count, const Buffer* const* buffers, const unsigned* sizes, const unsigned* strides, const unsigned* offsets) = 0;
		virtual void bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format, unsigned offset) = 0;
		// TODO: bind compute pipeline

		virtual bool create_queue(QueueType type, Queue* queue) = 0;
		virtual bool create_command_pool(CommandPool* command_pool, const Queue& queue) = 0;
		// Begins in OPEN state
		virtual bool create_command_list(CommandList* cmd_list, const CommandPool& command_pool, const char* debug_name = nullptr) = 0;

		virtual bool close_command_list(CommandList* cmd_list) = 0;

		virtual bool reset_command_pool(CommandPool* command_pool) = 0;

		virtual void start_render_pass(CommandList* cmd_list, Swapchain* swapchain, const float* clear_color_values, const RenderTarget* depth_stencil, UINT frame_index) = 0;
		virtual void start_render_pass(CommandList* cmd_list, unsigned int rt_count, const RenderTarget* const* rts, const RenderTarget* depth_stencil) = 0;

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

		virtual void init_imgui(const DisplayWindow& window, const Swapchain& swapchain) = 0;
		virtual void start_imgui_frame() = 0;
		virtual void render_imgui_draw_data(CommandList* cmd_list) = 0;
		virtual void destroy_imgui() = 0;

		virtual void compatibility_set_constant_buffers(unsigned slot, unsigned count, Buffer* const* buffers, PipelineStage stage) = 0;
		virtual void compatibility_set_shader_buffers(unsigned slot, unsigned count, Descriptor* const* descriptors, PipelineStage stage) = 0;
		virtual void compatibility_set_uav_buffers(unsigned slot, unsigned count, Buffer* const* buffers) = 0;
		virtual void compatibility_set_textures(unsigned slot, unsigned count, Descriptor* const* descriptors, AccessFlags flag, PipelineStage stage) = 0;
		virtual void compatibility_set_samplers(unsigned slot, unsigned count, Sampler* const* samplers, PipelineStage stage) = 0;

		// Wait for device to idle, should only be used on program exit
		virtual void wait_idle(Queue* queue) = 0;
		virtual ~Context() = default;
	};
}

#define THROW_IF_FALSE(result) \
do { \
	if (!result) \
	{ \
		throw std::runtime_error("Something went wrong!\n"); \
	} \
} while (0)

#define THROW_IF_TRUE(result) \
	do { \
if ((result)) \
{ \
   throw std::runtime_error("Something went wrong!\n"); \
} \
} while (0)
