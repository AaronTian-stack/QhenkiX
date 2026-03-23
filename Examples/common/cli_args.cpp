#include <cstdio>

#include <argparse/argparse.hpp>

#include "example_shared/cli_args.h"

bool parse_cli_args(int argc, char* argv[], const char* program_name, CliOptions* out_options)
{
    if (!out_options)
    {
        return false;
    }

    argparse::ArgumentParser program(program_name);

    auto& api_group = program.add_mutually_exclusive_group();
    api_group.add_argument("-dx11").flag().help("use DirectX 11");
    api_group.add_argument("-dx12").flag().help("use DirectX 12");
    api_group.add_argument("-vk", "--vulkan").flag().help("use Vulkan");
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
        return false;
    }

    if (program.get<bool>("-dx11"))
    {
        out_options->api = qhenki::gfx::API::D3D11;
    }
    else if (program.get<bool>("-vk"))
    {
        out_options->api = qhenki::gfx::API::Vulkan;
    }
    else
    {
        out_options->api = qhenki::gfx::API::D3D12;
    }

    out_options->debug_layer = program.get<bool>("--debug");
    out_options->tearing = program.get<bool>("--tearing");
    out_options->fullscreen = program.get<bool>("--fullscreen");
    return true;
}
