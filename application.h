#pragma once
#include "d3d11context.h"
#include "displaywindow.h"

/**
 * Encapsulates the objects that control DisplayWindow, Audio, Input, File System, Preferences.
 * Sets up default states for the objects, but initialization can be overridden.
 */
class Application
{
private:
	DisplayWindow display_window;
	
	//Audio
	//Input
	//Files 
	//Preferences

protected:
	D3D11Context d3d11_context;
	virtual void init_display_window();

	virtual void render();
	virtual void resize(int width, int height);
	virtual void destroy();

public:
	void run();
};

