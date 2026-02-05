#include "qhenki/math/transform.h"
#include "qhenki/math/transform_simd.h"

using namespace qhenki::math;

void Transform::invert()
{
    const XMVECTOR inv_rot = XMQuaternionInverse(XMLoadFloat4(&rotation));
    const XMVECTOR inv_trans = XMVector3Rotate(XMLoadFloat3(&translation) * -1.0f, inv_rot);
    XMStoreFloat4(&rotation, inv_rot);
    XMStoreFloat3(&translation, inv_trans);
}

Transform Transform::invert() const
{
    Transform result = *this;
    result.invert();
    return result;
}

void Transform::affine_invert()
{
    const XMVECTOR s = XMLoadFloat3(&scale);
    const XMVECTOR inv_scale = XMVectorReciprocal(s);
    const XMVECTOR inv_rot = XMQuaternionInverse(XMLoadFloat4(&rotation));
    const XMVECTOR inv_trans = XMVector3Rotate(XMLoadFloat3(&translation), inv_rot) * -1.0f * inv_scale;
    XMStoreFloat3(&scale, inv_scale);
    XMStoreFloat4(&rotation, inv_rot);
    XMStoreFloat3(&translation, inv_trans);
}

Transform Transform::affine_invert() const
{
    Transform result = *this;
    result.affine_invert();
    return result;
}

XMFLOAT4X4 Transform::to_matrix() const
{
    const XMMATRIX m = TransformSIMD::load(*this).to_matrix();
    XMFLOAT4X4 result;
    XMStoreFloat4x4(&result, m);
    return result;
}

XMVECTOR Transform::inverse_transform_direction(const XMFLOAT3& d) const
{
    return inverse_transform_direction(XMLoadFloat3(&d));
}

XMVECTOR Transform::inverse_transform_direction(const XMVECTOR d) const
{
    return XMVector3Rotate(d, XMQuaternionInverse(XMLoadFloat4(&rotation)));
}

XMVECTOR Transform::inverse_transform_point(const XMFLOAT3& p) const
{
    return inverse_transform_point(XMLoadFloat3(&p));
}

XMVECTOR Transform::inverse_transform_point(const XMVECTOR p) const
{
    XMVECTOR v = p - XMLoadFloat3(&translation);
    v = XMVector3Rotate(v, XMQuaternionInverse(XMLoadFloat4(&rotation)));
    v = v * XMVectorReciprocal(XMLoadFloat3(&scale));
    return v;
}

XMVECTOR Transform::inverse_transform_vector(const XMFLOAT3& v) const
{
    return inverse_transform_vector(XMLoadFloat3(&v));
}

XMVECTOR Transform::inverse_transform_vector(const XMVECTOR v) const
{
    XMVECTOR result = XMVector3Rotate(v, XMQuaternionInverse(XMLoadFloat4(&rotation)));
    result = result * XMVectorReciprocal(XMLoadFloat3(&scale));
    return result;
}

Transform& Transform::look_at(const XMFLOAT3& p, const XMFLOAT3& up)
{
    return look_at(XMLoadFloat3(&p), XMLoadFloat3(&up));
}

Transform& Transform::look_at(const XMVECTOR p, const XMVECTOR up)
{
    const XMVECTOR pos = XMLoadFloat3(&translation);
    const XMVECTOR forward = XMVector3Normalize(p - pos);
    const XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));
    const XMVECTOR actual_up = XMVector3Cross(forward, right);

    // Build rotation matrix from basis vectors (rows are axes for row-major)
    const XMMATRIX rot_matrix = XMMATRIX(right,
                                         actual_up,
                                         forward,
                                         g_XMIdentityR3 // (0, 0, 0, 1)
    );
    XMStoreFloat4(&rotation, XMQuaternionRotationMatrix(rot_matrix));
    return *this;
}

XMVECTOR Transform::transform_direction(const XMFLOAT3& d) const
{
    return transform_direction(XMLoadFloat3(&d));
}

XMVECTOR Transform::transform_direction(const XMVECTOR d) const
{
    return XMVector3Rotate(d, XMLoadFloat4(&rotation));
}

XMVECTOR Transform::transform_point(const XMFLOAT3& p) const
{
    return transform_point(XMLoadFloat3(&p));
}

XMVECTOR Transform::transform_point(const XMVECTOR p) const
{
    XMVECTOR v = p * XMLoadFloat3(&scale);
    v = XMVector3Rotate(v, XMLoadFloat4(&rotation));
    v = v + XMLoadFloat3(&translation);
    return v;
}

XMVECTOR Transform::transform_vector(const XMFLOAT3& v) const
{
    return transform_vector(XMLoadFloat3(&v));
}

XMVECTOR Transform::transform_vector(const XMVECTOR v) const
{
    XMVECTOR result = XMVector3Rotate(v, XMLoadFloat4(&rotation));
    result = result * XMLoadFloat3(&scale);
    return result;
}

void Transform::translate_local(const XMFLOAT3& t)
{
    translate_local(XMLoadFloat3(&t));
}

void Transform::translate_local(const XMVECTOR t)
{
    const XMVECTOR offset = XMVector3Rotate(t, XMLoadFloat4(&rotation));
    XMStoreFloat3(&translation, XMLoadFloat3(&translation) + offset);
}

void Transform::translate_global(const XMFLOAT3& t)
{
    translate_global(XMLoadFloat3(&t));
}

void Transform::translate_global(const XMVECTOR t)
{
    XMStoreFloat3(&translation, XMLoadFloat3(&translation) + t);
}

XMVECTOR Transform::operator*(const XMFLOAT3& rhs) const
{
    return operator*(XMLoadFloat3(&rhs));
}

XMVECTOR Transform::operator*(const XMVECTOR rhs) const
{
    return XMVector3Transform(rhs, TransformSIMD::load(*this).to_matrix());
}

#define AXIS(V, ROTQUAT) return XMVector3Rotate(V, ROTQUAT)

XMVECTOR Transform::axis_x() const
{
    AXIS(g_XMIdentityR0, XMLoadFloat4(&rotation));
}

XMVECTOR Transform::axis_y() const
{
    AXIS(g_XMIdentityR1, XMLoadFloat4(&rotation));
}

XMVECTOR Transform::axis_z() const
{
    AXIS(g_XMIdentityR2, XMLoadFloat4(&rotation));
}

Transform::Transform()
    : scale(identity_scale()),
      rotation(identity_rotation()),
      translation(0.f, 0.f, 0.f)
{
}

Transform::Transform(const XMFLOAT3& translation)
    : scale(identity_scale()),
      rotation(identity_rotation()),
      translation(translation)
{
}

Transform::Transform(const XMFLOAT4& rotation, const XMFLOAT3& translation)
    : scale(identity_scale()),
      rotation(rotation),
      translation(translation)
{
}

Transform::Transform(const XMFLOAT3& scale, const XMFLOAT4& rotation, const XMFLOAT3& translation)
    : scale(scale),
      rotation(rotation),
      translation(translation)
{
}

XMVECTOR qhenki::math::axis_x(const XMVECTOR quat)
{
    AXIS(g_XMIdentityR0, quat);
}

XMVECTOR qhenki::math::axis_y(const XMVECTOR quat)
{
    AXIS(g_XMIdentityR1, quat);
}

XMVECTOR qhenki::math::axis_z(const XMVECTOR quat)
{
    AXIS(g_XMIdentityR2, quat);
}
