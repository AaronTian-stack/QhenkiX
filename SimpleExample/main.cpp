#include "exampleapp.h"

int main()
{
	// Run simple example application for the qhenki::graphicsX Game Framework.

	ExampleApp app;
	app.run(qhenki::graphics::D3D12, std::this_thread::get_id());

	return 0;
}
