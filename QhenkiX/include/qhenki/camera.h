#pragma once

#include <DirectXMath.h>

#include "math/transform.h"

using namespace DirectX;

namespace qhenki
{
	struct CameraMatrices
	{
		XMFLOAT4X4 view_projection{};
		XMFLOAT4X4 inverse_view_projection{};
	};

	class Camera
	{
	protected:
		CameraMatrices m_matrices{};

	public:
		Transform transform{};

		Camera() {}
		Camera(const float vw, const float vh) : viewport_width(vw), viewport_height(vh) {}
		virtual ~Camera() {};

		float near_plane = 0.05f;
		float far_plane = 10000.f;

		float viewport_width = 0;
		float viewport_height = 0;
		
		const CameraMatrices& matrices = m_matrices;

		/**
		 * Updates view, projection, view_projection, inverse_view_projection
		 */
		virtual void update(bool update_frustum = false) = 0;

		void unproject(XMFLOAT3& screen, float viewport_x, float viewport_y, float viewport_width, float viewport_height);

		void project(XMFLOAT3& world, float viewport_x, float viewport_y, float viewport_width, float viewport_height);

		//friend class Viewport;
	};
}