#pragma once

#include "camera.h"

class OrthographicCamera : public Camera
{
	float zoom_ = 1.f;

public:
	const float& zoom = zoom_;

	OrthographicCamera();
	OrthographicCamera(float viewport_width, float viewport_height);

	void update() override;

	void set_to_ortho(bool y_down);
	void set_to_ortho(bool y_down, float viewport_width, float viewport_height);
};

