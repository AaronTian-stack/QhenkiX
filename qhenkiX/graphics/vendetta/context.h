#pragma once
#include "shader.h"
#include "swapchain.h"
#include <d3dcommon.h>
#include <graphics/displaywindow.h>

#include "pipeline.h"

namespace vendetta
{
	class Context
	{
	public:
		virtual void create() = 0;
		//virtual void destroy() = 0;

		// Creates swapchain based off specified description assumed to be already filled
		virtual bool create_swapchain(DisplayWindow& window, Swapchain& swapchain) = 0;
		virtual bool resize_swapchain(Swapchain& swapchain, int width, int height) = 0;
		virtual bool present(Swapchain& swapchain) = 0;

		virtual bool create_shader(Shader& shader, const std::wstring& path, ShaderType type, std::vector<D3D_SHADER_MACRO> macros) = 0;
		// Create graphics pipeline based off specified description assumed to be already filled
		virtual bool create_pipeline(GraphicsPipeline& pipeline, Shader& vertex_shader, vendetta::Shader& pixel_shader) = 0;
		// virtual bool bind_pipeline(GraphicsPipeline& pipeline) = 0;

		// TODO: compute pipeline

		// virtual bool set_viewport(const Viewport& viewport) = 0;

		// Wait for device to idle, should only be used on program exit
		virtual void wait_all() = 0;
		virtual ~Context() = default;
	};
}
