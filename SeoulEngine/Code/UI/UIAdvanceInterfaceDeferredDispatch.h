/**
 * \file UIAdvanceInterfaceDeferredDispatch.h
 * \brief Specialization of Falcon::AdvancedInterface with queueing behavior.
 *
 * UI::AdvanceInterfaceDeferredDispatch is used in several contexts, where
 * events fire while processing Falcon::Advance() need to be queued and
 * deferred later, in a different context.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UI_ADVANCE_INTERFACE_DEFERRED_DISPATCH_H
#define UI_ADVANCE_INTERFACE_DEFERRED_DISPATCH_H

#include "FalconAdvanceInterface.h"
#include "SharedPtr.h"
#include "Vector.h"

namespace Seoul::UI
{

namespace AdvanceInterfaceDeferredDispatchInternal
{

enum EntryType
{
	kDispatchNoBubble,
	kDispatchBubble,
	kOnAddToParent,
};

struct DeferredEntry SEOUL_SEALED
{
	static DeferredEntry CreateOnAddToParent(
		Falcon::MovieClipInstance* pParent,
		Falcon::Instance* pInstance,
		const HString& sClassName);

	static DeferredEntry CreateDispatchEvent(
		const HString& sEventName,
		Falcon::SimpleActions::EventType eType,
		Falcon::Instance* pInstance);

	DeferredEntry();
	~DeferredEntry();

	EntryType m_eType;
	SharedPtr<Falcon::MovieClipInstance> m_pParent;
	SharedPtr<Falcon::Instance> m_pInstance;
	HString m_sData;
}; // struct DeferredEntry

} // namespace UI::AdvanceInterfaceDeferredDispatchInternal

/**
 * Utility class, defers execution of FalconDispatchEvent() and FalconDispatchEnterFrameEvent
 * until an explicit call of DispatchEvents().
 */
class AdvanceInterfaceDeferredDispatch SEOUL_SEALED : public Falcon::AdvanceInterface
{
public:
	AdvanceInterfaceDeferredDispatch();
	~AdvanceInterfaceDeferredDispatch();

	// Return a new instance of UI::AdvanceInterfaceDeferredDispatch that
	// is an effective copy of this UIAdvanceInterfaceDeferredDispatch.
	AdvanceInterfaceDeferredDispatch* Clone() const;

	Bool DispatchEvents();
	Bool HasEventsToDispatch() const
	{
		return !m_vDeferredEntries.IsEmpty();
	}

	// Iterate over all reference Falcon::Instances* contained
	// in this DeferredDispatch and mark them as watched. Should
	// be followed by an equal number of calls to MarkNotWatched().
	void MarkWatched();
	void MarkNotWatched();

	// Falcon::AddInterface overrides
	virtual void FalconOnAddToParent(
		Falcon::MovieClipInstance* pParent,
		Falcon::Instance* pInstance,
		const HString& sClassName) SEOUL_OVERRIDE;

	virtual void FalconOnClone(
		Falcon::Instance const* pFromInstance,
		Falcon::Instance* pToInstance) SEOUL_OVERRIDE;
	// /Falcon::AddInterface overrides

	// Falcon::AdvanceInterface overrides
	virtual void FalconDispatchEnterFrameEvent(
		Falcon::Instance* pInstance) SEOUL_OVERRIDE;

	virtual void FalconDispatchEvent(
		const HString& sEventName,
		Falcon::SimpleActions::EventType eType,
		Falcon::Instance* pInstance) SEOUL_OVERRIDE;

	virtual float FalconGetDeltaTimeInSeconds() const SEOUL_OVERRIDE;

	virtual bool FalconLocalize(
		const HString& sLocalizationToken,
		String& rsLocalizedText) SEOUL_OVERRIDE;
	// /Falcon::AdvanceInterface overrides

	/** Update the interface that will actually be used to fulfill requests. */
	void SetInterface(Falcon::AdvanceInterface* pInterface)
	{
		m_pInterface = pInterface;
	}

private:
	typedef Vector<AdvanceInterfaceDeferredDispatchInternal::DeferredEntry, MemoryBudgets::UIRuntime> DeferredEntries;
	DeferredEntries m_vDeferredEntries;
	Falcon::AdvanceInterface* m_pInterface;

	SEOUL_DISABLE_COPY(AdvanceInterfaceDeferredDispatch);
}; // class UI::AdvanceInterfaceDeferredDispatch

} // namespace Seoul::UI

#endif // include guard
