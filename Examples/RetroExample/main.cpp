#include "retro_example_app.h"

int main(int argc, char* argv[])
{
    constexpr bool tearing = true;
    constexpr bool fullscreen = false;

    RetroExampleApp app;

    qhenki::gfx::SwapchainDesc swapchain_desc = {
        .width = 0,
        .height = 0,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .buffer_count = qhenki::Application::m_frames_in_flight,
        .tearing = tearing,
    };

    RetroExampleApp::Payload payload{fullscreen};
    app.run(qhenki::gfx::API::D3D12, true, &payload, swapchain_desc);

    return 0;
}
