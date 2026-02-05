#pragma once

#include "camera.h"

namespace qhenki::component
{
struct OrthographicCamera : Camera
{
    float zoom_ = 1.0f;

    // TODO: orthographic frustum

    OrthographicCamera();
    OrthographicCamera(float viewport_width, float viewport_height);

    void update(bool update_frustum) override;
};
} // namespace qhenki::component
