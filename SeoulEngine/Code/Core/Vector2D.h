/**
 * \file Vector2D.h
 * \brief 2D linear algebra vector class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VECTOR2D_H
#define VECTOR2D_H

#include "HashFunctions.h"
#include "SeoulAssert.h"
#include "SeoulMath.h"
#include "SeoulTypes.h"
#include "SeoulTypeTraits.h"

namespace Seoul
{

/**
 * 2D vector class
 *
 * 2D vector class.  It provides operator overloads for the most common vector
 * operations.  The components are accessible through the X and Y members;
 * they are also accessible through operator[].
 */
struct Vector2D SEOUL_SEALED
{
	Float X;
	Float Y;

	/**
	 * Default constructor.
	 */
	Vector2D()
		: X(0.0f)
		, Y(0.0f)
	{
	}

	/**
	 * Constructs a vector, assigning f to all 2 components.
	 *
	 * @param[in] f scalr value used for X and Y components
	 */
	explicit Vector2D(Float f)
		: X(f)
		, Y(f)
	{
	}

	/**
	 * Constructs a vector out of the 2 given values.
	 *
	 * @param[in] fX X component
	 * @param[in] fY Y component
	 */
	Vector2D(Float fX, Float fY)
		: X(fX)
		, Y(fY)
	{
	}

	/**
	 * Copy constructor.  Copies another 2D vector.
	 *
	 * @param[in] vOther Vector to copy
	 */
	Vector2D(const Vector2D& v)
		: X(v.X)
		, Y(v.Y)
	{
	}

	/**
	 * @return A read-only array reference to the data in this Vector2D.
	 */
	Float const* GetData() const
	{
		return &X;
	}

	/**
	 * @return A writeable array reference to the data in this Vector2D.
	 */
	Float* GetData()
	{
		return &X;
	}

	/**
	 * Component access (non-const objects)
	 *
	 * Returns a reference to the i 'th component.  Validates
	 * i in Debug mode only.
	 *
	 * @param[in] i Index of the component to return
	 * @return Reference to the i 'th component
	 */
	Float& operator[](Int i)
	{
		SEOUL_ASSERT(i >= 0 && i <= 1);
		return GetData()[i];
	}

	/**
	 * Component access (const objects)
	 *
	 * Returns the i 'th component.  Validates i in Debug mode
	 * only.
	 *
	 * @param[in] i Index of the component to return
	 * @return The i 'th component
	 */
	Float operator[](Int i) const
	{
		SEOUL_ASSERT(i >= 0 && i <= 1);
		return GetData()[i];
	}

	/**
	 * Computes the vector sum of two vectors
	 *
	 * @param[in] vOther Other vector to add to this
	 *
	 * @return Vector sum of this and vOther
	 */
	Vector2D operator + (const Vector2D & vOther) const
	{
		return Vector2D(X + vOther.X,
			            Y + vOther.Y);
	}

	/**
	 * Computes the vector difference of two vectors
	 *
	 * @param[in] vOther Other vector to subtract from this
	 *
	 * @return Vector difference of this and vOther
	 */
	Vector2D operator - (const Vector2D & vOther) const
	{
		return Vector2D(
			X - vOther.X,
			Y - vOther.Y);
	}

	/**
	 * Computes the negation of this vector
	 *
	 * @return The vector negation of this
	 */
	Vector2D operator-() const
	{
		return Vector2D(
			-X,
			-Y);
	}

	/**
	 * Computes the scalar product of this vector and a scalar
	 *
	 * @param[in] fScalar The scalar to multiply by
	 *
	 * @return Scalar product of this and fScalar
	 */
	Vector2D operator*(Float fScalar) const
	{
		return Vector2D(
			X * fScalar,
			Y * fScalar);
	}

	/**
	 * Computes the scalar division of this vector by a scalar
	 *
	 * @param[in] fScalar The scalar to divide by
	 *
	 * @return Scalar division of this by fScalar
	 */
	Vector2D operator / (Float fScalar) const
	{
		return Vector2D(
			X / fScalar,
			Y / fScalar);
	}

