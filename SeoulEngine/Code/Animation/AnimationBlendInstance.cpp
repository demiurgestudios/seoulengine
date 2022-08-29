/**
 * \file AnimationBlendInstance.cpp
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

#include "AnimationBlendDefinition.h"
#include "AnimationBlendInstance.h"
#include "AnimationNetworkInstance.h"

#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)

namespace Seoul::Animation
{

BlendInstance::BlendInstance(NetworkInstance& r, const SharedPtr<BlendDefinition const>& pBlend, const NodeCreateData& creationData)
	: m_r(r)
	, m_pBlend(pBlend)
	, m_pChildA(m_pBlend->GetChildA().IsValid()
		? m_pBlend->GetChildA()->CreateInstance(r, creationData)
		: nullptr)
	, m_pChildB(m_pBlend->GetChildB().IsValid()
		? m_pBlend->GetChildB()->CreateInstance(r, creationData)
		: nullptr)
{
}

BlendInstance::~BlendInstance()
{
}

Float BlendInstance::GetCurrentMixParameter() const
{
	return m_r.GetParameter(m_pBlend->GetMixParameterId());
}

Float BlendInstance::GetCurrentMaxTime() const
{
	Float const fA = (m_pChildA.IsValid() ? m_pChildA->GetCurrentMaxTime() : 0.0f);
	Float const fB = (m_pChildB.IsValid() ? m_pChildB->GetCurrentMaxTime() : 0.0f);
	return Max(fA, fB);
}

Bool BlendInstance::GetTimeToEvent(HString sEventName, Float& fTimeToEvent) const
{
	// NOTE: This does not account for blend alpha!  We're assuming that both
	// children have the capability to fire events.  In practice, that's not going to
	// be true; one or the other will be under their EventMixThreshold.
	// We are doing it this way because the future alpha is basically unknowable.
	if (m_pChildA.IsValid())
	{
		if (m_pChildB.IsValid())
		{
			Float fTimeToEventA;
			Bool bFoundEventA = m_pChildA->GetTimeToEvent(sEventName, fTimeToEventA);

			Float fTimeToEventB;
			Bool bFoundEventB = m_pChildB->GetTimeToEvent(sEventName, fTimeToEventB);

			if (bFoundEventA)
			{
				if (bFoundEventB)
				{
					fTimeToEvent = Min(fTimeToEventA, fTimeToEventB);
				}
				else
				{
					fTimeToEvent = fTimeToEventA;
				}

				return true;
			}
			else if (bFoundEventB)
			{
				fTimeToEvent = fTimeToEventB;
				return true;
			}
		}
		else
		{
			return m_pChildA->GetTimeToEvent(sEventName, fTimeToEvent);
		}
	}
	else if (m_pChildB.IsValid())
	{
		return m_pChildB->GetTimeToEvent(sEventName, fTimeToEvent);
	}

	return false;
}

/** @return True if all playing clips are finished (one-offs that have completed). */
void BlendInstance::AllDonePlaying(Bool& rbDone, Bool& rbLooping) const
{
	rbDone = true;
	rbLooping = false;
	if (m_pChildA.IsValid())
	{
		Bool a = false, b = true;
		m_pChildA->AllDonePlaying(a, b);
		rbDone = rbDone && a;
		rbLooping = rbLooping || b;
	}

	if (m_pChildB.IsValid())
	{
		Bool a = false, b = true;
		m_pChildB->AllDonePlaying(a, b);
		rbDone = rbDone && a;
		rbLooping = rbLooping || b;
	}
}

Bool BlendInstance::IsInStateTransition() const
{
	Bool bReturn = false;
	if (m_pChildA.IsValid())
	{
		bReturn = bReturn || m_pChildA->IsInStateTransition();
	}

	if (m_pChildB.IsValid())
	{
		bReturn = bReturn || m_pChildB->IsInStateTransition();
	}

	return bReturn;
}

void BlendInstance::TriggerTransition(HString name)
{
	if (m_pChildA.IsValid())
	{
		m_pChildA->TriggerTransition(name);
	}

	if (m_pChildB.IsValid())
	{
		m_pChildB->TriggerTransition(name);
	}
}

Bool BlendInstance::Tick(Float fDeltaTimeInSeconds, Float fAlpha, Bool bBlendDiscreteState)
{
	Float32 const fMix = GetCurrentMixParameter();

	Bool bReturn = false;

	Float fDeltaTimeA = fDeltaTimeInSeconds;
	Float fDeltaTimeB = fDeltaTimeInSeconds;

	// If enabled, apply time synchronization between the child nodes.
	// This time warps each node to keep their overall max time in sync
	// Useful for blending between 2 looping animations (e.g. walk and run).
	if (m_pBlend->GetSynchronizeTime() && m_pChildA.IsValid() && m_pChildB.IsValid())
	{
		Float const fTimeA = m_pChildA->GetCurrentMaxTime();
		Float const fTimeB = m_pChildB->GetCurrentMaxTime();
		Float const fTargetTime = Lerp(fTimeA, fTimeB, fMix);

		if (fTargetTime > 0.0f)
		{
			fDeltaTimeA *= (fTimeA / fTargetTime);
			fDeltaTimeB *= (fTimeB / fTargetTime);
		}
	}

	if (m_pChildA.IsValid())
	{
		bReturn = m_pChildA->Tick(fDeltaTimeA, (1.0f - fMix) * fAlpha, bBlendDiscreteState) || bReturn;
	}

	if (m_pChildB.IsValid())
	{
		bReturn = m_pChildB->Tick(fDeltaTimeB, fMix * fAlpha, bBlendDiscreteState) || bReturn;
	}

	return bReturn;
}

} // namespace Seoul::Animation

#endif // /#if (SEOUL_WITH_ANIMATION_2D || SEOUL_WITH_ANIMATION_3D)
