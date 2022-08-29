/**
 * \file UIHitShapeInstance.h
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

#pragma once
#ifndef UI_HIT_SHAPE_INSTANCE_H
#define UI_HIT_SHAPE_INSTANCE_H

#include "FalconInstance.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"

namespace Seoul::UI
{

/** Custom subclass of Falcon::Instance, allows hit shapes without rendering. */
class HitShapeInstance SEOUL_SEALED : public Falcon::Instance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(HitShapeInstance);

	HitShapeInstance(const Falcon::Rectangle& bounds);
	~HitShapeInstance();

	virtual Instance* Clone(Falcon::AddInterface& rInterface) const SEOUL_OVERRIDE
	{
		HitShapeInstance* pReturn = SEOUL_NEW(MemoryBudgets::UIRuntime) HitShapeInstance(m_Bounds);
		CloneTo(rInterface, *pReturn);
		return pReturn;
	}

	virtual Bool ComputeLocalBounds(Falcon::Rectangle& rBounds) SEOUL_OVERRIDE;

	virtual void ComputeMask(
		const Matrix2x3& mParent,
		const Falcon::ColorTransformWithAlpha& cxParent,
		Falcon::Render::Poser& rPoser) SEOUL_OVERRIDE;

#if SEOUL_ENABLE_CHEATS
	virtual void PoseInputVisualization(
		Falcon::Render::Poser& rPoser,
		const Matrix2x3& mParent,
		RGBA color) SEOUL_OVERRIDE;
#endif

	virtual Bool HitTest(
		const Matrix2x3& mParent,
		Float32 fWorldX,
		Float32 fWorldY,
		Bool bIgnoreVisibility = false) const SEOUL_OVERRIDE;

	virtual Falcon::InstanceType GetType() const SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(HitShapeInstance);

	Falcon::Rectangle m_Bounds;

	SEOUL_DISABLE_COPY(HitShapeInstance);
}; // class UI::HitShapeInstance

} // namespace Seoul::UI

#endif // include guard
