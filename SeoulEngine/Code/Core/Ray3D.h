/**
 * \file Ray3D.h
 * \brief Geometric primitive presents an infinite ray in 3D
 * sapce.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef RAY3D_H
#define RAY3D_H

#include "Vector3D.h"
namespace Seoul { struct AABB; }
namespace Seoul { struct Plane; }
namespace Seoul { struct Sphere; }

namespace Seoul
{

struct Ray3D SEOUL_SEALED
{
	Vector3D m_vDirection;
	Vector3D m_vPosition;

	Ray3D()
		: m_vDirection(Vector3D::Zero())
		, m_vPosition(Vector3D::Zero())
	{
	}

	Ray3D(const Vector3D& vPosition, const Vector3D& vDirection)
		: m_vDirection(vDirection)
		, m_vPosition(vPosition)
	{
	}

	/** Given a distance, compute the 3D point on the ray. */
	Vector3D Derive(Float fDistance) const
	{
		return m_vPosition + m_vDirection * fDistance;
	}

	// Geometric intersection test - ray intersection,
	// one-sided. Intersections "behind" the ray do not occur -
	// when this function returns true, rfDistance will contain
	// a value >= 0.0.
	Bool Intersects(const Plane& plane, Float& rfDistance) const;
	Bool Intersects(const Sphere& sphere, Float& rfDistance) const;

	// Convenience around LineIntersects variations, derives
	// the position after successful intersection.
	template <typename T>
	Bool Intersects(const T& geometric, Vector3D& rvPoint) const
	{
		Float fDistance = 0.0f;
		if (!Intersects(geometric, fDistance))
		{
			return false;
		}

		rvPoint = Derive(fDistance);
		return true;
	}

	// Geometric intersection test - "line" intersection as
	// it treats the ray as a line and will return true
	// for intersections "behind" the ray (rfDistance will be
	// negative in this case).
	Bool LineIntersects(const Plane& plane, Float& rfDistance) const;

	// Convenience around LineIntersects variations, derives
	// the position after successful intersection.
	template <typename T>
	Bool LineIntersects(const T& geometric, Vector3D& rvPoint) const
	{
		Float fDistance = 0.0f;
		if (!LineIntersects(geometric, fDistance))
		{
			return false;
		}

		rvPoint = Derive(fDistance);
		return true;
	}
}; // struct Ray3D
template <> struct CanMemCpy<Ray3D> { static const Bool Value = true; };
template <> struct CanZeroInit<Ray3D> { static const Bool Value = true; };

inline Bool operator==(const Ray3D& a, const Ray3D& b)
{
	return (a.m_vDirection == b.m_vDirection && a.m_vPosition == b.m_vPosition);
}

inline Bool operator!=(const Ray3D& a, const Ray3D& b)
{
	return (a.m_vDirection != b.m_vDirection || a.m_vPosition != b.m_vPosition);
}

} // namespace Seoul

#endif // include guard
