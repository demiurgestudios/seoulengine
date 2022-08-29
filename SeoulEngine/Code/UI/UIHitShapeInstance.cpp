/**
 * \file UIHitShapeInstance.cpp
 * \brief SeoulEngine subclass/extension of Falcon::Instance for hit testing.
 *
 * A UI::HitShapeInstance implements a Falcon::Instance that provides an input
 * hit shape, with no other rendering or behavior. It is a useful substitute
 * to "alpha = 0.0" Falcon::ShapeInstances when an input hit shape needs
 * to be invisible.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconRenderPoser.h"
#include "ReflectionDefine.h"
#include "UIHitShapeInstance.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(UI::HitShapeInstance, TypeFlags::kDisableNew)
	SEOUL_PARENT(Falcon::Instance)
SEOUL_END_TYPE()

namespace UI
{

HitShapeInstance::HitShapeInstance(const Falcon::Rectangle& bounds)
	: Falcon::Instance(0)
	, m_Bounds(bounds)
{
}

HitShapeInstance::~HitShapeInstance()
{
}

Bool HitShapeInstance::ComputeLocalBounds(Falcon::Rectangle& rBounds)
{
	rBounds = m_Bounds;
	return true;
}

void HitShapeInstance::ComputeMask(
	const Matrix2x3& mParent,
	const Falcon::ColorTransformWithAlpha& cxParent,
	Falcon::Render::Poser& rPoser)
{
	// TODO: Reconsider - we don't consider the alpha
	// to match Flash behavior. To be confirmed - what
	// happens if you (just) set the visibility of a mask to
	// false and logically it makes sense for visibility and alpha==0.0
	// to have the same behavior (or, in other words, visibility should
	// possibly not be considered here).
	if (!GetVisible())
	{
		return;
	}
	
	// Unlike many code paths, alpha == 0.0 is not considered here. Flash
	// does not hide the mask (or the shapes it reveals) if the cumulative
	// alpha at that mask is 0.0.

	Matrix2x3 const mWorld = (mParent * GetTransform());
	auto const rect = m_Bounds;

	rPoser.ClipStackAddRectangle(mWorld, rect);
}

Bool HitShapeInstance::HitTest(
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Bool bIgnoreVisibility /*= false*/) const
{
	if (!bIgnoreVisibility)
	{
		if (!GetVisible())
		{
			return false;
		}
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Matrix2x3 const mInverseWorld = mWorld.Inverse();

	Vector2D const vObjectSpace = Matrix2x3::TransformPosition(mInverseWorld, Vector2D(fWorldX, fWorldY));
	Float32 const fObjectSpaceX = vObjectSpace.X;
	Float32 const fObjectSpaceY = vObjectSpace.Y;

	if (fObjectSpaceX < m_Bounds.m_fLeft) { return false; }
	if (fObjectSpaceY < m_Bounds.m_fTop) { return false; }
	if (fObjectSpaceX > m_Bounds.m_fRight) { return false; }
	if (fObjectSpaceY > m_Bounds.m_fBottom) { return false; }

	return true;
}

#if SEOUL_ENABLE_CHEATS
void HitShapeInstance::PoseInputVisualization(
	Falcon::Render::Poser& rPoser,
	const Matrix2x3& mParent,
	RGBA color)
{
	auto const& bounds = m_Bounds;

	// TODO: Draw the appropriate shape for exact hit testing.
	auto const mWorld(mParent * GetTransform());
	auto const worldBounds = Falcon::TransformRectangle(mWorld, bounds);
	rPoser.PoseInputVisualization(
		worldBounds,
		bounds,
		mWorld,
		color);
}
#endif

Falcon::InstanceType HitShapeInstance::GetType() const
{
	return Falcon::InstanceType::kCustom;
}

} // namespace UI

} // namespace Seoul
