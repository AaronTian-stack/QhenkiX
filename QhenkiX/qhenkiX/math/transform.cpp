#include "transform.h"

void Transform::invert()
{
	const auto in = basis_.transpose();
	const auto translation = XMVectorScale(XMLoadFloat3(&translation_), -1.f); // -(R transpose = R inverse) * t
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
	const auto translation = XMVectorScale(XMLoadFloat3(&translation_), -1.f); // -(R inverse) * t
	XMStoreFloat3(&translation_, XMVector3Transform(translation, in));
}

Transform Transform::affine_invert() const
{
	Transform result = *this;
	result.affine_invert();
	return result;
}

XMMATRIX Transform::to_matrix_simd() const
{
	XMMATRIX m = XMLoadFloat3x3(&basis_.basis_);
	m.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	XMMATRIX translate = XMMatrixTranslation(-translation_.x, -translation_.y, -translation_.z); // negate
	return XMMatrixMultiply(translate, m);
}

XMFLOAT4X4 Transform::to_matrix() const
{
	XMFLOAT4X4 result;
	XMMATRIX m = to_matrix_simd();
	XMStoreFloat4x4(&result, m);
	return result;
}

void Transform::translate_local(const XMFLOAT3& t)
{
	XMVECTOR offset = XMVector3TransformNormal(XMLoadFloat3(&t), XMLoadFloat3x3(&basis_.basis_));
	XMStoreFloat3(&translation_, XMVectorAdd(offset, XMLoadFloat3(&translation_)));
}

void Transform::translate_global(const XMFLOAT3& t)
{
	XMStoreFloat3(&translation_, XMVectorAdd(XMLoadFloat3(&translation_), XMLoadFloat3(&t)));
}
