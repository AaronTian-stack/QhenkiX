#include "application.h"

/**
 * This could be overridden to set up the display window with custom settings.
 * For example opening a window to allow the user to select some settings.
 * Or loading resolution settings from a file.
 */
void Application::init_display_window()
{
	DisplayInfo info
	{
		.width = 1280,
		.height = 720,
		.fullscreen = false,
		.undecorated = false,
		.resizable = true,
		.title = "QhenkiX Application | DX11",
	};
	
	window_.create_window(info, 0);
}

void Application::run()
{
	init_display_window();
	d3d11_.create(window_);
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
					window_.display_info_.width = event.window.data1;
					window_.display_info_.height = event.window.data2;
					d3d11_.resize_swapchain(event.window.data1, event.window.data2);
					resize(event.window.data1, event.window.data2);
				}
			}
        }

        render();

		// TODO: proper application frame limiting
		//window.wait();
    }
	d3d11_.destroy();
    window_.destroy();
    destroy();
}
