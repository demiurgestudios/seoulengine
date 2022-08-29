/**
 * \file Vector4D.h
 * \brief 4D linear algebra vector class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef VECTOR4D_H
#define VECTOR4D_H

#include "SeoulAssert.h"
#include "SeoulMath.h"
#include "SeoulTypeTraits.h"
#include "Vector3D.h"

namespace Seoul
{

/**
 * 4D vector class
 *
 * 4D vector class.  It provides operator overloads for the most common vector
 * operations.  The components are accessible through the X, Y, Z, and W members;
 * they are also accessible through operator[].
 */
struct Vector4D SEOUL_SEALED
{
	Float X;
	Float Y;
	Float Z;
	Float W;

	/**
	 * Default constructor.  Performs no initialization.
	 */
	Vector4D()
		: X(0.0f)
		, Y(0.0f)
		, Z(0.0f)
		, W(0.0f)
	{
	}

	/**
	 * Constructs a vector, assigning f to all 4 components.
	 *
	 * @param[in] f scalr value used for X, Y, Z, adn W components
	 */
	explicit Vector4D(Float f)
		: X(f)
		, Y(f)
		, Z(f)
		, W(f)
	{
	}

	/**
	 * Constructs a vector out of the 4 given values.
	 *
	 * @param[in] fX X component
	 * @param[in] fY Y component
	 * @param[in] fZ Z component
	 * @param[in] fW W component
	 */
	Vector4D(Float fX, Float fY, Float fZ, Float fW)
		: X(fX)
		, Y(fY)
		, Z(fZ)
		, W(fW)
	{
	}

	/**
	 * Copy constructor.  Copies another 4D vector.
	 *
	 * @param[in] vOther Vector to copy
	 */
	Vector4D(const Vector4D & vOther)
		: X(vOther.X)
		, Y(vOther.Y)
		, Z(vOther.Z)
		, W(vOther.W)
	{
	}

	/**
	 * Constructs a 4D vector out of a two 2D vectors.
	 */
	Vector4D(const Vector2D& vXY, const Vector2D& vZW)
		: X(vXY.X)
		, Y(vXY.Y)
		, Z(vZW.X)
		, W(vZW.Y)
	{
	}

	/**
	 * Constructs a 4D vector out of a 3D vector and a float.
	 */
	Vector4D(const Vector3D & vOther, Float f)
		: X(vOther.X)
		, Y(vOther.Y)
		, Z(vOther.Z)
		, W(f)
	{
	}

	/**
	 * @return A read-only array reference to the data in this Vector4D.
	 */
	Float const* GetData() const
	{
		return &X;
	}

	/**
	 * @return A writeable array reference to the data in this Vector4D.
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
		SEOUL_ASSERT(i >= 0 && i <= 3);
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
		SEOUL_ASSERT(i >= 0 && i <= 3);
		return GetData()[i];
	}

	/**
	 * Computes the vector sum of two vectors
	 *
	 * @param[in] vOther Other vector to add to this
	 *
	 * @return Vector sum of this and vOther
	 */
	Vector4D operator + (const Vector4D & vOther) const
	{
		return Vector4D(
			X + vOther.X,
			Y + vOther.Y,
			Z + vOther.Z,
			W + vOther.W);
	}

	/**
	 * Computes the vector difference of two vectors
	 *
	 * @param[in] vOther Other vector to subtract from this
	 *
	 * @return Vector difference of this and vOther
	 */
	Vector4D operator-(const Vector4D & vOther) const
	{
		return Vector4D(
			X - vOther.X,
			Y - vOther.Y,
			Z - vOther.Z,
			W - vOther.W);
	}

	/**
	 * Computes the negation of this vector
	 *
	 * @return The vector negation of this
	 */
	Vector4D operator - () const
	{
		return Vector4D(
			-X,
			-Y,
			-Z,
			-W);
	}

	/**
	 * Computes the scalar product of this vector and a scalar
	 *
	 * @param[in] fScalar The scalar to multiply by
	 *
	 * @return Scalar product of this and fScalar
	 */
	Vector4D operator * (Float fScalar) const
	{
		return Vector4D(
			X * fScalar,
			Y * fScalar,
			Z * fScalar,
			W * fScalar);
	}

	/**
	 * Computes the scalar division of this vector by a scalar
	 *
	 * @param[in] fScalar The scalar to divide by
	 *
	 * @return Scalar division of this by fScalar
	 */
	Vector4D operator/(Float fScalar) const
	{
		return Vector4D(
			X / fScalar,
			Y / fScalar,
			Z / fScalar,
			W / fScalar);
	}

	/**
	 * Assigns another vector to this vector
	 *
	 * @param[in] vOther The vector being assigned to this
	 *
	 * @return A const reference to this
	 */
	Vector4D& operator=(const Vector4D & vOther)
	{
		X = vOther.X;
		Y = vOther.Y;
		Z = vOther.Z;
		W = vOther.W;

		return *this;
	}

