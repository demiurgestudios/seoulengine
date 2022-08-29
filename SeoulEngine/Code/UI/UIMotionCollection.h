/**
 * \file UIMotionCollection.h
 * \brief A UI::MotionCollection is a Utility, provides functionality to manage UI::Motion instance
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_MOTION_COLLECTION_H
#define UI_MOTION_COLLECTION_H

#include "FalconInstance.h"
#include "HashSet.h"
#include "List.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "UIMotion.h"

namespace Seoul::UI
{

class MotionCollection SEOUL_SEALED
{
public:
	MotionCollection();
	~MotionCollection();

	// Inserts the UI::Motion at the end of the UI::Motion evaluation list, and returns its id
	Int AddMotion(const SharedPtr<Motion>& pMotion);

	// Update all UI::Motion instances owned and release/destroy any that have completed.
	void Advance(Float fDeltaTimeInSeconds);

	// Mark a UI::Motion as cancelled. This will be applied prior to the next Advance() of the UIMotion.
	void CancelMotion(Int iIdentifier)
	{
		(void)m_PendingCancels.Insert(iIdentifier);
	}

	// Remove all motions matching the given instance.
	void CancelAllMotions(const SharedPtr<Falcon::Instance>& pInstance);

private:
	typedef HashSet<Int, MemoryBudgets::UIRuntime> Cancels;
	Cancels m_PendingCancels;
	Cancels m_CurrentCancels;

	typedef List<SharedPtr<Motion>, MemoryBudgets::UIRuntime> MotionList;
	MotionList m_lRunningMotions;
	MotionList::Iterator m_iAdvanceCurrent;

	Int m_iMotionId;
	Atomic32 m_InAdvance;

	SEOUL_DISABLE_COPY(MotionCollection);
}; // class UI::MotionCollection

} // namespace Seoul::UI

#endif // include guard
