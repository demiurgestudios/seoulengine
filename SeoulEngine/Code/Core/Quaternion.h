/**
 * \file Quaternion.h
 * \brief SeoulEngine implementation of the quaternion number system,
 * primarily used to represent 3D rotations.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	QUATERNION_H
#define	QUATERNION_H

#include "SeoulMath.h"
#include "Vector3D.h"

namespace Seoul
{

// Forward declarations to avoid pulling in extra headers
struct Matrix3D;
struct Matrix4D;
struct Matrix3x4;

/**
 * Quaternion structure
 *
 * This structure provides methods with working with quaternions, a
 * 4-dimensional division ring that is an extension of the complex numbers.
 * Unit quaternions have the useful property that they can also represent 3D
 * rotations and rotations very compactly.
 *
 * A quaternion represents the number W + Xi + Yj + Zk, where i, j, and k are
 * the square roots of -1 that form the basis of the pure imaginary components.
 *
 * See also http://en.wikipedia.org/wiki/Quaternion .
 */
struct Quaternion SEOUL_SEALED
{
	/** First imaginary component */
	Float X;

	/** Second imaginary component */
	Float Y;

	/** Third imaginary component */
	Float Z;

	/** Real component */
	Float W;

	/**
	 * Default constructor
	 */
	Quaternion()
		: X(0)
		, Y(0)
		, Z(0)
		, W(0)
	{
	}

	/**
	 * Constructor
	 *
	 * Constructs a quaternion given its 4 components
	 *
	 * @param[in] x First imaginary component
	 * @param[in] y Second imaginary component
	 * @param[in] z Third imaginary component
	 * @param[in] w Real component
	 */
	Quaternion(Float x, Float y, Float z, Float w)
		: X(x)
		, Y(y)
		, Z(z)
		, W(w)
	{
	}

	/**
	 * Constructor
	 *
	 * Constructs a quaternion given its real component as a number and its
	 * imaginary components in vector.
	 *
	 * @param[in] v Imaginary components, as a vector
	 * @param[in] w Real component
	 */
	Quaternion(const Vector3D & v, Float w)
		: X(v.X)
		, Y(v.Y)
		, Z(v.Z)
		, W(w)
	{
	}

	/**
	 * Copy constructor
	 *
	 * Constructs a copy of another quaternion.
	 *
	 * @param[in] quat Quaternion to copy
	 */
	Quaternion(const Quaternion & quat)
		: X(quat.X)
		, Y(quat.Y)
		, Z(quat.Z)
		, W(quat.W)
	{
	}

	/**
	 * Quaternion addition operator
	 *
	 * Quaternion addition operator.  Adds two quaternions component-wise.
	 *
	 * @param[in] quat Quaternion to add to this
	 *
	 * @return The sum of this quaternion and quat
	 */
	Quaternion operator+(const Quaternion & quat) const
	{
		return Quaternion(
			X + quat.X,
			Y + quat.Y,
			Z + quat.Z,
			W + quat.W);
	}

	/**
	 * Quaternion subtraction operator
	 *
	 * Quaternion subtraction operator.  Subtracts two quaternions
	 * component-wise.
	 *
	 * @param[in] quat Quaternion to subtract from this
	 *
	 * @return The quaternion difference this minus quat
	 */
	Quaternion operator-(const Quaternion & quat) const
	{
		return Quaternion(
			X - quat.X,
			Y - quat.Y,
			Z - quat.Z,
			W - quat.W);
	}

	/**
	 * Unary quaternion negation operator
	 *
	 * Unary quaternion negation operator.  Returns the negation of this
	 * quaternion.
	 *
	 * @return The negation of this quaternion
	 */
	Quaternion operator-() const
	{
		return Quaternion(-X, -Y, -Z, -W);
	}

	/**
	 * Quaternion multiplication operator
	 *
	 * Quaternion multiplication operator.  Multiplies two quaternions
	 * according to i^2 = j^2 = k^2 = ijk = -1 and the distributive law.  Note
	 * that quaternion multiplication is non-commutative.
	 *
	 * @param[in] quat Quaternion to multiply by
	 *
	 * @return The quaternion product of this and quat
	 */
	Quaternion operator*(const Quaternion & quat) const
	{
		return Quaternion(
			W * quat.X + X * quat.W + Y * quat.Z - Z * quat.Y,
			W * quat.Y - X * quat.Z + Y * quat.W + Z * quat.X,
			W * quat.Z + X * quat.Y - Y * quat.X + Z * quat.W,
			W * quat.W - X * quat.X - Y * quat.Y - Z * quat.Z);
	}