	/**
	 * Assigns another vector to this vector
	 *
	 * @param[in] vOther The vector being assigned to this
	 *
	 * @return A const reference to this
	 */
	Vector2D& operator=(const Vector2D & vOther)
	{
		X = vOther.X;
		Y = vOther.Y;

		return *this;
	}

	/**
	 * Adds another vector to this vector
	 *
	 * @param[in] vOther The vector to add to this
	 *
	 * @return A const reference to this
	 */
	Vector2D& operator+=(const Vector2D & vOther)
	{
		X += vOther.X;
		Y += vOther.Y;

		return *this;
	}

	/**
	 * Subtracts another vector from this vector
	 *
	 * @param[in] vOther The vector to subtract from this
	 *
	 * @return A const reference to this
	 */
	Vector2D& operator-=(const Vector2D & vOther)
	{
		X -= vOther.X;
		Y -= vOther.Y;

		return *this;
	}

	/**
	 * Multiplies this vector by a scalar
	 *
	 * @param[in] fScalar The scalar to multiply this vector by
	 *
	 * @return A const reference to this
	 */
	Vector2D& operator*=(Float fScalar)
	{
		X *= fScalar;
		Y *= fScalar;

		return *this;
	}

	/**
	 * Divides this vector by a scalar
	 *
	 * @param[in] fScalar The scalar to divide this vector by
	 *
	 * @return A const reference to this
	 */
	Vector2D& operator/=(Float fScalar)
	{
		X /= fScalar;
		Y /= fScalar;

		return *this;
	}

	/**
	 * Tests this vector for equality with another vector
	 *
	 * @param[in] vOther The vector to compare against
	 *
	 * @return true if this exactly equals vOther, or false otherwise
	 */
	Bool operator==(const Vector2D& vOther) const
	{
		return (X == vOther.X) && (Y == vOther.Y);
	}

	/**
	 * Tests this vector for inequality with another vector
	 *
	 * @param[in] vOther The vector to compare against
	 *
	 * @return true if this does not exactly equal vOther, or false
	 *         otherwise
	 */
	Bool operator!=(const Vector2D& vOther) const
	{
		return (X != vOther.X) || (Y != vOther.Y);
	}

	/**
	 * Returns a vector whose components are the absolute value
	 * of the components of this.
	 */
	Vector2D Abs() const
	{
		return Vector2D(::Seoul::Abs(X), ::Seoul::Abs(Y));
	}

	/**
	 * Tests this vector for equality with another vector with some amount of
	 * tolerance.  Two vectors are considered equal if each component differs by
	 * no more than the given tolerance.
	 *
	 * @param[in] vOther The vector to compare against
	 * @param[in] fTolerance The maximum amount of tolerance between each component
	 *        for the two vectors to be considered equal
	 *
	 * @return true if each component of this differs from the
	 *         corresponding component of vOther by no more than
	 *         fTolerance, or false otherwise
	 */
	Bool Equals(
		const Vector2D& vOther,
		Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(X, vOther.X, fTolerance) &&
			::Seoul::Equals(Y, vOther.Y, fTolerance);
	}

	/**
	 * @return The angle in radians in the XY plane of this vector,
	 * treated as a normal.
	 *
	 * @param[in] fZeroTolerance Normalization tolerance, if the vector
	 * is close to zero, an angle of zero will be returned.
	 */
	Float GetAngle(Float fZeroTolerance = 1e-6f) const
	{
		float const fLength = Length();
		if (Seoul::IsZero(fLength, fZeroTolerance))
		{
			return 0.0f;
		}

		Float const fReturn = Atan2(X / fLength, Y / fLength);
		return fReturn;
	}

	/**
	 * @return The max component of this vector's
	 * components.
	 */
	Float GetMaxComponent() const
	{
		return ::Seoul::Max(X, Y);
	}

	/**
	 * @return The min component of this vector's
	 * components.
	 */
	Float GetMinComponent() const
	{
		return ::Seoul::Min(X, Y);
	}

