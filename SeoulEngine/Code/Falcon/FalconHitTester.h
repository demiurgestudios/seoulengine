/**
 * \file FalconHitTester.h
 * \brief Utility for 3D depth projection during input hit testing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_HIT_TESTER_H
#define FALCON_HIT_TESTER_H

#include "FalconTypes.h"
#include "Prereqs.h"
#include "Vector2D.h"
#include "Vector4D.h"

namespace Seoul::Falcon
{

class HitTester SEOUL_SEALED
{
public:
	HitTester(
		const Vector4D& vViewProjectionTransform,
		const Rectangle& worldCullRectangle,
		Float fPerspectiveFactor)
		: m_vViewProjectionTransform(vViewProjectionTransform)
		, m_WorldCullRectangle(worldCullRectangle)
		, m_fPerspectiveFactor(fPerspectiveFactor)
		, m_fRawDepth3D(0.0f)
		, m_iIgnoreDepthProjection(0)
	{
	}

	HitTester(const HitTester& b)
		: m_vViewProjectionTransform(b.m_vViewProjectionTransform)
		, m_WorldCullRectangle(b.m_WorldCullRectangle)
		, m_fPerspectiveFactor(b.m_fPerspectiveFactor)
		, m_fRawDepth3D(b.m_fRawDepth3D)
		, m_iIgnoreDepthProjection(b.m_iIgnoreDepthProjection)
	{
	}

	HitTester& operator=(const HitTester& b)
	{
		m_vViewProjectionTransform = b.m_vViewProjectionTransform;
		m_WorldCullRectangle = b.m_WorldCullRectangle;
		m_fPerspectiveFactor = b.m_fPerspectiveFactor;
		m_fRawDepth3D = b.m_fRawDepth3D;
		m_iIgnoreDepthProjection = b.m_iIgnoreDepthProjection;

		return *this;
	}

	Vector2D DepthProject(Float fX, Float fY) const;
	Vector2D DepthProject(const Vector2D& v) const
	{
		return DepthProject(v.X, v.Y);
	}
	Vector2D InverseDepthProject(Float fX, Float fY) const;

	void PopDepth3D(Float f, Bool bIgnoreDepthProjection)
	{
		m_fRawDepth3D -= f;
		if (bIgnoreDepthProjection) { --m_iIgnoreDepthProjection; }
	}

	void PushDepth3D(Float f, Bool bIgnoreDepthProjection)
	{
		m_fRawDepth3D += f;
		if (bIgnoreDepthProjection) { ++m_iIgnoreDepthProjection; }
	}

	Pair<Float, Int> ReplaceDepth3D(Float f, Int iIgnoreDepthProjection)
	{
		auto const fReturn = m_fRawDepth3D;
		auto const iReturn = m_iIgnoreDepthProjection;
		m_fRawDepth3D = f;
		m_iIgnoreDepthProjection = iIgnoreDepthProjection;
		return MakePair(fReturn, iReturn);
	}

private:
	/** W is 1.0f / Clamp(1.0f - (depth * perspective)), used for 3D planar projection. */
	Float ComputeCurrentOneOverW() const
	{
		Float const fW = 1.0f / Clamp(1.0f - (GetModifiedDepth3D() * m_fPerspectiveFactor), 1e-4f, 1.0f);
		return fW;
	}

	/** W is Clamp(1.0f - (depth * perspective)), used for 3D planar projection. */
	Float ComputeCurrentW() const
	{
		Float const fW = Clamp(1.0f - (GetModifiedDepth3D() * m_fPerspectiveFactor), 0.0f, 1.0f);
		return fW;
	}

	/** Return project depth value, factoring in m_iIgnoreDepthProjection. */
	Float GetModifiedDepth3D() const { return (0 == m_iIgnoreDepthProjection ? m_fRawDepth3D : 0.0f); }

	Vector4D m_vViewProjectionTransform;
	Rectangle m_WorldCullRectangle;
	Float m_fPerspectiveFactor;
	Float m_fRawDepth3D;
	Int32 m_iIgnoreDepthProjection;
}; // class HitTester

} // namespace Seoul::Falcon

#endif // include guard