	/**
	 * Quaternion scalar multiplication operator
	 *
	 * Quaternion scalar multiplication operator.  Multiplies this quaternion
	 * component-wise by a real scalar.
	 *
	 * @param[in] fScalar Scalar to multiply by
	 *
	 * @return The scalar product of this times fScalar
	 */
	Quaternion operator*(Float fScalar) const
	{
		return Quaternion(
			X * fScalar,
			Y * fScalar,
			Z * fScalar,
			W * fScalar);
	}

	/**
	 * Quaternion scalar division operator
	 *
	 * Quaternion scalar division operator.  Divides this quaternion
	 * component-wise by a real scalar.
	 *
	 * @param[in] fScalar Scalar to divide by
	 *
	 * @return The scalar division of this by fScalar
	 */
	Quaternion operator/(Float fScalar) const
	{
		return Quaternion(
			X / fScalar,
			Y / fScalar,
			Z / fScalar,
			W / fScalar);
	}

	/**
	 * Quaternion assignment operator
	 *
	 * Quaternion assignment operator.  Assigns this quaternion to another
	 * quaternion.
	 *
	 * @param[in] quat Quaternion to assign to this
	 *
	 * @return A reference to this quaternion, for assignment chaining
	 */
	Quaternion& operator=(const Quaternion & quat)
	{
		X = quat.X;
		Y = quat.Y;
		Z = quat.Z;
		W = quat.W;

		return *this;
	}

	/**
	 * Quaternion addition-assignment operator
	 *
	 * Quaternion addition-assignment operator.  Adds the given quaternion to
	 * this quaternion.
	 *
	 * @param[in] quat Quaternion to add to this
	 *
	 * @return A reference to this quaternion, for assignment chaining
	 */
	Quaternion& operator+=(const Quaternion & quat)
	{
		X += quat.X;
		Y += quat.Y;
		Z += quat.Z;
		W += quat.W;

		return *this;
	}

	/**
	 * Quaternion subtraction-assignment operator
	 *
	 * Quaternion subtraction-assignment operator.  Subtracts the given
	 * quaternion from this quaternion.
	 *
	 * @param[in] quat Quaternion to subtract from this
	 *
	 * @return A reference to this quaternion, for assignment chaining
	 */
	Quaternion& operator-=(const Quaternion & quat)
	{
		X -= quat.X;
		Y -= quat.Y;
		Z -= quat.Z;
		W -= quat.W;

		return *this;
	}

	/**
	 * Quaternion multiplication-assignment operator
	 *
	 * Quaternion multiplication-assignment operator.  Multiplies this
	 * quaternion by the given quaternion.
	 *
	 * @param[in] quat Quaternion to multiply this by
	 *
	 * @return A reference to this quaternion, for assignment chaining
	 */
	Quaternion& operator*=(const Quaternion & quat)
	{
		return (*this = *this * quat);
	}

	/**
	 * Quaternion scalar multiplication-assignment operator
	 *
	 * Quaternion scalar multiplication-assignment operator.  Multiplies this
	 * quaternion by the given real scalar.
	 *
	 * @param[in] fScalar Real scalar to multiply this by
	 *
	 * @return A reference to this quaternion, for assignment chaining
	 */
	Quaternion& operator*=(Float fScalar)
	{
		X *= fScalar;
		Y *= fScalar;
		Z *= fScalar;
		W *= fScalar;

		return *this;
	}

	/**
	 * Quaternion scalar division-assignment operator
	 *
	 * Quaternion scalar division-assignment operator.  Divides this
	 * quaternion by the given real scalar.
	 *
	 * @param[in] fScalar Real scalar to divide this by
	 *
	 * @return A reference to this quaternion, for assignment chaining
	 */
	Quaternion& operator/=(Float fScalar)
	{
		X /= fScalar;
		Y /= fScalar;
		Z /= fScalar;
		W /= fScalar;

		return *this;
	}

