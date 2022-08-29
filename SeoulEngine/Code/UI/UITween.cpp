/**
 * \file UITween.cpp
 * \brief A UI::Tween is applied to a Falcon::Instance to perform
 * runtime, usually procedural animation of a property. UI::Tween
 * can also be used as a basic timing mechanis, to fire a callback
 * after a period of game time has passed without directly
 * affecting a Falcon::Instance property.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconMovieClipInstance.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "Renderer.h"
#include "ScopedAction.h"
#include "SeoulMath.h"
#include "UITween.h"

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(CheckedPtr<UI::Tween>)
SEOUL_BEGIN_TYPE(UI::Tween, TypeFlags::kDisableNew)
	SEOUL_PROPERTY_N("Identifier", m_iIdentifier)
	SEOUL_PROPERTY_N("StartValue", m_fStartValue)
	SEOUL_PROPERTY_N("EndValue", m_fEndValue)
	SEOUL_PROPERTY_N("DurationInSeconds", m_fDurationInSeconds)
	SEOUL_PROPERTY_N("Target", m_eTarget)
	SEOUL_PROPERTY_N("TweenType", m_eTweenType)
	SEOUL_PROPERTY_N("ElapsedInSeconds", m_fElapsedInSeconds)
SEOUL_END_TYPE()

namespace UI
{

Tween::Tween(Int iIdentifier)
	: m_pInstance()
	, m_pCompletionInterface()
	, m_ppHead(nullptr)
	, m_ppTail(nullptr)
	, m_pNext(nullptr)
	, m_pPrev(nullptr)
	, m_iIdentifier(iIdentifier)
	, m_fStartValue(0.0f)
	, m_fEndValue(0.0f)
	, m_fDurationInSeconds(0.0f)
	, m_eTarget(TweenTarget::kTimer)
	, m_eTweenType(TweenType::kLine)
	, m_fElapsedInSeconds(0.0f)
{
}

Tween::~Tween()
{
	// Sanity check
	SEOUL_ASSERT(!m_ppHead.IsValid());
	SEOUL_ASSERT(!m_ppTail.IsValid());
	SEOUL_ASSERT(!m_pNext.IsValid());
	SEOUL_ASSERT(!m_pPrev.IsValid());
}

/** Advance time and apply this UI::Tween's new value to its Falcon::Instance. */
Bool Tween::Advance(Float fDeltaTimeInSeconds)
{
	// Advance time.
	m_fElapsedInSeconds += fDeltaTimeInSeconds;

	// Compute alpha, clamped to [0, 1].
	Float const fT = Clamp(m_fElapsedInSeconds / m_fDurationInSeconds, 0.0f, 1.0f);

	// Initial value is the starting value.
	Float fValue = m_fStartValue;

	// Compute the tweened value based on the curve type.
	switch (m_eTweenType)
	{
	case TweenType::kInOutCubic:
		{
			Float fAlpha = (fT * 2.0f);
			if (fAlpha < 1.0f)
			{
				fValue = Lerp(m_fStartValue, m_fEndValue, 0.5f * (fAlpha * fAlpha * fAlpha));
			}
			else
			{
				fAlpha -= 2.0f;
				fValue = Lerp(m_fStartValue, m_fEndValue, 0.5f * (fAlpha * fAlpha * fAlpha + 2.0f));
			}
		}
		break;
	case TweenType::kInOutQuadratic:
		{
			Float fAlpha = (fT * 2.0f);
			if (fAlpha < 1.0f)
			{
				fValue = Lerp(m_fStartValue, m_fEndValue, 0.5f * (fAlpha * fAlpha));
			}
			else
			{
				fAlpha -= 1.0f;
				fValue = Lerp(m_fStartValue, m_fEndValue, -0.5f * (fAlpha * (fAlpha - 2.0f) - 1.0f));
			}
		}
		break;
	case TweenType::kInOutQuartic:
		{
			Float fAlpha = (fT * 2.0f);
			if (fAlpha < 1.0f)
			{
				fValue = Lerp(m_fStartValue, m_fEndValue, 0.5f * (fAlpha * fAlpha * fAlpha * fAlpha));
			}
			else
			{
				fAlpha -= 2.0f;
				fValue = Lerp(m_fStartValue, m_fEndValue, -0.5f * (fAlpha * fAlpha * fAlpha * fAlpha - 2.0f));
			}
		}
		break;
	case TweenType::kLine:
		fValue = Lerp(m_fStartValue, m_fEndValue, fT);
		break;
	case TweenType::kSinStartFast:
		fValue = ((m_fEndValue - m_fStartValue) * Sin(fT * (fPi / 2.0f))) + m_fStartValue;
		break;
	case TweenType::kSinStartSlow:
		fValue = ((m_fEndValue - m_fStartValue) * (Cos(fPi + (fT * (fPi / 2.0f))) + 1.0f) ) + m_fStartValue;
		break;
	default:
		SEOUL_FAIL("Unknown UITweenType, programmer error.");
		break;
	};

	// TODO: Giving up and adding a check for this since we're
	// seeing occasional null access SIGSEGV in shipped production builds,
	// although this should be impossible (all paths that create a tween ensure
	// a non-null instance pointer and all paths that reset the instance
	// pointer do so immediately before placing it in a free list).
	SEOUL_ASSERT(m_pInstance.IsValid());
	if (m_pInstance.IsValid())
	{
		// Now apply the value based on the target type.
		switch (m_eTarget)
		{
		case TweenTarget::kAlpha:
			m_pInstance->SetAlpha(fValue);
			break;
		case TweenTarget::kDepth3D:
			m_pInstance->SetDepth3D(fValue);
			break;
		case TweenTarget::kPositionX:
			m_pInstance->SetPositionX(fValue);
			break;
		case TweenTarget::kPositionY:
			m_pInstance->SetPositionY(fValue);
			break;
		case TweenTarget::kRotation:
			m_pInstance->SetRotationInDegrees(fValue);
			break;
		case TweenTarget::kScaleX:
			m_pInstance->SetScaleX(fValue);
			break;
		case TweenTarget::kScaleY:
			m_pInstance->SetScaleY(fValue);
			break;
		case TweenTarget::kTimer:
			// Nop - just used to fire an event after a certain time.
			break;
		default:
			SEOUL_FAIL("Unknown UITweenTarget, programmer error.");
			break;
		};
	}

	// Tween is complete if it has reached an alpha of 1.0f (final duration).
	return (fT >= 1.0f);
}

