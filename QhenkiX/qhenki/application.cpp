#include "qhenki/application.h"

#include "graphics/d3d11/d3d11_context.h"
#include "graphics/d3d12/d3d12_context.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "qhenki/display_window.h"
#include "qhenki/RHI/context.h"
#include "qhenki/utility/string_util.h"

using namespace qhenki;

/**
 * This could be overridden to set up the display window with custom settings.
 * For example opening a settings window first to allow the user to select some settings.
 * Or loading resolution settings from a file.
 */
void Application::init_display_window(void* payload)
{
    util::FormatResult<256> result;
    const char* title = "QhenkiX Application";
    switch (m_graphics_api)
    {
    case gfx::API::D3D11:
    {
        result = util::format_string("%s | DX11", title);
        break;
    }
    case gfx::API::D3D12:
    {
        result = util::format_string("%s | DX12", title);
        break;
    }
    default:
    {
        result = util::format_string("%s | undefined", title);
        break;
    }
    }

    const DisplayInfo info{
        .width = 1280,
        .height = 720,
        .fullscreen = false,
        .undecorated = false,
        .resizable = true,
        .title = result.buffer.data(),
    };

    m_window.create_window(info, 0);
}

void Application::run(const gfx::API api,
                      const bool enable_debug_layer,
                      void* init_window_payload,
                      std::optional<gfx::SwapchainDesc> initial_swapchain_desc)
{
    m_graphics_api = api;
    init_display_window(init_window_payload);
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

    const std::string create_error = m_context->create(enable_debug_layer);
    if (!create_error.empty())
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", create_error.c_str(), nullptr);
        return;
    }

    THROW_IF_FALSE(m_context->create_queue(qhenki::gfx::QueueType::GRAPHICS, &m_graphics_queue));

    gfx::SwapchainDesc swapchain_desc;

    if (initial_swapchain_desc.has_value())
    {
        swapchain_desc = initial_swapchain_desc.value();
        if (swapchain_desc.width == 0 || swapchain_desc.height == 0)
        {
            swapchain_desc.width = m_window.m_display_info.width;
            swapchain_desc.height = m_window.m_display_info.height;
        }
    }
    else
    {
        swapchain_desc = {
            .width = m_window.m_display_info.width,
            .height = m_window.m_display_info.height,
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .buffer_count = m_frames_in_flight,
            .tearing = true,
        };
    }

    THROW_IF_FALSE(
        m_context->create_swapchain(m_window, swapchain_desc, &m_swapchain, &m_graphics_queue, &m_frame_index));

    gfx::DescriptorHeapDesc rtv_heap_desc{
        .type = gfx::DescriptorHeapDesc::Type::RTV,
        .visibility = gfx::DescriptorHeapDesc::Visibility::CPU,
        .descriptor_count = 256, // TODO: expose max count to context
    };
    THROW_IF_FALSE(m_context->create_descriptor_heap(rtv_heap_desc, &m_rtv_heap, "Swapchain RTV Heap"));

    // Make swapchain RTVs (stored internally)
    THROW_IF_FALSE(m_context->create_swapchain_descriptors(m_swapchain, &m_rtv_heap));

    // Create fences
    THROW_IF_FALSE(m_context->create_fence(&m_fence_frame_ready, m_fence_frame_ready_val[m_frame_index]));

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
                m_window.m_display_info.width = event.window.data1;
                m_window.m_display_info.height = event.window.data2;
                m_context->resize_swapchain(
                    &m_swapchain, event.window.data1, event.window.data2, &m_rtv_heap, m_frame_index);
                // The swapchain backbuffer index might have changed after resize to use the same index as the last
                // present, so the last fence value is no longer valid
                // Reset both wait values to avoid infinite wait (one of them should be decremented)
                const auto completed_fence_value = m_context->get_fence_value(m_fence_frame_ready);
                for (unsigned i = 0; i < m_frames_in_flight; i++)
                {
                    m_fence_frame_ready_val[i] = completed_fence_value;
                }
                resize(event.window.data1, event.window.data2);
            }
            m_input_manager.handle_extra_events(event);
            if (ImGui::GetCurrentContext())
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
            }
        }
        m_input_manager.update(m_window.get_window()); // After all SDL events
        render();
    }
    m_context->wait_idle(&m_graphics_queue);
    destroy();
}

Application::~Application() = default;