	/**
	 * Quaternion equality comparison operator
	 *
	 * Quaternion equality comparison operator.  Compares this quaternion to
	 * another quaternion for equality.
	 *
	 * @param[in] quat Quaternion to compare this to for equality
	 *
	 * @return True if this is equal to quat, or false if this is not
	 *         equal to quat
	 */
	Bool operator==(const Quaternion & quat) const
	{
		return
			X == quat.X &&
			Y == quat.Y &&
			Z == quat.Z &&
			W == quat.W;
	}

	/**
	 * Quaternion inequality comparison operator
	 *
	 * Quaternion inequality comparison operator.  Compares this quaternion to
	 * another quaternion for inequality.
	 *
	 * @param[in] quat Quaternion to compare this to for inequality
	 *
	 * @return True if this is not equal to quat, or false if this is
	 *         equal to quat
	 */
	Bool operator!=(const Quaternion & quat) const
	{
		return
			X != quat.X ||
			Y != quat.Y ||
			Z != quat.Z ||
			W != quat.W;
	}

	/**
	 * Tests this quaternion for equality with another quaternion with some amount of
	 * tolerance.  Two quaternions are considered equal if each component differs by
	 * no more than the given tolerance.
	 *
	 * @param[in] qOther The quaternion to compare against
	 * @param[in] fTolerance The maximum amount of tolerance between each component
	 *        for the two quaternions to be considered equal
	 *
	 * @return true if each component of this differs from the
	 *         corresponding component of qOther by no more than
	 *         fTolerance, or false otherwise
	 */
	Bool Equals(
		const Quaternion& qOther,
		Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(X, qOther.X, fTolerance) &&
			::Seoul::Equals(Y, qOther.Y, fTolerance) &&
			::Seoul::Equals(Z, qOther.Z, fTolerance) &&
			::Seoul::Equals(W, qOther.W, fTolerance);
	}

	/**
	 * Determines if this quaternion is within a given tolerance of the zero
	 * quaternion.
	 *
	 * @param[in] fTolerance The tolerance
	 *
	 * @return true if each component is within fTolerance of zero, or
	 *         false otherwise
	 *
	 * Testing a quaternion against zero is equivalent to testing
	 * if the quaternion is valid or not. A zero quaternion is invalid.
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
	 * Returns the conjugate of this quaternion
	 *
	 * Returns the conjugate of this quaternion.  The conjugate has the same
	 * real component but inverted imaginary components.
	 *
	 * @return The conjugate of this quaternion
	 */
	Quaternion Conjugate() const
	{
		return Quaternion(-X, -Y, -Z, W);
	}

	// Convert unit quaternions to orthonormal matrices
	Matrix3D GetMatrix3D() const;
	Matrix4D GetMatrix4D() const;

	/**
	 * Gets the rotation around the X axis in radians from this Quaternion.
	 */
	Float GetRotationX() const
	{
		return 2.0f * Atan2(X, W);
	}

	/**
	 * Gets the rotation around the Y axis in radians from quaternion q.
	 */
	Float GetRotationY() const
	{
		return 2.0f * Atan2(Y, W);
	}

	/**
	 * Gets the rotation around the Z axis in radians from quaternion q.
	 */
	Float GetRotationZ() const
	{
		return 2.0f * Atan2(Z, W);
	}

	/**
	 * Returns the magnitude of this quaternion
	 *
	 * Returns the magnitude of this quaternion.  Applications should try to
	 * use LengthSquared() instead when possible to avoid the expensive square
	 * root call.
	 *
	 * @return The magnitude of this quaternion
	 */
	Float Length() const
	{
		return Sqrt(LengthSquared());
	}

	/**
	 * Returns the magnitude squared of this quaternion
	 *
	 * Returns the magnitude squared of this quaternion.
	 *
	 * @return The magnitude squared of this quaternion
	 */
	Float LengthSquared() const
	{
		return Dot(*this, *this);
	}

	/**
	 * Normalizes this quaternion
	 *
	 * Normalizes this quaternion by dividing all components by the magnitude.
	 * If the quaternion is approximately the zero quaternion, it is not
	 * modified to avoid potentially dividing by zero.  It is considered
	 * approximately zero if the magnitude squared is less than fTolerance.
	 *
	 * @param[in] fTolerance Minimum squared magnitude the quaternion must have
	 *                       in order that it not be considered zero
	 *
	 * @return True if the quaternion was successfully normalized, or false if
	 *         its magnitude squared was less than fTolerance
	 */
	Bool Normalize(Float fTolerance = Epsilon * Epsilon)
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

