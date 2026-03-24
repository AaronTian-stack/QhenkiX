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
    const auto s = XMMatrixScalingFromVector(scale);
    const auto r = XMMatrixRotationQuaternion(rotation);
    const auto t = XMMatrixTranslationFromVector(translation);
    return s * r * t;
}

TransformSIMD TransformSIMD::operator*(const TransformSIMD& rhs) const
{
    TransformSIMD result;
    result.rotation = XMQuaternionMultiply(rotation, rhs.rotation);
    result.scale = XMVectorMultiply(rhs.scale, scale);
    result.translation = XMVectorAdd(XMVector3Rotate(XMVectorMultiply(translation, rhs.scale), rhs.rotation),
                                     rhs.translation);
    return result;
}
