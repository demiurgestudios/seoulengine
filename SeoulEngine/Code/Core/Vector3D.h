/**
 * \file Vector3D.h
 * \brief 3D linear algebra vector class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VECTOR3D_H
#define VECTOR3D_H

#include "SeoulAssert.h"
#include "SeoulMath.h"
#include "SeoulTypeTraits.h"
#include "Vector2D.h"

namespace Seoul
{

/**
 * 3D vector class
 *
 * 3D vector class.  It provides operator overloads for the most common vector
 * operations.  The components are accessible through the X, Y, and Z members;
 * they are also accessible through operator[].
 */
struct Vector3D SEOUL_SEALED
{
	Float X;
	Float Y;
	Float Z;

	/**
	 * Default constructor.
	 */
	Vector3D()
		: X(0.0f)
		, Y(0.0f)
		, Z(0.0f)
	{
	}

	/**
	 * Constructs a vector, assigning f to all 3 components.
	 *
	 * @param[in] f scalr value used for X, Y, and Z components
	 */
	explicit Vector3D(Float f)
		: X(f)
		, Y(f)
		, Z(f)
	{
	}

	/**
	 * Constructs a vector out of the 3 given values.
	 *
	 * @param[in] fX X component
	 * @param[in] fY Y component
	 * @param[in] fZ Z component
	 */
	Vector3D(Float fX, Float fY, Float fZ)
		: X(fX)
		, Y(fY)
		, Z(fZ)
	{
	}

	/**
	 * Copy constructor.  Copies another 3D vector.
	 *
	 * @param[in] vOther Vector to copy
	 */
	Vector3D(const Vector3D & vOther)
		: X(vOther.X)
		, Y(vOther.Y)
		, Z(vOther.Z)
	{
	}

	/**
	 * Constructs a 3D vector out of a 2D vector and a float.
	 */
	Vector3D(const Vector2D & vOther, Float f)
		: X(vOther.X)
		, Y(vOther.Y)
		, Z(f)
	{
	}

	/**
	 * @return A read-only array reference to the data in this Vector3D.
	 */
	Float const* GetData() const
	{
		return &X;
	}

	/**
	 * @return A writeable array reference to the data in this Vector3D.
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
		SEOUL_ASSERT(i >= 0 && i <= 2);
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
		SEOUL_ASSERT(i >= 0 && i <= 2);
		return GetData()[i];
	}

	/**
	 * Computes the vector sum of two vectors
	 *
	 * @param[in] vOther Other vector to add to this
	 *
	 * @return Vector sum of this and vOther
	 */
	Vector3D operator+(const Vector3D & vOther) const
	{
		return Vector3D(
			X + vOther.X,
			Y + vOther.Y,
			Z + vOther.Z);
	}

	/**
	 * Computes the vector difference of two vectors
	 *
	 * @param[in] vOther Other vector to subtract from this
	 *
	 * @return Vector difference of this and vOther
	 */
	Vector3D operator-(const Vector3D & vOther) const
	{
		return Vector3D(
			X - vOther.X,
			Y - vOther.Y,
			Z - vOther.Z);
	}

	/**
	 * Computes the negation of this vector
	 *
	 * @return The vector negation of this
	 */
	Vector3D operator-() const
	{
		return Vector3D(
			-X,
			-Y,
			-Z);
	}

	/**
	 * Computes the scalar product of this vector and a scalar
	 *
	 * @param[in] fScalar The scalar to multiply by
	 *
	 * @return Scalar product of this and fScalar
	 */
	Vector3D operator*(Float fScalar) const
	{
		return Vector3D(
			X * fScalar,
			Y * fScalar,
			Z * fScalar);
	}

	/**
	 * Computes the scalar division of this vector by a scalar
	 *
	 * @param[in] fScalar The scalar to divide by
	 *
	 * @return Scalar division of this by fScalar
	 */
	Vector3D operator/(Float fScalar) const
	{
		return Vector3D(
			X / fScalar,
			Y / fScalar,
			Z / fScalar);
	}

