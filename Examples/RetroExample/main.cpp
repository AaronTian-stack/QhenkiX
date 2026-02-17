#include <cstdio>

#include <argparse/argparse.hpp>
#include "retro_example_app.h"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("RetroExample");

    auto& api_group = program.add_mutually_exclusive_group();
    api_group.add_argument("-dx11").flag().help("use DirectX 11");
    api_group.add_argument("-dx12").flag().help("use DirectX 12");
    program.add_argument("-d", "--debug").flag().help("enable graphics API debug layer");
    program.add_argument("-t", "--tearing").flag().help("enable tearing (vsync off)");
    program.add_argument("-f", "--fullscreen").flag().help("start in fullscreen mode");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err)
    {
        fprintf(stderr, "%s\n", err.what());
        fprintf(stderr, "%s", program.help().str().c_str());
        return 1;
    }

    const auto api = program.get<bool>("-dx11") ? qhenki::gfx::API::D3D11 : qhenki::gfx::API::D3D12;

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
