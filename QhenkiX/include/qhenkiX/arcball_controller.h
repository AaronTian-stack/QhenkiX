#pragma once
#include <DirectXMath.h>

#include "camera.h"

using namespace DirectX;

namespace qhenki
{
	/**
	 * Provides utility to control a Transform in arcball style with no roll.
	 */
	class ArcBallController
	{
		Transform* m_transform = nullptr;
		XMFLOAT3 m_target_position{};
		float m_target_distance = 2.f;
	public:
		ArcBallController() = default;
		explicit ArcBallController(Transform* transform) : m_transform(transform) {}

		void set_camera(Transform* transform);
		
		float get_target_distance() const { return m_target_distance; }
		void set_target_distance(float distance);

		void translate(float x, float y);
		void rotate(float x, float y);
	};
}
