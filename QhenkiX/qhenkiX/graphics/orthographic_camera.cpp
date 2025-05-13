#include "orthographic_camera.h"

OrthographicCamera::OrthographicCamera() : Camera()
{
}

OrthographicCamera::OrthographicCamera(float viewport_width, float viewport_height) : Camera(viewport_width, viewport_height)
{
}

void OrthographicCamera::update(bool update_frustum)
{
	XMStoreFloat4x4(&projection_, XMMatrixOrthographicLH(viewport_width, viewport_height, near, far));
	XMStoreFloat4x4(&view_, XMMatrixLookToLH(XMLoadFloat3(&position_), XMLoadFloat3(&forward_), XMLoadFloat3(&up_)));
	XMStoreFloat4x4(&view_projection_, XMMatrixMultiply(XMLoadFloat4x4(&view_), XMLoadFloat4x4(&projection_)));
	XMStoreFloat4x4(&inverse_view_projection_, XMMatrixInverse(nullptr, XMLoadFloat4x4(&view_projection_)));
	if (update_frustum)
	{
		// TODO
	}
}

void OrthographicCamera::set_to_ortho(bool y_down)
{
	assert(false);
	// TODO: use screen resolution
}

void OrthographicCamera::set_to_ortho(bool y_down, float viewport_width, float viewport_height)
{
	if (y_down)
	{
		up_ = { 0, -1, 0 };
		forward_ = { 0, 0, 1 };
	}
	else
	{
		up_ = { 0, 1, 0 };
		forward_ = { 0, 0, -1 };
	}
	XMStoreFloat3(&position_, XMVectorScale(XMVectorSet(viewport_width, viewport_height, 0.f, 0.f), zoom * 0.5f));
	this->viewport_width = viewport_width;
	this->viewport_height = viewport_height;
	Camera::update();
}
