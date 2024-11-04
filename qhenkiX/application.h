#pragma once

#include "graphics/displaywindow.h"
#include "graphics/d3d/d3d11_context.h"

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

	DisplayWindow window_;
	D3D11Context d3d11_;

	virtual void init_display_window();

	virtual void create() {}
	virtual void render() {}
	virtual void resize(int width, int height) {}
	virtual void destroy() {}

public:
	virtual ~Application() = default;
	void run();
};

