/**
 * \file Frustum.cpp
 * \brief A Frustum is a 6-sided, convex bounding region defined by planes.
 * A Frustum is typically used to define the region formed by a camera
 * projection matrix but can also be used to define regions such as spot
 * light shadow regions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Frustum.h"
#include "GeometryUtil.h"

namespace Seoul
{

/** Sanity - many methods of Frustum assume this. */
SEOUL_STATIC_ASSERT_MESSAGE(Frustum::PLANE_COUNT == 6, "Update GetAAB, GetCorners.");

/**
 * Calculates an AABB that tightly fits the 8 corners of the
 * frustum.
 */
AABB Frustum::GetAABB() const
{
	// Start with an inverse max size AABB.
	AABB ret = AABB::InverseMaxAABB();

	Vector3D vPoint;

	// Near Top Left
	if (GetIntersection(vPoint, m_aPlanes[kNear], m_aPlanes[kTop], m_aPlanes[kLeft]))
	{
		ret.AbsorbPoint(vPoint);
	}

	// Near Top Right
	if (GetIntersection(vPoint, m_aPlanes[kNear], m_aPlanes[kTop], m_aPlanes[kRight]))
	{
		ret.AbsorbPoint(vPoint);
	}

	// Near Bottom Right
	if (GetIntersection(vPoint, m_aPlanes[kNear], m_aPlanes[kBottom], m_aPlanes[kRight]))
	{
		ret.AbsorbPoint(vPoint);
	}

	// Near Bottom Left
	if (GetIntersection(vPoint, m_aPlanes[kNear], m_aPlanes[kBottom], m_aPlanes[kLeft]))
	{
		ret.AbsorbPoint(vPoint);
	}

	// Far Top Left
	if (GetIntersection(vPoint, m_aPlanes[kFar], m_aPlanes[kTop], m_aPlanes[kLeft]))
	{
		ret.AbsorbPoint(vPoint);
	}

	// Far Top Right
	if (GetIntersection(vPoint, m_aPlanes[kFar], m_aPlanes[kTop], m_aPlanes[kRight]))
	{
		ret.AbsorbPoint(vPoint);
	}

	// Far Bottom Right
	if (GetIntersection(vPoint, m_aPlanes[kFar], m_aPlanes[kBottom], m_aPlanes[kRight]))
	{
		ret.AbsorbPoint(vPoint);
	}

	// Far Bottom Left
	if (GetIntersection(vPoint, m_aPlanes[kFar], m_aPlanes[kBottom], m_aPlanes[kLeft]))
	{
		ret.AbsorbPoint(vPoint);
	}

	return ret;
}

/**
 * Calculates the 8 vertices of the frustum.
 *
 *  The 8 vertices of the frustum are calculated and stored in the array
 *  vCorners in the order:
 *      aCorners[0] = Near Top Left
 *      aCorners[1] = Near Top Right
 *      aCorners[2] = Near Bottom Right
 *      aCorners[3] = Near Bottom Left
 *      aCorners[4] = Far Top Left
 *      aCorners[5] = Far Top Right
 *      aCorners[6] = Far Bottom Right
 *      aCorners[7] = Far Bottom Left
 */
void Frustum::GetCornerVertices(Vector3D aCorners[8]) const
{
	SEOUL_VERIFY(GetIntersection(aCorners[0], m_aPlanes[kNear], m_aPlanes[kTop], m_aPlanes[kLeft])); // Near Top Left
	SEOUL_VERIFY(GetIntersection(aCorners[1], m_aPlanes[kNear], m_aPlanes[kTop], m_aPlanes[kRight])); // Near Top Right
	SEOUL_VERIFY(GetIntersection(aCorners[2], m_aPlanes[kNear], m_aPlanes[kBottom], m_aPlanes[kRight])); // Near Bottom Right
	SEOUL_VERIFY(GetIntersection(aCorners[3], m_aPlanes[kNear], m_aPlanes[kBottom], m_aPlanes[kLeft])); // Near Bottom Left
	SEOUL_VERIFY(GetIntersection(aCorners[4], m_aPlanes[kFar], m_aPlanes[kTop], m_aPlanes[kLeft])); // Far Top Left
	SEOUL_VERIFY(GetIntersection(aCorners[5], m_aPlanes[kFar], m_aPlanes[kTop], m_aPlanes[kRight])); // Far Top Right
	SEOUL_VERIFY(GetIntersection(aCorners[6], m_aPlanes[kFar], m_aPlanes[kBottom], m_aPlanes[kRight])); // Far Bottom Right
	SEOUL_VERIFY(GetIntersection(aCorners[7], m_aPlanes[kFar], m_aPlanes[kBottom], m_aPlanes[kLeft])); // Far Bottom Left
}

