#include "qhenki/perspective_camera.h"

#include "qhenki/math/transform_simd.h"

using namespace qhenki;

void PerspectiveCamera::update(bool update_frustum)
{
    const auto proj = XMMatrixPerspectiveFovLH(fov, viewport_width / viewport_height, near_plane, far_plane);

    const auto world = math::TransformSIMD::load(transform).to_matrix();
    const auto view = XMMatrixInverse(nullptr, world);

    const auto view_proj = XMMatrixTranspose(view * proj);
    const auto inv_view_proj = XMMatrixInverse(nullptr, view_proj);

    XMStoreFloat4x4(&m_matrices.view_projection, view_proj);
    XMStoreFloat4x4(&m_matrices.inverse_view_projection, inv_view_proj);

    if (update_frustum)
    {
        BoundingFrustum::CreateFromMatrix(frustum, proj);
    }
}
