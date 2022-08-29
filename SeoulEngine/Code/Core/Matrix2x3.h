/**
 * \file Matrix2x3.h
 * \brief Matrix2x3 represents a 2x3 scale/rotation + translation matrix.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MATRIX2X3_H
#define MATRIX2X3_H

#include "Matrix2D.h"
#include "SeoulMath.h"
#include "SeoulTypeTraits.h"

namespace Seoul
{

struct Matrix2x3 SEOUL_SEALED
{
	Float M00, M10,
		  M01, M11;

	union
	{
		Float M02;
		Float TX;
	};
	union
	{
		Float M12;
		Float TY;
	};

	Matrix2x3()
		: M00(0.0f), M10(0.0f)
		, M01(0.0f), M11(0.0f)
		, M02(0.0f), M12(0.0f)
	{
	}

	Matrix2x3(Float m00, Float m01, Float m02,
		      Float m10, Float m11, Float m12)
		: M00(m00), M10(m10)
		, M01(m01), M11(m11)
		, M02(m02), M12(m12)
	{
	}

	Matrix2x3 operator*(const Matrix2x3& b) const
	{
		Matrix2x3 ret;
		ret.M00 = M00 * b.M00 + M01 * b.M10;
		ret.M01 = M00 * b.M01 + M01 * b.M11;
		ret.M02 = M00 * b.M02 + M01 * b.M12 + M02;

		ret.M10 = M10 * b.M00 + M11 * b.M10;
		ret.M11 = M10 * b.M01 + M11 * b.M11;
		ret.M12 = M10 * b.M02 + M11 * b.M12 + M12;
		return ret;
	}

	Matrix2x3& operator*=(const Matrix2x3& b)
	{
		*this = (*this * b);
		return *this;
	}

	Bool operator==(const Matrix2x3& matrix) const
	{
		return
			(M00 == matrix.M00) && (M01 == matrix.M01) && (M02 == matrix.M02) &&
			(M10 == matrix.M10) && (M11 == matrix.M11) && (M12 == matrix.M12);
	}

	Bool operator!=(const Matrix2x3& matrix) const
	{
		return
			(M00 != matrix.M00) || (M01 != matrix.M01) || (M02 != matrix.M02) ||
			(M10 != matrix.M10) || (M11 != matrix.M11) || (M12 != matrix.M12);
	}

	Float DeterminantUpper2x2() const
	{
		return (M00 * M11) - (M01 * M10);
	}

	/**
	 * Returns true if all of the components of this
	 * matrix are equal to matrix within the tolerance
	 * fTolerance, false otherwise.
	 */
	Bool Equals(
		const Matrix2x3& mMatrix,
		Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(M00, mMatrix.M00, fTolerance) &&
			::Seoul::Equals(M01, mMatrix.M01, fTolerance) &&
			::Seoul::Equals(M02, mMatrix.M02, fTolerance) &&

			::Seoul::Equals(M10, mMatrix.M10, fTolerance) &&
			::Seoul::Equals(M11, mMatrix.M11, fTolerance) &&
			::Seoul::Equals(M12, mMatrix.M12, fTolerance);
	}

	/**
	 * Returns a Vector2D containing the anti-diagonal components
	 * of the upper 2x2 portion of this Matrix2x3 (lower-left to upper-right).
	 */
	Vector2D GetAntiDiagonal() const
	{
		return Vector2D(M10, M01);
	}

	/** @return The indicated column of this matrix. */
	Vector2D GetColumn(Int32 iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 2);
		return Vector2D(
			(0 == iIndex) ? M00 : M01,
			(0 == iIndex) ? M10 : M11);
	}

	/**
	 * Returns a Vector2D containing the diagonal components
	 * of the upper 2x2 portion of this Matrix2x3.
	 */
	Vector2D GetDiagonal() const
	{
		return Vector2D(M00, M11);
	}

	/** @return The indicated row of this matrix. */
	Vector2D GetRow(Int32 iIndex) const
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 2);
		return Vector2D(
			(0 == iIndex) ? M00 : M10,
			(0 == iIndex) ? M01 : M11);
	}

	/**
	 * Gets the translation components of this matrix
	 * as a Vector2D.
	 */
	Vector2D GetTranslation() const
	{
		return Vector2D(TX, TY);
	}

	/** Return the upper 2x2 portion of this matrix. */
	Matrix2D GetUpper2x2() const
	{
		Matrix2D ret;
		ret.M00 = M00;
		ret.M01 = M01;
		ret.M10 = M10;
		ret.M11 = M11;
		return ret;
	}

	Matrix2x3 Inverse() const
	{
		Float const fDeterminant = (M00 * M11) - (M01 * M10);
		if (Seoul::IsZero(fDeterminant))
		{
			return *this;
		}

		const Float fInverseDeterminant = (1.0f / fDeterminant);
		Matrix2x3 mReturn;
		mReturn.M00 =  M11 * fInverseDeterminant;
		mReturn.M11 =  M00 * fInverseDeterminant;
		mReturn.M01 = -M01 * fInverseDeterminant;
		mReturn.M10 = -M10 * fInverseDeterminant;

		mReturn.M02 = -(mReturn.M00 * M02 + mReturn.M01 * M12);
		mReturn.M12 = -(mReturn.M10 * M02 + mReturn.M11 * M12);

		return mReturn;
	}

	/** Set the indicated column of this matrix. */
	void SetColumn(Int32 iIndex, const Vector2D& v)
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 2);
		if (0 == iIndex)
		{
			M00 = v.X;
			M10 = v.Y;
		}
		else
		{
			M01 = v.X;
			M11 = v.Y;
		}
	}

	/** Set the indicated row of this matrix. */
	void SetRow(Int32 iIndex, const Vector2D& v)
	{
		SEOUL_ASSERT(iIndex >= 0 && iIndex < 2);
		if (0 == iIndex)
		{
			M00 = v.X;
			M01 = v.Y;
		}
		else
		{
			M10 = v.X;
			M11 = v.Y;
		}
	}

	/** Updates the translation components of this matrix. */
	void SetTranslation(const Vector2D& v)
	{
		TX = v.X;
		TY = v.Y;
	}
	
	/** Update the upper 2x2 portion of this matrix from matrix. */
	void SetUpper2x2(const Matrix2D& m)
	{
		M00 = m.M00;
		M01 = m.M01;
		M10 = m.M10;
		M11 = m.M11;
	}

	static Matrix2x3 Identity()
	{
		Matrix2x3 ret;
		ret.M00 = 1.0f;
		ret.M01 = 0.0f;
		ret.M10 = 0.0f;
		ret.M11 = 1.0f;
		ret.TX = 0.0f;
		ret.TY = 0.0f;
		return ret;
	}

	static Matrix2x3 CreateFrom(const Matrix2D& m, const Vector2D& v)
	{
		Matrix2x3 ret;
		ret.M00 = m.M00;
		ret.M01 = m.M01;
		ret.M10 = m.M10;
		ret.M11 = m.M11;
		ret.TX = v.X;
		ret.TY = v.Y;
		return ret;
	}

	/** Convenience variation of CreateRotation, for angle in degrees. */
	static Matrix2x3 CreateRotationFromDegrees(Float fAngleInDegrees)
	{
		return CreateRotation(DegreesToRadians(fAngleInDegrees));
	}

	/** Create a 2x3 m that specifies a rotation in the XY plane. */
	static Matrix2x3 CreateRotation(Float fAngleInRadians)
	{
		Float const fCosAngle = Cos(fAngleInRadians);
		Float const fSinAngle = Sin(fAngleInRadians);

		return Matrix2x3(
			fCosAngle, -fSinAngle, 0.0f,
			fSinAngle, fCosAngle, 0.0f);
	}

	static Matrix2x3 CreateScale(Float fSX, Float fSY)
	{
		Matrix2x3 ret = Identity();
		ret.M00 = fSX;
		ret.M11 = fSY;
		return ret;
	}

	static Matrix2x3 CreateTranslation(Float fX, Float fY)
	{
		Matrix2x3 ret = Identity();
		ret.TX = fX;
		ret.TY = fY;
		return ret;
	}

	static Matrix2x3 CreateTranslation(const Vector2D& v)
	{
		Matrix2x3 ret = Identity();
		ret.TX = v.X;
		ret.TY = v.Y;
		return ret;
	}

	static Bool Decompose(
		const Matrix2x3& m,
		Matrix2D& rmPreRotation,
		Matrix2D& rmRotation,
		Vector2D& rvTranslation)
	{
		if (Matrix2D::Decompose(m.GetUpper2x2(), rmPreRotation, rmRotation))
		{
			rvTranslation.X = m.TX;
			rvTranslation.Y = m.TY;
			return true;
		}
		return false;
	}

	static Vector2D TransformDirection(const Matrix2x3& m, const Vector2D& v)
	{
		Vector2D ret;
		ret.X = m.M00 * v.X + m.M01 * v.Y;
		ret.Y = m.M10 * v.X + m.M11 * v.Y;
		return ret;
	}

	static Float TransformDirectionX(const Matrix2x3& m, Float fX)
	{
		return Vector2D(m.M00 * fX, m.M10 * fX).Length();
	}

	static Float TransformDirectionY(const Matrix2x3& m, Float fY)
	{
		return Vector2D(m.M01 * fY, m.M11 * fY).Length();
	}

	static Vector2D TransformPosition(const Matrix2x3& m, const Vector2D& v)
	{
		Vector2D ret;
		ret.X = m.M00 * v.X + m.M01 * v.Y + m.M02;
		ret.Y = m.M10 * v.X + m.M11 * v.Y + m.M12;
		return ret;
	}
}; // struct Matrix2x3
template <> struct CanMemCpy<Matrix2x3> { static const Bool Value = true; };
template <> struct CanZeroInit<Matrix2x3> { static const Bool Value = true; };

// Sanity checking assert, since a lot of low-level optimizations
// depend on this being true.
SEOUL_STATIC_ASSERT(6u * sizeof(Float) == sizeof(Matrix2x3));

/** Tolerance equality test between a and b. */
inline Bool Equals(const Matrix2x3& mA, const Matrix2x3& mB, Float fTolerance = fEpsilon)
{
	return mA.Equals(mB, fTolerance);
}
} // namespace Seoul

#endif // include guard