	/**
	 * Assigns another vector to this vector
	 *
	 * @param[in] vOther The vector being assigned to this
	 *
	 * @return A const reference to this
	 */
	Vector3D& operator=(const Vector3D & vOther)
	{
		X = vOther.X;
		Y = vOther.Y;
		Z = vOther.Z;

		return *this;
	}

	/**
	 * Adds another vector to this vector
	 *
	 * @param[in] vOther The vector to add to this
	 *
	 * @return A const reference to this
	 */
	Vector3D& operator+=(const Vector3D & vOther)
	{
		X += vOther.X;
		Y += vOther.Y;
		Z += vOther.Z;

		return *this;
	}

	/**
	 * Subtracts another vector from this vector
	 *
	 * @param[in] vOther The vector to subtract from this
	 *
	 * @return A const reference to this
	 */
	Vector3D& operator-=(const Vector3D & vOther)
	{
		X -= vOther.X;
		Y -= vOther.Y;
		Z -= vOther.Z;

		return *this;
	}

	/**
	 * Multiplies this vector by a scalar
	 *
	 * @param[in] fScalar The scalar to multiply this vector by
	 *
	 * @return A const reference to this
	 */
	Vector3D& operator*=(Float fScalar)
	{
		X *= fScalar;
		Y *= fScalar;
		Z *= fScalar;

		return *this;
	}

	/**
	 * Divides this vector by a scalar
	 *
	 * @param[in] fScalar The scalar to divide this vector by
	 *
	 * @return A const reference to this
	 */
	Vector3D& operator/=(Float fScalar)
	{
		X /= fScalar;
		Y /= fScalar;
		Z /= fScalar;

		return *this;
	}

	/**
	 * Tests this vector for equality with another vector
	 *
	 * @param[in] vOther The vector to compare against
	 *
	 * @return true if this exactly equals vOther, or false otherwise
	 */
	Bool operator==(const Vector3D & vOther) const
	{
		return (X == vOther.X) && (Y == vOther.Y) && (Z == vOther.Z);
	}

	/**
	 * Tests this vector for inequality with another vector
	 *
	 * @param[in] vOther The vector to compare against
	 *
	 * @return true if this does not exactly equal vOther, or false
	 *         otherwise
	 */
	Bool operator!=(const Vector3D & vOther) const
	{
		return (X != vOther.X) || (Y != vOther.Y) || (Z != vOther.Z);
	}

	/**
	 * Returns a vector whose components are the absolute value
	 * of the components of this.
	 */
	Vector3D Abs() const
	{
		return Vector3D(::Seoul::Abs(X), ::Seoul::Abs(Y), ::Seoul::Abs(Z));
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
		const Vector3D & vOther,
		Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(X, vOther.X, fTolerance) &&
			::Seoul::Equals(Y, vOther.Y, fTolerance) &&
			::Seoul::Equals(Z, vOther.Z, fTolerance);
	}

	/**
	 * @return The max component of this vector's
	 * components.
	 */
	Float GetMaxComponent() const
	{
		return ::Seoul::Max(X, Y, Z);
	}

	/**
	 * @return The min component of this vector's
	 * components.
	 */
	Float GetMinComponent() const
	{
		return ::Seoul::Min(X, Y, Z);
	}

	/**
	 * Returns a 2D vector made up of the X and Y
	 * components of this Vector3D.
	 */
	Vector2D GetXY() const
	{
		return Vector2D(X, Y);
	}

	/**
	 * Returns a 2D vector made up of the X and Z
	 * components of this Vector3D.
	 */
	Vector2D GetXZ() const
	{
		return Vector2D(X, Z);
	}

	/**
	 * Returns a 2D vector made up of the Y and Z
	 * components of this Vector3D.
	 */
	Vector2D GetYZ() const
	{
		return Vector2D(Y, Z);
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
			::Seoul::IsZero(Y, fTolerance) &&
			::Seoul::IsZero(Z, fTolerance);
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
		Z /= fLength;

		return true;
	}

	static Vector3D One()
	{
		return Vector3D(1.0f, 1.0f, 1.0f);
	}

