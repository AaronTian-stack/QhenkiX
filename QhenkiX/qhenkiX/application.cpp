#include "application.h"

#include <iostream>
#include <graphics/d3d11/d3d11_context.h>
#include <graphics/d3d12/d3d12_context.h>

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
	
	m_window_.create_window(info, 0);
}

void Application::run(const qhenki::graphics::GraphicsAPI api, std::thread::id main_thread)
{
	m_graphics_api_ = api;
	init_display_window();
	switch (api)
	{
	case qhenki::graphics::D3D11:
		m_context_ = mkU<D3D11Context>();
		break;
	case qhenki::graphics::D3D12:
		m_context_ = mkU<D3D12Context>();
		break;
	case qhenki::graphics::Vulkan: 
		break;
	default:
		throw std::runtime_error("Not implemented API");
	}
	m_context_->create();

	m_context_->create_queue(qhenki::graphics::QueueType::GRAPHICS, m_graphics_queue_);

	const qhenki::graphics::SwapchainDesc swapchain_desc =
	{
		.width = m_window_.m_display_info_.width,
		.height = m_window_.m_display_info_.height,
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
	};
	m_context_->create_swapchain(m_window_, swapchain_desc, m_swapchain_, m_graphics_queue_, m_frames_in_flight, m_frame_index_);
	create();
	// Starts the main loop
    bool quit = false;
    while (!quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
			if (event.type == SDL_EVENT_WINDOW_RESIZED)
			{
				m_window_.m_display_info_.width = event.window.data1;
				m_window_.m_display_info_.height = event.window.data2;
				m_context_->resize_swapchain(m_swapchain_, event.window.data1, event.window.data2);
				resize(event.window.data1, event.window.data2);
			}
        }

        render();

		// TODO: proper application frame limiting
    }
	m_context_->wait_all();
	destroy();
}

Application::~Application() {}