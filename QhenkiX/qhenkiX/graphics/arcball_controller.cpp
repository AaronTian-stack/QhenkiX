#include <cassert>
#include <cmath>
#include <algorithm>

#include "arcball_controller.h"
#include <DirectXMath.h>

using namespace DirectX;
using namespace qhenki;

void ArcBallController::set_camera(Transform* transform)
{
    assert(transform);
    m_transform_ = transform;

    // Set camera to be at target distance
    auto diff = XMLoadFloat3(&m_target_position_) - XMLoadFloat3(&transform->translation_);
    auto dist = XMVector3Length(diff);
    float len = XMVectorGetX(dist);

    auto cam_pos = XMLoadFloat3(&transform->translation_);
    if (std::abs(len) < 1e-6f) // Camera is at target position
    {
		// Just use the camera's forward vector
		const auto axis_z = transform->basis_.axis_z();
		XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&axis_z)) * -m_target_distance_; // Negative move backwards
		XMStoreFloat3(&transform->translation_, cam_pos + forward);
	}
	else
	{
        // Set camera to look at target position
        transform->look_at(m_target_position_, { 0.f, 1.f, 0.f }); // TODO: check up vector
        diff = XMVector3Normalize(diff);
        XMStoreFloat3(&transform->translation_, cam_pos + diff * (m_target_distance_ - len));
    }
}

void ArcBallController::set_target_distance(float distance)
{
    distance = std::max(distance, 0.01f);
	assert(m_transform_);
	m_target_distance_ = distance;
	// Set camera to be at target distance
	auto diff = XMLoadFloat3(&m_transform_->translation_) - XMLoadFloat3(&m_target_position_);
	float len = XMVectorGetX(XMVector3Length(diff));
	//if (std::abs(target_distance - len) < 1e-6f) // Camera is at target position
	//	return;
	auto cam_pos = XMLoadFloat3(&m_transform_->translation_);
	diff = XMVector3Normalize(diff);
	XMStoreFloat3(&m_transform_->translation_, cam_pos + diff * (m_target_distance_ - len));
}

void ArcBallController::translate(float x, float y)
{
    assert(m_transform_);
    m_transform_->translate_local({ x, y, 0.f });
	auto t = m_transform_->transform_vector({ x, y, 0.f });
	XMStoreFloat3(&m_target_position_, XMVectorAdd(XMLoadFloat3(&m_target_position_), t));
}

const float max_tolerance = 0.00174533f; // .01 degree

void ArcBallController::rotate(float x, float y)
{
    assert(m_transform_);

	XMVECTOR v = XMVector3AngleBetweenVectors(
		XMLoadFloat3(&m_target_position_) - XMLoadFloat3(&m_transform_->translation_),
		XMVectorSet(0.f, 1.f, 0.f, 0.f)
	);
	auto angle = XMVectorGetX(v);

	if (angle + y - max_tolerance < 0)
	{
		y = max_tolerance - angle;
	}
	if (angle + y + max_tolerance > XM_PI)
	{
		y = XM_PI - angle - max_tolerance;
	}

    // Y is amount to rotate around local X axis as if it were a global axis
    m_transform_->rotate_around(m_target_position_, m_transform_->basis_.axis_x(), y);
    // X is amount to rotate around global Y axis (ensures stable rotation)
    m_transform_->rotate_around(m_target_position_, { 0.f, 1.f, 0.f }, x);
}
