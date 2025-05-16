#include "camera.h"

using namespace qhenki;

void Camera::unproject(XMFLOAT3& screen, float viewport_x, float viewport_y, float viewport_width, float viewport_height)
{
    float ndc_x = (screen.x - viewport_x) / viewport_width * 2.0f - 1.0f;
    float ndc_y = 1.0f - ((screen.y - viewport_y) / viewport_height) * 2.0f;
    float ndc_z = screen.z * 2.0f - 1.0f; // Assuming screen.z is in [0, 1] (depth)

    XMVECTOR ndc = XMVectorSet(ndc_x, ndc_y, ndc_z, 1.0f);

    XMMATRIX inv_view_proj = XMLoadFloat4x4(&matrices_.inverse_view_projection_);
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
