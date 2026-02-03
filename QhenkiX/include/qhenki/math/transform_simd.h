#pragma once

#include <DirectXMath.h>

#include "transform.h"

using namespace DirectX;

namespace qhenki::math
{
// For chaining transforms in SIMD registers
struct TransformSIMD
{
    XMVECTOR scale;
    XMVECTOR rotation; // Quaternion (x, y, z, w)
    XMVECTOR translation;

    static TransformSIMD load(const Transform& t);
    void store(Transform& t) const;

    XMMATRIX to_matrix() const;

    TransformSIMD operator*(const TransformSIMD& rhs) const;
};
} // namespace qhenki::math
