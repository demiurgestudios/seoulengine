/**
 * \file Matrix3D.h
 * \brief Matrix3D represents a 3x3 square m.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	MATRIX3D_H
#define	MATRIX3D_H

#include "Axis.h"
#include "SeoulMath.h"
#include "SeoulTypes.h"
#include "Vector3D.h"

namespace Seoul
{

struct Matrix3D SEOUL_SEALED
{
	Float M00, M10, M20,
		  M01, M11, M21,
		  M02, M12, M22;

	Matrix3D()
		: M00(0.0f), M10(0.0f), M20(0.0f)
		, M01(0.0f), M11(0.0f), M21(0.0f)
		, M02(0.0f), M12(0.0f), M22(0.0f)
	{
	}

	explicit Matrix3D(Float f)
		: M00(f), M10(f), M20(f)
		, M01(f), M11(f), M21(f)
		, M02(f), M12(f), M22(f)
	{
	}

	Matrix3D(Float m00, Float m01, Float m02,
		     Float m10, Float m11, Float m12,
			 Float m20, Float m21, Float m22)
		: M00(m00), M10(m10), M20(m20)
		, M01(m01), M11(m11), M21(m21)
		, M02(m02), M12(m12), M22(m22)
	{
	}

	Matrix3D(const Matrix3D & m)
		: M00(m.M00), M10(m.M10), M20(m.M20)
		, M01(m.M01), M11(m.M11), M21(m.M21)
		, M02(m.M02), M12(m.M12), M22(m.M22)
	{
	}

	Matrix3D operator + (const Matrix3D & m) const
	{
		return Matrix3D(M00 + m.M00, M01 + m.M01, M02 + m.M02,
			            M10 + m.M10, M11 + m.M11, M12 + m.M12,
						M20 + m.M20, M21 + m.M21, M22 + m.M22);
	}

	Matrix3D operator-(const Matrix3D & m) const
	{
		return Matrix3D(M00 - m.M00, M01 - m.M01, M02 - m.M02,
			            M10 - m.M10, M11 - m.M11, M12 - m.M12,
						M20 - m.M20, M21 - m.M21, M22 - m.M22);
	}

	Matrix3D operator-() const
	{
		return Matrix3D(-M00, -M01, -M02,
			            -M10, -M11, -M12,
						-M20, -M21, -M22);
	}

	Matrix3D operator*(const Matrix3D & m) const
	{
		return Matrix3D(
			M00 * m.M00 + M01 * m.M10 + M02 * m.M20,
			M00 * m.M01 + M01 * m.M11 + M02 * m.M21,
			M00 * m.M02 + M01 * m.M12 + M02 * m.M22,

			M10 * m.M00 + M11 * m.M10 + M12 * m.M20,
			M10 * m.M01 + M11 * m.M11 + M12 * m.M21,
			M10 * m.M02 + M11 * m.M12 + M12 * m.M22,

			M20 * m.M00 + M21 * m.M10 + M22 * m.M20,
			M20 * m.M01 + M21 * m.M11 + M22 * m.M21,
			M20 * m.M02 + M21 * m.M12 + M22 * m.M22);
	}

	Vector3D operator*(const Vector3D & v) const
	{
		return Vector3D(
			M00 * v.X + M01 * v.Y + M02 * v.Z,
			M10 * v.X + M11 * v.Y + M12 * v.Z,
			M20 * v.X + M21 * v.Y + M22 * v.Z);
	}

	Matrix3D operator*(Float f) const
	{
		return Matrix3D(
			M00 * f, M01 * f, M02 * f,
			M10 * f, M11 * f, M12 * f,
			M20 * f, M21 * f, M22 * f);
	}

	Matrix3D operator / (Float f) const
	{
		return Matrix3D(
			M00 / f, M01 / f, M02 / f,
			M10 / f, M11 / f, M12 / f,
			M20 / f, M21 / f, M22 / f);
	}

	Matrix3D& operator=(const Matrix3D& m)
	{
		M00 = m.M00; M01 = m.M01; M02 = m.M02;
		M10 = m.M10; M11 = m.M11; M12 = m.M12;
		M20 = m.M20; M21 = m.M21; M22 = m.M22;

		return *this;
	}

	Matrix3D& operator+=(const Matrix3D& m)
	{
		M00 += m.M00; M01 += m.M01; M02 += m.M02;
		M10 += m.M10; M11 += m.M11; M12 += m.M12;
		M20 += m.M20; M21 += m.M21; M22 += m.M22;

		return *this;
	}

	Matrix3D& operator-=(const Matrix3D& m)
	{
		M00 -= m.M00; M01 -= m.M01; M02 -= m.M02;
		M10 -= m.M10; M11 -= m.M11; M12 -= m.M12;
		M20 -= m.M20; M21 -= m.M21; M22 -= m.M22;

		return *this;
	}

	Matrix3D& operator*=(const Matrix3D& m)
	{
		return (*this = *this * m);
	}

	Matrix3D& operator*=(Float f)
	{
		M00 *= f; M01 *= f; M02 *= f;
		M10 *= f; M11 *= f; M12 *= f;
		M20 *= f; M21 *= f; M22 *= f;

		return *this;
	}

	Matrix3D& operator/=(Float f)
	{
		M00 /= f; M01 /= f; M02 /= f;
		M10 /= f; M11 /= f; M12 /= f;
		M20 /= f; M21 /= f; M22 /= f;

		return *this;
	}

	Bool operator==(const Matrix3D& m) const
	{
		return
			(M00 == m.M00) && (M01 == m.M01) && (M02 == m.M02) &&
			(M10 == m.M10) && (M11 == m.M11) && (M12 == m.M12) &&
			(M20 == m.M20) && (M21 == m.M21) && (M22 == m.M22);
	}

	Bool operator!=(const Matrix3D& m) const
	{
		return
			(M00 != m.M00) || (M01 != m.M01) || (M02 != m.M02) ||
			(M10 != m.M10) || (M11 != m.M11) || (M12 != m.M12) ||
			(M20 != m.M20) || (M21 != m.M21) || (M22 != m.M22);
	}

	/**
	 * Returns true if all of the components of this
	 * m are equal to m within the tolerance
	 * fTolerance, false otherwise.
	 */
	Bool Equals(
		const Matrix3D& m,
		Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(M00, m.M00, fTolerance) &&
			::Seoul::Equals(M01, m.M01, fTolerance) &&
			::Seoul::Equals(M02, m.M02, fTolerance) &&

			::Seoul::Equals(M10, m.M10, fTolerance) &&
			::Seoul::Equals(M11, m.M11, fTolerance) &&
			::Seoul::Equals(M12, m.M12, fTolerance) &&

			::Seoul::Equals(M20, m.M20, fTolerance) &&
			::Seoul::Equals(M21, m.M21, fTolerance) &&
			::Seoul::Equals(M22, m.M22, fTolerance);
	}

	/**
	 * Returns a Vector3D containing the diagonal components
	 * of this Matrix3D.
	 */
	Vector3D GetDiagonal() const
	{
		return Vector3D(M00, M11, M22);
	}

	/**
	 * Returns true if this Matrix3D is orthonormal, false
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
	 * Returns true if all of the components of this
	 * matrix are equal to 0.0f within the tolerance
	 * fTolerance, false otherwise.
	 */
	Bool IsZero(Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::IsZero(M00, fTolerance) &&
			::Seoul::IsZero(M01, fTolerance) &&
			::Seoul::IsZero(M02, fTolerance) &&

			::Seoul::IsZero(M10, fTolerance) &&
			::Seoul::IsZero(M11, fTolerance) &&
			::Seoul::IsZero(M12, fTolerance) &&

			::Seoul::IsZero(M20, fTolerance) &&
			::Seoul::IsZero(M21, fTolerance) &&
			::Seoul::IsZero(M22, fTolerance);
	}

	/**
	 * @return A read-only array reference to the data in this Matrix3D.
	 */
	Float const* GetData() const
	{
		return &M00;
	}

	/**
	 * @return A writeable array reference to the data in this Matrix3D.
	 */
	Float* GetData()
	{
		return &M00;
	}

	/**
	 * Access the elements of this Matrix3D by
	 * the row iRow and the column iColumn.
	 */
	Float operator()(Int32 iRow, Int32 iColumn) const
	{
		SEOUL_ASSERT(iRow >= 0 && iRow < 3 && iColumn >= 0 && iColumn < 3);

		// The elements of a Matrix3D are stored
		// in column-major order.
		return GetData()[iColumn * 3 + iRow];
	}
	/**
	 * Access the elements of this Matrix3D by
	 * the row iRow and the column iColumn.
	 */
	Float& operator()(Int32 iRow, Int32 iColumn)
	{
		SEOUL_ASSERT(iRow >= 0 && iRow < 3 && iColumn >= 0 && iColumn < 3);

		// The elements of a Matrix3D are stored
		// in column-major order.
		return GetData()[iColumn * 3 + iRow];
	}


	Matrix3D Transpose() const
	{
		return Matrix3D(
			M00, M10, M20,
			M01, M11, M21,
			M02, M12, M22);
	}

	Float Determinant() const
	{
		return   M00 * (M11 * M22 - M12 * M21)
			   - M01 * (M10 * M22 - M12 * M20)
			   + M02 * (M10 * M21 - M11 * M20);
	}

	Matrix3D Inverse() const
	{
		Float fDet = Determinant();
		if (::Seoul::IsZero(fDet))
		{
			return Identity();
		}

		Float fInvDet = 1.0f / fDet;

		return Matrix3D(fInvDet * (M11 * M22 - M12 * M21),
			            fInvDet * (M02 * M21 - M01 * M22),
						fInvDet * (M01 * M12 - M02 * M11),

						fInvDet * (M12 * M20 - M10 * M22),
						fInvDet * (M00 * M22 - M02 * M20),
						fInvDet * (M02 * M10 - M00 * M12),

						fInvDet * (M10 * M21 - M11 * M20),
						fInvDet * (M01 * M20 - M00 * M21),
						fInvDet * (M00 * M11 - M01 * M10));
	}

	/**
	 * Gets the translation components of this m
	 *  as a Vector2D.
	 */
	Vector2D GetTranslation() const
	{
		return Vector2D(M02, M12);
	}

	/**
	 * Replaces the translation components of
	 * this m with the translation specified in
	 * vTranslation.
	 */
	void SetTranslation(const Vector2D& vTranslation)
	{
		M02 = vTranslation.X;
		M12 = vTranslation.Y;
	}

	/**
	 * Performs m * v.
	 */
	static Vector3D Transform(
		const Matrix3D& m,
		const Vector3D& v)
	{
		return Vector3D(m.M00 * v.X + m.M01 * v.Y + m.M02 * v.Z,
			           m.M10 * v.X + m.M11 * v.Y + m.M12 * v.Z,
					   m.M20 * v.X + m.M21 * v.Y + m.M22 * v.Z);
	}

	/** Gets the corresponding basis axis from this m and
	 *  normalizes it.
	 */
	Vector3D GetUnitAxis(Int i) const
	{
		SEOUL_ASSERT(i >= 0 && i <= 2);
		const Matrix3D& m = *this;
		Vector3D ret(m(i, 0), m(i, 1), m(i, 2));
		ret.Normalize();
		return ret;
	}

	/** Gets the orthonormal basis of this m, normalizing
	 *  each axis so they are unit length.
	 *
	 *  \warning This function does not orthonormalize the transform,
	 *  so the out vectors of this method will not form an orthonormal
	 *  basis if this m is not already an orthonormal transform.
	 */
	void GetUnitAxes(Vector3D& X, Vector3D& Y, Vector3D& Z) const
	{
		X = GetUnitAxis((Int)Axis::kX);
		Y = GetUnitAxis((Int)Axis::kY);
		Z = GetUnitAxis((Int)Axis::kZ);
	}

	/**
	 * Get column iIndex of this m as a Vector3D.
	 */
	Vector3D GetColumn(Int iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 3);
		const Matrix3D& m = *this;
		Vector3D ret(m(0, iIndex), m(1, iIndex), m(2, iIndex));
		return ret;
	}

	/**
	 * Updates the values in column iIndex to the value of
	 * vColumnVector.
	 */
	void SetColumn(Int iIndex, const Vector3D& vColumnVector)
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 3);
		Matrix3D& m = *this;
		m(0, iIndex) = vColumnVector.X;
		m(1, iIndex) = vColumnVector.Y;
		m(2, iIndex) = vColumnVector.Z;
	}

	/**
	 * Get row iIndex of this m as a Vector3D.
	 */
	Vector3D GetRow(Int iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 3);
		const Matrix3D& m = *this;
		Vector3D ret(m(iIndex, 0), m(iIndex, 1), m(iIndex, 2));
		return ret;
	}

	/**
	 * Updates the values in row iIndex to the value of
	 * vRowVector.
	 */
	void SetRow(Int iIndex, const Vector3D& vRowVector)
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 3);
		Matrix3D& m = *this;
		m(iIndex, 0) = vRowVector.X;
		m(iIndex, 1) = vRowVector.Y;
		m(iIndex, 2) = vRowVector.Z;
	}

	static Matrix3D Zero()
	{
		return Matrix3D(
			0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f);
	}

	static Matrix3D Identity()
	{
		return Matrix3D(
			1.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 1.0f);
	}

	/**
	 * Decompose the matrix m into its orthonormal (rotation)
	 * and non-orthonormal (scale and skew, pre-rotation) parts.
	 *
	 * @return true if the Decomposition was successful, false otherwise.
	 */
	static Bool Decompose(
		const Matrix3D& m,
		Matrix3D& rmPreRotation,
		Matrix3D& rmRotation)
	{
		// Done immediately if we can't decompose.
		if (!QRDecomposition(m, rmRotation, rmPreRotation))
		{
			return false;
		}

		// We want to move any reflection to the pre-rotation matrix.
		// If the determinant is negative, negative M00, M10, and M20 in
		// the orthonormal rotation matrix, and M00 in the non-orthonormal
		// scale/skew/pre-rotation matrix.
		auto f = rmRotation.Determinant();
		if (Sign(f) < 0.0f)
		{
			rmRotation.M00 = -rmRotation.M00;
			rmRotation.M10 = -rmRotation.M10;
			rmRotation.M20 = -rmRotation.M20;
			rmPreRotation.M00 = -rmPreRotation.M00;
		}

		return true;
	}

	static Bool QRDecomposition(
		const Matrix3D& mTransform,
		Matrix3D& rmQ,
		Matrix3D& rmR);

	static Matrix3D CreateRotationX(Float fAngleInRadians);
	static Matrix3D CreateRotationY(Float fAngleInRadians);
	static Matrix3D CreateRotationZ(Float fAngleInRadians);

	static Matrix3D CreateScale(Float fX, Float fY, Float fZ);
	static Matrix3D CreateScale(const Vector3D& vScale);

	/**
	 * @return A m that is the linear interpolation of m0
	 * and m1 based on the weighting factor fT
	 */
	static Matrix3D Lerp(const Matrix3D& m0, const Matrix3D& m1, Float fT)
	{
		return m0 * (1.0f - fT) + m1 * fT;
	}
}; // struct Matrix3D
template <> struct CanMemCpy<Matrix3D> { static const Bool Value = true; };
template <> struct CanZeroInit<Matrix3D> { static const Bool Value = true; };

inline Matrix3D operator*(Float f, const Matrix3D & m)
{
	return (m * f);
}

/** Tolerance equality test between a and b. */
inline Bool Equals(const Matrix3D& mA, const Matrix3D& mB, Float fTolerance = fEpsilon)
{
	return mA.Equals(mB, fTolerance);
}

/**
 * @return A m that is the linear interpolation of m0
 * and m1 based on the weighting factor fT
 */
inline Matrix3D Lerp(const Matrix3D& m0, const Matrix3D& m1, Float fT)
{
	return Matrix3D::Lerp(m0, m1, fT);
}

} // namespace Seoul

#endif // include guard