/** Add this UI::Tween to the doubly linked list defined by rpHead and rpTail. */
void Tween::InsertBack(CheckedPtr<Tween>& rpHead, CheckedPtr<Tween>& rpTail)
{
	// Remove this Tween from its current list, if any.
	Remove();

	// Track the head and tail pointers.
	m_ppHead = &rpHead;
	m_ppTail = &rpTail;

	// Previous is the current tail.
	m_pPrev = rpTail;

	// If current tail is defined, point it at this Tween.
	if (m_pPrev.IsValid())
	{
		SEOUL_ASSERT(!m_pPrev->m_pNext.IsValid());
		m_pPrev->m_pNext = this;
	}

	// Tail is now this tween.
	rpTail = this;

	// If the head is empty, point it at this tween as well.
	if (!rpHead.IsValid())
	{
		rpHead = this;
	}
}

/** Remove this UI::Tween from its current list, if it has one. */
void Tween::Remove()
{
	if (m_ppHead.IsValid())
	{
		// If we're the head, point the head at our next.
		if (this == *m_ppHead)
		{
			SEOUL_ASSERT(!m_pPrev.IsValid());
			*m_ppHead = m_pNext;
		}

		// If we're the tail, point the tail at our previous.
		if (this == *m_ppTail)
		{
			SEOUL_ASSERT(!m_pNext.IsValid());
			*m_ppTail = m_pPrev;
		}

		// Update the previous if non-null.
		if (m_pPrev.IsValid())
		{
			m_pPrev->m_pNext = m_pNext;
		}

		// Update the next if non-null.
		if (m_pNext.IsValid())
		{
			m_pNext->m_pPrev = m_pPrev;
		}

		// Reset all pointers.
		m_pNext.Reset();
		m_pPrev.Reset();
		m_ppTail.Reset();
		m_ppHead.Reset();
	}
	// Sanity checks
	else
	{
		SEOUL_ASSERT(!m_ppHead.IsValid());
		SEOUL_ASSERT(!m_ppTail.IsValid());
		SEOUL_ASSERT(!m_pPrev.IsValid());
		SEOUL_ASSERT(!m_pNext.IsValid());
	}
}

TweenCollection::TweenCollection()
	: m_PendingCancels()
	, m_CurrentCancels()
	, m_pTweenHead()
	, m_pTweenTail()
	, m_pFreeTweenHead()
	, m_pFreeTweenTail()
	, m_pAdvanceCurrent()
	, m_iTweenId(0)
{
}

TweenCollection::~TweenCollection()
{
	Destroy(m_pTweenHead, m_pTweenTail);
	Destroy(m_pFreeTweenHead, m_pFreeTweenTail);
}

/**
 * @return A new UI::Tween, inserted at the end of the tween evaluation list.
 */
CheckedPtr<Tween> TweenCollection::AcquireTween()
{
	CheckedPtr<Tween> pReturn;
	if (m_pFreeTweenHead.IsValid())
	{
		pReturn = m_pFreeTweenHead.Get();
		pReturn->Remove();

		pReturn->~Tween();
		new (pReturn.Get()) Tween(m_iTweenId);
	}
	else
	{
		pReturn = SEOUL_NEW(MemoryBudgets::UIRuntime) Tween(m_iTweenId);
	}

	++m_iTweenId;

	pReturn->InsertBack(m_pTweenHead, m_pTweenTail);
	return pReturn;
}

