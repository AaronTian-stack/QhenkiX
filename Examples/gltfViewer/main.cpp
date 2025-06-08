#include "gltf_viewerapp.h"

int main()
{
	gltfViewerApp app;
	app.run(qhenki::gfx::API::D3D12);

	return 0;
}
