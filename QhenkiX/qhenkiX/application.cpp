#include "qhenkiX/application.h"

#include <iostream>
#include "graphics/d3d11/d3d11_context.h"
#include "graphics/d3d12/d3d12_context.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "qhenkiX/display_window.h"
#include "qhenkiX/RHI/context.h"

using namespace qhenki;

/**
 * This could be overridden to set up the display window with custom settings.
 * For example opening a settings window first to allow the user to select some settings.
 * Or loading resolution settings from a file.
 */
void Application::init_display_window()
{
	std::string title = "QhenkiX Application";
	switch (m_graphics_api)
	{
		case gfx::API::D3D11:
			title += " | DX11";
			break;
		case gfx::API::D3D12:
			title += " | DX12";
			break;
		default:
			break;
	}

	DisplayInfo info
	{
		.width = 1280,
		.height = 720,
		.fullscreen = false,
		.undecorated = false,
		.resizable = true,
		.title = title,
	};

	m_window_.create_window(info, 0);
}

void Application::run(const gfx::API api)
{
	m_graphics_api = api;
	m_main_thread_id = std::this_thread::get_id();
	init_display_window();
	switch (api)
	{
	case gfx::API::D3D11:
		m_context = mkU<gfx::D3D11Context>();
		break;
	case gfx::API::D3D12:
		m_context = mkU<gfx::D3D12Context>();
		break;
	case gfx::API::Vulkan:
	default:
		throw std::runtime_error("API not implemented");
	}
	m_context->create();

	THROW_IF_FALSE(m_context->create_queue(qhenki::gfx::QueueType::GRAPHICS, &m_graphics_queue));

	const gfx::SwapchainDesc swapchain_desc =
	{
		.width = m_window_.m_display_info.width,
		.height = m_window_.m_display_info.height,
		.format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.buffer_count = m_frames_in_flight,
	};
	THROW_IF_FALSE(m_context->create_swapchain(m_window_, swapchain_desc, m_swapchain, 
			m_graphics_queue, m_frame_index));

	gfx::DescriptorHeapDesc rtv_heap_desc
	{
		.type = gfx::DescriptorHeapDesc::Type::RTV,
		.visibility = gfx::DescriptorHeapDesc::Visibility::CPU,
		.descriptor_count = 256, // TODO: expose max count to context
	};
	THROW_IF_FALSE(m_context->create_descriptor_heap(rtv_heap_desc, m_rtv_heap, L"swapchain RTV heap"));
	
	// Make swapchain RTVs (stored internally)
	THROW_IF_FALSE(m_context->create_swapchain_descriptors(m_swapchain, m_rtv_heap));

	// Create fences
	THROW_IF_FALSE(m_context->create_fence(&m_fence_frame_ready, m_fence_frame_ready_val[get_frame_index()]));

	create();
	// Starts the main loop
    while (!m_QUIT)
    {
		m_input_manager.reset_mouse_scroll();
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
				m_QUIT = true;
            }
			if (event.type == SDL_EVENT_WINDOW_RESIZED)
			{
				m_window_.m_display_info.width = event.window.data1;
				m_window_.m_display_info.height = event.window.data2;
				m_context->resize_swapchain(m_swapchain, event.window.data1, event.window.data2, m_rtv_heap, m_frame_index);
				resize(event.window.data1, event.window.data2);
			}
			m_input_manager.handle_extra_events(event);
			if (ImGui::GetCurrentContext())
				ImGui_ImplSDL3_ProcessEvent(&event);
        }
		m_input_manager.update(m_window_.get_window()); // After all SDL events
        render();
    }
	m_context->wait_idle(m_graphics_queue);
	destroy();
}

Application::~Application() = default;
