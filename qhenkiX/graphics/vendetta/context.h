#pragma once
#include "shader.h"
#include "swapchain.h"
#include <d3dcommon.h>
#include <graphics/displaywindow.h>

namespace vendetta
{
	class Context
	{
		virtual void create(DisplayWindow& window) = 0;
		virtual void destroy() = 0;

		virtual bool create_swapchain(Swapchain* swapchain) = 0;
		virtual bool resize_swapchain(Swapchain* swapchain, int width, int height) = 0;

		virtual bool create_shader(Shader* shader, const char* path, ShaderType type, std::vector<D3D_SHADER_MACRO> macros) = 0;

		~Context() = default;
	};
}
