#pragma once
#include "camera.h"
#include <DirectXCollision.h>

namespace qhenki
{
	class PerspectiveCamera : public Camera
	{
	public:
		PerspectiveCamera() : Camera() {}
		PerspectiveCamera(float fov, float viewport_width, float viewport_height) : Camera(viewport_width, viewport_height), fov(fov) {}

		float fov = XM_PIDIV2; // Field of view in radians
		BoundingFrustum frustum;

		void update(bool update_frustum) override;
		void update() override { update(true); }
	};
}