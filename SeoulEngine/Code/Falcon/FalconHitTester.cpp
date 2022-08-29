/**
 * \file FalconHitTester.cpp
 * \brief Utility for 3D depth projection during input hit testing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconHitTester.h"

namespace Seoul::Falcon
{

/**
 * Project a 2D point to its 3D post projection position - meant for bounds compensation
 * and other CPU side computations. Rendering projection is done by the GPU so that
 * texture sampling is perspective correct.
 */
Vector2D HitTester::DepthProject(Float fX, Float fY) const
{
	auto const v = Vector2D(fX, fY);
	Float const fW = ComputeCurrentOneOverW();
	auto const& vp = m_vViewProjectionTransform;
	auto const vScale = vp.GetXY();
	auto const vShift = vp.GetZW();

	// Project the point into projection space.
	auto const vProj = Vector2D::ComponentwiseMultiply(v, vScale) + vShift;

	// Now divide by W to place the coordinate in clip space [-1, 1].
	auto const vPostProj = (vProj * fW);

	// Because our UI world space is just a 2D space, we can convert clip space
	// back into Falcon world space with a rescale and shift.
	auto const vUnproj(Vector2D(
		(vPostProj.X * 0.5f + 0.5f) * m_WorldCullRectangle.GetWidth() + m_WorldCullRectangle.m_fLeft,
		(vPostProj.Y * -0.5f + 0.5f) * m_WorldCullRectangle.GetHeight() + m_WorldCullRectangle.m_fTop));

	return vUnproj;
}

Vector2D HitTester::InverseDepthProject(Float fX, Float fY) const
{
	// If the current depth is no zero, we need
	// to unproject the mouse coordinates. They are
	// in mouse world space, which is 3D projection
	// space, and need to be recompensated back into
	// 2D world space.
	auto const fDepth3D = GetModifiedDepth3D();
	if (SEOUL_UNLIKELY(fDepth3D > 1e-4f))
	{
		Float const fW = ComputeCurrentW();
		auto const& vp = m_vViewProjectionTransform;
		auto const vScale = vp.GetXY();
		auto const vShift = vp.GetZW();

		// Convert Falcon world space into clip space.
		auto const vProj(Vector2D(
			(((fX - m_WorldCullRectangle.m_fLeft) / m_WorldCullRectangle.GetWidth()) - 0.5f) * 2.0f,
			(((fY - m_WorldCullRectangle.m_fTop) / m_WorldCullRectangle.GetHeight()) - 0.5f) * -2.0f));

		// Now multiply by W to deproject the point.
		auto const vPostProj = (vProj * fW);

		// Finally, apply the inverse of the view projection transform to place the point
		// back in world vUnproj.
		auto const vUnproj = Vector2D::ComponentwiseDivide((vPostProj - vShift), vScale);

		return vUnproj;
	}
	else
	{
		return Vector2D(fX, fY);
	}
}

} // namespace Seoul::Falcon
