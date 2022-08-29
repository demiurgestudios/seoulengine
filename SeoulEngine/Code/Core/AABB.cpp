/**
 * \file AABB.cpp
 * \brief Axis-aligned bounding box. Basic geometric primitive often
 * used for spatial sorting.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AABB.h"
#include "Vector.h"

namespace Seoul
{

//----------------------------------------------------------------------------
// AABB
//----------------------------------------------------------------------------
/**
 * Returns true if aabb is contained completely with this AABB,
 * false otherwise.
 */
Bool AABB::Contains(const AABB& aabb) const
{
	if (aabb.m_vMin.X < m_vMin.X) return false;
	if (aabb.m_vMax.X > m_vMax.X) return false;
	if (aabb.m_vMin.Y < m_vMin.Y) return false;
	if (aabb.m_vMax.Y > m_vMax.Y) return false;
	if (aabb.m_vMin.Z < m_vMin.Z) return false;
	if (aabb.m_vMax.Z > m_vMax.Z) return false;

	return true;
}

/**
 * Returns true if aabb overlaps this AABB, false otherwise.
 */
Bool AABB::Intersects(const AABB& aabb) const
{
	if (m_vMin.X > aabb.m_vMax.X) return false;
	if (m_vMax.X < aabb.m_vMin.X) return false;
	if (m_vMin.Y > aabb.m_vMax.Y) return false;
	if (m_vMax.Y < aabb.m_vMin.Y) return false;
	if (m_vMin.Z > aabb.m_vMax.Z) return false;
	if (m_vMax.Z < aabb.m_vMin.Z) return false;

	return true;
}

/**
 * Returns true if v is inside this AABB, within the
 * tolerance fTolerance.
 */
Bool AABB::Intersects(const Vector3D& v, Float fTolerance) const
{
	if (m_vMin.X > v.X + fTolerance) return false;
	if (m_vMax.X < v.X - fTolerance) return false;
	if (m_vMin.Y > v.Y + fTolerance) return false;
	if (m_vMax.Y < v.Y - fTolerance) return false;
	if (m_vMin.Z > v.Z + fTolerance) return false;
	if (m_vMax.Z < v.Z - fTolerance) return false;

	return true;
}

/**
 * Given a Vector<> of AABBs, returns an AABB that tightly
 * encloses all of those AABBs.
 *
 * This function assumes that all of the AABBs in vAABBs
 * are defined such that the components of Min are <= the components
 * of Max.
 */
AABB AABB::CalculateFromAABBs(AABB const* pBegin, AABB const* pEnd)
{
	// Start with an inverted max box.
	AABB ret = AABB::InverseMaxAABB();

	for (auto p = pBegin; pEnd != p; ++p)
	{
		ret.m_vMin = Vector3D::Min(ret.m_vMin, p->m_vMin);
		ret.m_vMax = Vector3D::Max(ret.m_vMax, p->m_vMax);
	}

	return ret;
}

/**
 * Given a Vector<> of points, returns an AABB that tightly
 * encloses all of those points.
 */
AABB AABB::CalculateFromPoints(Vector3D const* pBegin, Vector3D const* pEnd)
{
	// Start with an inverted max box.
	AABB ret = AABB::InverseMaxAABB();

	for (auto p = pBegin; pEnd != p; ++p)
	{
		ret.m_vMin = Vector3D::Min(ret.m_vMin, *p);
		ret.m_vMax = Vector3D::Max(ret.m_vMax, *p);
	}

	return ret;
}

/**
 * Transforms aabb by matrix mTransform and returns
 * the resulting AABB.
 *
 * The returned AABB is an AABB that most tightly fits the
 * 8 corners of aabb after they have been transformed into the
 * space defined by mTransform.
 */
AABB AABB::Transform(const Matrix4D& mTransform, const AABB& aabb)
{
	Vector3D cornerBuffer[8];

	cornerBuffer[0] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMin.X, aabb.m_vMin.Y, aabb.m_vMin.Z));
	cornerBuffer[1] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMin.X, aabb.m_vMin.Y, aabb.m_vMax.Z));
	cornerBuffer[2] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMin.X, aabb.m_vMax.Y, aabb.m_vMin.Z));
	cornerBuffer[3] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMin.X, aabb.m_vMax.Y, aabb.m_vMax.Z));
	cornerBuffer[4] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMax.X, aabb.m_vMin.Y, aabb.m_vMin.Z));
	cornerBuffer[5] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMax.X, aabb.m_vMin.Y, aabb.m_vMax.Z));
	cornerBuffer[6] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMax.X, aabb.m_vMax.Y, aabb.m_vMin.Z));
	cornerBuffer[7] = Matrix4D::TransformPosition(mTransform, Vector3D(aabb.m_vMax.X, aabb.m_vMax.Y, aabb.m_vMax.Z));

	AABB ret = AABB::InverseMaxAABB();
	for (int i = 0; i < 8; i++)
	{
		ret.m_vMin = Vector3D::Min(ret.m_vMin, cornerBuffer[i]);
		ret.m_vMax = Vector3D::Max(ret.m_vMax, cornerBuffer[i]);
	}

	return ret;
}

} // namespace Seoul
