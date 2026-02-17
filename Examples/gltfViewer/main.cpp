#include <cstdio>

#include <argparse/argparse.hpp>
#include "gltf_viewer_app.h"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("gltfViewer");

    auto& api_group = program.add_mutually_exclusive_group();
    api_group.add_argument("-dx11").flag().help("use DirectX 11");
    api_group.add_argument("-dx12").flag().help("use DirectX 12");
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

    const bool tearing = program.get<bool>("--tearing");
    const bool fullscreen = program.get<bool>("--fullscreen");

    gltfViewerApp app;

    qhenki::gfx::SwapchainDesc swapchain_desc = {
        .width = 0,
        .height = 0,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .buffer_count = qhenki::Application::m_frames_in_flight,
        .tearing = tearing,
    };

    gltfViewerApp::Payload payload{fullscreen};
    app.run(api, true, &payload, swapchain_desc);

    return 0;
}
