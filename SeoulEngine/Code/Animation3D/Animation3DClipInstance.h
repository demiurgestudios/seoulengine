/**
 * \file Animation3DClipInstance.h
 * \brief An instance of an Animation3D::Clip. Necessary for runtime
 * playback of the clip's animation timelines.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANIMATION3D_CLIP_INSTANCE_H
#define ANIMATION3D_CLIP_INSTANCE_H

#include "AnimationClipSettings.h"
#include "CheckedPtr.h"
#include "Prereqs.h"
#include "SharedPtr.h"
#include "Vector.h"
namespace Seoul { namespace Animation3D { class ClipDefinition; } }
namespace Seoul { namespace Animation3D { class IEvaluator; } }
namespace Seoul { namespace Animation3D { class DataInstance; } }
namespace Seoul { namespace Animation3D { class EventEvaluator; } }

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

class ClipInstance SEOUL_SEALED
{
public:
	ClipInstance(DataInstance& r, const SharedPtr<ClipDefinition>& pClipDefinition, const Animation::ClipSettings& settings);
	~ClipInstance();

	// The number of active animation evaluators in this clip.
	UInt32 GetActiveEvaluatorCount() const { return m_vEvaluators.GetSize(); }

	// Used for event dispatch, pass a time range. Looping should be implemented
	// by passing all time ranges (where fPrevTime >= 0.0f and fTime <= GetMaxTime())
	// iteratively until the final time is reached.
	void EvaluateRange(Float fStartTime, Float fEndTime, Float fAlpha);

	// Apply the clip to the state of DataInstance.
	void Evaluate(Float fTime, Float fAlpha, Bool bBlendDiscreteState);

	/** @return The max time (in seconds) of all timelines in this animation clip. */
	Float GetMaxTime() const { return m_fMaxTime; }

	// returns true if the animation event was found after the current animation time, and sets the event time
	// returns false if the animation event was not found
	Bool GetNextEventTime(HString sEventName, Float fStartTime, Float& fEventTime) const;

private:
	const Animation::ClipSettings m_Settings;
	DataInstance& m_r;
	SharedPtr<ClipDefinition> m_pClipDefinition;
	Float m_fMaxTime;

	typedef Vector<CheckedPtr<IEvaluator>, MemoryBudgets::Animation3D> Evaluators;
	Evaluators m_vEvaluators;
	CheckedPtr<EventEvaluator> m_pEventEvaluator;

	void InternalConstructEvaluators();

	SEOUL_DISABLE_COPY(ClipInstance);
}; // class ClipInstance

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D

#endif // include guard