	/**
	 * Determines if this vector is within a given tolerance of the zero vector.
	 *
	 * @param[in] fTolerance The tolerance
	 *
	 * @return true if each component is within fTolerance of zero, or
	 *         false otherwise
	 */
	Bool IsZero(Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::IsZero(X, fTolerance) &&
			::Seoul::IsZero(Y, fTolerance);
	}

	/**
	 * Computes the length of this vector.  Note that this
	 * involves taking the square root.
	 *
	 * @return The length of this vector
	 */
	Float Length() const
	{
		return Sqrt(LengthSquared());
	}

	/**
	 * Computes the squared length of this vector.  This should be preferred to
	 * Length() whenever possible (e.g. comparing lengths, sphere tests,
	 * etc.), since it does not involve taking the square root.
	 *
	 * @return The squared length of this vector
	 */
	Float LengthSquared() const
	{
		return Dot(*this, *this);
	}

	/**
	 * Normalizes this vector to unit length.  If this vector's length squared
	 * is below the given tolerance, it is considered to be the zero vector and
	 * nothing further happens.
	 *
	 * @param[in] fTolerance The tolerance for considering this vector to be the
	 *        zero vector
	 *
	 * @return true if the vector's length squared was greater than
	 *         fTolerance and it was successfully normalized, or false
	 *         otherwise
	 */
	Bool Normalize(Float fTolerance = fEpsilon * fEpsilon)
	{
		Float fLengthSquared = LengthSquared();
		if (::Seoul::IsZero(fLengthSquared, fTolerance))
		{
			return false;
		}

		Float fLength = Sqrt(fLengthSquared);
		X /= fLength;
		Y /= fLength;

		return true;
	}
	static Vector2D One()
	{
		return Vector2D(1.0f, 1.0f);
	}

	static Vector2D UnitX()
	{
		return Vector2D(1.0f, 0.0f);
	}

	static Vector2D UnitY()
	{
		return Vector2D(0.0f, 1.0f);
	}

	static Vector2D Zero()
	{
		return Vector2D(0.0f, 0.0f);
	}

	/**
	 * Returns a vector whose components are the
	 * components of vValue clamped between the corresponding
	 * components of vMax and vMin.
	 */
	static Vector2D Clamp(const Vector2D& vValue, const Vector2D& vMin, const Vector2D& vMax)
	{
		return Vector2D(
			::Seoul::Clamp(vValue.X, vMin.X, vMax.X),
			::Seoul::Clamp(vValue.Y, vMin.Y, vMax.Y));
	}

	/** @return A new vector with  X = (vA.X / vB.X), Y = (vA.Y / vB.Y) */
	static Vector2D ComponentwiseDivide(const Vector2D& vA, const Vector2D& vB)
	{
		return Vector2D(
			vA.X / vB.X,
			vA.Y / vB.Y);
	}

	/** @return A new vector with X = (vA.X * vB.X), Y = (vA.Y * vB.Y), Z = (vA.Z * vB.Z) */
	static Vector2D ComponentwiseMultiply(const Vector2D& vA, const Vector2D& vB)
	{
		return Vector2D(
			vA.X * vB.X,
			vA.Y * vB.Y);
	}

	/**
	 * Computes the cross product (a x b).
	 *
	 * The cross product in 2D is a scalar, not a 2D vector.
	 */
	static Float Cross(const Vector2D& vA, const Vector2D& vB)
	{
		Float ret = ((vA.X * vB.Y) - (vA.Y * vB.X));

		return ret;
	}

	/**
	 * @return The dot product between vA and vB.
	 */
	static Float Dot(const Vector2D& vA, const Vector2D& vB)
	{
		return (vA.X * vB.X) + (vA.Y * vB.Y);
	}

	/** 2D Gram-Schmidt projection, used as part of orthogonalization. */
	static Vector2D GramSchmidtProjectionOperator(
		const Vector2D& e,
		const Vector2D& a)
	{
		return (e * (Dot(e, a) / Dot(e, e)));
	}

