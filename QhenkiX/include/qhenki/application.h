#pragma once

#include <smartpointer.h>
#include <thread>

#include "display_window.h"

#include "input/input_manager.h"

#include "RHI/context.h"

namespace qhenki
{
namespace gfx
{
enum class API
{
    D3D11,
    D3D12,
    Vulkan,
};
} // namespace gfx

/**
 * Encapsulates the objects that control DisplayWindow, Audio, Input, File System, Preferences.
 * Sets up default states for the objects, but initialization can be overridden.
 */
class Application
{
public:
    static constexpr unsigned m_frames_in_flight = 2;

private:
    gfx::API m_graphics_api = gfx::API::D3D11;

protected:
    // Audio
    input::InputManager m_input_manager{}; // Input
    // Files
    // Preferences

    bool m_QUIT = false; // Set to true when the application should quit
    unsigned m_frame_index = 0;
    DisplayWindow m_window;
    uPtr<gfx::Context> m_context;
    gfx::Swapchain m_swapchain{};
    gfx::Queue m_graphics_queue{};    // A graphics queue is given to the application by default
    gfx::DescriptorHeap m_rtv_heap{}; // Default RTV heap that also contains swapchain descriptors

    gfx::Fence m_fence_frame_ready{};
    std::array<uint64_t, m_frames_in_flight> m_fence_frame_ready_val{0, 0};

    virtual void init_display_window(void* payload);

    virtual void create()
    {
    }
    // The swapchain is automatically resized before this is called
    virtual void render()
    {
    }
    virtual void resize(int width, int height)
    {
    }
    virtual void destroy()
    {
    }

public:
    // Call this from the main thread
    void run(gfx::API api,
             bool enable_debug_layer,
             void* init_window_payload,
             std::optional<gfx::SwapchainDesc> initial_swapchain_desc);
    gfx::API get_graphics_api() const
    {
        return m_graphics_api;
    }
    virtual ~Application() = 0;
};
} // namespace qhenki
