#include "exampleapp.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
	qhenki::gfx::API api = qhenki::gfx::API::D3D12;
	
	for (int i = 1; i < argc; i++)
	{
		if (std::string(argv[i]) == "-api" && i + 1 < argc)
		{
			int apiValue = std::atoi(argv[i + 1]);
			if (apiValue == 0)
			{
				api = qhenki::gfx::API::D3D12;
			}
			else if (apiValue == 1)
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
	app.run(api, false);

	return 0;
}
