/**
 * \file UIAdvanceInterfaceDeferredDispatch.cpp
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

#include "Engine.h"
#include "FalconMovieClipInstance.h"
#include "LocManager.h"
#include "UIAdvanceInterfaceDeferredDispatch.h"
#include "UIData.h"

namespace Seoul::UI
{

namespace AdvanceInterfaceDeferredDispatchInternal
{

DeferredEntry DeferredEntry::CreateOnAddToParent(
	Falcon::MovieClipInstance* pParent,
	Falcon::Instance* pInstance,
	const HString& sClassName)
{
	DeferredEntry ret;
	ret.m_eType = kOnAddToParent;
	ret.m_pParent.Reset(pParent);
	ret.m_pInstance.Reset(pInstance);
	ret.m_sData = sClassName;
	return ret;
}

DeferredEntry DeferredEntry::CreateDispatchEvent(
	const HString& sEventName,
	Falcon::SimpleActions::EventType eType,
	Falcon::Instance* pInstance)
{
	DeferredEntry ret;
	ret.m_eType = (Falcon::SimpleActions::EventType::kEventDispatchBubble == eType ? kDispatchBubble : kDispatchNoBubble);
	ret.m_pParent.Reset();
	ret.m_pInstance.Reset(pInstance);
	ret.m_sData = sEventName;
	return ret;
}

DeferredEntry::DeferredEntry()
	: m_eType(kDispatchNoBubble)
	, m_pParent()
	, m_pInstance()
	, m_sData()
{
}

DeferredEntry::~DeferredEntry()
{
}

} // namespace UI::AdvanceInterfaceDeferredDispatchInternal

using namespace AdvanceInterfaceDeferredDispatchInternal;

AdvanceInterfaceDeferredDispatch::AdvanceInterfaceDeferredDispatch()
	: m_vDeferredEntries()
	, m_pInterface(nullptr)
{
}

AdvanceInterfaceDeferredDispatch::~AdvanceInterfaceDeferredDispatch()
{
}

/**
 * @return A new instance of UI::AdvanceInterfaceDeferredDispatch that is
 * an effective copy of this UIAdvanceInterfaceDeferredDispatch.
 *
 * The clone contains a copy of Dispatch Entries but the m_pInterface
 * member is always nullptr.
 */
AdvanceInterfaceDeferredDispatch* AdvanceInterfaceDeferredDispatch::Clone() const
{
	AdvanceInterfaceDeferredDispatch* pReturn = SEOUL_NEW(MemoryBudgets::UIRuntime) AdvanceInterfaceDeferredDispatch;
	pReturn->m_vDeferredEntries = m_vDeferredEntries;
	return pReturn;
}

Bool AdvanceInterfaceDeferredDispatch::DispatchEvents()
{
	if (!m_pInterface)
	{
		return false;
	}

	UInt32 const zSize = m_vDeferredEntries.GetSize();
	for (UInt32 i = 0; i < zSize; ++i)
	{
		const DeferredEntry& entry = m_vDeferredEntries[i];
		switch (entry.m_eType)
		{
		case kDispatchNoBubble:
			if (entry.m_sData.IsEmpty())
			{
				m_pInterface->FalconDispatchEnterFrameEvent(entry.m_pInstance.GetPtr());
			}
			else
			{
				m_pInterface->FalconDispatchEvent(entry.m_sData, Falcon::SimpleActions::EventType::kEventDispatch, entry.m_pInstance.GetPtr());
			}
			break;
		case kDispatchBubble:
			if (entry.m_sData.IsEmpty())
			{
				m_pInterface->FalconDispatchEnterFrameEvent(entry.m_pInstance.GetPtr());
			}
			else
			{
				m_pInterface->FalconDispatchEvent(entry.m_sData, Falcon::SimpleActions::EventType::kEventDispatchBubble, entry.m_pInstance.GetPtr());
			}
			break;
		case kOnAddToParent:
			m_pInterface->FalconOnAddToParent(entry.m_pParent.GetPtr(), entry.m_pInstance.GetPtr(), entry.m_sData);
			break;
		default:
			SEOUL_FAIL("Out-of-sync enum.");
			break;
		};
	}
	m_vDeferredEntries.Clear();

	return true;
}

void AdvanceInterfaceDeferredDispatch::MarkWatched()
{
	for (auto const& e : m_vDeferredEntries)
	{
		if (e.m_pInstance.IsValid())
		{
			e.m_pInstance->AddWatcher();
		}
		if (e.m_pParent.IsValid())
		{
			e.m_pParent->AddWatcher();
		}
	}
}

void AdvanceInterfaceDeferredDispatch::MarkNotWatched()
{
	for (auto const& e : m_vDeferredEntries)
	{
		if (e.m_pParent.IsValid())
		{
			e.m_pParent->RemoveWatcher();
		}
		if (e.m_pInstance.IsValid())
		{
			e.m_pInstance->RemoveWatcher();
		}
	}
}

void AdvanceInterfaceDeferredDispatch::FalconOnAddToParent(
	Falcon::MovieClipInstance* pParent,
	Falcon::Instance* pInstance,
	const HString& sClassName)
{
	m_vDeferredEntries.PushBack(DeferredEntry::CreateOnAddToParent(pParent, pInstance, sClassName));
}

void AdvanceInterfaceDeferredDispatch::FalconOnClone(
	Falcon::Instance const* pFromInstance,
	Falcon::Instance* pToInstance)
{
	// TODO: This list should be very short in most cases,
	// and OnClone() will only be called for entries in the list.
	for (auto& rEntry : m_vDeferredEntries)
	{
		if (rEntry.m_pInstance.GetPtr() == pFromInstance)
		{
			rEntry.m_pInstance.Reset(pToInstance);
		}

		if ((Falcon::Instance*)rEntry.m_pParent.GetPtr() == pFromInstance)
		{
			// Sanity check.
			SEOUL_ASSERT(pToInstance->GetType() == Falcon::InstanceType::kMovieClip);
			rEntry.m_pParent.Reset((Falcon::MovieClipInstance*)pToInstance);
		}
	}
}

void AdvanceInterfaceDeferredDispatch::FalconDispatchEnterFrameEvent(
	Falcon::Instance* pInstance)
{
	m_vDeferredEntries.PushBack(DeferredEntry::CreateDispatchEvent(HString(), Falcon::SimpleActions::EventType::kEventDispatch, pInstance));
}

void AdvanceInterfaceDeferredDispatch::FalconDispatchEvent(
	const HString& sEventName,
	Falcon::SimpleActions::EventType eType,
	Falcon::Instance* pInstance)
{
	m_vDeferredEntries.PushBack(DeferredEntry::CreateDispatchEvent(sEventName, eType, pInstance));
}

Float AdvanceInterfaceDeferredDispatch::FalconGetDeltaTimeInSeconds() const
{
	return Engine::Get()->GetSecondsInTick();
}

Bool AdvanceInterfaceDeferredDispatch::FalconLocalize(
	const HString& sLocalizationToken,
	String& rsLocalizedText)
{
	String const sText(LocManager::Get()->Localize(sLocalizationToken));
	rsLocalizedText = String(sText.CStr(), sText.GetSize());
	return true;
}

} // namespace Seoul::UI
