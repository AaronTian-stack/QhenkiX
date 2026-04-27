#include "qhenki/perspective_camera_component.h"

#include "qhenki/math/transform_simd.h"

using namespace qhenki::component;

PerspectiveCameraComponent::PerspectiveCameraComponent(float fov, float viewport_width, float viewport_height)
    : Camera(viewport_width, viewport_height),
      fov(fov)
{
}

void PerspectiveCameraComponent::update(const bool update_frustum)
{
    const auto proj = XMMatrixPerspectiveFovLH(fov, viewport_width / viewport_height, near_plane, far_plane);
    if (update_frustum)
    {
        BoundingFrustum::CreateFromMatrix(frustum, proj);
    }
}
