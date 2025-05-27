#pragma once
#include <DirectXMath.h>

using namespace DirectX;

namespace qhenki
{
	struct Basis
	{
		XMFLOAT3X3 basis_;

		Basis() : basis_(
			1.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 1.0f) {
		}
		explicit Basis(const XMFLOAT3& x, const XMFLOAT3& y, const XMFLOAT3& z) : basis_(
			x.x, y.x, z.x,
			x.y, y.y, z.y,
			x.z, y.z, z.z) {
		}
		explicit Basis(const XMFLOAT3X3& basis) : basis_(basis) {}
		explicit Basis(const XMFLOAT3& axis, float angle);
		static Basis identity();

		XMFLOAT3 axis_x() const { return { basis_._11, basis_._12, basis_._13 }; }
		XMFLOAT3 axis_y() const { return { basis_._21, basis_._22, basis_._23 }; }
		XMFLOAT3 axis_z() const { return { basis_._31, basis_._32, basis_._33 }; }

		void orthonormalize();
		Basis orthonormalized() const;

		void normalize();
		Basis normalized() const;

		/**
		 * Rotate basis around global axis.
		 * @param axis The axis to rotate around. Does not need to be normalized.
		 * @param angle The angle in radians.
		 * @return A reference to this basis.
		 */
		Basis& rotate_axis(const XMFLOAT3& axis, float angle);
		/**
		 * Rotate basis around local axis.
		 * @param axis Local axis relative to basis frame of reference to rotate around. Does not need to be normalized.
		 * @param angle The angle in radians.
		 * @return A reference to this basis.
		 */
		Basis& rotate_local(const XMFLOAT3& axis, float angle);

		Basis operator*(const Basis& rhs) const;
		Basis& operator*=(const Basis& rhs);

		// Orthonormalizes before computing
		XMFLOAT4 to_rotation_quaternion() const;

		// Does not orthonormalize before computing so may be incorrect
		XMFLOAT4 to_quaternion() const;
		// TODO: euler angles

		Basis& look_to(const XMFLOAT3& p, const XMFLOAT3& up);

		XMMATRIX to_matrix() const;

		XMMATRIX invert();
		XMMATRIX transpose();
		Basis invert() const;
		Basis transpose() const;
	};
}
