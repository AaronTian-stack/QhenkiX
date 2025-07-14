#include "imgui_exampleapp.h"

int main()
{
	// Run ImGUI example application
	ImGUIExampleApp app;
	app.run(qhenki::gfx::API::D3D12, false);

	return 0;
}
