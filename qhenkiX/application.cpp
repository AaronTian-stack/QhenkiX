#include "application.h"

#include <graphics/d3d11/d3d11_context.h>

/**
 * This could be overridden to set up the display window with custom settings.
 * For example opening a settings window first to allow the user to select some settings.
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
		.title = "QhenkiX Application",
	};
	
	window_.create_window(info, 0);
}

void Application::run()
{
	init_display_window();
	// TODO: create certain context
	context_ = mkU<D3D11Context>();
	context_->create();
	swapchain_.desc =
	{
		.width = window_.display_info_.width,
		.height = window_.display_info_.height,
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
	};
	context_->create_swapchain(window_, swapchain_);
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
					//context_->resize_swapchain(event.window.data1, event.window.data2);
					resize(event.window.data1, event.window.data2);
				}
			}
        }

        render();

		// TODO: proper application frame limiting
    }
	context_->wait_all();
	destroy();
}
