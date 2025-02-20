#pragma once
#include "shader.h"
#include "swapchain.h"
#include <graphics/displaywindow.h>

#include "buffer.h"
#include "command_list.h"
#include "command_pool.h"
#include "pipeline.h"
#include "queue.h"
#include "render_target.h"
#include "shader_compiler.h"

namespace qhenki::gfx
{
	// TODO: replace all D3D types with qhenki::gfx types
	class Context
	{
	protected:
		uPtr<ShaderCompiler> shader_compiler;

	public:
		virtual void create() = 0;

		// Creates swapchain based off specified description
		virtual bool create_swapchain(DisplayWindow& window, const SwapchainDesc& swapchain_desc, Swapchain& swapchain,
		                              Queue& direct_queue, unsigned buffer_count, unsigned& frame_index) = 0;
		virtual bool resize_swapchain(Swapchain& swapchain, int width, int height) = 0;
		virtual bool present(Swapchain& swapchain) = 0;

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
        virtual bool create_shader_dynamic(ShaderCompiler* compiler, Shader& shader, const CompilerInput& input) = 0;
		virtual bool create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline& pipeline, Shader& vertex_shader, Shader& pixel_shader, PipelineLayout* in_layout, PipelineLayout* out_layout, wchar_t const* debug_name = nullptr) = 0;
		virtual bool bind_pipeline(CommandList& cmd_list, GraphicsPipeline& pipeline) = 0;

		virtual bool create_buffer(const BufferDesc& desc, const void* data, Buffer& buffer, wchar_t const* debug_name = nullptr) = 0;

        /**
        * Write only. Do not read or performance issues may happen.
		* For persistent mapped buffers (pinned memory only), unmap should do nothing.
		**/
		virtual void* map_buffer(const Buffer& buffer) = 0;
		virtual void unmap_buffer(const Buffer& buffer) = 0;

		virtual void bind_vertex_buffers(CommandList& cmd_list, unsigned start_slot, unsigned buffer_count, const Buffer* buffers, const unsigned* offsets) = 0;
		virtual void bind_index_buffer(CommandList& cmd_list, const Buffer& buffer, DXGI_FORMAT format, unsigned offset) = 0;
		// TODO: bind compute pipeline

		virtual bool create_queue(QueueType type, Queue& queue) = 0;
		virtual bool create_command_pool(CommandPool& command_pool, const Queue& queue) = 0;
		// Begins in recording state
		virtual bool create_command_list(CommandList& cmd_list, const CommandPool& command_pool) = 0;

		// TODO: optional clear
		virtual void start_render_pass(CommandList& cmd_list, Swapchain& swapchain, const RenderTarget* depth_stencil) = 0;
		virtual void start_render_pass(CommandList& cmd_list, unsigned int rt_count, const RenderTarget* rts, const RenderTarget* depth_stencil) = 0;

		virtual void set_viewports(unsigned count, const D3D12_VIEWPORT* viewport) = 0;

		virtual void draw(CommandList& cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset) = 0;
		virtual void draw_indexed(CommandList& cmd_list, uint32_t index_count, uint32_t start_index_offset, int32_t base_vertex_offset) = 0;
		// TODO: draw indirect
		// TODO: draw indirect count

		// Wait for device to idle, should only be used on program exit
		virtual void wait_all() = 0;
		virtual ~Context() = default;
	};
}
