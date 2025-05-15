#pragma once
#include "basis.h"

struct Transform
{
	Basis basis_;
	XMFLOAT3 translation_;

	// Assumes basis is orthonormalized (faster)
	void invert();
	Transform invert() const;

	// Can handle scale
	void affine_invert();
	Transform affine_invert() const;

	XMMATRIX to_matrix_simd() const;
	XMFLOAT4X4 to_matrix() const;

	void translate_local(const XMFLOAT3& t);
	void translate_global(const XMFLOAT3& t);

	// TODO: operators

	Transform() : basis_(Basis::identity()), translation_(0.f, 0.f, 0.f) {}
	Transform(const XMFLOAT3& translation) : basis_(Basis::identity()), translation_(translation) {}
	Transform(const Basis& basis, const XMFLOAT3& translation) : basis_(basis), translation_(translation) {}
};