	/**
	 * @return A vector that is the linear interpolation of v0
	 *  and v1 based on the weighting factor fT
	 */
	static Vector2D Lerp(const Vector2D& v0, const Vector2D& v1, Float fT)
	{
		return v0 * (1.0f - fT) + v1 * fT;
	}

	/**
	 * Returns a vector whose components are the
	 * maximum of the components of vA and
	 * vB.
	 */
	static Vector2D Max(const Vector2D& vA, const Vector2D& vB)
	{
		return Vector2D(
			::Seoul::Max(vA.X, vB.X),
			::Seoul::Max(vA.Y, vB.Y));
	}

	/**
	 * Returns a vector whose components are the
	 * minimum of the components of vA and
	 * vB.
	 */
	static Vector2D Min(const Vector2D& vA, const Vector2D& vB)
	{
		return Vector2D(
			::Seoul::Min(vA.X, vB.X),
			::Seoul::Min(vA.Y, vB.Y));
	}

	/**
	 * Normalizes a vector to unit length.  If the vector's length squared
	 * is below the given tolerance, it is considered to be the zero vector and
	 * nothing further happens.
	 *
	 * @param[in] fTolerance The tolerance for considering the vector to be the
	 *        zero vector
	 *
	 * @return true if the vector's length squared was greater than
	 *         fTolerance and it was successfully normalized, or false
	 *         otherwise
	 */
	static Vector2D Normalize(const Vector2D& v)
	{
		Vector2D vReturn(v);
		(void)vReturn.Normalize();
		return vReturn;
	}

	/**
	 * @return A Vector2D that is perpendicular to the Vector2D v.
	 *
	 *  This operation in 2D is typically equivalent to the cross
	 *  product operator in other dimensions for many algorithms.
	 */
	static Vector2D Perpendicular(const Vector2D& v)
	{
		return Vector2D(v.Y, -v.X);
	}

	/** @return v with all components rounded to the nearest integer, using banker's rounding. */
	static Vector2D Round(const Vector2D& v)
	{
		return Vector2D(Seoul::Round(v.X), Seoul::Round(v.Y));
	}

	/**
	 * Calculates the cross product between vA and vB,
	 * and then normalizes the result, to produce a vector with unit length.
	 *
	 * @return A unit length vector which is the cross product of vA
	 * and vB.
	 */
	static Float UnitCross(const Vector2D& vA, const Vector2D& vB)
	{
		return Seoul::fSign(Cross(vA, vB));
	}
}; // struct Vector2D
template <> struct CanMemCpy<Vector2D> { static const Bool Value = true; };
template <> struct CanZeroInit<Vector2D> { static const Bool Value = true; };

SEOUL_STATIC_ASSERT_MESSAGE(sizeof(Vector2D) == 8, "IO readers/writers (amongst other code) assumes Vector2D is 8 bytes.");

/**
 * Computes the left-multiplication of a 2D vector by a scalar.
 *
 * @param[in] f The scalar to multiply by
 * @param[in] v The vector being multiplied by
 *
 * @return The scalar product of v by f
 */
inline Vector2D operator*(Float f, const Vector2D& v)
{
	return (v * f);
}

/** Tolerance equality test between a and b. */
inline Bool Equals(const Vector2D& vA, const Vector2D& vB, Float fTolerance = fEpsilon)
{
	return vA.Equals(vB, fTolerance);
}

inline UInt32 GetHash(const Vector2D& v)
{
	UInt32 uHash = 0u;
	IncrementalHash(uHash, GetHash(v.X));
	IncrementalHash(uHash, GetHash(v.Y));
	return uHash;
}

/**
 * @return A vector that is the linear interpolation of v0
 *  and v1 based on the weighting factor fT
 */
inline Vector2D Lerp(const Vector2D& v0, const Vector2D& v1, Float fT)
{
	return Vector2D::Lerp(v0, v1, fT);
}

} // namespace Seoul

#endif // include guard
