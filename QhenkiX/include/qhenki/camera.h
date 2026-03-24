#pragma once

#include "qhenki/utility/directxmath_compat.h"

using namespace DirectX;

namespace qhenki::component
{
class Camera
{
public:
    struct Matrices
    {
        XMFLOAT4X4 view_proj{};
        XMFLOAT4X4 inv_view_proj{};
    };

protected:
    Matrices m_camera_matrices;

public:
    Camera() = default;
    Camera(float vw, float vh);
    virtual ~Camera() = default;

    float near_plane = 0.05f;
    float far_plane = 10000.f;

    float viewport_width = 0;
    float viewport_height = 0;

    // Update projection matrix and frustum if requested
    virtual void update(bool update_frustum = false) = 0;

    void unproject(
        XMFLOAT3& screen, float viewport_x, float viewport_y, float viewport_width, float viewport_height) const;

    void project(XMFLOAT3& world, float viewport_x, float viewport_y, float viewport_width, float viewport_height);
};
} // namespace qhenki::component
