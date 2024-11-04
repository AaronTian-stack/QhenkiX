#pragma once

#include "scaling_viewport.h"

class FitViewport : public ScalingViewport
{
public:
	FitViewport(float world_width, float world_height, Camera* camera) : ScalingViewport(&ScalingStatic::fit, world_width, world_height, camera)
	{
	}
};

