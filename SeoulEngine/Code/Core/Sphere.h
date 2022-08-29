/**
 * \file Sphere.h
 * \brief Bounding sphere, basic geometric primitive often
 * used for spatial sorting and culling.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SPHERE_H
#define SPHERE_H

#include "Vector3D.h"

namespace Seoul
{

struct Sphere SEOUL_SEALED
{
	Vector3D m_vCenter;
	Float m_fRadius;

	Sphere()
		: m_vCenter(0, 0, 0)
		, m_fRadius(0.0f)
	{
	}

	Sphere(const Vector3D& vCenter, Float fRadius)
		: m_vCenter(vCenter)
		, m_fRadius(fRadius)
	{
	}

	Sphere(Float fCenterX, Float fCenterY, Float fCenterZ, Float fRadius)
		: m_vCenter(fCenterX, fCenterY, fCenterZ)
		, m_fRadius(fRadius)
	{
	}

	Sphere(const Sphere& sphere)
		: m_vCenter(sphere.m_vCenter)
		, m_fRadius(sphere.m_fRadius)
	{
	}

	Sphere& operator=(const Sphere & sphere)
	{
		m_vCenter = sphere.m_vCenter;
		m_fRadius = sphere.m_fRadius;

		return *this;
	}

	/** @return True if v is exactly contained within this Sphere, false otherwise. */
	Bool ContainsPoint(const Vector3D& v) const
	{
		return (m_vCenter - v).LengthSquared() <= m_fRadius * m_fRadius;
	}

	/** (radius * radius) of the sphere. */
	Float RadiusSquared() const
	{
		return (m_fRadius * m_fRadius);
	}
}; // struct Sphere

/** Exact equality of 2 Sphere instances (no tolerance). */
static inline Bool operator==(const Sphere& a, const Sphere& b)
{
	return (
		a.m_vCenter == b.m_vCenter &&
		b.m_fRadius == b.m_fRadius);
}

/** Exact inequality of 2 Sphere instances (no tolerance). */
static inline Bool operator!=(const Sphere& a, const Sphere& b)
{
	return !(a == b);
}

} // namespace Seoul

#endif // include guard
