/**
 * \file Animation3DPlayClipInstance.h
 * \brief Runtime instantiation of a clip playback animation network
 * node. This is a leaf in the animation graph, and handles the
 * job of actually playing an animation clip.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_PLAY_CLIP_INSTANCE_H
#define ANIMATION3D_PLAY_CLIP_INSTANCE_H

#include "AnimationClipSettings.h"
#include "AnimationNodeInstance.h"
#include "ScopedPtr.h"
namespace Seoul { namespace Animation { class NetworkInstance; } }
namespace Seoul { namespace Animation { class PlayClipDefinition; } }
namespace Seoul { namespace Animation3D { class ClipInstance; } }

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

class PlayClipInstance SEOUL_SEALED : public Animation::NodeInstance
{
public:
	PlayClipInstance(
		Animation::NetworkInstance& r,
		const SharedPtr<Animation::PlayClipDefinition const>& pPlayClip,
		const Animation::ClipSettings& settings);
	~PlayClipInstance();

	Float GetCurrentTime() const { return m_fTime; }
	const SharedPtr<Animation::PlayClipDefinition const>& GetPlayClip() const { return m_pPlayClip; }
	Bool IsDone() const { return m_bDone; }
	virtual Float GetCurrentMaxTime() const SEOUL_OVERRIDE;

	// returns true if the animation event was found after the current animation time, and sets the time to event
	// returns false if the animation event was not found
	virtual Bool GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const SEOUL_OVERRIDE;

	virtual Animation::NodeType GetType() const SEOUL_OVERRIDE { return Animation::NodeType::kPlayClip; }
	virtual void AllDonePlaying(Bool& rbDone, Bool& rbLooping) const SEOUL_OVERRIDE;
	virtual Bool IsInStateTransition() const SEOUL_OVERRIDE { return false; }
	virtual void TriggerTransition(HString name) SEOUL_OVERRIDE;
	virtual Bool Tick(Float fDeltaTimeInSeconds, Float fAlpha, Bool bBlendDiscreteState) SEOUL_OVERRIDE;

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(PlayClipInstance);

	// returns true if the animation event was found after the current animation time, and sets the event time
	// returns false if the animation event was not found
	Bool GetNextEventTime(HString sEventName, Float fStartTime, Float& fEventTime) const;

	void OnComplete();

	Animation::ClipSettings const m_Settings;
	Animation::NetworkInstance& m_r;
	SharedPtr<Animation::PlayClipDefinition const> m_pPlayClip;
	ScopedPtr<ClipInstance> m_pClipInstance;
	Float32 m_fTime;
	Bool m_bDone;

	SEOUL_DISABLE_COPY(PlayClipInstance);
}; // class PlayClipInstance

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
