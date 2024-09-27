#pragma once

#include "graphics/d3d11context.h"
#include "graphics/displaywindow.h"

/**
 * Encapsulates the objects that control DisplayWindow, Audio, Input, File System, Preferences.
 * Sets up default states for the objects, but initialization can be overridden.
 */
class Application
{
	DisplayWindow window;
	
	//Audio
	//Input
	//Files 
	//Preferences

protected:
	D3D11Context d3d11;
	virtual void init_display_window();

	virtual void create();
	virtual void render();
	virtual void resize(int width, int height);
	virtual void destroy();

public:
	virtual ~Application() = default;
	void run();
};

