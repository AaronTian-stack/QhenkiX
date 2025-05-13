#pragma once
#include "camera.h"
#include <DirectXCollision.h>

class PerspectiveCamera : public Camera
{
public:
	PerspectiveCamera(float fov, float viewport_width, float viewport_height) : Camera(viewport_width, viewport_height), fov(fov) {}

	float fov = XM_PIDIV4; // Field of view in radians
	BoundingFrustum frustum;

	void update(bool update_frustum) override;
};