void TweenCollection::CancelAllTweens(const SharedPtr<Falcon::Instance>& pInstance)
{
	CheckedPtr<Tween> pTween(m_pTweenHead);
	while (pTween.IsValid())
	{
		if (pTween->GetInstance() == pInstance)
		{
			// Prior to deletion, check if we're about to erase the current
			// element of the Advance() loop.
			auto const bAdvanceCurrent = (pTween == m_pAdvanceCurrent);

			// Put the tween into the free list.
			pTween = Free(pTween);

			// Update current if applicable.
			if (bAdvanceCurrent) { m_pAdvanceCurrent = pTween; }
		}
		else
		{
			pTween = pTween->GetNext();
		}
	}
}

/** Advance time for all tweens, completing and removing those that have reached their duration. */
void TweenCollection::Advance(Falcon::AdvanceInterface& rInterface, Float fDeltaTimeInSeconds)
{
	auto const scope(MakeScopedAction(
		[&]()
	{
		++m_InAdvance;
	},
		[&]()
	{
		--m_InAdvance;

		// Reset current pointer always.
		m_pAdvanceCurrent.Reset();
	}));
	
	// Advance is not designed to be re-entrant, so check for that.
	SEOUL_ASSERT(1 == m_InAdvance);

	// Swap pending with current. We will check both during advance,
	// but only clear current when we're done. This enforces:
	// - clears are applied ASAP, so a tween will never
	//   advance if a Cancel() has been called for it.
	// - any clears that are pending on enter to Advance() are cleared,
	//   even those which are not applied (for example, because the
	//   corresponding tween has already completed).
	m_CurrentCancels.Swap(m_PendingCancels);

	m_pAdvanceCurrent = m_pTweenHead;
	while (m_pAdvanceCurrent.IsValid())
	{
		Int const iId = m_pAdvanceCurrent->GetIdentifier();

		// Check if the current tween has been cancelled.
		Bool const bCancelled =
			m_PendingCancels.HasKey(iId) ||
			m_CurrentCancels.HasKey(iId);

		// Don't advance if cancelled. Otherwise, if Advance() returns
		// true, then the tween has completed.
		if (bCancelled || m_pAdvanceCurrent->Advance(fDeltaTimeInSeconds))
		{
			// Invoke the completion event, if one is registered, unless
			// the tween was cancelled.
			if (!bCancelled)
			{
				SharedPtr<TweenCompletionInterface> pCompletionInterface(
					m_pAdvanceCurrent->GetCompletionInterface());

				// Erase *before* calling completion, in case completion
				// modifies m_lRunningMotion.
				m_pAdvanceCurrent = Free(m_pAdvanceCurrent);

				if (pCompletionInterface.IsValid())
				{
					pCompletionInterface->OnComplete();
				}
			}
			// Otherwise, remove the id from both pending and current. This
			// ensures that we don't try to double apply a cancel, if it
			// was added to the pending set by the completion event.
			else
			{
				// Erase from running.
				m_pAdvanceCurrent = Free(m_pAdvanceCurrent);

				// Erase from cancels.
				(void)m_PendingCancels.Erase(iId);
				(void)m_CurrentCancels.Erase(iId);
			}
		}
		else
		{
			// Get the next tween.
			m_pAdvanceCurrent = m_pAdvanceCurrent->GetNext();
		}
	}

	// Empty out current at the end - this flushes all clears,
	// even those which were not applied because (for example)
	// the tween had already completed.
	m_CurrentCancels.Clear();
}

/** Internal, cleanup and DELETE all UI::Tweens objects on collection destruction. */
void TweenCollection::Destroy(CheckedPtr<Tween>& rpHead, CheckedPtr<Tween>& rpTail)
{
	while (rpHead.IsValid())
	{
		CheckedPtr<Tween> p = rpHead;
		p->Remove();

		SafeDelete(p);
	}

	SEOUL_ASSERT(!rpHead.IsValid());
	SEOUL_ASSERT(!rpTail.IsValid());
}

/** Move an instance to the free list. */
CheckedPtr<Tween> TweenCollection::Free(CheckedPtr<Tween>& rp)
{
	// Early out if null.
	if (!rp.IsValid()) { return CheckedPtr<Tween>(); }

	// Put the tween into the free list.
	auto pTween(rp);
	rp.Reset();

	// Cached next before evaluating current.
	auto pNext(pTween->GetNext());

	// Insert into the free list.
	pTween->SetCompletionInterface(SharedPtr<TweenCompletionInterface>());
	pTween->ResetInstance();
	pTween->InsertBack(m_pFreeTweenHead, m_pFreeTweenTail);

	// Return next.
	return pNext;
}

} // namespace UI

} // namespace Seoul
