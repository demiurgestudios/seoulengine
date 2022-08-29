/**
 * \file IPoseable.h
 * \brief Abstract base class (interface) for all objects that
 * can be posed for render.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IPOSEABLE_H
#define IPOSEABLE_H

#include "Geometry.h"
#include "Matrix4D.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
namespace Seoul { class Effect; }
namespace Seoul { class RenderCommandStreamBuilder; }
namespace Seoul { class RenderPass; }

namespace Seoul
{

/**
 * "Posing" is the process of preparing objects for the render phase.
 */
class IPoseable SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(IPoseable);

	virtual ~IPoseable()
	{
	}

	/**
	 * PrePose allows this IPoseable to perform actions
	 * that must occur on the main thread.
	 */
	virtual void PrePose(
		Float fDeltaTime,
		RenderPass& rPass,
		IPoseable* pParent = nullptr)
	{
		// Nop
	}

	/**
	 * When called, Pose is expected to populate
	 * the RenderCommandStreamBuilder of rPass.
	 */
	virtual void Pose(
		Float fDeltaTime,
		RenderPass& rPass,
		IPoseable* pParent = nullptr)
	{
		// Nop
	}

	/**
	 * Called if the render thread is running behind and the Pose() for
	 * the current frame is being skipped. Can be used by Poseables that
	 * perform cleanup in the Pose() call that must always happen, even if
	 * a Pose() is not happening for the current frame.
	 */
	virtual void SkipPose(Float fDeltaTime)
	{
		// Nop
	}

}; // class IPoseable

} // namespace Seoul

#endif // include guard
