/**
 * \file AABB.h
 * \brief Axis-aligned bounding box. Basic geometric primitive often
 * used for spatial sorting.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef AABB_H
#define AABB_H

#include "Axis.h"
#include "Matrix4D.h"
#include "Prereqs.h"
#include "Sphere.h"
#include "Vector3D.h"

namespace Seoul
{

/**
 * Axis-aligned bounding box.
 */
struct AABB SEOUL_SEALED
{
	/**
	 * The minimum corner of the AABB.
	 *
	 * Most AABB functions assume that all components of Min
	 * are <= all components of Max. However, there are valid usage
	 * scenarios for breaking this rule, such as starting with
	 * an "inverted" maximum box when constructing a box from
	 * a set of points (see CalculateFromPoints).
	 */
	Vector3D m_vMin;

	/**
	 * The maximum corner of the AABB.
	 */
	Vector3D m_vMax;

	Bool operator==(const AABB& aabb) const
	{
		return (m_vMax == aabb.m_vMax) && (m_vMin == aabb.m_vMin);
	}

	Bool operator!=(const AABB& aabb) const
	{
		return !((*this) == aabb);
	}

	/**
	 * If the point is outside this AABB, expands the dimensions
	 * of the AABB to enclose the point. Otherwise, leaves the AABB
	 * unchanged.
	 */
	void AbsorbPoint(const Vector3D& vPoint)
	{
		m_vMin = Vector3D::Min(m_vMin, vPoint);
		m_vMax = Vector3D::Max(m_vMax, vPoint);
	}

	/**
	 * Returns the center point of the AABB.
	 *
	 * This method returns a new Vector object, since the Center
	 * is a derived value and must be calculated from the min and
	 * max of the AABB.
	 */
	Vector3D GetCenter() const
	{
		return 0.5f * (m_vMax + m_vMin);
	}

	/**
	 * The distance between the Min and Max corners of the
	 * AABB.
	 */
	Float GetDiagonalLength() const
	{
		Float ret = GetDimensions().Length();

		return ret;
	}

	/**
	 * The EffectiveRadius of an AABB in the given direction vDirection
	 * can be  treated like the radius of a Sphere for that direction. It is
	 * the farthest distance between the center of the AABB and the surface
	 * of the AABB in that direction.
	 */
	Float GetEffectiveRadius(const Vector3D& vDirection) const
	{
		Vector3D rst = GetDimensions();
		Vector3D absDirection(vDirection.Abs());

		Float dot = Vector3D::Dot(rst, absDirection);
		Float ret = 0.5f * dot;

		return ret;
	}

	/**
	 * Returns a Vector3D whose components are the width, height,
	 * and depth of the AABB or twice the Extents of the AABB.
	 */
	Vector3D GetDimensions() const
	{
		return (m_vMax - m_vMin);
	}

	/**
	 * Returns a Vector3D whose components are half the width, height,
	 * and depth of the AABB or half the Dimensions of the AABB.
	 */
	Vector3D GetExtents() const
	{
		return 0.5f * GetDimensions();
	}

	/**
	 * Calculates 1.0 / GetSurfaceArea() of the AABB.
	 */
	Float GetInverseSurfaceArea() const
	{
		return (1.0f / GetSurfaceArea());
	}

	/**
	 * Returns the axis enum corresponding to the longest dimension
	 * of the AABB.
	 */
	Axis GetMaxAxis() const
	{
		const Vector3D dimensions = GetDimensions();
		Float max = Max(dimensions.X, dimensions.Y, dimensions.Z);

		if (max == dimensions.X)
		{
			return Axis::kX;
		}
		else if (max == dimensions.Y)
		{
			return Axis::kY;
		}
		else
		{
			return Axis::kZ;
		}
	}

	/**
	 * Returns the surface area of the AABB.
	 *
	 * Surface area is often used to determine the # of points of entry
	 * into the AABB or the probability that a random ray will intersect
	 * the AABB. See kdTree.h for a usage example of the surface area of
	 * an AABB.
	 */
	Float GetSurfaceArea() const
	{
		Vector3D whd = GetDimensions();
		Float sa = 2.0f * ((whd.Z * whd.X) + (whd.Z * whd.Y) + (whd.X * whd.Y));

		return sa;
	}

	Bool Contains(const AABB& aabb) const;

	/**
	 * @return True if this AABB is equal to aabb within the
	 * tolerance fTolerance, false otherwise.
	 */
	Bool Equals(const AABB& aabb, Float fTolerance = fEpsilon) const
	{
		return
			m_vMin.Equals(aabb.m_vMin, fTolerance) &&
			m_vMax.Equals(aabb.m_vMax, fTolerance);
	}

	/**
	 * Expand AABB size in all dimensions by specified amount, keeping the center
	 * unchanged. This is split between the positive and negative directions along
	 * each dimension.
	 */
	void Expand(Float fDeltaAmount)
	{
		Vector3D const vOffset(fDeltaAmount / 2.0f);
		m_vMin -= vOffset;
		m_vMax += vOffset;
	}

