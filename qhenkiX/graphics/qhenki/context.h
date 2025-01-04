#pragma once
#include "shader.h"
#include "swapchain.h"
#include <d3dcommon.h>
#include <graphics/displaywindow.h>

#include "buffer.h"
#include "command_list.h"
#include "pipeline.h"
#include "render_target.h"

namespace qhenki
{
	class Context
	{
	public:
		virtual void create() = 0;
		//virtual void destroy() = 0;

		// Creates swapchain based off specified description
		virtual bool create_swapchain(DisplayWindow& window, const SwapchainDesc& swapchain_desc, Swapchain& swapchain) = 0;
		virtual bool resize_swapchain(Swapchain& swapchain, int width, int height) = 0;
		virtual bool present(Swapchain& swapchain) = 0;

		virtual bool create_shader(Shader& shader, const std::wstring& path, ShaderType type, std::vector<D3D_SHADER_MACRO> macros) = 0;
		virtual bool create_pipeline(const GraphicsPipelineDesc& desc, GraphicsPipeline& pipeline, Shader& vertex_shader, Shader& pixel_shader) = 0;
		virtual bool bind_pipeline(CommandList& cmd_list, GraphicsPipeline& pipeline) = 0;

		virtual bool create_buffer(const BufferDesc& desc, const void* data, Buffer& buffer) = 0;

		virtual void bind_vertex_buffers(CommandList& cmd_list, unsigned start_slot, unsigned buffer_count, const Buffer* buffers, const unsigned* offsets) = 0;

		// TODO: compute pipeline

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
