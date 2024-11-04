#include "camera.h"
#include <cmath>

void Camera::rotate(float angle, float a_x, float a_y, float a_z)
{
	XMMATRIX rotation = XMMatrixRotationAxis(XMVectorSet(a_x, a_y, a_z, 0), angle);
	XMStoreFloat3(&forward_, XMVector3Transform(XMLoadFloat3(&forward_), rotation));
	XMStoreFloat3(&up_, XMVector3Transform(XMLoadFloat3(&up_), rotation));
}

void Camera::rotate(float angle, XMFLOAT3 axis)
{
	rotate(angle, axis.x, axis.y, axis.z);
}

void Camera::rotate(XMFLOAT4X4 matrix)
{
	XMMATRIX rotation = XMLoadFloat4x4(&matrix);
	XMStoreFloat3(&forward_, XMVector3Transform(XMLoadFloat3(&forward_), rotation));
	XMStoreFloat3(&up_, XMVector3Transform(XMLoadFloat3(&up_), rotation));
}

void Camera::rotate(XMFLOAT4 quaternion)
{
	XMVECTOR quat = XMLoadFloat4(&quaternion);
	XMStoreFloat3(&forward_, XMVector3Rotate(XMLoadFloat3(&forward_), quat));
	XMStoreFloat3(&up_, XMVector3Rotate(XMLoadFloat3(&up_), quat));
}

void Camera::translate(float x, float y, float z)
{
	XMVECTOR translation = XMVectorSet(x, y, z, 0);
	XMStoreFloat3(&position_, XMVectorAdd(XMLoadFloat3(&position_), translation));
}

void Camera::translate(XMFLOAT3 t)
{
	translate(t.x, t.y, t.z);
}

void Camera::unproject(XMFLOAT3& screen, float viewport_x, float viewport_y, float viewport_width, float viewport_height)
{
	assert(false);
	float x = screen.x - viewport_x;
	// TODO: need render target height!
	//float y = window_height - screen.y - viewportY;
}

void Camera::unproject(XMFLOAT3& screen)
{
	unproject(screen, 0, 0, viewport_width, viewport_height);
}

void Camera::project(XMFLOAT3& world, float viewport_x, float viewport_y, float viewport_width, float viewport_height)
{
	assert(false);
}

void Camera::project(XMFLOAT3& world)
{
	assert(false);
}

void Camera::look_at(float x, float y, float z)
{
	const XMVECTOR target = XMVectorSet(x, y, z, 0);
	const XMVECTOR forward = XMLoadFloat3(&forward_);
	const XMVECTOR eye = XMLoadFloat3(&position_);
	XMVECTOR up = XMLoadFloat3(&up_);

	const XMVECTOR tmp = XMVector3Normalize(XMVectorSubtract(target, eye));
	if (!XMVector3Equal(tmp, XMVectorZero()))
	{
		const float dot = XMVectorGetX(XMVector3Dot(tmp, up));
		if (std::fabs(dot - 1.f) < 0.000000001f)
		{
			up = XMVectorScale(forward, -1);
		}
		else if (fabs(dot + 1.f) < 0.000000001f)
		{
			up = forward;
		}
		XMStoreFloat3(&forward_, tmp);
		XMStoreFloat3(&up_, XMVector3Normalize(up));
	}
}

void Camera::look_at(XMFLOAT3 target)
{
	look_at(target.x, target.y, target.z);
}
