#include "perspective_camera.h"

using namespace qhenki;

void PerspectiveCamera::update(bool update_frustum)
{
	auto proj = XMMatrixPerspectiveFovLH(fov, viewport_width / viewport_height, near_plane, far_plane);

	transform_.basis_.orthonormalize();
	auto view = transform_.to_matrix_simd();

	auto view_proj = XMMatrixTranspose(view * proj);
	auto inv_view_proj = XMMatrixInverse(nullptr, view_proj);

	XMStoreFloat4x4(&matrices_.view_projection_, view_proj);
	XMStoreFloat4x4(&matrices_.inverse_view_projection_, inv_view_proj);

	if (update_frustum)
	{
		BoundingFrustum::CreateFromMatrix(frustum, proj);
	}
}