	/**
	 * Returns the multiplicative inverse of this quaternion
	 *
	 * Returns the multiplicative inverse of this quaternion.  For all non-zero
	 * quaternions q, it holds that q*q.Inverse() and q.Inverse()*q equal the
	 * unit real quaternion, up to floating-point error.  If this quaternion is
	 * zero, or approximately zero, the results are undefined.
	 *
	 * @return The multiplicative inverse of this quaternion
	 */
	Quaternion Inverse() const
	{
		return Conjugate() / LengthSquared();
	}

	/** Get an identity Quaternion (rotation is left unchanged if an identity Quaternion is applied). */
	static Quaternion Identity()
	{
		return Quaternion(0.f, 0.f, 0.f, 1.f);
	}

	/** Gets an invalid (all zero) Quaternion. Normally not used or useful in rotation calculations. */
	static Quaternion Invalid()
	{
		return Quaternion(0.0f, 0.0f, 0.0f, 0.0f);
	}

	/**
	 * Returns the conjugate of a quaternion
	 *
	 * Returns the conjugate of a quaternion.  The conjugate has the same
	 * real component but inverted imaginary components.
	 *
	 * @return The conjugate of a quaternion
	 */
	static Quaternion Conjugate(const Quaternion& q)
	{
		return q.Conjugate();
	}

	/**
	 * @return The dot production of two Quaternions.
	 */
	static Float Dot(const Quaternion& a, const Quaternion& b)
	{
		return (a.X * b.X) + (a.Y * b.Y) + (a.Z * b.Z) + (a.W * b.W);
	}

	/**
	 * Constructs a unit quaternion corresponding to a rotation about an arbitrary axis
	 *
	 * Constructs a unit quaternion corresponding to a rotation about an arbitrary axis.
	 * Positive angles correspond to counterclockwise rotations, and negative angles
	 * correspond to clockwise rotations.
	 *
	 * @param[in] vAxis The axis of rotation
	 * @param[in] fAngle Rotation angle about the Z-axis
	 *
	 * @return A unit quaternion representing a counterclockwise rotation of
	 *         fAngle about vAxis
	 */
	static Quaternion CreateFromAxisAngle(const Vector3D& vAxis, Float fAngle)
	{
		Float const fHalfAngle = (0.5f * fAngle);
		Float const fS = Sin(fHalfAngle);
		Float const fC = Cos(fHalfAngle);

		return Quaternion(
			fS * vAxis.X,
			fS * vAxis.Y,
			fS * vAxis.Z,
			fC);
	}

	// Construct a Quaternion that applies a rotation
	// to the given facing direction, from
	// a basis direction (or the default along -Z).
	static Quaternion CreateFromDirection(
		const Vector3D& vDirection,
		const Vector3D& vBasisDirection = -Vector3D::UnitZ());

	// Construct unit quaternion from rotation a rotation matrix.
	static Quaternion CreateFromRotationMatrix(const Matrix3D& m);
	static Quaternion CreateFromRotationMatrix(const Matrix3x4& m);
	static Quaternion CreateFromRotationMatrix(const Matrix4D& m);

	/**
	 * Constructs a unit quaternion corresponding to a rotation about the
	 *        X-axis
	 *
	 * Constructs a unit quaternion corresponding to a rotation about the X-axis.
	 * Positive angles correspond to counterclockwise rotations, and negative angles
	 * correspond to clockwise rotations.
	 *
	 * @param[in] fAngle Rotation angle about the X-axis
	 *
	 * @return A unit quaternion representing a counterclockwise rotation of
	 *         fAngle about the X-axis
	 */
	static Quaternion CreateFromRotationX(Float fAngle)
	{
		fAngle *= 0.5f;
		return Quaternion(Sin(fAngle), 0.0f, 0.0f, Cos(fAngle));
	}