	/**
	 * Adds another vector to this vector
	 *
	 * @param[in] vOther The vector to add to this
	 *
	 * @return A const reference to this
	 */
	Vector4D& operator+=(const Vector4D & vOther)
	{
		X += vOther.X;
		Y += vOther.Y;
		Z += vOther.Z;
		W += vOther.W;

		return *this;
	}

	/**
	 * Subtracts another vector from this vector
	 *
	 * @param[in] vOther The vector to subtract from this
	 *
	 * @return A const reference to this
	 */
	Vector4D& operator-=(const Vector4D & vOther)
	{
		X -= vOther.X;
		Y -= vOther.Y;
		Z -= vOther.Z;
		W -= vOther.W;

		return *this;
	}

	/**
	 * Multiplies this vector by a scalar
	 *
	 * @param[in] fScalar The scalar to multiply this vector by
	 *
	 * @return A const reference to this
	 */
	Vector4D& operator*=(Float fScalar)
	{
		X *= fScalar;
		Y *= fScalar;
		Z *= fScalar;
		W *= fScalar;

		return *this;
	}

	/**
	 * Divides this vector by a scalar
	 *
	 * @param[in] fScalar The scalar to divide this vector by
	 *
	 * @return A const reference to this
	 */
	Vector4D& operator/=(Float fScalar)
	{
		X /= fScalar;
		Y /= fScalar;
		Z /= fScalar;
		W /= fScalar;

		return *this;
	}

	/**
	 * Tests this vector for equality with another vector
	 *
	 * @param[in] vOther The vector to compare against
	 *
	 * @return true if this exactly equals vOther, or false otherwise
	 */
	Bool operator==(const Vector4D & vOther) const
	{
		return (X == vOther.X) && (Y == vOther.Y) && (Z == vOther.Z) && (W == vOther.W);
	}

	/**
	 * Tests this vector for inequality with another vector
	 *
	 * @param[in] vOther The vector to compare against
	 *
	 * @return true if this does not exactly equal vOther, or false
	 *         otherwise
	 */
	Bool operator!=(const Vector4D & vOther) const
	{
		return (X != vOther.X) || (Y != vOther.Y) || (Z != vOther.Z) || (W != vOther.W);
	}

	/**
	 * Returns a vector whose components are the absolute value
	 * of the components of this.
	 */
	Vector4D Abs() const
	{
		return Vector4D(::Seoul::Abs(X), ::Seoul::Abs(Y), ::Seoul::Abs(Z), ::Seoul::Abs(W));
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
		const Vector4D & vOther,
		Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(X, vOther.X, fTolerance) &&
			::Seoul::Equals(Y, vOther.Y, fTolerance) &&
			::Seoul::Equals(Z, vOther.Z, fTolerance) &&
			::Seoul::Equals(W, vOther.W, fTolerance);
	}

	/**
	 * @return The max component of this vector's
	 * components.
	 */
	Float GetMaxComponent() const
	{
		return ::Seoul::Max(X, Y, Z, W);
	}

	/**
	 * @return The min component of this vector's
	 * components.
	 */
	Float GetMinComponent() const
	{
		return ::Seoul::Min(X, Y, Z, W);
	}

	/**
	 * @return A Vector2D from the XY components of this
	 * Vector4D.
	 */
	Vector2D GetXY() const
	{
		return Vector2D(X, Y);
	}

	/**
	 * @return A Vector2D from the ZW components of this
	 * Vector4D.
	 */
	Vector2D GetZW() const
	{
		return Vector2D(Z, W);
	}

	/**
	 * Returns a Vector3D from the XYZ components of 
	 * this Vector4D.
	 */
	Vector3D GetXYZ() const
	{
		return Vector3D(X, Y, Z);
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
			::Seoul::IsZero(Z, fTolerance) &&
			::Seoul::IsZero(W, fTolerance);
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
		W /= fLength;

		return true;
	}

	static Vector4D One()
	{
		return Vector4D(1.0f, 1.0f, 1.0f, 1.0f);
	}

	static Vector4D UnitX()
	{
		return Vector4D(1.0f, 0.0f, 0.0f, 0.0f);
	}

	static Vector4D UnitY()
	{
		return Vector4D(0.0f, 1.0f, 0.0f, 0.0f);
	}

	static Vector4D UnitZ()
	{
		return Vector4D(0.0f, 0.0f, 1.0f, 0.0f);
	}

	static Vector4D UnitW()
	{
		return Vector4D(0.0f, 0.0f, 0.0f, 1.0f);
	}

	static Vector4D Zero()
	{
		return Vector4D(0.0f, 0.0f, 0.0f, 0.0f);
	}

