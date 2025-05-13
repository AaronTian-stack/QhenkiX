#pragma once

#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
protected:
    // Left hand coordinate system
    XMFLOAT3 position_ = { 0, 0, 0 };
    XMFLOAT3 forward_ = { 0, 0, 1 };
    XMFLOAT3 up_ = { 0, 1, 0 };

	XMFLOAT4X4 projection_{};
	XMFLOAT4X4 view_{};
	XMFLOAT4X4 view_projection_{};
	XMFLOAT4X4 inverse_view_projection_{};

public:
	Camera() = default;
	Camera(const float vw, const float vh) : viewport_width(vw), viewport_height(vh) {}
	virtual ~Camera() = default;

	float near = 0.01f;
	float far = 100.f;

	float viewport_width = 0;
	float viewport_height = 0;

	const XMFLOAT3& position = position_;
	const XMFLOAT3& forward = forward_;
	const XMFLOAT3& up = up_;
	const XMFLOAT4X4& projection = projection_;
	const XMFLOAT4X4& view = view_;
	const XMFLOAT4X4& view_projection = view_projection_;
	const XMFLOAT4X4& inverse_view_projection = inverse_view_projection_;
	/**
	 * Updates view, projection, view_projection, inverse_view_projection
	 */
	virtual void update(bool update_frustum) = 0;
	virtual void update() { update(true); }
	void rotate(float angle, float a_x, float a_y, float a_z);
	void rotate(float angle, XMFLOAT3 axis);
	void rotate(XMFLOAT4X4 matrix);
	void rotate(XMFLOAT4 quaternion);

	void translate(float x, float y, float z);
	void translate(XMFLOAT3 t);

	void unproject(XMFLOAT3& screen, float viewport_x, float viewport_y, float viewport_width, float viewport_height);

	void project(XMFLOAT3& world, float viewport_x, float viewport_y, float viewport_width, float viewport_height);

	void look_at(float x, float y, float z);
	void look_at(XMFLOAT3 target);

	friend class Viewport;
};

