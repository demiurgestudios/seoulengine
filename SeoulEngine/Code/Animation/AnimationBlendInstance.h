/**
 * \file AnimationBlendInstance.h
 * \brief Runtime instantiation of a blend animation network
 * node. Used for runtime playback of a defined blend in an animation
 * graph.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION_BLEND_INSTANCE_H
#define ANIMATION_BLEND_INSTANCE_H

#include "AnimationNodeInstance.h"
namespace Seoul { namespace Animation { class BlendDefinition; } }
namespace Seoul { namespace Animation { class NetworkInstance; } }
namespace Seoul { namespace Animation { struct NodeCreateData; } }

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

class BlendInstance SEOUL_SEALED : public NodeInstance
{
public:
	BlendInstance(NetworkInstance& r, const SharedPtr<BlendDefinition const>& pBlend, const NodeCreateData& creationData);
	~BlendInstance();

	const SharedPtr<BlendDefinition const>& GetBlend() const { return m_pBlend; }
	const SharedPtr<NodeInstance>& GetChildA() const { return m_pChildA; }
	const SharedPtr<NodeInstance>& GetChildB() const { return m_pChildB; }
	Float GetCurrentMixParameter() const;

	virtual Float GetCurrentMaxTime() const SEOUL_OVERRIDE;

	// returns true if the animation event was found after the current animation time, and sets the time to event
	// returns false if the animation event was not found
	virtual Bool GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const SEOUL_OVERRIDE;

	virtual NodeType GetType() const SEOUL_OVERRIDE { return NodeType::kBlend; }
	virtual void AllDonePlaying(Bool& rbDone, Bool& rbLooping) const SEOUL_OVERRIDE;
	virtual Bool IsInStateTransition() const SEOUL_OVERRIDE;
	virtual void TriggerTransition(HString name) SEOUL_OVERRIDE;
	virtual Bool Tick(Float fDeltaTimeInSeconds, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(BlendInstance);

	NetworkInstance& m_r;
	SharedPtr<BlendDefinition const> m_pBlend;
	SharedPtr<NodeInstance> m_pChildA;
	SharedPtr<NodeInstance> m_pChildB;

	SEOUL_DISABLE_COPY(BlendInstance);
}; // class BlendInstance

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

#endif // include guard
