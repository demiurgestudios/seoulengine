/**
 * \file Triangle3D.h
 * \brief Struct representing a triangle geometric shape in 3D space.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef TRIANGLE_3D_H
#define TRIANGLE_3D_H

#include "AABB.h"
#include "Plane.h"
#include "Prereqs.h"

namespace Seoul
{

struct Triangle3D SEOUL_SEALED
{
public:
	Vector3D m_vP0;
	Vector3D m_vP1;
	Vector3D m_vP2;

	/** Default to a zero size triangle. */
	Triangle3D()
		: m_vP0(0, 0, 0)
		, m_vP1(0, 0, 0)
		, m_vP2(0, 0, 0)
	{
	}

	/** Construct a triangle from 3 corners. */
	Triangle3D(const Vector3D& vP0, const Vector3D& vP1, const Vector3D& vP2)
		: m_vP0(vP0)
		, m_vP1(vP1)
		, m_vP2(vP2)
	{
	}

	/** @return True if this Triangle3D is degenerate (all corners are equal). */
	Bool IsDegenerate() const
	{
		return (Seoul::Equals(GetArea(), 0.0f));
	}

	/** @return An AABB tightly enclosing this Triangle3D. */
	AABB GetAABB() const
	{
		AABB ret;
		ret.m_vMin = Vector3D::Min(m_vP0, Vector3D::Min(m_vP1, m_vP2));
		ret.m_vMax = Vector3D::Max(m_vP0, Vector3D::Max(m_vP1, m_vP2));
		return ret;
	}

	/** @return The 2D surface area of this Triangle3D. */
	Float32 GetArea() const
	{
		Float32 ret = (0.5f * Vector3D::Cross((m_vP2 - m_vP0), (m_vP1 - m_vP0)).Length());

		return ret;
	}

	/** @return The geometric center of this Triangle3D. */
	Vector3D GetCenter() const
	{
		Vector3D const vReturn((m_vP0 + m_vP1 + m_vP2) / 3.0f);
		return vReturn;
	}

	/**
	 * @return A Vector3D with all 3 components set to the min value
	 * of the corresponding component of each of the 3 corners of
	 * this Triangle3D.
	 */
	Vector3D GetMin() const
	{
		Vector3D const vReturn(Vector3D::Min(m_vP0, Vector3D::Min(m_vP1, m_vP2)));
		return vReturn;
	}

	/**
	 * @return A Vector3D with all 3 components set to the max value
	 * of the corresponding component of each of the 3 corners of
	 * this Triangle3D.
	 */
	Vector3D GetMax() const
	{
		Vector3D const vReturn(Vector3D::Max(m_vP0, Vector3D::Max(m_vP1, m_vP2)));
		return vReturn;
	}

	/**
	 * @return The front-facing normal of this Triangle3D.
	 *
	 * Triangle3D in SeoulEngine uses CCW winding to
	 * define the front-face.
	 */
	Vector3D GetNormal() const
	{
		Vector3D const vReturn(Vector3D::UnitCross(m_vP2 - m_vP1, m_vP0 - m_vP1));
		return vReturn;
	}

	/** @return The plane of this Triangle3D as a Seoul::Plane instance. */
	Plane GetPlane() const
	{
		return Plane::CreateFromCorners(m_vP0, m_vP1, m_vP2);
	}

	/**
	 * @return True if this Triangle3D has corners equal to b
	 * within distance tolerance fTolerance.
	 */
	Bool Equals(const Triangle3D& b, Float32 fTolerance = fEpsilon) const
	{
		return
			m_vP0.Equals(b.m_vP0, fTolerance) &&
			m_vP1.Equals(b.m_vP1, fTolerance) &&
			m_vP2.Equals(b.m_vP2, fTolerance);
	}

	// Intersect test - returns true if v is contained within this Triangle3D.
	Bool Intersects(const Vector3D& v) const;
}; // struct Triangle3D

/** Exact equality of two Triangle3D (no floating point tolerance). */
static inline Bool operator==(const Triangle3D& a, const Triangle3D& b)
{
	return
		(a.m_vP0 == b.m_vP0) &&
		(a.m_vP1 == b.m_vP1) &&
		(a.m_vP2 == b.m_vP2);
}

/** Exact inequality of two Triangle3D (no floating point tolerance). */
static inline Bool operator!=(const Triangle3D& a, const Triangle3D& b)
{
	return !(a == b);
}

} // namespace Seoul

#endif // include guard