	static Vector3D UnitX()
	{
		return Vector3D(1.0f, 0.0f, 0.0f);
	}

	static Vector3D UnitY()
	{
		return Vector3D(0.0f, 1.0f, 0.0f);
	}

	static Vector3D UnitZ()
	{
		return Vector3D(0.0f, 0.0f, 1.0f);
	}

	static Vector3D Zero()
	{
		return Vector3D(0.0f, 0.0f, 0.0f);
	}

	/**
	 * Returns a vector whose components are the
	 * components of vValue clamped between the corresponding
	 * components of vMax and vMin.
	 */
	static Vector3D Clamp(const Vector3D& vValue, const Vector3D& vMin, const Vector3D& vMax)
	{
		return Vector3D(
			::Seoul::Clamp(vValue.X, vMin.X, vMax.X),
			::Seoul::Clamp(vValue.Y, vMin.Y, vMax.Y),
			::Seoul::Clamp(vValue.Z, vMin.Z, vMax.Z));
	}

	/** @return A new vector with  X = (vA.X / vB.X), Y = (vA.Y / vB.Y), Z = (vA.Z / vB.Z) */
	static Vector3D ComponentwiseDivide(const Vector3D& vA, const Vector3D& vB)
	{
		return Vector3D(
			vA.X / vB.X,
			vA.Y / vB.Y,
			vA.Z / vB.Z);
	}

	/** @return A new vector with X = (vA.X * vB.X), Y = (vA.Y * vB.Y), Z = (vA.Z * vB.Z) */
	static Vector3D ComponentwiseMultiply(const Vector3D& vA, const Vector3D& vB)
	{
		return Vector3D(
			vA.X * vB.X,
			vA.Y * vB.Y,
			vA.Z * vB.Z);
	}

	/**
	 * @return The cross product between vA and vB.
	 */
	static Vector3D Cross(const Vector3D& vA, const Vector3D& vB)
	{
		return Vector3D(vA.Y * vB.Z - vA.Z * vB.Y,
			            vA.Z * vB.X - vA.X * vB.Z,
						vA.X * vB.Y - vA.Y * vB.X);
	}

	/**
	 * @return The dot product between vA and vB.
	 */
	static Float Dot(const Vector3D& vA, const Vector3D& vB)
	{
		return (vA.X * vB.X) + (vA.Y * vB.Y) + (vA.Z * vB.Z);
	}

	/** 3D Gram-Schmidt projection, used as part of orthogonalization. */
	static Vector3D GramSchmidtProjectionOperator(
		const Vector3D& e,
		const Vector3D& a)
	{
		return (e * (Dot(e, a) / Dot(e, e)));
	}

	/**
	 * @return A vector that is the linear interpolation of v0
	 *  and v1 based on the weighting factor fT
	 */
	static Vector3D Lerp(const Vector3D& v0, const Vector3D& v1, Float fT)
	{
		return v0 * (1.0f - fT) + v1 * fT;
	}

	/**
	 * Returns a vector whose components are the
	 * maximum of the components of vA and
	 * vB.
	 */
	static Vector3D Max(const Vector3D& vA, const Vector3D& vB)
	{
		return Vector3D(
			::Seoul::Max(vA.X, vB.X),
			::Seoul::Max(vA.Y, vB.Y),
			::Seoul::Max(vA.Z, vB.Z));
	}

	/**
	 * Returns a vector whose components are the
	 * maximum of the components of vA, vB and
	 * vC.
	 */
	static Vector3D Max(const Vector3D& vA, const Vector3D& vB, const Vector3D& vC)
	{
		return Vector3D(
			::Seoul::Max(vA.X, vB.X, vC.X),
			::Seoul::Max(vA.Y, vB.Y, vC.Y),
			::Seoul::Max(vA.Z, vB.Z, vC.Z));
	}

