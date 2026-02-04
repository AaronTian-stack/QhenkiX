#pragma once

#include <DirectXMath.h>

using namespace DirectX;

namespace qhenki::math
{
class Transform
{
    // Assumes rotation is normalized
    void invert();
    Transform invert() const;

    // Can handle scale
    void affine_invert();
    Transform affine_invert() const;

public:
    XMFLOAT3 scale;
    XMFLOAT4 rotation; // Quaternion (x, y, z, w)
    XMFLOAT3 translation;

    XMFLOAT4X4 to_matrix() const;

    /**
     * Multiplies inverse of transform's rotation with direction. Thus does not consider translation or scale.
     * @param d Direction to transform
     * @return Transformed direction
     */
    XMVECTOR inverse_transform_direction(const XMFLOAT3& d) const;
    XMVECTOR inverse_transform_direction(XMVECTOR d) const;
    /**
     * Multiplies inverse of transform with point. Considers translation, rotation, and scale.
     * @param p Point to transform
     * @return Transformed point
     */
    XMVECTOR inverse_transform_point(const XMFLOAT3& p) const;
    XMVECTOR inverse_transform_point(XMVECTOR p) const;
    /**
     * Multiplies inverse of transform with vector. Considers rotation and scale, but not translation.
     * @param v Vector to transform
     * @return Transformed vector
     */
    XMVECTOR inverse_transform_vector(const XMFLOAT3& v) const;
    XMVECTOR inverse_transform_vector(XMVECTOR v) const;

    Transform& look_at(const XMFLOAT3& p, const XMFLOAT3& up);
    Transform& look_at(XMVECTOR p, XMVECTOR up);

    XMVECTOR transform_direction(const XMFLOAT3& d) const;
    XMVECTOR transform_direction(XMVECTOR d) const;
    XMVECTOR transform_point(const XMFLOAT3& p) const;
    XMVECTOR transform_point(XMVECTOR p) const;
    XMVECTOR transform_vector(const XMFLOAT3& v) const;
    XMVECTOR transform_vector(XMVECTOR v) const;

    void translate_local(const XMFLOAT3& t);
    void translate_local(XMVECTOR t);
    void translate_global(const XMFLOAT3& t);
    void translate_global(XMVECTOR t);

    XMVECTOR operator*(const XMFLOAT3& rhs) const;
    XMVECTOR operator*(XMVECTOR rhs) const;

    XMVECTOR axis_x() const;
    XMVECTOR axis_y() const;
    XMVECTOR axis_z() const;

    // To multiply two or more transforms together, use TransformSIMD

    static XMFLOAT3 identity_scale()
    {
        return {1.f, 1.f, 1.f};
    }
    static XMFLOAT4 identity_rotation()
    {
        return {0.f, 0.f, 0.f, 1.f};
    }

    Transform();
    Transform(const XMFLOAT3& translation);
    Transform(const XMFLOAT4& rotation, const XMFLOAT3& translation);
    Transform(const XMFLOAT3& scale, const XMFLOAT4& rotation, const XMFLOAT3& translation);
};
XMVECTOR axis_x(XMVECTOR quat);
XMVECTOR axis_y(XMVECTOR quat);
XMVECTOR axis_z(XMVECTOR quat);
} // namespace qhenki::math
