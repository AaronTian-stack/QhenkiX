#include "transform.h"

using namespace qhenki;

void Transform::invert()
{
	const auto in = basis_.transpose();
	const auto translation = XMLoadFloat3(&translation_) * -1.f; // -(R transpose = R inverse) * t
	XMStoreFloat3(&translation_, XMVector3Transform(translation, in));
}

Transform Transform::invert() const
{
	Transform result = *this;
	result.invert();
	return result;
}

void Transform::affine_invert()
{
	const auto in = basis_.invert();
	const auto translation = XMLoadFloat3(&translation_) * -1.f;
	XMStoreFloat3(&translation_, XMVector3Transform(translation, in)); // -(R inverse) * t
}

Transform Transform::affine_invert() const
{
	Transform result = *this;
	result.affine_invert();
	return result;
}

XMMATRIX Transform::to_matrix_simd() const
{
	auto eye = XMLoadFloat3(&translation_);
	auto axis_y = basis_.axis_y();
	auto up = XMLoadFloat3(&axis_y);

	auto axis_z = basis_.axis_z();
	auto forward = XMLoadFloat3(&axis_z);

	return XMMatrixLookToLH(eye, forward, up);
}

XMFLOAT4X4 Transform::to_matrix() const
{
	XMMATRIX m = to_matrix_simd();
	XMFLOAT4X4 result;
	XMStoreFloat4x4(&result, m);
	return result;
}

XMVECTOR Transform::inverse_transform_direction(const XMFLOAT3& d) const
{
	Basis no_scale = basis_.orthonormalized();
	Transform inv(no_scale, translation_);
	return inv.inverse_transform_vector(d);
}

XMVECTOR Transform::inverse_transform_point(const XMFLOAT3& p) const
{
	auto v = XMLoadFloat3(&p) - XMLoadFloat3(&translation_);
	XMFLOAT3 result;
	XMStoreFloat3(&result, v); // TODO: redundant load/store
	return inverse_transform_vector(result);
}

XMVECTOR Transform::inverse_transform_vector(const XMFLOAT3& v) const
{
	Transform inv = affine_invert();
	return inv * v;
}

Transform& Transform::look_at(const XMFLOAT3& p, const XMFLOAT3& up)
{
	XMFLOAT3 diff;
	XMStoreFloat3(&diff, XMLoadFloat3(&p) - XMLoadFloat3(&translation_));
	basis_.look_to(p, up);
	return *this;
}

void Transform::rotate_around(const XMFLOAT3& pivot, const XMFLOAT3& global_axis, float angle)
{
	XMVECTOR pivot_vec = XMLoadFloat3(&pivot);
	XMVECTOR axis_vec = XMLoadFloat3(&global_axis);
	XMVECTOR translation_vec = XMLoadFloat3(&translation_);
	// Translate to origin
	translation_vec -= pivot_vec;
	// Rotate
	XMMATRIX rotation = XMMatrixRotationAxis(axis_vec, angle);
	translation_vec = XMVector3Transform(translation_vec, rotation);
	// Translate back
	translation_vec += pivot_vec;
	XMStoreFloat3(&translation_, translation_vec);
	basis_.rotate_axis(global_axis, angle);
}

XMVECTOR Transform::transform_direction(const XMFLOAT3& d) const
{
	Basis no_scale = basis_.orthonormalized();
	Transform t(no_scale, translation_);
	return t.transform_vector(d);
}

XMVECTOR Transform::transform_point(const XMFLOAT3& p) const
{
	XMVECTOR v = XMLoadFloat3(&p) - XMLoadFloat3(&translation_);
	XMMATRIX m = basis_.to_matrix();
	return XMVector3Transform(v, m);
}

XMVECTOR Transform::transform_vector(const XMFLOAT3& v) const
{
	XMVECTOR vec = XMLoadFloat3(&v);
	XMMATRIX m = basis_.to_matrix();
	return XMVector3TransformNormal(vec, m);
}

void Transform::translate_local(const XMFLOAT3& t)
{
	XMVECTOR offset = XMVector3TransformNormal(XMLoadFloat3(&t), XMLoadFloat3x3(&basis_.basis_));
	XMStoreFloat3(&translation_, offset + XMLoadFloat3(&translation_));
}

void Transform::translate_global(const XMFLOAT3& t)
{
	XMStoreFloat3(&translation_, XMLoadFloat3(&translation_) + XMLoadFloat3(&t));
}

XMVECTOR Transform::operator*(const XMFLOAT3& rhs) const
{
	return XMVector3Transform(XMLoadFloat3(&rhs), to_matrix_simd());
}

Transform Transform::operator*(const Transform& rhs) const
{
	Transform result = *this;
	result *= rhs;
	return result;
}

Transform& Transform::operator*=(const Transform& rhs)
{
	// d2
	XMVECTOR d2 = XMLoadFloat3(&rhs.translation_);
	// R1
	XMMATRIX r1 = XMLoadFloat3x3(&basis_.basis_);
	// Multiply basis together R1 * R2 = RESULT BASIS
	XMMATRIX new_basis = XMMatrixMultiply(r1, rhs.basis_.to_matrix());
	// R1 * d2
	XMVECTOR r1d2 = XMVector3Transform(d2, r1);
	// R1 * d2 + d1 = RESULT POSITION
	XMVECTOR r1d2pd1 = r1d2 + XMLoadFloat3(&translation_);

	XMStoreFloat3x3(&basis_.basis_, new_basis);
	XMStoreFloat3(&translation_, r1d2pd1);

	return *this;
}
