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

    // inverse transform direction (no scale)
    /**
     * Multiplies inverse of transform with direction, does not consider scale.
     * @param d Direction to transform
     * @return
     */
    XMVECTOR inverse_transform_direction(const XMFLOAT3& d) const;
    XMVECTOR inverse_transform_direction(XMVECTOR d) const;
    /**
     * Multiplies inverse of transform with point, considers scale.
     * @param p Point to transform
     * @return
     */
    XMVECTOR inverse_transform_point(const XMFLOAT3& p) const;
    XMVECTOR inverse_transform_point(XMVECTOR p) const;
    /**
     * Multiplies inverse of transform with vector, considers scale.
     * @param v Vector to transform
     * @return
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

    Transform()
        : scale(identity_scale()),
          rotation(identity_rotation()),
          translation(0.f, 0.f, 0.f)
    {
    }
    Transform(const XMFLOAT3& translation)
        : scale(identity_scale()),
          rotation(identity_rotation()),
          translation(translation)
    {
    }
    Transform(const XMFLOAT4& rotation, const XMFLOAT3& translation)
        : scale(identity_scale()),
          rotation(rotation),
          translation(translation)
    {
    }
    Transform(const XMFLOAT3& scale, const XMFLOAT4& rotation, const XMFLOAT3& translation)
        : scale(scale),
          rotation(rotation),
          translation(translation)
    {
    }
};
} // namespace qhenki::math
