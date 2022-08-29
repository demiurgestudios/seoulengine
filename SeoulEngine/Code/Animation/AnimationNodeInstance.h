/**
 * \file AnimationNodeInstance.h
 * \brief Base class of the runtime instantiation of an animation
 * network node. Subclasses of this class are used to perform runtime
 * process of an animation network.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_NODE_INSTANCE_H
#define ANIMATION_NODE_INSTANCE_H

#include "AnimationNodeType.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SharedPtr.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class NodeInstance SEOUL_ABSTRACT
{
public:
	virtual ~NodeInstance();

	virtual Float GetCurrentMaxTime() const = 0;

	// returns true if the animation event was found after the current animation time, and sets the time to event
	// returns false if the animation event was not found
	virtual Bool GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const = 0;

	virtual NodeType GetType() const = 0;

	virtual void AllDonePlaying(Bool& rbDone, Bool& rbLooping) const = 0;
	virtual Bool IsInStateTransition() const = 0;

	virtual void TriggerTransition(HString name) = 0;
	virtual Bool Tick(Float fDeltaTimeInSeconds, Float fAlpha, Bool bBlendDiscreteState) = 0;

protected:
	NodeInstance();
	SEOUL_REFERENCE_COUNTED(NodeInstance);

	SEOUL_DISABLE_COPY(NodeInstance);
}; // class NodeInstance

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
