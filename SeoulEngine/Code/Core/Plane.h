/**
 * \file Plane.h
 * \brief Geometric primitive presents an infinite 3D plane, often used
 * for splitting 3D space in half-spaces.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PLANE_H
#define PLANE_H

#include "Prereqs.h"
#include "SeoulTypeTraits.h"
#include "Vector3D.h"
#include "Vector4D.h"
namespace Seoul { struct AABB; }
namespace Seoul { struct Sphere; }

namespace Seoul
{

/**
 * Defines the results of an intersection check between
 *  various shapes and a 3D plane.
 */
enum class PlaneTestResult
{
	/** Shape is entirely on the front side (dot normal of all points is positive). */
	kFront,

	/** Shape is entirely on the back side (dot normal of all points is negative). */
	kBack,

	/** Shape overlaps the plane (dot normals are both positive and negative, or zero). */
	kIntersects,
};

/**
 * Plane in 3D, infinite geometry, partitions a 3D space into two halves.
 */
struct Plane SEOUL_SEALED
{
	/**
	 * A coefficient of the plane equation: Ax + By + Cz + D = 0
	 */
	Float A;

	/**
	 * B coefficient of the plane equation: Ax + By + Cz + D = 0
	 */
	Float B;

	/**
	 * C coefficient of the plane equation: Ax + By + Cz + D = 0
	 */
	Float C;

	/**
	 * D coefficient of the plane equation: Ax + By + Cz + D = 0
	 */
	Float D;

	Plane()
		: A(0.0f)
		, B(0.0f)
		, C(0.0f)
		, D(0.0f)
	{
	}

	/**
	 * Returns a value which indicates the distance and
	 * side of a point relative to this Plane.
	 *
	 * If the returned value is positive, then the point is that
	 * distance from the plane on its front side. If the return value is
	 * negative, then the point is the abs() of the returned value distance
	 * from the plane on its back side. If the return value is very close
	 * to 0.0, then the point intersects the plane.
	 */
	Float DotCoordinate(const Vector3D& v) const
	{
		return A * v.X + B * v.Y + C * v.Z + D;
	}

	/**
	 * Returns the dot product of v with the normal of this Plane.
	 */
	Float DotNormal(const Vector3D& v) const
	{
		return A * v.X + B * v.Y + C * v.Z;
	}

	/**
	 * Threshold equality between this plane and plane B.
	 *
	 * @param[in] b The plane to compare against.
	 * @param[in] fTolerance The maximum amount of tolerance between
	 *                       each component for equality.
	 *
	 * @return true if each component of this differs from the
	 *         corresponding component of b by no more than
	 *         fTolerance, or false otherwise
	 */
	Bool Equals(
		const Plane& b,
		Float fTolerance = fEpsilon) const
	{
		return
			::Seoul::Equals(A, b.A, fTolerance) &&
			::Seoul::Equals(B, b.B, fTolerance) &&
			::Seoul::Equals(C, b.C, fTolerance) &&
			::Seoul::Equals(D, b.D, fTolerance);
	}

	/**
	 * Returns the Normal vector of the Plane.
	 */
	Vector3D GetNormal() const
	{
		return Vector3D(A, B, C);
	}

	// Intersection tests with Plane.
	PlaneTestResult Intersects(const AABB& b) const;
	PlaneTestResult Intersects(const Sphere& s) const;
	PlaneTestResult Intersects(const Vector3D& v) const;

	/**
	 * Normalizes the coefficients of this plane so they
	 * define a plane whose normal vector is of unit length.
	 *
	 * @return True if the plane's normal is well-defined (> 0.0),
	 * false otherwise.
	 */
	Bool Normalize(Float fTolerance = Epsilon)
	{
		Float fLengthSquared = A * A + B * B + C * C;

		if (fLengthSquared < fTolerance)
		{
			return false;
		}

		Float fLength = Sqrt(fLengthSquared);

		A /= fLength;
		B /= fLength;
		C /= fLength;
		D /= fLength;

		return true;
	}

	/** Exact equality of 2 planes. */
	Bool operator==(const Plane& b) const
	{
		return
			(A == b.A) &&
			(B == b.B) &&
			(C == b.C) &&
			(D == b.D);
	}

	/** Exact inequality of 2 planes. */
	Bool operator!=(const Plane& b) const
	{
		return !(*this == b);
	}

	/**
	 * Projects the point vPoint onto this plane along the
	 * direction of this plane's normal.
	 */
	Vector3D ProjectOnto(const Vector3D& vPoint) const
	{
		Float d = DotCoordinate(vPoint);
		Vector3D ret = (vPoint - (d * GetNormal()));

		return ret;
	}

	/**
	 * Sets the four coefficients of the plane equation from
	 * the four components of the Vector4D v.
	 */
	void Set(const Vector4D& v)
	{
		A = v.X;
		B = v.Y;
		C = v.Z;
		D = v.W;
	}

	/** Update the plane equation of this Plane. */
	void Set(Float fA, Float fB, Float fC, Float fD)
	{
		A = fA;
		B = fB;
		C = fC;
		D = fD;
	}

	/**
	 * Constructs a plane from the four coefficients of
	 * the plane equation.
	 */
	static Plane Create(Float fA, Float fB, Float fC, Float fD)
	{
		Plane ret;
		ret.A = fA;
		ret.B = fB;
		ret.C = fC;
		ret.D = fD;

		return ret;
	}

	/**
	 * Constructs a plane from a point vPoint and a
	 * direction vector vNormal.
	 */
	static Plane CreateFromPositionAndNormal(
		const Vector3D& vPoint,
		const Vector3D& vNormal)
	{
		Plane ret;
		ret.Set(Vector4D(vNormal, Vector3D::Dot(-vNormal, vPoint)));

		return ret;
	}

	/**
	 * Constructs a plane from three corner points,
	 * forming a triangle.
	 */
	static Plane CreateFromCorners(
		const Vector3D& vP0,
		const Vector3D& vP1,
		const Vector3D& vP2)
	{
		Vector3D const vA(vP0 - vP1);
		Vector3D const vB(vP2 - vP1);
		Vector3D const vNormal(Vector3D::UnitCross(vB, vA));

		return CreateFromPositionAndNormal(vP1, vNormal);
	}
}; // struct Plane
template <> struct CanMemCpy<Plane> { static const Bool Value = true; };
template <> struct CanZeroInit<Plane> { static const Bool Value = true; };

/** Tolerance equality test between a and b. */
inline Bool Equals(const Plane& a, const Plane& b, Float fTolerance = fEpsilon)
{
	return a.Equals(b, fTolerance);
}

} // namespace Seoul

#endif // include guard
