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
		// Create pipeline based off specified description assumed to be already filled
		virtual bool create_pipeline(Pipeline& pipeline, Shader& shader) = 0;

		// Wait for device to idle, should only be used on program exit
		virtual void wait_all() = 0;
		virtual ~Context() = default;
	};
}
