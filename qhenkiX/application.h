#pragma once

#include "graphics/displaywindow.h"
#include "graphics/d3d11/d3d11_context.h"

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

	enum GraphicsAPI
	{
		D3D11,
		Vulkan,
		OpenGL
	};

	// TODO: Default shaders. Should be initialized in some function
	GraphicsAPI graphics_api_ = D3D11;

	DisplayWindow window_;
	uPtr<vendetta::Context> context_ = nullptr;
	vendetta::Swapchain swapchain_{};

	virtual void init_display_window();

	virtual void create() {}
	virtual void render() {}
	virtual void resize(int width, int height) {}
	virtual void destroy() {}

public:
	void run();
};

