/**
 * \file Matrix3x4.h
 * \brief Matrix3x4 represents the upper 3 rows of a 4x4 square matrix.
 *
 * Matrix3x4 does *not* represent an actual 3x4 matrix, it is
 * a structure for efficient packing of 4x4 matrices with an implicit
 * 4th row of [0, 0, 0, 1].
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	MATRIX3X4_H
#define	MATRIX3X4_H

#include "Vector3D.h"

namespace Seoul
{

// Forward declarations
struct Matrix4D;

/** Represents the upper 3 rows of a 4x4 square matrix.
 *
 *  Matrix3x4 is *not* a 3x4 matrix. In particular, multiplication
 *  of a Matrix3x4 with a Matrix3x4 is a valid operation whereas
 *  multiplication of a 3x4 matrix by a 3x4 matrix is not a valid operation.
 *
 *  Matrix3x4 should be viewed as a Matrix4D with an implicit 4th row
 *  of [0, 0, 0, 1].
 */
struct Matrix3x4
{
	union
	{
		Float M[3][4];

		struct
		{
			Float M00, M01, M02, M03,
				  M10, M11, M12, M13,
				  M20, M21, M22, M23;
		};
	};

	Matrix3x4()
		: M00(0.0f), M01(0.0f), M02(0.0f), M03(0.0f),
		  M10(0.0f), M11(0.0f), M12(0.0f), M13(0.0f),
		  M20(0.0f), M21(0.0f), M22(0.0f), M23(0.0f)
	{
	}

	Matrix3x4(Float m00, Float m01, Float m02, Float m03,
		      Float m10, Float m11, Float m12, Float m13,
			  Float m20, Float m21, Float m22, Float m23)
		: M00(m00), M01(m01), M02(m02), M03(m03),
		  M10(m10), M11(m11), M12(m12), M13(m13),
		  M20(m20), M21(m21), M22(m22), M23(m23)
	{
	}

	explicit Matrix3x4(const Matrix4D& matrix);

	Matrix3x4(const Matrix3x4& matrix)
		: M00(matrix.M00), M01(matrix.M01), M02(matrix.M02), M03(matrix.M03),
		  M10(matrix.M10), M11(matrix.M11), M12(matrix.M12), M13(matrix.M13),
		  M20(matrix.M20), M21(matrix.M21), M22(matrix.M22), M23(matrix.M23)
	{
	}

	Matrix3x4& operator=(const Matrix3x4& other)
	{
		M00 = other.M00;	M01 = other.M01;	M02 = other.M02;	M03 = other.M03;
		M10 = other.M10;	M11 = other.M11;	M12 = other.M12;	M13 = other.M13;
		M20 = other.M20;	M21 = other.M21;	M22 = other.M22;	M23 = other.M23;

		return *this;
	}

	Bool operator==(const Matrix3x4 & matrix) const
	{
		return (M00 == matrix.M00) && (M01 == matrix.M01) && (M02 == matrix.M02) && (M03 == matrix.M03) &&
			   (M10 == matrix.M10) && (M11 == matrix.M11) && (M12 == matrix.M12) && (M13 == matrix.M13) &&
			   (M20 == matrix.M20) && (M21 == matrix.M21) && (M22 == matrix.M22) && (M23 == matrix.M23);
	}

	Bool operator!=(const Matrix3x4 & matrix) const
	{
		return (M00 != matrix.M00) || (M01 != matrix.M01) || (M02 != matrix.M02) || (M03 != matrix.M03) ||
			   (M10 != matrix.M10) || (M11 != matrix.M11) || (M12 != matrix.M12) || (M13 != matrix.M13) ||
			   (M20 != matrix.M20) || (M21 != matrix.M21) || (M22 != matrix.M22) || (M23 != matrix.M23);
	}

