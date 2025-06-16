#pragma once
#include "basis.h"

namespace qhenki
{
	struct TransformSIMD
	{
		XMMATRIX basis_;
		XMVECTOR translation_;
	};

	class Transform
	{
		// Assumes basis is orthonormalized (faster)
		void invert();
		Transform invert() const;

		// Can handle scale
		void affine_invert();
		Transform affine_invert() const;

	public:
		Basis basis_;
		XMFLOAT3 translation_;

		XMMATRIX to_matrix_simd() const;
		XMFLOAT4X4 to_matrix() const;

		// inverse transform direction (no scale)
		/**
		 * Multiplies inverse of transform with direction, does not consider scale.
		 * @param d Direction to transform
		 * @return 
		 */
		XMVECTOR inverse_transform_direction(const XMFLOAT3& d) const;
		/**
		 * Multiplies inverse of transform with point, considers scale.
		 * @param p Point to transform
		 * @return 
		 */
		XMVECTOR inverse_transform_point(const XMFLOAT3& p) const;
		/**
		 * Multiplies inverse of transform with vector, considers scale.
		 * @param v Vector to transform
		 * @return 
		 */
		XMVECTOR inverse_transform_vector(const XMFLOAT3& v) const;

		Transform& look_at(const XMFLOAT3& p, const XMFLOAT3& up);

		// void rotate_around_local(const XMFLOAT3& pivot, const XMFLOAT3& local_axis, float angle);
		void rotate_around(const XMFLOAT3& pivot, const XMFLOAT3& global_axis, float angle);

		XMVECTOR transform_direction(const XMFLOAT3& d) const;
		XMVECTOR transform_point(const XMFLOAT3& p) const;
		XMVECTOR transform_vector(const XMFLOAT3& v) const;

		void translate_local(const XMFLOAT3& t);
		void translate_global(const XMFLOAT3& t);

		XMVECTOR operator*(const XMFLOAT3& rhs) const;
		Transform operator*(const Transform& rhs) const;
		Transform& operator*=(const Transform& rhs);

		Transform() : basis_(Basis::identity()), translation_(0.f, 0.f, 0.f) {}
		Transform(const XMFLOAT3& translation) : basis_(Basis::identity()), translation_(translation) {}
		Transform(const Basis& basis, const XMFLOAT3& translation) : basis_(basis), translation_(translation) {}
	};
}
