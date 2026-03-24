#pragma once

#include "camera.h"
#include "qhenki/utility/directxmath_compat.h"

namespace qhenki::component
{
struct PerspectiveCameraComponent : Camera
{
    float fov = XM_PIDIV2; // Field of view in radians
    BoundingFrustum frustum;

    PerspectiveCameraComponent() = default;
    PerspectiveCameraComponent(float fov, float viewport_width, float viewport_height);
    void update(bool update_frustum) override;
};
} // namespace qhenki::component