	/**
	 * Returns a vector whose components are the
	 * components of vValue clamped between the corresponding
	 * components of vMax and vMin.
	 */
	static Vector4D Clamp(const Vector4D& vValue, const Vector4D& vMin, const Vector4D& vMax)
	{
		return Vector4D(
			::Seoul::Clamp(vValue.X, vMin.X, vMax.X),
			::Seoul::Clamp(vValue.Y, vMin.Y, vMax.Y),
			::Seoul::Clamp(vValue.Z, vMin.Z, vMax.Z),
			::Seoul::Clamp(vValue.W, vMin.W, vMax.W));
	}

	/** @return A new vector with  X = (vA.X / vB.X), Y = (vA.Y / vB.Y), Z = (vA.Z / vB.Z), W = (vA.W / vB.W) */
	static Vector4D ComponentwiseDivide(const Vector4D& vA, const Vector4D& vB)
	{
		return Vector4D(
			vA.X / vB.X,
			vA.Y / vB.Y,
			vA.Z / vB.Z,
			vA.W / vB.W);
	}

	/** @return A new vector with X = (vA.X * vB.X), Y = (vA.Y * vB.Y), Z = (vA.Z * vB.Z), W = (vA.W * vB.W) */
	static Vector4D ComponentwiseMultiply(const Vector4D& vA, const Vector4D& vB)
	{
		return Vector4D(
			vA.X * vB.X,
			vA.Y * vB.Y,
			vA.Z * vB.Z,
			vA.W * vB.W);
	}

	/**
	 * @return The dot product between vA and vB.
	 */
	static Float Dot(const Vector4D& vA, const Vector4D& vB)
	{
		return (vA.X * vB.X) + (vA.Y * vB.Y) + (vA.Z * vB.Z) + (vA.W * vB.W);
	}

	/**
	 * @return A vector that is the linear interpolation of v0
	 *  and v1 based on the weighting factor fT
	 */
	static Vector4D Lerp(const Vector4D& v0, const Vector4D& v1, Float fT)
	{
		return v0 * (1.0f - fT) + v1 * fT;
	}

	/**
	 * Returns a vector whose components are the
	 * maximum of the components of vA and
	 * vB.
	 */
	static Vector4D Max(const Vector4D& vA, const Vector4D& vB)
	{
		return Vector4D(
			::Seoul::Max(vA.X, vB.X),
			::Seoul::Max(vA.Y, vB.Y),
			::Seoul::Max(vA.Z, vB.Z),
			::Seoul::Max(vA.W, vB.W));
	}

	/**
	 * Returns a vector whose components are the
	 * minimum of the components of vA and
	 * vB.
	 */
	static Vector4D Min(const Vector4D& vA, const Vector4D& vB)
	{
		return Vector4D(
			::Seoul::Min(vA.X, vB.X),
			::Seoul::Min(vA.Y, vB.Y),
			::Seoul::Min(vA.Z, vB.Z),
			::Seoul::Min(vA.W, vB.W));
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
	static Vector4D Normalize(const Vector4D& v)
	{
		Vector4D vReturn(v);
		(void)vReturn.Normalize();
		return vReturn;
	}

	/** @return v with all components rounded to the nearest integer, using banker's rounding. */
	static Vector4D Round(const Vector4D& v)
	{
		return Vector4D(Seoul::Round(v.X), Seoul::Round(v.Y), Seoul::Round(v.Z), Seoul::Round(v.W));
	}
}; // struct Vector4D
template <> struct CanMemCpy<Vector4D> { static const Bool Value = true; };
template <> struct CanZeroInit<Vector4D> { static const Bool Value = true; };

SEOUL_STATIC_ASSERT_MESSAGE(sizeof(Vector4D) == 16, "IO readers/writers (amongst other code) assumes Vector4D is 16 bytes.");

/**
 * Computes the left-multiplication of a 4D vector by a scalar.
 *
 * @param[in] fScalar The scalar to multiply by
 * @param[in] vVector The vector being multiplied by
 *
 * @return The scalar product of v by f
 */
inline Vector4D operator*(Float f, const Vector4D & v)
{
	return (v * f);
}

/** Tolerance equality test between a and b. */
inline Bool Equals(const Vector4D& vA, const Vector4D& vB, Float fTolerance = fEpsilon)
{
	return vA.Equals(vB, fTolerance);
}

inline UInt32 GetHash(const Vector4D& v)
{
	UInt32 uHash = 0u;
	IncrementalHash(uHash, GetHash(v.X));
	IncrementalHash(uHash, GetHash(v.Y));
	IncrementalHash(uHash, GetHash(v.Z));
	IncrementalHash(uHash, GetHash(v.W));
	return uHash;
}

/**
 * @return A vector that is the linear interpolation of v0
 *  and v1 based on the weighting factor fT
 */
inline Vector4D Lerp(const Vector4D& v0, const Vector4D& v1, Float fT)
{
	return Vector4D::Lerp(v0, v1, fT);
}

} // namespace Seoul

#endif // include guard
