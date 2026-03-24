#include "example_app.h"
#include "example_shared/cli_args.h"
#include "example_shared/window_init.h"

int main(int argc, char* argv[])
{
    CliOptions options;
    if (!parse_cli_args(argc, argv, "SimpleExample", &options))
    {
        return 1;
    }

    qhenki::gfx::SwapchainDesc swapchain_desc = {
        .width = 0,
        .height = 0,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .buffer_count = qhenki::Application::m_frames_in_flight,
        .tearing = options.tearing,
    };

    ExampleApp app;
    WindowInitPayload payload{options.fullscreen};
    app.run(options.api, options.debug_layer, &payload, swapchain_desc);

    return 0;
}
