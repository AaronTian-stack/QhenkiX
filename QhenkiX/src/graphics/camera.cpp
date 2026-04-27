#include "qhenki/camera.h"

using namespace qhenki::component;

void Camera::unproject(XMFLOAT3& screen,
                       const float viewport_x,
                       const float viewport_y,
                       const float viewport_width,
                       const float viewport_height) const
{
    const auto ndc_x = (screen.x - viewport_x) / viewport_width * 2.0f - 1.0f;
    const auto ndc_y = 1.0f - (screen.y - viewport_y) / viewport_height * 2.0f;
    const auto ndc_z = screen.z * 2.0f - 1.0f; // Assuming screen.z is in [0, 1] (depth)

    const auto ndc = XMVectorSet(ndc_x, ndc_y, ndc_z, 1.0f);

    auto world = XMVector4Transform(ndc, XMLoadFloat4x4(&m_camera_matrices.inv_view_proj));

    const auto w = XMVectorGetW(world);
    if (w != 0.0f)
    {
        world = XMVectorScale(world, 1.0f / w);
    }

    XMStoreFloat3(&screen, world);
}

void Camera::project(XMFLOAT3& world, float viewport_x, float viewport_y, float viewport_width, float viewport_height)
{
    // TODO
}

Camera::Camera(const float vw, const float vh)
    : viewport_width(vw),
      viewport_height(vh)
{
}
