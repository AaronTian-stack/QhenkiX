#include "perspective_camera.h"

void PerspectiveCamera::update(bool update_frustum)
{
	XMStoreFloat4x4(&projection_, XMMatrixPerspectiveFovLH(fov, viewport_width / viewport_height, near, far));
	XMStoreFloat4x4(&view_, XMMatrixLookToLH(XMLoadFloat3(&position_), XMLoadFloat3(&forward_), XMLoadFloat3(&up_)));
	XMStoreFloat4x4(&view_projection_, XMMatrixMultiply(XMLoadFloat4x4(&view_), XMLoadFloat4x4(&projection_)));
	XMStoreFloat4x4(&inverse_view_projection_, XMMatrixInverse(nullptr, XMLoadFloat4x4(&view_projection_)));
	if (update_frustum)
	{
		BoundingFrustum::CreateFromMatrix(frustum, XMLoadFloat4x4(&view_projection_));
	}
}
