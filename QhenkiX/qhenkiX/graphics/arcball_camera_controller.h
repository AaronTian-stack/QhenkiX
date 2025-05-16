#pragma once
#include <DirectXMath.h>

#include "camera.h"

using namespace DirectX;

namespace qhenki
{
	/**
	 * Provides utility to control a Camera in arcball style with no roll.
	 */
	class ArcBallCameraController
	{
		Camera* camera = nullptr;
		XMFLOAT3 target_position{};
		float target_distance = 2.f;
	public:
		ArcBallCameraController() = default;
		ArcBallCameraController(Camera* camera) : camera(camera) {}

		void set_camera(Camera* camera);
		
		float get_target_distance() const { return target_distance; }
		void set_target_distance(float distance);

		void translate(float x, float y);
		void rotate(float x, float y);
	};
}