	/**
	 * Constructs a unit quaternion corresponding to a rotation about the
	 *        Y-axis
	 *
	 * Constructs a unit quaternion corresponding to a rotation about the Y-axis.
	 * Positive angles correspond to counterclockwise rotations, and negative angles
	 * correspond to clockwise rotations.
	 *
	 * @param[in] fAngle Rotation angle about the Y-axis
	 *
	 * @return A unit quaternion representing a counterclockwise rotation of
	 *         fAngle about the Y-axis
	 */
	static Quaternion CreateFromRotationY(Float fAngle)
	{
		fAngle *= 0.5f;
		return Quaternion(0.0f, Sin(fAngle), 0.0f, Cos(fAngle));
	}

	/**
	 * Constructs a unit quaternion corresponding to a rotation about the
	 *        Z-axis
	 *
	 * Constructs a unit quaternion corresponding to a rotation about the Z-axis.
	 * Positive angles correspond to counterclockwise rotations, and negative angles
	 * correspond to clockwise rotations.
	 *
	 * @param[in] fAngle Rotation angle about the Z-axis
	 *
	 * @return A unit quaternion representing a counterclockwise rotation of
	 *         fAngle about the Z-axis
	 */
	static Quaternion CreateFromRotationZ(Float fAngle)
	{
		fAngle *= 0.5f;
		return Quaternion(0.0f, 0.0f, Sin(fAngle), Cos(fAngle));
	}

	// Construct unit quaternion from Euler angles
	static Quaternion CreateFromYawPitchRollYXZ(Float fYaw, Float fPitch, Float fRoll);
	static Quaternion CreateFromYawPitchRollZXY(Float fYaw, Float fPitch, Float fRoll);
	static Quaternion CreateFromRollPitchYawYXZ(Float fRoll, Float fPitch, Float fYaw);

	/**
	 * Returns the multiplicative inverse of a quaternion
	 *
	 * Returns the multiplicative inverse of a quaternion.  For all non-zero
	 * quaternions q, it holds that q*q.Inverse() and q.Inverse()*q equal the
	 * unit real quaternion, up to floating-point error.  If this quaternion is
	 * zero, or approximately zero, the results are undefined.
	 *
	 * @return The multiplicative inverse of a quaternion
	 */
	static Quaternion Inverse(const Quaternion& q)
	{
		return q.Inverse();
	}

	// Vanilla linear interpolation
	static Quaternion Lerp(const Quaternion& q0, const Quaternion& q1, Float fAlpha)
	{
		return (q0 * (1.0f - fAlpha)) + (q1 * fAlpha);
	}

	/**
	 * Return a normalized version of this quat.  If it is zero length it will
	 * return a zero length quat as well.
	 */
	static Quaternion Normalize(const Quaternion& q)
	{
		Quaternion qReturn(q);
		(void)qReturn.Normalize();
		return qReturn;
	}

	// Spherical linear interpolation
	static Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, Float fT);

	/**
	 * Returns a vector which is vVector transformed by
	 * qQuaternion.
	 */
	static Vector3D Transform(
		const Quaternion& qQuaternion,
		const Vector3D& vVector)
	{
		auto const fW = qQuaternion.W;
		Vector3D const qvec(qQuaternion.X, qQuaternion.Y, qQuaternion.Z);

		auto const uv = Vector3D::Cross(qvec, vVector);

		auto const a = (2.0f * Vector3D::Dot(qvec, vVector) * qvec);
		auto const b = ((fW * fW) - Vector3D::Dot(qvec, qvec)) * vVector;
		auto const c = (2.0f * fW * uv);

		return (a + b + c);
	}
}; // struct Quaternion
template <> struct CanMemCpy<Quaternion> { static const Bool Value = true; };
template <> struct CanZeroInit<Quaternion> { static const Bool Value = true; };

/**
 * Quaternion scalar left-multiplication operator
 *
 * Quaternion scalar left-multiplication operator.  Multiplies this a quaternion
 * by a real scalar on the left.
 *
 * @param[in] fScalar Scalar to multiply by
 * @param[in] quat    Quaternion to multiply
 *
 * @return The scalar product of fScalar times quat
 */
inline Quaternion operator*(Float fScalar, const Quaternion& q)
{
	return Quaternion(
		q.X * fScalar,
		q.Y * fScalar,
		q.Z * fScalar,
		q.W * fScalar);
}

/** Tolerance equality test between a and b. */
inline Bool Equals(const Quaternion& qA, const Quaternion& qB, Float fTolerance = fEpsilon)
{
	return qA.Equals(qB, fTolerance);
}

} // namespace Seoul

#endif // include guard
