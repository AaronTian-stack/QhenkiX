#pragma once

#include "viewport.h"
#include "util/scaling.h"

class ScalingViewport : public Viewport
{
	Scaling* scaling_;

public:
	ScalingViewport(Scaling* scaling, float world_width, float world_height, Camera* camera) : Viewport(world_width, world_height, camera), scaling_(scaling)
	{
	}

	void update(D3D11Context& context, int screen_width, int screen_height, bool center_camera) override;
	Scaling* get_scaling() { return scaling_; }
	void set_scaling(Scaling* scaling) { scaling_ = scaling; }
};

