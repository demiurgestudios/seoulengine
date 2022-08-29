/**
 * \file Matrix2D.h
 * \brief Matrix2D represents a 2x2 square m.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MATRIX2D_H
#define MATRIX2D_H

#include "Axis.h"
#include "Prereqs.h"
#include "SeoulMath.h"
#include "SeoulTypeTraits.h"
#include "Vector2D.h"
namespace Seoul { struct Matrix2x3; }

namespace Seoul
{

struct Matrix2D SEOUL_SEALED
{
	// Data in Matrix2D is stored column major.
	Float M00, M10,
		  M01, M11;

	Matrix2D()
		: M00(0.0f), M10(0.0f)
		, M01(0.0f), M11(0.0f)
	{
	}

	explicit Matrix2D(Float f)
		: M00(f), M10(f)
		, M01(f), M11(f)
	{
	}

	Matrix2D(Float m00, Float m01,
		     Float m10, Float m11)
		: M00(m00), M10(m10)
		, M01(m01), M11(m11)
	{
	}

	Matrix2D(const Matrix2D & m)
		: M00(m.M00), M10(m.M10)
		, M01(m.M01), M11(m.M11)
	{
	}

	explicit Matrix2D(const Matrix2x3& m);

	Matrix2D operator+(const Matrix2D & m) const
	{
		return Matrix2D(M00 + m.M00, M01 + m.M01,
			            M10 + m.M10, M11 + m.M11);
	}

	Matrix2D operator-(const Matrix2D & m) const
	{
		return Matrix2D(M00 - m.M00, M01 - m.M01,
			            M10 - m.M10, M11 - m.M11);
	}

	Matrix2D operator-() const
	{
		return Matrix2D(-M00, -M01,
			            -M10, -M11);
	}

	/**
	 * Performs this * mMatrix.
	 */
	Matrix2D operator*(const Matrix2D & m) const
	{
		return Matrix2D(
			M00 * m.M00 + M01 * m.M10,
			M00 * m.M01 + M01 * m.M11,

			M10 * m.M00 + M11 * m.M10,
			M10 * m.M01 + M11 * m.M11);
	}

	Matrix2D operator*(Float f) const
	{
		return Matrix2D(
			M00 * f, M01 * f,
			M10 * f, M11 * f);
	}

	Matrix2D operator / (Float f) const
	{
		return Matrix2D(
			M00 / f, M01 / f,
			M10 / f, M11 / f);
	}

	Matrix2D& operator=(const Matrix2D& m)
	{
		M00 = m.M00; M01 = m.M01;
		M10 = m.M10; M11 = m.M11;

		return *this;
	}

	Matrix2D& operator+=(const Matrix2D& m)
	{
		M00 += m.M00; M01 += m.M01;
		M10 += m.M10; M11 += m.M11;

		return *this;
	}

	Matrix2D& operator-=(const Matrix2D& m)
	{
		M00 -= m.M00; M01 -= m.M01;
		M10 -= m.M10; M11 -= m.M11;

		return *this;
	}

	Matrix2D& operator*=(const Matrix2D& m)
	{
		return (*this = *this * m);
	}

	Matrix2D& operator*=(Float f)
	{
		M00 *= f; M01 *= f;
		M10 *= f; M11 *= f;

		return *this;
	}

	Matrix2D& operator/=(Float f)
	{
		M00 /= f; M01 /= f;
		M10 /= f; M11 /= f;

		return *this;
	}

	Bool operator==(const Matrix2D& m) const
	{
		return
			(M00 == m.M00) && (M01 == m.M01) &&
			(M10 == m.M10) && (M11 == m.M11);
	}

	Bool operator!=(const Matrix2D& m) const
	{
		return
			(M00 != m.M00) || (M01 != m.M01) ||
			(M10 != m.M10) || (M11 != m.M11);
	}

	/**
	 * Returns true if all of the components of this
	 * m are equal to m within the tolerance
	 * fTolerance, false otherwise.
	 */
	Bool Equals(
		const Matrix2D& m,
		Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(M00, m.M00, fTolerance) &&
			::Seoul::Equals(M01, m.M01, fTolerance) &&

			::Seoul::Equals(M10, m.M10, fTolerance) &&
			::Seoul::Equals(M11, m.M11, fTolerance);
	}

	/**
	 * Returns true if all of the components of this
	 * matrix are equal to 0.0f within the tolerance
	 * fTolerance, false otherwise.
	 */
	Bool IsZero(Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::IsZero(M00, fTolerance) &&
			::Seoul::IsZero(M01, fTolerance) &&

			::Seoul::IsZero(M10, fTolerance) &&
			::Seoul::IsZero(M11, fTolerance);
	}

	/**
	 * Returns a Vector2D containing the diagonal components
	 * of this Matrix2D.
	 */
	Vector2D GetDiagonal() const
	{
		return Vector2D(M00, M11);
	}

	Matrix2D Transpose() const
	{
		return Matrix2D(
			M00, M10,
			M01, M11);
	}

	Float Determinant() const
	{
		return (M00 * M11) - (M01 * M10);
	}

	Matrix2D Inverse() const
	{
		Float fDet = Determinant();
		if (::Seoul::IsZero(fDet, 1e-10f))
		{
			return Identity();
		}

		Float fInvDet = 1.0f / fDet;

		return Matrix2D(
			fInvDet * M11,
			fInvDet * -M01,
			fInvDet * -M10,
			fInvDet * M00);
	}

	/**
	 * Returns true if this Matrix2D is orthonormal, false
	 * otherwise.
	 *
	 * \warning This method is not cheap - it is recommended that you
	 * only use it for debug-time checks.
	 */
	Bool IsOrthonormal(Float fTolerance = fEpsilon) const
	{
		if (!Inverse().Equals(Transpose(), fTolerance))
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	/**
	 * Returns the inverse of this Matrix4D when this
	 * Matrix2D is orthonormal.
	 *
	 * \warning The return value of this method is undefined if this
	 * Matrix2D is not orthonormal. This method will SEOUL_ASSERT that 
	 * this Matrix2D is orthornormal in debug builds.
	 */
	Matrix2D OrthonormalInverse() const
	{
		// We only want to check this in debug because the check for
		// orthonormality is expensive and will add a lot of unwanted
		// overhead to the developer build.
		SEOUL_ASSERT_DEBUG(IsOrthonormal(1e-3f));

		// Orthonormal inverse of a Matrix2D is just its tranpose.
		return Transpose();
	}

	/**
	 * @return A read-only array reference to the data in this Matrix2D.
	 */
	Float const* GetData() const
	{
		return &M00;
	}

	/**
	 * @return A writeable array reference to the data in this Matrix2D.
	 */
	Float* GetData()
	{
		return &M00;
	}

	/**
	 * Access the elements of this Matrix2D by
	 * the row iRow and the column iColumn.
	 */
	Float operator()(Int32 iRow, Int32 iColumn) const
	{
		SEOUL_ASSERT(iRow >= 0 && iRow < 2 && iColumn >= 0 && iColumn < 2);

		// The elements of a Matrix2D are stored
		// in column-major order.
		return GetData()[iColumn * 2 + iRow];
	}
	/**
	 * Access the elements of this Matrix2D by
	 * the row iRow and the column iColumn.
	 */
	Float& operator()(Int32 iRow, Int32 iColumn)
	{
		SEOUL_ASSERT(iRow >= 0 && iRow < 2 && iColumn >= 0 && iColumn < 2);

		// The elements of a Matrix2D are stored
		// in column-major order.
		return GetData()[iColumn * 2 + iRow];
	}

	/**
	 * Get column iIndex of this matrix as a Vector2D.
	 */
	Vector2D GetColumn(Int iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 2);
		const Matrix2D& m = *this;

		Vector2D ret(m(0, iIndex), m(1, iIndex));
		return ret;
	}

	/**
	 * Get row iIndex of this matrix as a Vector2D.
	 */
	Vector2D GetRow(Int iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 2);
		const Matrix2D& m = *this;

		Vector2D ret(m(iIndex, 0), m(iIndex, 1));
		return ret;
	}

	/**
	 * Gets the corresponding basis axis from this matrix and
	 *  normalizes it.
	 */
	Vector2D GetUnitAxis(Int iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 2);
		const Matrix2D& m = *this;

		Vector2D ret(m(iIndex, 0), m(iIndex, 1));
		ret.Normalize();
		return ret;
	}

	/**
	 * Gets the orthonormal basis of this matrix, normalizing
	 * each axis so they are unit length.
	 *
	 * \warning This function does not orthonormalize the transform,
	 * so the out vectors of this method will not form an orthonormal
	 * basis if this matrix is not already an orthonormal transform.
	 */
	void GetUnitAxes(Vector2D& X, Vector2D& Y) const
	{
		X = GetUnitAxis((Int)Axis::kX);
		Y = GetUnitAxis((Int)Axis::kY);
	}

	/**
	 * Set column iIndex of this matrix from a Vector2D.
	 */
	void SetColumn(Int iIndex, const Vector2D& v)
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 2);
		Matrix2D& m = *this;

		m(0, iIndex) = v.X;
		m(1, iIndex) = v.Y;
	}

	/**
	 * Set row iIndex of this matrix from a Vector2D.
	 */
	void SetRow(Int iIndex, const Vector2D& v)
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 2);
		Matrix2D& m = *this;

		m(iIndex, 0) = v.X;
		m(iIndex, 1) = v.Y;
	}

	/**
	 * Performs mMatrix * vVector
	 */
	static Vector2D Transform(const Matrix2D& mMatrix, const Vector2D& vVector)
	{
		return Vector2D(
			mMatrix.M00 * vVector.X + mMatrix.M01 * vVector.Y,
			mMatrix.M10 * vVector.X + mMatrix.M11 * vVector.Y);
	}

	/**
	 * Returns a float representing the rotation
	 * of this Matrix2D, in radians. Decompose() is
	 * used to extract the Orthonormal() rotation portion of this
	 * matrix, ignorign any non-orthonormal effects.
	 */
	Float DecomposeRotation() const
	{
		Matrix2D mPreRotation;
		Matrix2D mRotation;
		Decompose(*this, mPreRotation, mRotation);

		return Atan2(mRotation.M10, mRotation.M00);
	}

	/**
	 * Sets this Matrix2D to be a rotation matrix
	 * of f, in radians.
	 */
	void SetRotation(Float fRadians)
	{
		*this = CreateRotation(fRadians);
	}

	static Matrix2D Zero()
	{
		return Matrix2D(
			0.0f, 0.0f,
			0.0f, 0.0f);
	}

	static Matrix2D Identity()
	{
		return Matrix2D(
			1.0f, 0.0f,
			0.0f, 1.0f);
	}

	/** Convenience variation of CreateRotation, for angle in degrees. */
	static Matrix2D CreateRotationFromDegrees(Float fAngleInDegrees)
	{
		return CreateRotation(DegreesToRadians(fAngleInDegrees));
	}

	/** Create a 2D m that specifies a rotation in the XY plane. */
	static Matrix2D CreateRotation(Float fAngleInRadians)
	{
		Float const fCosAngle = Cos(fAngleInRadians);
		Float const fSinAngle = Sin(fAngleInRadians);

		return Matrix2D(
			fCosAngle, -fSinAngle,
			fSinAngle, fCosAngle);
	}

	/** Create a 2D uniform scaling matrix. */
	static Matrix2D CreateScale(Float fScale)
	{
		return Matrix2D(fScale, 0.0f, 0.0f, fScale);
	}
	
	/** Create a 2D non-uniform scaling matrix. */
	static Matrix2D CreateScale(Float fX, Float fY)
	{
		return Matrix2D(fX, 0.0f, 0.0f, fY);
	}

	/** Create a 2D non-uniform scaling matrix. */
	static Matrix2D CreateScale(const Vector2D& vScale)
	{
		return CreateScale(vScale.X, vScale.Y);
	}

	/**
	 * Decompose the matrix m into its orthonormal (rotation)
	 * and non-orthonormal (scale and skew, pre-rotation) parts.
	 *
	 * @return true if the Decomposition was successful, false otherwise.
	 */
	static Bool Decompose(
		const Matrix2D& m,
		Matrix2D& rmPreRotation,
		Matrix2D& rmRotation)
	{
		// Done immediately if we can't decompose.
		if (!QRDecomposition(m, rmRotation, rmPreRotation))
		{
			return false;
		}

		// We want to move any reflection to the pre-rotation matrix.
		// If the determinant is negative, negative M00 and M10 in
		// the orthonormal rotation matrix, and M00 in the non-orthonormal
		// scale/skew/pre-rotation matrix.
		auto f = rmRotation.Determinant();
		if (Sign(f) < 0.0f)
		{
			rmRotation.M00 = -rmRotation.M00;
			rmRotation.M10 = -rmRotation.M10;
			rmPreRotation.M00 = -rmPreRotation.M00;
		}

		return true;
	}

	/**
	 * Decompose the matrix m into QR, where
	 * Q is an orthonormal transform and R is an upper-triangular transform.
	 *
	 * @return True if the matrix was successfully decomposed, false otherwise.
	 *
	 * Low-level - typically, you want Decompose() instead.
	 *
	 * Uses a modified version of Gram-Schmidt for numerical stability.
	 *
	 * \sa http://en.wikipedia.org/wiki/QR_decomposition
	 * \sa http://en.wikipedia.org/wiki/Gram-Schmidt#Numerical_stability
	 */
	static Bool QRDecomposition(
		const Matrix2D& m,
		Matrix2D& rmQ,
		Matrix2D& rmR)
	{
		Vector2D const a1 = Vector2D(m.M00, m.M10);
		Vector2D const a2 = Vector2D(m.M01, m.M11);

		// Column 0
		Vector2D e1 = a1;
		if (!e1.Normalize())
		{
			return false;
		}

		// Column 1
		Vector2D e2 = (a2 - Vector2D::GramSchmidtProjectionOperator(e1, a2));
		if (!e2.Normalize())
		{
			return false;
		}

		// Q (orthonormal part)
		rmQ.M00 = e1.X;
		rmQ.M10 = e1.Y;
		rmQ.M01 = e2.X;
		rmQ.M11 = e2.Y;

		// R (non-orthonormal part)
		rmR.M00 = Vector2D::Dot(e1, a1);
		rmR.M10 = 0.0f;
		rmR.M01 = Vector2D::Dot(e1, a2);
		rmR.M11 = Vector2D::Dot(e2, a2);
		return true;
	}

	/**
	 * @return A m that is the linear interpolation of m0
	 * and m1 based on the weighting factor fT
	 */
	static Matrix2D Lerp(const Matrix2D& m0, const Matrix2D& m1, Float fT)
	{
		return m0 * (1.0f - fT) + m1 * fT;
	}
}; // struct Matrix2D
template <> struct CanMemCpy<Matrix2D> { static const Bool Value = true; };
template <> struct CanZeroInit<Matrix2D> { static const Bool Value = true; };

inline Matrix2D operator*(Float f, const Matrix2D & m)
{
	return (m * f);
}

/** Tolerance equality test between a and b. */
inline Bool Equals(const Matrix2D& mA, const Matrix2D& mB, Float fTolerance = fEpsilon)
{
	return mA.Equals(mB, fTolerance);
}

/**
 * @return A m that is the linear interpolation of m0
 * and m1 based on the weighting factor fT
 */
inline Matrix2D Lerp(const Matrix2D& m0, const Matrix2D& m1, Float fT)
{
	return Matrix2D::Lerp(m0, m1, fT);
}

} // namespace Seoul

#endif // include guard
