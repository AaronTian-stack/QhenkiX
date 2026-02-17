#include <cstdio>

#include <argparse/argparse.hpp>
#include "example_app.h"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("SimpleExample");

    auto& api_group = program.add_mutually_exclusive_group();
    api_group.add_argument("-dx11").flag().help("use DirectX 11");
    api_group.add_argument("-dx12").flag().help("use DirectX 12");

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

    ExampleApp app;
    app.run(api, false, nullptr, std::nullopt);

    return 0;
}
