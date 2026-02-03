#include <algorithm>
#include <cassert>
#include <cmath>

#include <DirectXMath.h>
#include "qhenki/arcball_controller.h"

using namespace DirectX;
using namespace qhenki;
using namespace qhenki::math;

void ArcBallController::set_camera(Transform* transform)
{
    assert(transform);
    m_transform = transform;

    // Set camera to be at target distance
    auto diff = XMLoadFloat3(&m_target_position) - XMLoadFloat3(&transform->translation);
    const auto dist = XMVector3Length(diff);
    const float len = XMVectorGetX(dist);

    const auto cam_pos = XMLoadFloat3(&transform->translation);
    if (std::abs(len) < 1e-6f) // Camera is at target position
    {
        // Just use the camera's forward vector
        const auto axis_z = transform->axis_z();
        const auto forward = XMVector3Normalize(axis_z) * -m_target_distance; // Negative move backwards
        XMStoreFloat3(&transform->translation, cam_pos + forward);
    }
    else
    {
        transform->look_at(m_target_position, {0.f, 1.f, 0.f}); // TODO: check up vector
        diff = XMVector3Normalize(diff);
        XMStoreFloat3(&transform->translation, cam_pos + diff * (m_target_distance - len));
    }
}

void ArcBallController::set_target_distance(float distance)
{
    distance = std::max(distance, 0.01f);
    assert(m_transform);
    m_target_distance = distance;
    // Set camera to be at target distance
    auto diff = XMLoadFloat3(&m_transform->translation) - XMLoadFloat3(&m_target_position);
    const float len = XMVectorGetX(XMVector3Length(diff));
    const auto cam_pos = XMLoadFloat3(&m_transform->translation);
    diff = XMVector3Normalize(diff);
    XMStoreFloat3(&m_transform->translation, cam_pos + diff * (m_target_distance - len));
}

void ArcBallController::translate(float x, float y)
{
    assert(m_transform);
    m_transform->translate_local(XMVectorSet(x, y, 0.0f, 1.0f));
    const auto t = m_transform->transform_vector(XMVectorSet(x, y, 0.0f, 1.0f));
    XMStoreFloat3(&m_target_position, XMVectorAdd(XMLoadFloat3(&m_target_position), t));
}

const float max_tolerance = 0.00174533f; // .01 degree

void ArcBallController::rotate(float x, float y) const
{
    assert(m_transform);

    // Get camera position relative to target
    XMVECTOR cam_pos = XMLoadFloat3(&m_transform->translation);
    XMVECTOR target = XMLoadFloat3(&m_target_position);
    XMVECTOR offset = XMVectorSubtract(cam_pos, target);

    // Convert to spherical coordinates
    float radius = XMVectorGetX(XMVector3Length(offset));
    
    // Get current pitch and yaw from the offset vector
    XMVECTOR offset_normalized = XMVector3Normalize(offset);
    float current_pitch = std::acos(XMVectorGetY(offset_normalized));
    float current_yaw = std::atan2(XMVectorGetX(offset_normalized), XMVectorGetZ(offset_normalized));

    // Apply rotations
    float new_pitch = current_pitch + y;
    float new_yaw = current_yaw + x;

    // Clamp pitch to prevent going through poles
    new_pitch = std::max(max_tolerance, std::min(XM_PI - max_tolerance, new_pitch));

    // Convert back to Cartesian coordinates
    float sin_pitch = std::sin(new_pitch);
    float cos_pitch = std::cos(new_pitch);
    float sin_yaw = std::sin(new_yaw);
    float cos_yaw = std::cos(new_yaw);

    XMFLOAT3 new_offset{
        radius * sin_pitch * sin_yaw,
        radius * cos_pitch,
        radius * sin_pitch * cos_yaw
    };

    XMVECTOR new_pos = XMVectorAdd(target, XMLoadFloat3(&new_offset));
    XMStoreFloat3(&m_transform->translation, new_pos);

    // Make camera look at target with world-up
    m_transform->look_at(m_target_position, {0.f, 1.f, 0.f});
}
