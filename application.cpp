#include "application.h"

/**
 * This could be overridden to set up the display window with custom settings.
 * For example opening a window to allow the user to select some settings.
 * Or loading resolution settings from a file.
 */
void Application::init_display_window()
{
	DisplayInfo info;
	info.width = 1280;
	info.height = 720;
	info.title = "QhenkiX Application | DX11";
	info.fullscreen = false;
	info.undecorated = false;
	info.resizable = true;
	window.create_window(info, 0);
}

void Application::create()
{

}

void Application::run()
{
	init_display_window();
	d3d11.create(window);
	create();
	// starts the main loop
    bool quit = false;
    while (!quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
            }
			if (event.type == SDL_WINDOWEVENT)
			{
				if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					window.display_info.width = event.window.data1;
					window.display_info.height = event.window.data2;
					d3d11.resize_swapchain(event.window.data1, event.window.data2);
					resize(event.window.data1, event.window.data2);
				}
			}
        }

        render();

		// TODO: proper application frame limiting
		//window.wait();
    }
    window.destroy();
	d3d11.destroy();
    destroy();
}

void Application::render()
{
}

void Application::resize(int width, int height)
{
}

void Application::destroy()
{
}
