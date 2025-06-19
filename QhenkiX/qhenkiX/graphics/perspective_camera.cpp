#include "perspective_camera.h"

using namespace qhenki;

void PerspectiveCamera::update(bool update_frustum)
{
	auto proj = XMMatrixPerspectiveFovLH(fov, viewport_width / viewport_height, near_plane, far_plane);

	transform.basis.orthonormalize();
	auto view = transform.to_matrix_simd();

	auto view_proj = XMMatrixTranspose(view * proj);
	auto inv_view_proj = XMMatrixInverse(nullptr, view_proj);

	XMStoreFloat4x4(&m_matrices.view_projection, view_proj);
	XMStoreFloat4x4(&m_matrices.inverse_view_projection, inv_view_proj);

	if (update_frustum)
	{
		BoundingFrustum::CreateFromMatrix(frustum, proj);
	}
}
