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
    float ndc_x = (screen.x - viewport_x) / viewport_width * 2.0f - 1.0f;
    float ndc_y = 1.0f - ((screen.y - viewport_y) / viewport_height) * 2.0f;
    float ndc_z = screen.z * 2.0f - 1.0f; // Assuming screen.z is in [0, 1] (depth)

    XMVECTOR ndc = XMVectorSet(ndc_x, ndc_y, ndc_z, 1.0f);

    XMMATRIX inv_view_proj = XMLoadFloat4x4(&inverse_view_projection_);
    XMVECTOR world = XMVector4Transform(ndc, inv_view_proj);

    float w = XMVectorGetW(world);
    if (w != 0.0f) 
	{
        world = XMVectorScale(world, 1.0f / w);
    }

    XMStoreFloat3(&screen, world);
}

void Camera::project(XMFLOAT3& world, float viewport_x, float viewport_y, float viewport_width, float viewport_height)
{
	assert(false);
}

void Camera::look_at(float x, float y, float z)
{
	const XMVECTOR eye = XMLoadFloat3(&position_);
	const XMVECTOR target = XMVectorSet(x, y, z, 0);
	const XMVECTOR up = XMLoadFloat3(&up_);

	XMMATRIX view = XMMatrixLookAtLH(eye, target, up);
	XMVECTOR det;
	XMMATRIX invView = XMMatrixInverse(&det, view);

	XMStoreFloat3(&forward_, XMVector3Normalize(invView.r[2]));
	XMStoreFloat3(&up_, XMVector3Normalize(invView.r[1]));
}

void Camera::look_at(XMFLOAT3 target)
{
	look_at(target.x, target.y, target.z);
}
