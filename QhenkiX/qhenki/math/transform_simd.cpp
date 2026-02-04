#include "qhenki/math/transform_simd.h"

using namespace qhenki::math;

TransformSIMD TransformSIMD::load(const Transform& t)
{
    TransformSIMD out;
    out.scale = XMLoadFloat3(&t.scale);
    out.rotation = XMLoadFloat4(&t.rotation);
    out.translation = XMLoadFloat3(&t.translation);
    return out;
}

void TransformSIMD::store(Transform& t) const
{
    XMStoreFloat3(&t.scale, scale);
    XMStoreFloat4(&t.rotation, rotation);
    XMStoreFloat3(&t.translation, translation);
}

XMMATRIX TransformSIMD::to_matrix() const
{
    const XMMATRIX s = XMMatrixScalingFromVector(scale);
    const XMMATRIX r = XMMatrixRotationQuaternion(rotation);
    const XMMATRIX t = XMMatrixTranslationFromVector(translation);
    return XMMatrixMultiply(XMMatrixMultiply(s, r), t);
}

TransformSIMD TransformSIMD::operator*(const TransformSIMD& rhs) const
{
    TransformSIMD result;
    result.rotation = XMQuaternionMultiply(rhs.rotation, rotation);
    result.scale = scale * rhs.scale;
    result.translation = XMVector3Rotate(rhs.translation * scale, rotation) + translation;
    return result;
}
