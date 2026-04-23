#pragma once

#include <qhenki/application.h>

struct CliOptions
{
    qhenki::gfx::API api;
    bool debug_layer = false;
    bool tearing = false;
    bool fullscreen = false;
};

bool parse_cli_args(int argc, char* argv[], const char* program_name, CliOptions* out_options);
