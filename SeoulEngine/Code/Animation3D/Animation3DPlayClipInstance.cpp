/**
 * \file Animation3DPlayClipInstance.cpp
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

#include "AnimationEventInterface.h"
#include "AnimationNetworkInstance.h"
#include "AnimationPlayClipDefinition.h"
#include "Animation3DClipInstance.h"
#include "Animation3DData.h"
#include "Animation3DPlayClipInstance.h"
#include "Animation3DState.h"
#include "Logger.h"

#if SEOUL_WITH_ANIMATION_3D

namespace Seoul::Animation3D
{

PlayClipInstance::PlayClipInstance(
	Animation::NetworkInstance& r,
	const SharedPtr<Animation::PlayClipDefinition const>& pPlayClip,
	const Animation::ClipSettings& settings)
	: m_Settings(settings)
	, m_r(r)
	, m_pPlayClip(pPlayClip)
	, m_pClipInstance()
	, m_fTime(0.0f)
	, m_bDone(false)
{
	auto pClip = static_cast<const Data&>(m_r.GetDataInterface()).GetPtr()->GetClip(m_pPlayClip->GetName());
	if (!pClip.IsValid())
	{
		SEOUL_WARN("Network %s refers to non-existent animation clip: %s", r.GetNetworkHandle().GetKey().CStr(), m_pPlayClip->GetName().CStr());
		return;
	}
	m_pClipInstance.Reset(SEOUL_NEW(MemoryBudgets::Animation3D) ClipInstance(static_cast<State&>(r.GetStateInterface()).GetInstance(), pClip, settings));
}

PlayClipInstance::~PlayClipInstance()
{
}

Float PlayClipInstance::GetCurrentMaxTime() const
{
	return (m_pClipInstance.IsValid()
		? m_pClipInstance->GetMaxTime()
		: 0.0f);
}

Bool PlayClipInstance::GetNextEventTime(HString sEventName, Float fStartTime, Float& fEventTime) const
{
	return (m_pClipInstance.IsValid()
		? m_pClipInstance->GetNextEventTime(sEventName, fStartTime, fEventTime)
		: false);
}

Bool PlayClipInstance::GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const
{
	Float fEventTime;

	bool bFoundEvent = GetNextEventTime(sEventName, m_fTime, fEventTime);

	if (bFoundEvent)
	{
		SEOUL_ASSERT(fEventTime > m_fTime);

		fTimeToEvent = fEventTime - m_fTime;

		return true;
	}

	// event was not found after the current time.  Are we looping?
	if (!m_pPlayClip->GetLoop())
	{
		// we aren't looping, so we are done.
		return false;
	}

	// we are looping, so try looking from the start
	bFoundEvent = GetNextEventTime(sEventName, 0.0f, fEventTime);

	if (!bFoundEvent)
	{
		// it's really not there.
		return false;
	}

	// we are looping, and we found it
	Float fMaxTime = GetCurrentMaxTime();

	SEOUL_ASSERT(fMaxTime >= m_fTime);

	if (fMaxTime >= m_fTime)
	{
		// time until next event = (time until anim loops) + (time until the event fires after looping)
		fTimeToEvent = (fMaxTime - m_fTime) + fEventTime;
	}

	return true;
}

void PlayClipInstance::OnComplete()
{
	m_bDone = true;
	if (!m_pPlayClip->GetOnComplete().IsEmpty())
	{
		m_r.TriggerTransition(m_pPlayClip->GetOnComplete());
		auto pEventInterface(m_r.GetEventInterface());
		if (pEventInterface.IsValid())
		{
			pEventInterface->DispatchEvent(m_pPlayClip->GetOnComplete(), 0, 0.0f, String());
		}
	}
}

void PlayClipInstance::AllDonePlaying(Bool& rbDone, Bool& rbLooping) const
{
	rbDone = m_bDone;
	rbLooping = m_pPlayClip->GetLoop();
}

void PlayClipInstance::TriggerTransition(HString name)
{
	// Nop
}

Bool PlayClipInstance::Tick(Float fDeltaTimeInSeconds, Float fAlpha, Bool bBlendDiscreteState)
{
	// Early out if no evaluator.
	if (!m_pClipInstance.IsValid())
	{
		if (!m_bDone && !m_pPlayClip->GetLoop())
		{
			OnComplete();
		}

		return false;
	}

	Float fLastTime = m_fTime;
	m_fTime += fDeltaTimeInSeconds;

	// Get the max time of the clip we're playing.
	auto const fMaxTime = m_pClipInstance->GetMaxTime();

	// If we're looping, keep advancing through ranges until we reach the target time.
	if (m_pPlayClip->GetLoop() && fMaxTime > 1e-4f)
	{
		while (m_fTime > fMaxTime)
		{
			// Sanity chec, this should always be enforced as true.
			SEOUL_ASSERT(fLastTime <= fMaxTime);

			// If we have a range to evaluate, do so now.
			if (fMaxTime > fLastTime)
			{
				m_pClipInstance->EvaluateRange(fLastTime, fMaxTime, fAlpha);
			}

			// Subtract the range from m_fTime.
			m_fTime -= fMaxTime;

			// Reset last time to the begining.
			fLastTime = 0.0f;
		}
	}
	// Otherwise, just clamp the time against the max time.
	else
	{
		m_fTime = Min(m_fTime, fMaxTime);
	}

	// Now perform the (possibly only) range evaluation, from the last
	// time to the current.
	if (m_fTime > fLastTime)
	{
		m_pClipInstance->EvaluateRange(fLastTime, m_fTime, fAlpha);
	}

	// Apply instance (sampled) evaluations.
	m_pClipInstance->Evaluate(m_fTime, fAlpha, bBlendDiscreteState);

	// If not looping and not done, check if we've hit the end of the clip.
	if (!m_bDone && !m_pPlayClip->GetLoop() && m_fTime >= m_pClipInstance->GetMaxTime())
	{
		OnComplete();
	}

	return true;
}

} // namespace Seoul::Animation3D

#endif // /#if SEOUL_WITH_ANIMATION_3D
