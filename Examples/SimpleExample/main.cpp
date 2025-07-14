#include "exampleapp.h"

int main()
{
	// Run simple example application for the QhenkiX Game Framework.
	ExampleApp app;
	app.run(qhenki::gfx::API::D3D12, false);

	return 0;
}
