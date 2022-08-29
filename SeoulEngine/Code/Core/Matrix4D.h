/**
 * \file Matrix4D.h
 * \brief Matrix4D represents a 4x4 square matrix.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	MATRIX4D_H
#define	MATRIX4D_H

#include "Matrix3D.h"
#include "Quaternion.h"
#include "SeoulMath.h"
#include "SeoulTypes.h"
#include "Vector4D.h"
namespace Seoul { struct Plane; }

namespace Seoul
{

// Forward declarations
struct Matrix3x4;

// Base epsilon for infinite projections. Typically,
// multiply this value by n for more infinite
// depth layers, up to a very limited number
// (before z fighting will occur with normal depth shapes).
//
// This has been manually adjusted to be as large as possible
// while still maintaining separation for a 1/1000 ratio
// of near-to-far plane distance.
//
// Original value: 2.4e-7 from http://www.terathon.com/gdc07_lengyel.pdf, page 17.
static const Double kfInfiniteProjectionEpsilon = 4.8e-7;

// Base epsilon for biased projections. Typically,
// multiply this by n for more biased "planes" of depth.
//
// This has been manually adjusted to be as large as possible
// while still maintaining separation for a 1/1000 ratio
// of near-to-far plane distance.
//
// Original value: 4.8e-7 from http://www.terathon.com/gdc07_lengyel.pdf, page 26.
static const Double kfBiasProjectionEpsilon = 4.8e-6;

struct Matrix4D SEOUL_SEALED
{
	// Data in Matrix4D is stored column major. This makes SIMD
	// implementations cheaper and allows for cheaper submission
	// of Matrix4D parameters as shader Effect parameters.
	Float M00; Float M10; Float M20; Float M30;
	Float M01; Float M11; Float M21; Float M31;
	Float M02; Float M12; Float M22; Float M32;
	Float M03; Float M13; Float M23; Float M33;

	Matrix4D()
		: M00(0.0f), M10(0.0f), M20(0.0f), M30(0.0f)
		, M01(0.0f), M11(0.0f), M21(0.0f), M31(0.0f)
		, M02(0.0f), M12(0.0f), M22(0.0f), M32(0.0f)
		, M03(0.0f), M13(0.0f), M23(0.0f), M33(0.0f)
	{
	}

	explicit Matrix4D(Float fScalar)
		: M00(fScalar), M10(fScalar), M20(fScalar), M30(fScalar)
		, M01(fScalar), M11(fScalar), M21(fScalar), M31(fScalar)
		, M02(fScalar), M12(fScalar), M22(fScalar), M32(fScalar)
		, M03(fScalar), M13(fScalar), M23(fScalar), M33(fScalar)
	{
	}

	Matrix4D(Float m00, Float m01, Float m02, Float m03,
		Float m10, Float m11, Float m12, Float m13,
		Float m20, Float m21, Float m22, Float m23,
		Float m30, Float m31, Float m32, Float m33)
		: M00(m00), M10(m10), M20(m20), M30(m30)
		, M01(m01), M11(m11), M21(m21), M31(m31)
		, M02(m02), M12(m12), M22(m22), M32(m32)
		, M03(m03), M13(m13), M23(m23), M33(m33)
	{
	}

	Matrix4D(const Matrix4D & matrix)
		: M00(matrix.M00), M10(matrix.M10), M20(matrix.M20), M30(matrix.M30)
		, M01(matrix.M01), M11(matrix.M11), M21(matrix.M21), M31(matrix.M31)
		, M02(matrix.M02), M12(matrix.M12), M22(matrix.M22), M32(matrix.M32)
		, M03(matrix.M03), M13(matrix.M13), M23(matrix.M23), M33(matrix.M33)
	{
	}

	Matrix4D(const Matrix3x4& matrix);

	Matrix4D operator+ (const Matrix4D & matrix) const
	{
		return Matrix4D(M00 + matrix.M00, M01 + matrix.M01, M02 + matrix.M02, M03 + matrix.M03,
			M10 + matrix.M10, M11 + matrix.M11, M12 + matrix.M12, M13 + matrix.M13,
			M20 + matrix.M20, M21 + matrix.M21, M22 + matrix.M22, M23 + matrix.M23,
			M30 + matrix.M30, M31 + matrix.M31, M32 + matrix.M32, M33 + matrix.M33);
	}

	Matrix4D operator- (const Matrix4D & matrix) const
	{
		return Matrix4D(M00 - matrix.M00, M01 - matrix.M01, M02 - matrix.M02, M03 - matrix.M03,
			M10 - matrix.M10, M11 - matrix.M11, M12 - matrix.M12, M13 - matrix.M13,
			M20 - matrix.M20, M21 - matrix.M21, M22 - matrix.M22, M23 - matrix.M23,
			M30 - matrix.M30, M31 - matrix.M31, M32 - matrix.M32, M33 - matrix.M33);
	}

	Matrix4D operator- () const
	{
		return Matrix4D(-M00, -M01, -M02, -M03,
			-M10, -M11, -M12, -M13,
			-M20, -M21, -M22, -M23,
			-M30, -M31, -M32, -M33);
	}

	/**
	 * Performs this * mMatrix.
	 */
	Matrix4D operator*(const Matrix4D& mMatrix) const
	{
		Matrix4D const ret(
			M00 * mMatrix.M00 + M01 * mMatrix.M10 + M02 * mMatrix.M20 + M03 * mMatrix.M30,
			M00 * mMatrix.M01 + M01 * mMatrix.M11 + M02 * mMatrix.M21 + M03 * mMatrix.M31,
			M00 * mMatrix.M02 + M01 * mMatrix.M12 + M02 * mMatrix.M22 + M03 * mMatrix.M32,
			M00 * mMatrix.M03 + M01 * mMatrix.M13 + M02 * mMatrix.M23 + M03 * mMatrix.M33,

			M10 * mMatrix.M00 + M11 * mMatrix.M10 + M12 * mMatrix.M20 + M13 * mMatrix.M30,
			M10 * mMatrix.M01 + M11 * mMatrix.M11 + M12 * mMatrix.M21 + M13 * mMatrix.M31,
			M10 * mMatrix.M02 + M11 * mMatrix.M12 + M12 * mMatrix.M22 + M13 * mMatrix.M32,
			M10 * mMatrix.M03 + M11 * mMatrix.M13 + M12 * mMatrix.M23 + M13 * mMatrix.M33,

			M20 * mMatrix.M00 + M21 * mMatrix.M10 + M22 * mMatrix.M20 + M23 * mMatrix.M30,
			M20 * mMatrix.M01 + M21 * mMatrix.M11 + M22 * mMatrix.M21 + M23 * mMatrix.M31,
			M20 * mMatrix.M02 + M21 * mMatrix.M12 + M22 * mMatrix.M22 + M23 * mMatrix.M32,
			M20 * mMatrix.M03 + M21 * mMatrix.M13 + M22 * mMatrix.M23 + M23 * mMatrix.M33,

			M30 * mMatrix.M00 + M31 * mMatrix.M10 + M32 * mMatrix.M20 + M33 * mMatrix.M30,
			M30 * mMatrix.M01 + M31 * mMatrix.M11 + M32 * mMatrix.M21 + M33 * mMatrix.M31,
			M30 * mMatrix.M02 + M31 * mMatrix.M12 + M32 * mMatrix.M22 + M33 * mMatrix.M32,
			M30 * mMatrix.M03 + M31 * mMatrix.M13 + M32 * mMatrix.M23 + M33 * mMatrix.M33);
		return ret;
	}

	Matrix4D operator*(Float fScalar) const
	{
		return Matrix4D(M00 * fScalar, M01 * fScalar, M02 * fScalar, M03 * fScalar,
			M10 * fScalar, M11 * fScalar, M12 * fScalar, M13 * fScalar,
			M20 * fScalar, M21 * fScalar, M22 * fScalar, M23 * fScalar,
			M30 * fScalar, M31 * fScalar, M32 * fScalar, M33 * fScalar);
	}

	Matrix4D operator/(Float fScalar) const
	{
		return Matrix4D(M00 / fScalar, M01 / fScalar, M02 / fScalar, M03 / fScalar,
			M10 / fScalar, M11 / fScalar, M12 / fScalar, M13 / fScalar,
			M20 / fScalar, M21 / fScalar, M22 / fScalar, M23 / fScalar,
			M30 / fScalar, M31 / fScalar, M32 / fScalar, M33 / fScalar);
	}

	Matrix4D& operator=(const Matrix4D & matrix)
	{
		M00 = matrix.M00; M01 = matrix.M01; M02 = matrix.M02; M03 = matrix.M03;
		M10 = matrix.M10; M11 = matrix.M11; M12 = matrix.M12; M13 = matrix.M13;
		M20 = matrix.M20; M21 = matrix.M21; M22 = matrix.M22; M23 = matrix.M23;
		M30 = matrix.M30; M31 = matrix.M31; M32 = matrix.M32; M33 = matrix.M33;

		return *this;
	}

	Matrix4D& operator+=(const Matrix4D & matrix)
	{
		M00 += matrix.M00; M01 += matrix.M01; M02 += matrix.M02; M03 += matrix.M03;
		M10 += matrix.M10; M11 += matrix.M11; M12 += matrix.M12; M13 += matrix.M13;
		M20 += matrix.M20; M21 += matrix.M21; M22 += matrix.M22; M23 += matrix.M23;
		M30 += matrix.M30; M31 += matrix.M31; M32 += matrix.M32; M33 += matrix.M33;

		return *this;
	}

	Matrix4D& operator-=(const Matrix4D & matrix)
	{
		M00 -= matrix.M00; M01 -= matrix.M01; M02 -= matrix.M02; M03 -= matrix.M03;
		M10 -= matrix.M10; M11 -= matrix.M11; M12 -= matrix.M12; M13 -= matrix.M13;
		M20 -= matrix.M20; M21 -= matrix.M21; M22 -= matrix.M22; M23 -= matrix.M23;
		M30 -= matrix.M30; M31 -= matrix.M31; M32 -= matrix.M32; M33 -= matrix.M33;

		return *this;
	}

	Matrix4D& operator*=(const Matrix4D & matrix)
	{
		return (*this = *this * matrix);
	}

	Matrix4D& operator*=(Float fScalar)
	{
		M00 *= fScalar; M01 *= fScalar; M02 *= fScalar; M03 *= fScalar;
		M10 *= fScalar; M11 *= fScalar; M12 *= fScalar; M13 *= fScalar;
		M20 *= fScalar; M21 *= fScalar; M22 *= fScalar; M23 *= fScalar;
		M30 *= fScalar; M31 *= fScalar; M32 *= fScalar; M33 *= fScalar;

		return *this;
	}

	Matrix4D&operator/=(Float fScalar)
	{
		M00 /= fScalar; M01 /= fScalar; M02 /= fScalar; M03 /= fScalar;
		M10 /= fScalar; M11 /= fScalar; M12 /= fScalar; M13 /= fScalar;
		M20 /= fScalar; M21 /= fScalar; M22 /= fScalar; M23 /= fScalar;
		M30 /= fScalar; M31 /= fScalar; M32 /= fScalar; M33 /= fScalar;

		return *this;
	}

	Bool operator==(const Matrix4D & matrix) const
	{
		return (M00 == matrix.M00) && (M01 == matrix.M01) && (M02 == matrix.M02) && (M03 == matrix.M03) &&
			(M10 == matrix.M10) && (M11 == matrix.M11) && (M12 == matrix.M12) && (M13 == matrix.M13) &&
			(M20 == matrix.M20) && (M21 == matrix.M21) && (M22 == matrix.M22) && (M23 == matrix.M23) &&
			(M30 == matrix.M30) && (M31 == matrix.M31) && (M32 == matrix.M32) && (M33 == matrix.M33);
	}

	Bool operator!=(const Matrix4D & matrix) const
	{
		return (M00 != matrix.M00) || (M01 != matrix.M01) || (M02 != matrix.M02) || (M03 != matrix.M03) ||
			(M10 != matrix.M10) || (M11 != matrix.M11) || (M12 != matrix.M12) || (M13 != matrix.M13) ||
			(M20 != matrix.M20) || (M21 != matrix.M21) || (M22 != matrix.M22) || (M23 != matrix.M23) ||
			(M30 != matrix.M30) || (M31 != matrix.M31) || (M32 != matrix.M32) || (M33 != matrix.M33);
	}

	/**
	 * Returns true if all of the components of this
	 * matrix are equal to matrix within the tolerance
	 * fTolerance, false otherwise.
	 */
	Bool Equals(
		const Matrix4D& mMatrix,
		Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(M00, mMatrix.M00, fTolerance) &&
			::Seoul::Equals(M01, mMatrix.M01, fTolerance) &&
			::Seoul::Equals(M02, mMatrix.M02, fTolerance) &&
			::Seoul::Equals(M03, mMatrix.M03, fTolerance) &&

			::Seoul::Equals(M10, mMatrix.M10, fTolerance) &&
			::Seoul::Equals(M11, mMatrix.M11, fTolerance) &&
			::Seoul::Equals(M12, mMatrix.M12, fTolerance) &&
			::Seoul::Equals(M13, mMatrix.M13, fTolerance) &&

			::Seoul::Equals(M20, mMatrix.M20, fTolerance) &&
			::Seoul::Equals(M21, mMatrix.M21, fTolerance) &&
			::Seoul::Equals(M22, mMatrix.M22, fTolerance) &&
			::Seoul::Equals(M23, mMatrix.M23, fTolerance) &&

			::Seoul::Equals(M30, mMatrix.M30, fTolerance) &&
			::Seoul::Equals(M31, mMatrix.M31, fTolerance) &&
			::Seoul::Equals(M32, mMatrix.M32, fTolerance) &&
			::Seoul::Equals(M33, mMatrix.M33, fTolerance);
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
			::Seoul::IsZero(M03, fTolerance) &&

			::Seoul::IsZero(M10, fTolerance) &&
			::Seoul::IsZero(M11, fTolerance) &&
			::Seoul::IsZero(M12, fTolerance) &&
			::Seoul::IsZero(M13, fTolerance) &&

			::Seoul::IsZero(M20, fTolerance) &&
			::Seoul::IsZero(M21, fTolerance) &&
			::Seoul::IsZero(M22, fTolerance) &&
			::Seoul::IsZero(M23, fTolerance) &&

			::Seoul::IsZero(M30, fTolerance) &&
			::Seoul::IsZero(M31, fTolerance) &&
			::Seoul::IsZero(M32, fTolerance) &&
			::Seoul::IsZero(M33, fTolerance);
	}

	/**
	 * Returns a Vector4D containing the diagonal components
	 * of this Matrix4D.
	 */
	Vector4D GetDiagonal() const
	{
		return Vector4D(M00, M11, M22, M33);
	}

	Matrix4D Transpose() const
	{
		return Matrix4D(M00, M10, M20, M30,
			M01, M11, M21, M31,
			M02, M12, M22, M32,
			M03, M13, M23, M33);
	}

	Float Determinant() const
	{
		return   M00 * (  M11 * (M22 * M33 - M23 * M32)
			- M12 * (M21 * M33 - M23 * M31)
			+ M13 * (M21 * M32 - M22 * M31))
			- M01 * (  M10 * (M22 * M33 - M23 * M32)
			- M12 * (M20 * M33 - M23 * M30)
			+ M13 * (M20 * M32 - M22 * M30))
			+ M02 * (  M10 * (M21 * M33 - M23 * M31)
			- M11 * (M20 * M33 - M23 * M30)
			+ M13 * (M20 * M31 - M21 * M30))
			- M03 * (  M10 * (M21 * M32 - M22 * M31)
			- M11 * (M20 * M32 - M22 * M30)
			+ M12 * (M20 * M31 - M21 * M30));
	}

	Float DeterminantUpper3x3() const
	{
		return
			M00 * (M11 * M22 - M12 * M21) -
			M01 * (M10 * M22 - M12 * M20) +
			M02 * (M10 * M21 - M11 * M20);
	}

	// Assume this Matrix4D is already a standard projection transform,
	// converts it to a biased projection transform. Positive epsilon values
	// result in projected depth values closer to the camera.
	Matrix4D BiasedProjection(Double fEpsilon = kfBiasProjectionEpsilon) const;

	// Assume this Matrix4D is already a standard projection transform,
	// converts it to an infinite projection transform. Positive epsilon
	// values result in projected epth values closer to the camera, with
	// the minimum depth value being at "infinity". Note that negative values
	// or values < kfInfiniteProjectionEpsilon will result in incorrect
	// projection.
	Matrix4D InfiniteProjection(Double fEpsilon = kfInfiniteProjectionEpsilon) const;

	Matrix4D Inverse() const
	{
		Float fDet = Determinant();
		if (::Seoul::IsZero(fDet, 1e-10f))
		{
			return Identity();
		}

		Float fInvDet = 1.0f / fDet;

		return Matrix4D(fInvDet * ( M11 * (M22 * M33 - M23 * M32) - M12 * (M21 * M33 - M23 * M31) + M13 * (M21 * M32 - M22 * M31)),
			fInvDet * (-M01 * (M22 * M33 - M23 * M32) + M02 * (M21 * M33 - M23 * M31) - M03 * (M21 * M32 - M22 * M31)),
			fInvDet * ( M01 * (M12 * M33 - M13 * M32) - M02 * (M11 * M33 - M13 * M31) + M03 * (M11 * M32 - M12 * M31)),
			fInvDet * (-M01 * (M12 * M23 - M13 * M22) + M02 * (M11 * M23 - M13 * M21) - M03 * (M11 * M22 - M12 * M21)),

			fInvDet * (-M10 * (M22 * M33 - M23 * M32) + M12 * (M20 * M33 - M23 * M30) - M13 * (M20 * M32 - M22 * M30)),
			fInvDet * ( M00 * (M22 * M33 - M23 * M32) - M02 * (M20 * M33 - M23 * M30) + M03 * (M20 * M32 - M22 * M30)),
			fInvDet * (-M00 * (M12 * M33 - M13 * M32) + M02 * (M10 * M33 - M13 * M30) - M03 * (M10 * M32 - M12 * M30)),
			fInvDet * ( M00 * (M12 * M23 - M13 * M22) - M02 * (M10 * M23 - M13 * M20) + M03 * (M10 * M22 - M12 * M20)),

			fInvDet * ( M10 * (M21 * M33 - M23 * M31) - M11 * (M20 * M33 - M23 * M30) + M13 * (M20 * M31 - M21 * M30)),
			fInvDet * (-M00 * (M21 * M33 - M23 * M31) + M01 * (M20 * M33 - M23 * M30) - M03 * (M20 * M31 - M21 * M30)),
			fInvDet * ( M00 * (M11 * M33 - M13 * M31) - M01 * (M10 * M33 - M13 * M30) + M03 * (M10 * M31 - M11 * M30)),
			fInvDet * (-M00 * (M11 * M23 - M13 * M21) + M01 * (M10 * M23 - M13 * M20) - M03 * (M10 * M21 - M11 * M20)),

			fInvDet * (-M10 * (M21 * M32 - M22 * M31) + M11 * (M20 * M32 - M22 * M30) - M12 * (M20 * M31 - M21 * M30)),
			fInvDet * ( M00 * (M21 * M32 - M22 * M31) - M01 * (M20 * M32 - M22 * M30) + M02 * (M20 * M31 - M21 * M30)),
			fInvDet * (-M00 * (M11 * M32 - M12 * M31) + M01 * (M10 * M32 - M12 * M30) - M02 * (M10 * M31 - M11 * M30)),
			fInvDet * ( M00 * (M11 * M22 - M12 * M21) - M01 * (M10 * M22 - M12 * M20) + M02 * (M10 * M21 - M11 * M20)));
	}

	/**
	 * Returns true if this Matrix4D is orthonormal, false
	 * otherwise.
	 *
	 * \warning This method is not cheap - it is recommended that you
	 * only use it for debug-time checks.
	 */
	Bool IsOrthonormal(Float fTolerance = 1e-4f) const
	{
		Matrix3D mTestMatrix3D;
		GetRotation(mTestMatrix3D);

		if (!mTestMatrix3D.IsOrthonormal(fTolerance))
		{
			return false;
		}
		else
		{
			// Make sure the last row is [0.0f, 0.0f, 0.0f, 1.0f]
			if (!Seoul::Equals(M30, 0.0f, fTolerance) ||
				!Seoul::Equals(M31, 0.0f, fTolerance) ||
				!Seoul::Equals(M32, 0.0f, fTolerance) ||
				!Seoul::Equals(M33, 1.0f, fTolerance))
			{
				return false;
			}
		}

		return true;
	}

	/** For projection matrices, return true if this matrix is a perpsective projection. */
	Bool IsPerspective() const
	{
		return (M32 < 0.0f);
	}

	/**
	 * Returns the inverse of this Matrix4D when this
	 * Matrix4D is orthonormal.
	 *
	 * \warning The return value of this method is undefined if this
	 * Matrix4D is not orthonormal. This method will SEOUL_ASSERT that 
	 * this Matrix4D is orthornormal in debug builds.
	 */
	Matrix4D OrthonormalInverse() const
	{
		// We only want to check this in debug because the check for
		// orthonormality is expensive and will add a lot of unwanted
		// overhead to the developer build.
		SEOUL_ASSERT_DEBUG(IsOrthonormal(1e-3f));

		// Transpose this Matrix4D to invert the rotation part.
		Matrix4D ret = Transpose();

		// Zero out the translation part of the transposed Matrix4D (note
		// that the translation in the transpose will be in the last row
		// instead of the last column).
		ret.M30 = 0.0f;
		ret.M31 = 0.0f;
		ret.M32 = 0.0f;

		// Set a new translation which is the negative of the original translation,
		// transformed by the inverse of this Matrix4D's rotation part.
		ret.SetTranslation(Matrix4D::TransformPosition(ret, -GetTranslation()));

		return ret;
	}

	/**
	 * @return A read-only array reference to the data in this Matrix4D.
	 */
	Float const* GetData() const
	{
		return &M00;
	}

	/**
	 * @return A writeable array reference to the data in this Matrix4D.
	 */
	Float* GetData()
	{
		return &M00;
	}

	/**
	 * Access the elements of this Matrix4D by
	 * the row iRow and the column iColumn.
	 */
	Float operator()(Int32 iRow, Int32 iColumn) const
	{
		SEOUL_ASSERT(iRow >= 0 && iRow < 4 && iColumn >= 0 && iColumn < 4);

		// The elements of a Matrix4D are stored
		// in column-major order.
		return GetData()[iColumn * 4 + iRow];
	}

	/**
	 * Access the elements of this Matrix4D by
	 * the row iRow and the column iColumn.
	 */
	Float& operator()(Int32 iRow, Int32 iColumn)
	{
		SEOUL_ASSERT(iRow >= 0 && iRow < 4 && iColumn >= 0 && iColumn < 4);

		// The elements of a Matrix4D are stored
		// in column-major order.
		return GetData()[iColumn * 4 + iRow];
	}

	/**
	 * Get column iIndex of this matrix as a Vector4D.
	 */
	Vector4D GetColumn(Int iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 4);
		const Matrix4D& m = *this;

		Vector4D ret(m(0, iIndex), m(1, iIndex), m(2, iIndex), m(3, iIndex));
		return ret;
	}

	/**
	 * Get row iIndex of this matrix as a Vector4D.
	 */
	Vector4D GetRow(Int iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 4);
		const Matrix4D& m = *this;

		Vector4D ret(m(iIndex, 0), m(iIndex, 1), m(iIndex, 2), m(iIndex, 3));
		return ret;
	}

	/**
	 * Gets the corresponding basis axis from this matrix and
	 *  normalizes it.
	 */
	Vector3D GetUnitAxis(Int iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 3);
		const Matrix4D& m = *this;

		Vector3D ret(m(iIndex, 0), m(iIndex, 1), m(iIndex, 2));
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
	void GetUnitAxes(Vector3D& X, Vector3D& Y, Vector3D& Z) const
	{
		X = GetUnitAxis((Int)Axis::kX);
		Y = GetUnitAxis((Int)Axis::kY);
		Z = GetUnitAxis((Int)Axis::kZ);
	}

	/**
	 * Set column iIndex of this matrix from a Vector4D.
	 */
	void SetColumn(Int iIndex, const Vector4D& v)
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 4);
		Matrix4D& m = *this;

		m(0, iIndex) = v.X;
		m(1, iIndex) = v.Y;
		m(2, iIndex) = v.Z;
		m(3, iIndex) = v.W;
	}

	/**
	 * Set row iIndex of this matrix from a Vector4D.
	 */
	void SetRow(Int iIndex, const Vector4D& v)
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 4);
		Matrix4D& m = *this;

		m(iIndex, 0) = v.X;
		m(iIndex, 1) = v.Y;
		m(iIndex, 2) = v.Z;
		m(iIndex, 3) = v.W;
	}

	/**
	 * Performs mMatrix * vVector
	 */
	static Vector4D Transform(const Matrix4D& mMatrix, const Vector4D& vVector)
	{
		return Vector4D(
			mMatrix.M00 * vVector.X + mMatrix.M01 * vVector.Y + mMatrix.M02 * vVector.Z + mMatrix.M03 * vVector.W,
			mMatrix.M10 * vVector.X + mMatrix.M11 * vVector.Y + mMatrix.M12 * vVector.Z + mMatrix.M13 * vVector.W,
			mMatrix.M20 * vVector.X + mMatrix.M21 * vVector.Y + mMatrix.M22 * vVector.Z + mMatrix.M23 * vVector.W,
			mMatrix.M30 * vVector.X + mMatrix.M31 * vVector.Y + mMatrix.M32 * vVector.Z + mMatrix.M33 * vVector.W);
	}

	/**
	 * Performs mMatrix * vVector, where vector is
	 * treated as a direction vector (implicit w component of 0.0).
	 */
	static Vector3D TransformDirection(
		const Matrix4D& mMatrix,
		const Vector3D& vVector)
	{
		return Vector3D(
			mMatrix.M00 * vVector.X + mMatrix.M01 * vVector.Y + mMatrix.M02 * vVector.Z,
			mMatrix.M10 * vVector.X + mMatrix.M11 * vVector.Y + mMatrix.M12 * vVector.Z,
			mMatrix.M20 * vVector.X + mMatrix.M21 * vVector.Y + mMatrix.M22 * vVector.Z);
	}

	/**
	 * Performs mMatrix * vVector, where vector is
	 * treated as a position vector (implicit w component of 1.0).
	 */
	static Vector3D TransformPosition(
		const Matrix4D& mMatrix,
		const Vector3D& vVector)
	{
		return Vector3D(
			mMatrix.M00 * vVector.X + mMatrix.M01 * vVector.Y + mMatrix.M02 * vVector.Z + mMatrix.M03,
			mMatrix.M10 * vVector.X + mMatrix.M11 * vVector.Y + mMatrix.M12 * vVector.Z + mMatrix.M13,
			mMatrix.M20 * vVector.X + mMatrix.M21 * vVector.Y + mMatrix.M22 * vVector.Z + mMatrix.M23);
	}

	/**
	 * Sets the upper 3x3 portion of this to the rotation
	 * described by matrix mRotation
	 */
	void SetRotation(const Matrix3D& mRotation)
	{
		M00 = mRotation.M00; M01 = mRotation.M01; M02 = mRotation.M02;
		M10 = mRotation.M10; M11 = mRotation.M11; M12 = mRotation.M12;
		M20 = mRotation.M20; M21 = mRotation.M21; M22 = mRotation.M22;
	}

	/**
	 * Returns a Quaternion representing the rotation
	 * (upper 3x3) portion of this Matrix4D.
	 *
	 * \warning This function will return an invalid quaternion
	 * if the upper 3x3 portion of this Matrix4D is an invalid rotation.
	 */
	Quaternion GetRotation() const
	{
		// Quaternion::CreateFromRotationMatrix() does what we need.
		Quaternion const qReturn(Quaternion::CreateFromRotationMatrix(*this));

		return qReturn;
	}

	/**
	 * Returns the upper 3x3 portion of this Matrix4D as
	 * a Matrix3D in argument rMatrix3D.
	 */
	void GetRotation(Matrix3D& rMatrix3D) const
	{
		rMatrix3D.M00 = M00; rMatrix3D.M01 = M01; rMatrix3D.M02 = M02;
		rMatrix3D.M10 = M10; rMatrix3D.M11 = M11; rMatrix3D.M12 = M12;
		rMatrix3D.M20 = M20; rMatrix3D.M21 = M21; rMatrix3D.M22 = M22;
	}

	/**
	 * Sets the upper 3x3 portion of this matrix to the
	 * rotation described by qRotation.
	 */
	void SetRotation(const Quaternion& qRotation)
	{
		SetRotation(qRotation.GetMatrix3D());
	}

	/**
	 * Gets the translation components of this matrix
	 *  s a Vector3D.
	 */
	Vector3D GetTranslation() const
	{
		return Vector3D(M03, M13, M23);
	}

	/**
	 * Replaces the translation components of
	 * this matrix with the translation specified in
	 * vTranslation.
	 */
	void SetTranslation(const Vector3D& vTranslation)
	{
		M03 = vTranslation.X;
		M13 = vTranslation.Y;
		M23 = vTranslation.Z;
	}

	static Matrix4D Identity()
	{
		return Matrix4D(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
	}

	static Matrix4D Zero()
	{
		return Matrix4D(
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
	}

	static Matrix4D CreateRotationFromDirection(
		const Vector3D& vDirection,
		const Vector3D& vBasisDirection = -Vector3D::UnitZ());
	static Matrix4D CreateNormalTransform(const Matrix4D& matrix);
	static Matrix4D CreateRotationTranslation(
		const Matrix3D& mRotation,
		const Vector3D& vTranslation);
	static Matrix4D CreateRotationTranslation(
		const Quaternion& qRotation,
		const Vector3D& vTranslation);
	static Matrix4D CreateOrthographic(
		Float fLeft,
		Float fRight,
		Float fBottom,
		Float fTop,
		Float fNear,
		Float fFar);
	static Matrix4D CreatePerspectiveOffCenter(
		Float fLeft,
		Float fRight,
		Float fBottom,
		Float fTop,
		Float fNear,
		Float fFar);
	static Matrix4D CreatePerspectiveFromVerticalFieldOfView(
		Float fFovInRadians,
		Float fAspectRatio,
		Float fNear,
		Float fFar);
	static Matrix4D CreateRotationFromAxisAngle(
		const Vector3D& vAxis,
		Float fAngleInRadians);
	static Matrix4D CreateReflection(const Plane& plane);
	static Matrix4D CreateRotationX(Float fAngleInRadians);
	static Matrix4D CreateRotationY(Float fAngleInRadians);
	static Matrix4D CreateRotationZ(Float fAngleInRadians);
	static Matrix4D CreateScale(Float fScale);
	static Matrix4D CreateScale(Float fX, Float fY, Float fZ);
	static Matrix4D CreateScale(const Vector3D& vScale);
	static Matrix4D CreateTranslation(Float fX, Float fY, Float fZ);
	static Matrix4D CreateTranslation(const Vector3D& vTranslation);

	static Matrix4D CreateFromMatrix3D(const Matrix3D& m);

	static Bool Decompose(
		const Matrix4D& mTransform,
		Matrix3D& rmPreRotation,
		Matrix3D& rmRotation,
		Vector3D& rvTranslation);

	// Given a valid perspective or orthographic projection,
	// returns the near/far value. Return true if
	// mProjectionTransform was detected as a perspective transform.
	static Bool ExtractNearFar(
		const Matrix4D& mProjectionTransform,
		Float& rfNear,
		Float& rfFar);
	static Float ExtractFovInRadians(const Matrix4D& mProjectionTransform);
	static Float ExtractAspectRatio(const Matrix4D& mProjectionTransform);
	void UpdateAspectRatio(Float fAspectRatio);

	/**
	 * @return A matrix that is the linear interpolation of m0
	 * and m1 based on the weighting factor fT
	 */
	static Matrix4D Lerp(const Matrix4D& m0, const Matrix4D& m1, Float fT)
	{
		return m0 * (1.0f - fT) + m1 * fT;
	}
}; // struct Matrix4D
template <> struct CanMemCpy<Matrix4D> { static const Bool Value = true; };
template <> struct CanZeroInit<Matrix4D> { static const Bool Value = true; };

// Sanity checking assert, since a lot of low-level optimizations
// depend on this being true.
SEOUL_STATIC_ASSERT(16u * sizeof(Float) == sizeof(Matrix4D));

inline Matrix4D operator*(Float fScalar, const Matrix4D& mMatrix)
{
	return (mMatrix * fScalar);
}

/** Tolerance equality test between a and b. */
inline Bool Equals(const Matrix4D& mA, const Matrix4D& mB, Float fTolerance = fEpsilon)
{
	return mA.Equals(mB, fTolerance);
}

static inline UInt32 GetHash(const Matrix4D& m)
{
	UInt32 uHash = 0u;
	IncrementalHash(uHash, GetHash(m.M00));
	IncrementalHash(uHash, GetHash(m.M01));
	IncrementalHash(uHash, GetHash(m.M02));
	IncrementalHash(uHash, GetHash(m.M03));

	IncrementalHash(uHash, GetHash(m.M10));
	IncrementalHash(uHash, GetHash(m.M11));
	IncrementalHash(uHash, GetHash(m.M12));
	IncrementalHash(uHash, GetHash(m.M13));

	IncrementalHash(uHash, GetHash(m.M20));
	IncrementalHash(uHash, GetHash(m.M21));
	IncrementalHash(uHash, GetHash(m.M22));
	IncrementalHash(uHash, GetHash(m.M23));

	IncrementalHash(uHash, GetHash(m.M30));
	IncrementalHash(uHash, GetHash(m.M31));
	IncrementalHash(uHash, GetHash(m.M32));
	IncrementalHash(uHash, GetHash(m.M33));

	return uHash;
}

/**
 * @return A matrix that is the linear interpolation of m0
 * and m1 based on the weighting factor fT
 */
inline Matrix4D Lerp(const Matrix4D& m0, const Matrix4D& m1, Float fT)
{
	return Matrix4D::Lerp(m0, m1, fT);
}

} // namespace Seoul

#endif // include guard