/**
 * Intersects a frustum with a bounding sphere.
 * @return
 *   kDisjoint   If the sphere is completely outside the planes of the frustum.
 *   kIntersects If the sphere is touching one or more of the planes of the frustum.
 *   kContains   If the sphere is completely inside the planes of the frustum.
 */
FrustumTestResult Frustum::Intersects(const Sphere& sphere) const
{
	FrustumTestResult ret = FrustumTestResult::kContains;

	// Apply epsilon to radius as a margin.
	const Float kRadius = sphere.m_fRadius - fEpsilon;
	const Float kNegRadius = -kRadius;

	auto const iBegin = m_aPlanes.Begin();
	auto const iEnd = m_aPlanes.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		const Float d = i->DotCoordinate(sphere.m_vCenter);

		if (d < kNegRadius) { return FrustumTestResult::kDisjoint; }
		else if (d <= kRadius) { ret = FrustumTestResult::kIntersects; }
	}

	return ret;
}

/**
 * Intersects a frustum with an axis-aligned bounding box.
 * @return
 *   kDisjoint   If the AABB is completely outside the planes of the frustum.
 *   kIntersects If the AABB is touching one or more of the planes of the frustum.
 *   kContains   If the AABB is completely inside the planes of the frustum.
 */
FrustumTestResult Frustum::Intersects(const AABB& aabb) const
{
	FrustumTestResult ret = FrustumTestResult::kContains;
	Vector3D center = aabb.GetCenter();

	const Vector3D rst = aabb.GetDimensions();
	auto const iBegin = m_aPlanes.Begin();
	auto const iEnd = m_aPlanes.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		// Apply epsilon to radius as a margin.
		const Float radius = (0.5f * Vector3D::Dot(rst, i->GetNormal().Abs())) - fEpsilon;
		const Float negRadius = -radius;
		const Float d = i->DotCoordinate(center);

		if (d < negRadius) { return FrustumTestResult::kDisjoint; }
		else if (d <= radius) { ret = FrustumTestResult::kIntersects; }
	}

	return ret;
}

/**
 * Intersects a frustum with a point.
 * @return
 *   kDisjoint   If the point is completely outside the planes of the frustum.
 *   kIntersects If the point is touching one or more of the planes of the frustum.
 *   kContains   If the point is completely inside the planes of the frustum.
 */
FrustumTestResult Frustum::Intersects(const Vector3D& point) const
{
	FrustumTestResult ret = FrustumTestResult::kContains;

	auto const iBegin = m_aPlanes.Begin();
	auto const iEnd = m_aPlanes.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		const Float d = i->DotCoordinate(point);

		if (d < -fEpsilon) { return FrustumTestResult::kDisjoint; }
		else if (d <= fEpsilon) { ret = FrustumTestResult::kIntersects; }
	}

	return ret;
}

/**
 * Calculates the Frustum's 6 bounding planes using a view and
 * projection matrix.
 *
 * See Section 16.14.1 of
 *   Akenine-Moller, T., Haines, E., Hoffman, N. 2008.
 *   "Real-Time Rendering: Third Edition", AK Peters, Ltd.
 */
void Frustum::Set(const Matrix4D& mProjectionMatrix, const Matrix4D& mViewMatrix)
{
	// Calculate the frustum planes.
	const Matrix4D m = (mProjectionMatrix * mViewMatrix);
	Vector4D r4 = m.GetRow(3);

	m_aPlanes[kLeft].Set(r4 + m.GetRow(0));
	m_aPlanes[kLeft].Normalize();
	m_aPlanes[kRight].Set(r4 - m.GetRow(0));
	m_aPlanes[kRight].Normalize();

	m_aPlanes[kBottom].Set(r4 + m.GetRow(1));
	m_aPlanes[kBottom].Normalize();
	m_aPlanes[kTop].Set(r4 - m.GetRow(1));
	m_aPlanes[kTop].Normalize();

	m_aPlanes[kNear].Set(m.GetRow(2));
	m_aPlanes[kNear].Normalize();
	m_aPlanes[kFar].Set(r4 - m.GetRow(2));
	m_aPlanes[kFar].Normalize();
}

} // namespace Seoul
