/**
 * \file Matrix3D.cpp
 * \brief Matrix3D represents a 3x3 square matrix.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "Matrix3D.h"

namespace Seoul
{

/**
 * Helper function, used by Matrix3D::QRDecomposition.
 * Projection of e onto a.
 *
 * \sa http://en.wikipedia.org/wiki/QR_decomposition
 */
inline Vector3D InternalInlineGramSchmidtProjectionOperator(
	const Vector3D& e,
	const Vector3D& a)
{
	return ((Vector3D::Dot(e, a) / Vector3D::Dot(e, e)) * e);
}

/**
 * Decompose the matrix mTransform into QR, where
 * Q is an orthonormal transform and R is an upper-triangular transform.
 *
 * @return True if the matrix was successfully decomposed, false otherwise.
 *
 * Uses a modified version of Gram-Schmidt for numerical stability.
 *
 * \sa http://en.wikipedia.org/wiki/QR_decomposition
 * \sa http://en.wikipedia.org/wiki/Gram-Schmidt#Numerical_stability
 */
Bool Matrix3D::QRDecomposition(
	const Matrix3D& mTransform,
	Matrix3D& rmQ,
	Matrix3D& rmR)
{
	const Vector3D a1 = mTransform.GetColumn((Int)Axis::kX);
	const Vector3D a2 = mTransform.GetColumn((Int)Axis::kY);
	const Vector3D a3 = mTransform.GetColumn((Int)Axis::kZ);

	// Column 1
	Vector3D e1 = a1;
	if (!e1.Normalize())
	{
		return false;
	}

	// Column 2
	Vector3D e2 = a2 - InternalInlineGramSchmidtProjectionOperator(e1, a2);
	if (!e2.Normalize())
	{
		return false;
	}

	// Column 3
	Vector3D e3 = a3 - InternalInlineGramSchmidtProjectionOperator(e1, a3);

	// This is the modification vs. standard Gram-Schmidt, we use
	// e3 instead of a3 for the second factor of the third column calculation.
	e3 = e3 - InternalInlineGramSchmidtProjectionOperator(e2, e3);
	if (!e3.Normalize())
	{
		return false;
	}

	// Set Q, the orthonormal part
	rmQ.SetColumn((Int)Axis::kX, e1);
	rmQ.SetColumn((Int)Axis::kY, e2);
	rmQ.SetColumn((Int)Axis::kZ, e3);

	// Set R, the scaling part.
	rmR = Matrix3D(
		Vector3D::Dot(e1, a1), Vector3D::Dot(e1, a2), Vector3D::Dot(e1, a3),
		0.0f,                  Vector3D::Dot(e2, a2), Vector3D::Dot(e2, a3),
		0.0f,                  0.0f,                  Vector3D::Dot(e3, a3));

	// Only do this in debug, too slow for the developer build.
#if SEOUL_DEBUG
	// Tolerance constants here were empirically derived.
	SEOUL_ASSERT(rmQ.IsOrthonormal(1e-4f));
	SEOUL_ASSERT((rmQ * rmR).Equals(mTransform, 1e-4f));
#endif

	return true;
}

/*
 * Returns a rotation transform which will rotate points
 * by fAngleInRadians around the x axis.
 */
Matrix3D Matrix3D::CreateRotationX(Float fAngleInRadians)
{
	Float cos = Cos(fAngleInRadians);
	Float sin = Sin(fAngleInRadians);

	return Matrix3D(
		1.0f, 0.0f, 0.0f,
		0.0f,  cos, -sin,
		0.0f,  sin,  cos);
}

/**
 * Returns a rotation transform which will rotate points
 * by fAngleInRadians around the y axis.
 */
Matrix3D Matrix3D::CreateRotationY(Float fAngleInRadians)
{
	Float cos = Cos(fAngleInRadians);
	Float sin = Sin(fAngleInRadians);

	return Matrix3D(
		cos,  0.0f,  sin,
		0.0f, 1.0f, 0.0f,
		-sin, 0.0f,  cos);
}

/**
 * Returns a rotation transform which will rotate points
 * by fAngleInRadians around the z axis.
 */
Matrix3D Matrix3D::CreateRotationZ(Float fAngleInRadians)
{
	Float cos = Cos(fAngleInRadians);
	Float sin = Sin(fAngleInRadians);

	return Matrix3D(
		cos,  -sin, 0.0f,
		sin,   cos, 0.0f,
		0.0f, 0.0f, 1.0f);
}

/**
 * Create a scaling 3x3 transform from the scaling terms
 * fX, fY, and fZ.
 */
Matrix3D Matrix3D::CreateScale(Float fX, Float fY, Float fZ)
{
	return Matrix3D(
		fX, 0.0f, 0.0f,
		0.0f, fY, 0.0f,
		0.0f, 0.0f, fZ);
}

/**
 * Create a scaling 3x3 transform from the scaling terms vScale.
 */
Matrix3D Matrix3D::CreateScale(const Vector3D& vScale)
{
	return Matrix3D(
		vScale.X, 0.0f, 0.0f,
		0.0f, vScale.Y, 0.0f,
		0.0f, 0.0f, vScale.Z);
}

} // namespace Seoul