	Bool Equals(const Matrix3x4& matrix, Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(M00, matrix.M00, fTolerance) &&
			::Seoul::Equals(M01, matrix.M01, fTolerance) &&
			::Seoul::Equals(M02, matrix.M02, fTolerance) &&
			::Seoul::Equals(M03, matrix.M03, fTolerance) &&

			::Seoul::Equals(M10, matrix.M10, fTolerance) &&
			::Seoul::Equals(M11, matrix.M11, fTolerance) &&
			::Seoul::Equals(M12, matrix.M12, fTolerance) &&
			::Seoul::Equals(M13, matrix.M13, fTolerance) &&

			::Seoul::Equals(M20, matrix.M20, fTolerance) &&
			::Seoul::Equals(M21, matrix.M21, fTolerance) &&
			::Seoul::Equals(M22, matrix.M22, fTolerance) &&
			::Seoul::Equals(M23, matrix.M23, fTolerance);
	}

	/**
	 * Get column iIndex of this matrix as a Vector3D.
	 */
	Vector3D GetColumn(Int iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 3);
		Vector3D ret(M[0][iIndex], M[1][iIndex], M[2][iIndex]);
		return ret;
	}

	/**
	 * Updates the values in column iIndex to the value of
	 * vColumnVector.
	 */
	void SetColumn(Int iIndex, const Vector3D& vColumnVector)
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 3);
		M[0][iIndex] = vColumnVector.X;
		M[1][iIndex] = vColumnVector.Y;
		M[2][iIndex] = vColumnVector.Z;
	}

	/** Gets the translation components of this matrix
	 *  as a Vector3D.
	 */
	inline Vector3D GetTranslation() const
	{
		return Vector3D(M03, M13, M23);
	}

	/** Replaces the translation components of
	 *  this matrix with the translation specified in
	 *  vTranslation.
	 */
	void SetTranslation(const Vector3D& vTranslation)
	{
		M03 = vTranslation.X;
		M13 = vTranslation.Y;
		M23 = vTranslation.Z;
	}

	static Matrix3x4 Zero()
	{
		return Matrix3x4(
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
	}

	static Matrix3x4 Identity()
	{
		return Matrix3x4(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f);
	}
}; // struct Matrix3x4
template <> struct CanMemCpy<Matrix3x4> { static const Bool Value = true; };
template <> struct CanZeroInit<Matrix3x4> { static const Bool Value = true; };

/**
 * Multiplication of two 3x4 matrices.
 *
 * See also the description of Matrix3x4. Multiplication of
 * two 3x4 matrices is not valid, however, Matrix3x4 is considered to
 * be a 4x4 matrix with an implicit 4th row of [0, 0, 0, 1].
 */
inline Matrix3x4 operator*(const Matrix3x4& m1, const Matrix3x4& m2)
{
	return Matrix3x4(m1.M00 * m2.M00 + m1.M01 * m2.M10 + m1.M02 * m2.M20,
		            m1.M00 * m2.M01 + m1.M01 * m2.M11 + m1.M02 * m2.M21,
					m1.M00 * m2.M02 + m1.M01 * m2.M12 + m1.M02 * m2.M22,
					m1.M00 * m2.M03 + m1.M01 * m2.M13 + m1.M02 * m2.M23 + m1.M03,

					m1.M10 * m2.M00 + m1.M11 * m2.M10 + m1.M12 * m2.M20,
					m1.M10 * m2.M01 + m1.M11 * m2.M11 + m1.M12 * m2.M21,
					m1.M10 * m2.M02 + m1.M11 * m2.M12 + m1.M12 * m2.M22,
					m1.M10 * m2.M03 + m1.M11 * m2.M13 + m1.M12 * m2.M23 + m1.M13,

					m1.M20 * m2.M00 + m1.M21 * m2.M10 + m1.M22 * m2.M20,
		            m1.M20 * m2.M01 + m1.M21 * m2.M11 + m1.M22 * m2.M21,
					m1.M20 * m2.M02 + m1.M21 * m2.M12 + m1.M22 * m2.M22,
					m1.M20 * m2.M03 + m1.M21 * m2.M13 + m1.M22 * m2.M23 + m1.M23);
}

/** Tolerance equality test between a and b. */
inline Bool Equals(const Matrix3x4& mA, const Matrix3x4& mB, Float fTolerance = fEpsilon)
{
	return mA.Equals(mB, fTolerance);
}

} // namespace Seoul

#endif // include guard