	Bool Intersects(const AABB& aabb) const;
	Bool Intersects(const Vector3D& v, Float fTolerance = fEpsilon) const;

	/**
	 * Returns true if aabb is a Really Big (TM) AABB, false
	 * otherwise.
	 *
	 * A Really Big (TM) AABB is defined as an AABB for which
	 * fMultiple * AABB::GetSurfaceArea() results in a NaN or
	 * infinity value.
	 */
	Bool IsHuge(Float fMultiple = 2.5f) const
	{
		Float fTwoTimesSurfaceArea = fMultiple * GetSurfaceArea();

		return IsNaN(fTwoTimesSurfaceArea) || IsInf(fTwoTimesSurfaceArea);
	}

	/**
	 * Returns true if aabb is a valid AABB with min components
	 * <= max components, false otherwise.
	 *
	 * An AABB with a zero-size side *is* a valid AABB.
	 */
	Bool IsValid() const
	{
		return (
			m_vMin.X <= m_vMax.X &&
			m_vMin.Y <= m_vMax.Y &&
			m_vMin.Z <= m_vMax.Z);
	}

	/**
	 * Returns an AABB defined by a center point and extents.
	 *
	 * Extents are one-half the dimensions of the AABB.
	 */
	static AABB CreateFromCenterAndExtents(
		const Vector3D& vCenter,
		const Vector3D& vExtents)
	{
		AABB aabb;
		aabb.m_vMin = (vCenter - vExtents);
		aabb.m_vMax = (vCenter + vExtents);
		return aabb;
	}

	/**
	 * Returns an AABB defined by a minimum and maximum corner.
	 */
	static AABB CreateFromMinAndMax(const Vector3D& vMin, const Vector3D& vMax)
	{
		AABB aabb;
		aabb.m_vMin = vMin;
		aabb.m_vMax = vMax;
		return aabb;
	}

	/**
	 * Given two AABBs, this method returns a merged AABB which
	 * tightly enclose a and b.
	 */
	static AABB CalculateMerged(const AABB& a, const AABB& b)
	{
		return AABB::CreateFromMinAndMax(
			Vector3D::Min(a.m_vMin, b.m_vMin),
			Vector3D::Max(a.m_vMax, b.m_vMax));
	}

	/**
	 * Given a sphere sphere, this method calculates an AABB
	 * which tightly encloses the sphere.
	 */
	static AABB CalculateFromSphere(const Sphere& sphere)
	{
		return AABB::CreateFromCenterAndExtents(
			Vector3D(sphere.m_vCenter),
			Vector3D(sphere.m_fRadius));
	}

	static AABB CalculateFromAABBs(AABB const* pBegin, AABB const* pEnd);
	static AABB CalculateFromPoints(Vector3D const* pBegin, Vector3D const* pEnd);

	// Transforms aabb by matrix mTransform and returns
	// the resulting AABB.
	//
	// The returned AABB is an AABB that most tightly fits the
	// 8 corners of aabb after they have been transformed into the
	// space defined by mTransform.
	static AABB Transform(const Matrix4D& mTransform, const AABB& aabb);

	/**
	 * Clamps a point vPoint to fall within the AABB aabb.
	 */
	static Vector3D Clamp(const Vector3D& vPoint, const AABB& aabb)
	{
		return Vector3D(
			Seoul::Clamp(vPoint.X, aabb.m_vMin.X, aabb.m_vMax.X),
			Seoul::Clamp(vPoint.Y, aabb.m_vMin.Y, aabb.m_vMax.Y),
			Seoul::Clamp(vPoint.Z, aabb.m_vMin.Z, aabb.m_vMax.Z));
	}

	/**
	 * An inverse max AABB is an AABB whose min vector
	 * is equal to a vector whose components are all maximal float
	 * values and whose max vector is equal to a vector whose
	 * components are all minimal float values.
	 *
	 * An inverse max AABB is useful for starting an accumulation
	 * function that builds an enclosing AABB for a collection of points.
	 *
	 * \warning An inverse max AABB is not a valid AABB - it will fail when
	 * checked by AABB::IsValid().
	 */
	static AABB InverseMaxAABB()
	{
		// We use 0.5 * FloatMax as the max box, so GetDimensions() of a max box returns a non-NaN value.
		return AABB::CreateFromMinAndMax(
			0.5f * Vector3D(FloatMax, FloatMax, FloatMax),
			0.5f * Vector3D(-FloatMax, -FloatMax, -FloatMax));
	}

	/** Max AABB is the maximum, valid AABB expected to be handleable in SeoulEngine. */
	static AABB MaxAABB()
	{
		// We use 0.5 * FloatMax as the max box, so GetDimensions() of a max box returns a non-NaN value.
		return CreateFromMinAndMax(
			0.5f * Vector3D(-FloatMax, -FloatMax, -FloatMax),
			0.5f * Vector3D(FloatMax, FloatMax, FloatMax));
	}
}; // struct AABB

/** Tolerance equality test between a and b. */
inline Bool Equals(const AABB& a, const AABB& b, Float fTolerance = fEpsilon)
{
	return a.Equals(b, fTolerance);
}

} // namespace Seoul

#endif // include guard
