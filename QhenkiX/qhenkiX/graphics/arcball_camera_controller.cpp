#include <cassert>
#include <cmath>
#include <algorithm>

#include "arcball_camera_controller.h"
#include <DirectXMath.h>

using namespace DirectX;
using namespace qhenki;

void ArcBallCameraController::set_camera(Camera* camera)
{
    assert(camera);
    this->camera = camera;

    // Set camera to be at target distance
    auto diff = XMLoadFloat3(&target_position) - XMLoadFloat3(&camera->transform_.translation_);
    auto dist = XMVector3Length(diff);
    float len = XMVectorGetX(dist);

    auto cam_pos = XMLoadFloat3(&camera->transform_.translation_);
    if (std::abs(len) < 1e-6f) // Camera is at target position
    {
		// Just use the camera's forward vector
		const auto axis_z = camera->transform_.basis_.axis_z();
		XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&axis_z)) * -target_distance; // Negative move backwards
		XMStoreFloat3(&camera->transform_.translation_, cam_pos + forward);
	}
	else
	{
        // Set camera to look at target position
        camera->transform_.look_at(target_position, { 0.f, 1.f, 0.f }); // TODO: check up vector
        diff = XMVector3Normalize(diff);
        XMStoreFloat3(&camera->transform_.translation_, cam_pos + diff * (target_distance - len));
    }
}

void ArcBallCameraController::set_target_distance(float distance)
{
    distance = std::max(distance, 0.01f);
	assert(camera);
	target_distance = distance;
	// Set camera to be at target distance
	auto diff = XMLoadFloat3(&camera->transform_.translation_) - XMLoadFloat3(&target_position);
	float len = XMVectorGetX(XMVector3Length(diff));
	//if (std::abs(target_distance - len) < 1e-6f) // Camera is at target position
	//	return;
	auto cam_pos = XMLoadFloat3(&camera->transform_.translation_);
	diff = XMVector3Normalize(diff);
	XMStoreFloat3(&camera->transform_.translation_, cam_pos + diff * (target_distance - len));
}

void ArcBallCameraController::translate(float x, float y)
{
    assert(camera);
    camera->transform_.translate_local({ x, y, 0.f });
}

void ArcBallCameraController::rotate(float x, float y)
{
    assert(camera);
    // Y is amount to rotate around local X axis as if it were a global axis (ensures stable rotation)
    camera->transform_.rotate_around(target_position, camera->transform_.basis_.axis_x(), y);
    // X is amount to rotate around global Y axis
    camera->transform_.rotate_around(target_position, { 0.f, 1.f, 0.f }, x);
}
