#pragma once

#include "qhenki/utility/directxmath_compat.h"

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

    /**
     * Multiplies two transforms together (rhs * lhs).
     * @param rhs Right hand side transform
     * @return rhs * this
     */
    TransformSIMD operator*(const TransformSIMD& rhs) const;
};
} // namespace qhenki::math
