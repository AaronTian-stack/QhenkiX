#pragma once
#include "shader.h"
#include "swapchain.h"
#include <d3dcommon.h>
#include <graphics/displaywindow.h>

namespace vendetta
{
	class Context
	{
	public:
		virtual void create() = 0;
		virtual void destroy() = 0;

		// creates swapchain based off specified description assumed to be already filled
		virtual void create_swapchain(DisplayWindow& window, Swapchain& swapchain) = 0;
		virtual bool resize_swapchain(Swapchain& swapchain, int width, int height) = 0;

		virtual bool create_shader(Shader* shader, const char* path, ShaderType type, std::vector<D3D_SHADER_MACRO> macros) = 0;

		// wait for device to idle
		virtual void wait_all() = 0;
		virtual ~Context() = default;
	};
}
