#include "qhenki/orthographic_camera_component.h"

using namespace qhenki::component;

OrthographicCamera::OrthographicCamera() = default;

OrthographicCamera::OrthographicCamera(const float viewport_width, const float viewport_height)
    : Camera(viewport_width, viewport_height)
{
}

void OrthographicCamera::update(bool update_frustum)
{
    const float width = viewport_width / zoom_;
    const float height = viewport_height / zoom_;
    const auto proj = XMMatrixOrthographicLH(width, height, near_plane, far_plane);
    if (update_frustum)
    {
        // TODO: Implement custom frustum update since BoundingFrustum doesn't support orthographic projection
    }
}
