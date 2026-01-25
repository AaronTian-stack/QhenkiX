#include <iostream>
#include <string>
#include "exampleapp.h"

int main(int argc, char* argv[])
{
    auto api = qhenki::gfx::API::D3D12;

    for (int i = 1; i < argc; i++)
    {
        if (std::string(argv[i]) == "-api" && i + 1 < argc)
        {
            const int api_value = std::atoi(argv[i + 1]);
            if (api_value == 0)
            {
                api = qhenki::gfx::API::D3D12;
            }
            else if (api_value == 1)
            {
                api = qhenki::gfx::API::D3D11;
            }
            else
            {
                std::cerr << "Invalid API value. Use 0 for D3D12 or 1 for D3D11." << std::endl;
                return 1;
            }
            i++;
        }
    }

    // Run simple example application for the QhenkiX Game Framework.
    ExampleApp app;
    app.run(api, false, nullptr, std::nullopt);

    return 0;
}
