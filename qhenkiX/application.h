#pragma once

#include "graphics/displaywindow.h"
#include "graphics/d3d11/d3d11_context.h"

namespace vendetta
{
	enum GraphicsAPI
	{
		D3D11,
		D3D12,
		Vulkan,
	};
}

/**
 * Encapsulates the objects that control DisplayWindow, Audio, Input, File System, Preferences.
 * Sets up default states for the objects, but initialization can be overridden.
 */
class Application
{
	//Audio
	//Input
	//Files 
	//Preferences

protected:
	// TODO: Default shaders. Should be initialized in some function
	vendetta::GraphicsAPI graphics_api_ = vendetta::D3D11;

	DisplayWindow window_;
	uPtr<vendetta::Context> context_ = nullptr;
	vendetta::Swapchain swapchain_{};

	virtual void init_display_window();

	virtual void create() {}
	virtual void render() {}
	virtual void resize(int width, int height) {}
	virtual void destroy() {}

public:
	void run(vendetta::GraphicsAPI api);
};

