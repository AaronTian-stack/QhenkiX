#include "basis.h"

Basis Basis::identity()
{
	return {};
}

void Basis::orthonormalize()
{
    // Columns
    XMVECTOR x = XMVectorSet(basis_._11, basis_._21, basis_._31, 0.0f);
    XMVECTOR y = XMVectorSet(basis_._12, basis_._22, basis_._32, 0.0f);
    XMVECTOR z = XMVectorSet(basis_._13, basis_._23, basis_._33, 0.0f);

    // Gram Schmidt
    x = XMVector3Normalize(x);
    
    {
        XMVECTOR x_dot_y = XMVector3Dot(x, y);
        y = XMVectorSubtract(y, XMVectorScale(x, XMVectorGetX(x_dot_y)));
        y = XMVector3Normalize(y);
    }
    
    {
        XMVECTOR x_dot_z = XMVector3Dot(x, z);
        XMVECTOR y_dot_z = XMVector3Dot(y, z);
        z = XMVectorSubtract(z, XMVectorScale(x, XMVectorGetX(x_dot_z)));
        z = XMVectorSubtract(z, XMVectorScale(y, XMVectorGetX(y_dot_z)));
        z = XMVector3Normalize(z);
    }

	XMStoreFloat3x3(&basis_, XMMatrixTranspose(XMMATRIX(x, y, z, XMVectorZero())));
}

Basis Basis::orthonormalized() const
{
	Basis result = *this;
	result.orthonormalize();
	return result;
}

Basis& Basis::rotate_axis(const XMFLOAT3& axis, float angle)
{
	XMMATRIX rotation = XMMatrixRotationAxis(XMLoadFloat3(&axis), angle);
	XMStoreFloat3x3(&basis_, XMMatrixMultiply(XMLoadFloat3x3(&basis_), rotation));
	return *this;
}

Basis& Basis::rotate_local(const XMFLOAT3& axis, float angle)
{
	XMVECTOR world_axis = XMVector3TransformNormal(XMLoadFloat3(&axis), XMLoadFloat3x3(&basis_));
	XMMATRIX rotation = XMMatrixRotationAxis(world_axis, angle);
	XMStoreFloat3x3(&basis_, XMMatrixMultiply(XMLoadFloat3x3(&basis_), rotation));
	return *this;
}

Basis Basis::operator*(const Basis& rhs) const
{
	XMMATRIX result = XMMatrixMultiply(XMLoadFloat3x3(&basis_), XMLoadFloat3x3(&rhs.basis_));
	Basis basis;
	XMStoreFloat3x3(&basis.basis_, result);
	return basis;
}

Basis& Basis::operator*=(const Basis& rhs)
{
	XMMATRIX result = XMMatrixMultiply(XMLoadFloat3x3(&basis_), XMLoadFloat3x3(&rhs.basis_));
	XMStoreFloat3x3(&basis_, result);
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
	XMMatrixDecompose(&scale, &quat, &trans, XMLoadFloat3x3(&basis_)); // SRT assumption
	XMFLOAT4 ret;
	XMStoreFloat4(&ret, quat);
	return ret;
}

Basis& Basis::look_to(const XMFLOAT3& p, const XMFLOAT3& up)
{
	XMStoreFloat3x3(&basis_,XMMatrixLookToLH(XMVectorZero(), XMLoadFloat3(&p), XMLoadFloat3(&up)));
	return *this;
}

XMMATRIX Basis::invert()
{
	XMMATRIX m = XMLoadFloat3x3(&basis_);
	m = XMMatrixInverse(nullptr, m);
	XMStoreFloat3x3(&basis_, m);
	return m;
}

XMMATRIX Basis::transpose()
{
	XMMATRIX m = XMLoadFloat3x3(&basis_);
	m = XMMatrixTranspose(m);
	XMStoreFloat3x3(&basis_, m);
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
