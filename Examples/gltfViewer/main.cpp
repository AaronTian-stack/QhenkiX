#include "gltf_viewerapp.h"

int main()
{
	gltfViewerApp app;
	app.run(qhenki::gfx::API::D3D11);

	return 0;
}
