#pragma once

#include <smartpointer.h>

#include "display_window.h"
#include "input/input_manager.h"
#include "rhi/context.h"

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
    // TODO: Audio

    input::InputManager m_input_manager{};

    // TODO: Files
    // TODO: Preferences

    // Set to true when the application should quit
    bool m_QUIT = false;

    DisplayWindow m_window;
    uPtr<gfx::Context> m_context;
    gfx::Swapchain m_swapchain{};

    // Convenience fence to track when the current frame is ready for rendering (signal at end of present)
    gfx::Fence m_fence_frame_ready{};
    std::array<uint64_t, m_frames_in_flight> m_fence_frame_ready_val{0, 0};

    virtual void init_display_window(void* payload);

    virtual void create()
    {
    }
    virtual void update()
    {
    }
    virtual void render()
    {
    }

    /**
     * Function called whenever window is resized. This function is also called once after create().
     * @param width New width of the window
     * @param height New height of the window
     */
    virtual void resize(unsigned width, unsigned height)
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
