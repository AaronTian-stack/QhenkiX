#include "basis.h"

using namespace qhenki;

Basis::Basis(const XMFLOAT3& axis, float angle)
{
	XMMATRIX rotation = XMMatrixRotationAxis(XMLoadFloat3(&axis), angle);
	XMStoreFloat3x3(&basis, rotation);
}

Basis Basis::identity()
{
	return {};
}

void Basis::orthonormalize()
{
    // Columns
    XMVECTOR x = XMVectorSet(basis._11, basis._21, basis._31, 0.0f);
    XMVECTOR y = XMVectorSet(basis._12, basis._22, basis._32, 0.0f);
    XMVECTOR z = XMVectorSet(basis._13, basis._23, basis._33, 0.0f);

    // Gram Schmidt
    x = XMVector3Normalize(x);
    
    {
        XMVECTOR x_dot_y = XMVector3Dot(x, y);
        y -= x * XMVectorGetX(x_dot_y);
        y = XMVector3Normalize(y);
    }
    
    {
        XMVECTOR x_dot_z = XMVector3Dot(x, z);
        XMVECTOR y_dot_z = XMVector3Dot(y, z);
        z = XMVectorSubtract(z, x * XMVectorGetX(x_dot_z));
        z = XMVectorSubtract(z, y * XMVectorGetX(y_dot_z));
        z = XMVector3Normalize(z);
    }

	XMStoreFloat3x3(&basis, XMMatrixTranspose({ x, y, z, XMVectorZero() }));
}

Basis Basis::orthonormalized() const
{
	Basis result = *this;
	result.orthonormalize();
	return result;
}

void Basis::normalize()
{
	XMVECTOR x = XMVectorSet(basis._11, basis._21, basis._31, 0.0f);
	XMVECTOR y = XMVectorSet(basis._12, basis._22, basis._32, 0.0f);
	XMVECTOR z = XMVectorSet(basis._13, basis._23, basis._33, 0.0f);
	x = XMVector3Normalize(x);
	y = XMVector3Normalize(y);
	z = XMVector3Normalize(z);
	XMStoreFloat3x3(&basis, XMMatrixTranspose({ x, y, z, XMVectorZero() }));
}

Basis Basis::normalized() const
{
	Basis result = *this;
	result.normalize();
	return result;
}

Basis& Basis::rotate_axis(const XMFLOAT3& axis, const float angle)
{
	XMMATRIX rotation = XMMatrixRotationAxis(XMLoadFloat3(&axis), angle);
	XMStoreFloat3x3(&basis, XMMatrixMultiply(XMLoadFloat3x3(&basis), rotation));
	return *this;
}

Basis& Basis::rotate_local(const XMFLOAT3& axis, const float angle)
{
	XMVECTOR world_axis = XMVector3TransformNormal(XMLoadFloat3(&axis), XMLoadFloat3x3(&basis));
	XMMATRIX rotation = XMMatrixRotationAxis(world_axis, angle);
	XMStoreFloat3x3(&basis, XMMatrixMultiply(XMLoadFloat3x3(&basis), rotation));
	return *this;
}

Basis Basis::operator*(const Basis& rhs) const
{
	XMMATRIX result = XMMatrixMultiply(XMLoadFloat3x3(&basis), XMLoadFloat3x3(&rhs.basis));
	Basis basis;
	XMStoreFloat3x3(&basis.basis, result);
	return basis;
}

Basis& Basis::operator*=(const Basis& rhs)
{
	XMMATRIX result = XMMatrixMultiply(XMLoadFloat3x3(&basis), XMLoadFloat3x3(&rhs.basis));
	XMStoreFloat3x3(&basis, result);
	return *this;
}

XMFLOAT4 Basis::to_rotation_quaternion() const
{
	auto m = orthonormalized();
	return m.to_quaternion();
}

XMFLOAT4 Basis::to_quaternion() const
{
	XMVECTOR scale, quat, trans; // trans not used since loading 3x3 matrix
	XMMatrixDecompose(&scale, &quat, &trans, XMLoadFloat3x3(&basis)); // SRT assumption
	XMFLOAT4 ret;
	XMStoreFloat4(&ret, quat);
	return ret;
}

Basis& Basis::look_to(const XMFLOAT3& p, const XMFLOAT3& up)
{
	auto z = XMVector3Normalize(XMLoadFloat3(&p));
	auto y = XMVector3Normalize(XMLoadFloat3(&up));
	auto x = XMVector3Cross(z, y);
	y = XMVector3Cross(x, z);
	XMStoreFloat3x3(&basis, XMMatrixTranspose({ x, y, z, XMVectorZero() }));
	return *this;
}

XMMATRIX Basis::to_matrix() const
{
	return XMLoadFloat3x3(&basis);
}

XMMATRIX Basis::invert()
{
	XMMATRIX m = XMLoadFloat3x3(&basis);
	m = XMMatrixInverse(nullptr, m);
	XMStoreFloat3x3(&basis, m);
	return m;
}

XMMATRIX Basis::transpose()
{
	XMMATRIX m = XMLoadFloat3x3(&basis);
	m = XMMatrixTranspose(m);
	XMStoreFloat3x3(&basis, m);
	return m;
}

Basis Basis::invert() const
{
	Basis result = *this;
	result.invert();
	return result;
}

Basis Basis::transpose() const
{
	Basis result = *this;
	result.transpose();
	return result;
}
