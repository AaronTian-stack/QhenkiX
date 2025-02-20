#include "scaling_viewport.h"

#include <cmath>

void ScalingViewport::update(D3D11Context& context, int screen_width, int screen_height, bool center_camera)
{
	auto scaled = scaling_->apply(world_width_, world_height_, screen_width, screen_height);
	int vw = std::round(scaled.x);
	int vh = std::round(scaled.y);
	set_screen_bounds(static_cast<float>((screen_width - vw) / 2), static_cast<float>((screen_height - vh)) / 2, 
		static_cast<float>(vw), static_cast<float>(vh));
	apply(context, center_camera);
}
