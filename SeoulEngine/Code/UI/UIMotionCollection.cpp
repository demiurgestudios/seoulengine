/**
* \file UIMotionCollection.cpp
* \brief A UI::MotionCollection is a Utility, provides functionality to manage UI::Motion instance
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "UIMotionCollection.h"
#include "ReflectionDefine.h"
#include "FalconMovieClipInstance.h"
#include "ScopedAction.h"
#include "UIMotion.h"

namespace Seoul::UI
{

MotionCollection::MotionCollection()
	: m_PendingCancels()
	, m_CurrentCancels()
	, m_lRunningMotions()
	, m_iAdvanceCurrent(m_lRunningMotions.End())
	, m_iMotionId(0)
{
}

MotionCollection::~MotionCollection()
{
	while (!m_lRunningMotions.IsEmpty())
	{
		m_lRunningMotions.PopFront();
	}
}


Int MotionCollection::AddMotion(const SharedPtr<Motion>& pMotion)
{
	pMotion->SetIdentifier(++m_iMotionId);
	m_lRunningMotions.PushBack(pMotion);
	return m_iMotionId;
}

void MotionCollection::CancelAllMotions(const SharedPtr<Falcon::Instance>& pInstance)
{
	auto const iBegin = m_lRunningMotions.Begin();
	auto const iEnd = m_lRunningMotions.End();
	for (auto t = iBegin; iEnd != t; )
	{
		if ((*t)->GetInstance() == pInstance)
		{
			// After this erase is called, don't access pMotion.
			auto const bAdvanceCurrent = (t == m_iAdvanceCurrent);
			t = m_lRunningMotions.Erase(t);
			if (bAdvanceCurrent) { m_iAdvanceCurrent = t; }
		}
		else
		{
			++t;
		}
	}
}

/** Advance time for all UI::Motions, completing and removing those that have reached their duration. */
void MotionCollection::Advance(Float fDeltaTimeInSeconds)
{
	auto const scope(MakeScopedAction(
	[&]()
	{
		++m_InAdvance;
	},
	[&]()
	{
		--m_InAdvance;

		// Reset current iterator always.
		m_iAdvanceCurrent = m_lRunningMotions.End();
	}));

	// Advance is not designed to be re-entrant, so check for that.
	SEOUL_ASSERT(1 == m_InAdvance);

	// Swap pending with current. We will check both during advance,
	// but only clear current when we're done. This enforces:
	// - clears are applied ASAP, so a UI::Motion will never
	//   advance if a Cancel() has been called for it.
	// - any clears that are pending on enter to Advance() are cleared,
	//   even those which are not applied (for example, because the
	//   corresponding UI::Motion has already completed).
	m_CurrentCancels.Swap(m_PendingCancels);

	auto const iBegin = m_lRunningMotions.Begin();
	auto const iEnd = m_lRunningMotions.End();
	for (m_iAdvanceCurrent = iBegin; iEnd != m_iAdvanceCurrent; )
	{
		// Cache the id for further processing.
		Int const iId = (*m_iAdvanceCurrent)->GetIdentifier();

		// Check if the current UI::Motion has been cancelled.
		Bool const bCancelled =
			m_PendingCancels.HasKey(iId) ||
			m_CurrentCancels.HasKey(iId);

		// Don't advance if canceled. Otherwise, if Advance() returns
		// true, then the UI::Motion has completed.
		if (bCancelled || (*m_iAdvanceCurrent)->Advance(fDeltaTimeInSeconds))
		{
			// Invoke the completion event, if one is registered, unless
			// the UI::Motion was canceled.
			if (!bCancelled)
			{
				// Cache the completion interface.
				SharedPtr<MotionCompletionInterface> pCompletionInterface(
					(*m_iAdvanceCurrent)->GetCompletionInterface());

				// Erase *before* calling completion, in case completion
				// modifies m_lRunningMotion.
				m_iAdvanceCurrent = m_lRunningMotions.Erase(m_iAdvanceCurrent);

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
				m_iAdvanceCurrent = m_lRunningMotions.Erase(m_iAdvanceCurrent);

				// Erase from cancels.
				(void)m_PendingCancels.Erase(iId);
				(void)m_CurrentCancels.Erase(iId);
			}
		}
		else
		{
			// Advance.
			++m_iAdvanceCurrent;
		}
	}

	// Empty out current at the end - this flushes all clears,
	// even those which were not applied because (for example)
	// the UI::Motion had already completed.
	m_CurrentCancels.Clear();
}

} // namespace Seoul::UI
