#include <argparse/argparse.hpp>
#include "retro_example_app.h"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("RetroExample");

    program.add_argument("-api").default_value(0).help("what graphics API to use. 0: D3D12, 1: D3D11").scan<'i', int>();
    program.add_argument("-d", "--debug").flag().help("enable graphics API debug layer");
    program.add_argument("-t", "--tearing").flag().help("enable tearing (vsync off)");
    program.add_argument("-f", "--fullscreen").flag().help("start in fullscreen mode");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    const auto api_index = program.get<int>("-api");
    if (api_index < 0 || api_index > 1)
    {
        std::cerr << "Invalid API index" << std::endl;
        return 1;
    }
    const auto api = api_index == 0 ? qhenki::gfx::API::D3D12 : qhenki::gfx::API::D3D11;

    const bool debug_layer = program.get<bool>("--debug");
    const bool tearing = program.get<bool>("--tearing");
    const bool fullscreen = program.get<bool>("--fullscreen");

    RetroExampleApp app;

    qhenki::gfx::SwapchainDesc swapchain_desc = {
        .width = 0,
        .height = 0,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .buffer_count = qhenki::Application::m_frames_in_flight,
        .tearing = tearing,
    };

    RetroExampleApp::Payload payload{fullscreen};
    app.run(api, debug_layer, &payload, swapchain_desc);

    return 0;
}