	/**
	 * Returns a vector whose components are the
	 * maximum of the components of vA, vB,
	 * vC, vD.
	 */
	static Vector3D Max(const Vector3D& vA, const Vector3D& vB, const Vector3D& vC, const Vector3D& vD)
	{
		return Vector3D(
			::Seoul::Max(vA.X, vB.X, vC.X, vD.X),
			::Seoul::Max(vA.Y, vB.Y, vC.Y, vD.Y),
			::Seoul::Max(vA.Z, vB.Z, vC.Z, vD.Z));
	}

	/**
	 * Returns a vector whose components are the
	 * minimum of the components of vA and
	 * vB.
	 */
	static Vector3D Min(const Vector3D& vA, const Vector3D& vB)
	{
		return Vector3D(
			::Seoul::Min(vA.X, vB.X),
			::Seoul::Min(vA.Y, vB.Y),
			::Seoul::Min(vA.Z, vB.Z));
	}

	/**
	 * Returns a vector whose components are the
	 * minimum of the components of vA, vB and
	 * vC.
	 */
	static Vector3D Min(const Vector3D& vA, const Vector3D& vB, const Vector3D& vC)
	{
		return Vector3D(
			::Seoul::Min(vA.X, vB.X, vC.X),
			::Seoul::Min(vA.Y, vB.Y, vC.Y),
			::Seoul::Min(vA.Z, vB.Z, vC.Z));
	}

	/**
	 * Returns a vector whose components are the
	 * minimum of the components of vA, vB,
	 * vC, vD.
	 */
	static Vector3D Min(const Vector3D& vA, const Vector3D& vB, const Vector3D& vC, const Vector3D& vD)
	{
		return Vector3D(
			::Seoul::Min(vA.X, vB.X, vC.X, vD.X),
			::Seoul::Min(vA.Y, vB.Y, vC.Y, vD.Y),
			::Seoul::Min(vA.Z, vB.Z, vC.Z, vD.Z));
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
	static Vector3D Normalize(const Vector3D& v)
	{
		Vector3D vReturn(v);
		(void)vReturn.Normalize();
		return vReturn;
	}

	/** @return v with all components rounded to the nearest integer, using banker's rounding. */
	static Vector3D Round(const Vector3D& v)
	{
		return Vector3D(Seoul::Round(v.X), Seoul::Round(v.Y), Seoul::Round(v.Z));
	}

	/**
	 * Calculates the cross product between vA and vB,
	 * and then normalizes the result, to produce a vector with unit length.
	 *
	 * @return A unit length vector which is the cross product of vA
	 * and vB.
	 */
	static Vector3D UnitCross(const Vector3D& vA, const Vector3D& vB)
	{
		return Normalize(Cross(vA, vB));
	}
}; // struct Vector3D
template <> struct CanMemCpy<Vector3D> { static const Bool Value = true; };
template <> struct CanZeroInit<Vector3D> { static const Bool Value = true; };

SEOUL_STATIC_ASSERT_MESSAGE(sizeof(Vector3D) == 12, "IO readers/writers (amongst other code) assumes Vector3D is 12 bytes.");

/**
 * Computes the left-multiplication of a 3D vector by a scalar.
 *
 * @param[in] f The scalar to multiply by
 * @param[in] v The vector being multiplied by
 *
 * @return The scalar product of v by f
 */
inline Vector3D operator*(Float f, const Vector3D& v)
{
	return (v * f);
}

/** Tolerance equality test between a and b. */
inline Bool Equals(const Vector3D& vA, const Vector3D& vB, Float fTolerance = fEpsilon)
{
	return vA.Equals(vB, fTolerance);
}

inline UInt32 GetHash(const Vector3D& v)
{
	UInt32 uHash = 0u;
	IncrementalHash(uHash, GetHash(v.X));
	IncrementalHash(uHash, GetHash(v.Y));
	IncrementalHash(uHash, GetHash(v.Z));
	return uHash;
}

/**
 * @return A vector that is the linear interpolation of v0
 *  and v1 based on the weighting factor fT
 */
inline Vector3D Lerp(const Vector3D& v0, const Vector3D& v1, Float fT)
{
	return Vector3D::Lerp(v0, v1, fT);
}

} // namespace Seoul

#endif // include guard
