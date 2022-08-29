/**
 * \file ScriptSceneTicker.cpp
 * \brief Base class and list structure for tracking scene tickers,
 * which are objects coupled to ScriptScene objects and must
 * be polled once per frame.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "ScriptSceneState.h"
#include "ScriptSceneStateBinder.h"
#include "ScriptSceneTicker.h"

#if SEOUL_WITH_SCENE

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptSceneTicker, TypeFlags::kDisableNew)
	SEOUL_METHOD(InsertInScene)
	SEOUL_METHOD(RemoveFromList)
SEOUL_END_TYPE()

ScriptSceneTicker::~ScriptSceneTicker()
{
	RemoveFromList();
}

ScriptSceneTicker::ScriptSceneTicker()
	: m_pOwner()
	, m_pNext()
	, m_pPrev()
{
}

void ScriptSceneTicker::InsertInList(ScriptSceneTickerList& rList)
{
	// Remove this node from its current owning list, if defined.
	RemoveFromList();

	// If the list has a head instance, point its previous pointer
	// at this instance.
	if (rList.m_pHead)
	{
		rList.m_pHead->m_pPrev = this;
	}

	// Our next is the existing head.
	m_pNext = rList.m_pHead;

	// The head is now this instance.
	rList.m_pHead = this;

	// Cache the owner.
	m_pOwner = &rList;
}

void ScriptSceneTicker::InsertInScene(Script::FunctionInterface* pInterface)
{
	// Grab our scene object - error out if no specified.
	ScriptSceneStateBinder* pBinder = pInterface->GetUserData<ScriptSceneStateBinder>(1u);
	if (nullptr == pBinder)
	{
		pInterface->RaiseError(1u, "expected scene user data");
		return;
	}

	CheckedPtr<ScriptSceneState> pSceneState(pBinder->GetSceneStatePtr());

	// The scene may have been released, silently ignore.
	if (pSceneState.IsValid())
	{
		pSceneState->InsertTicker(*this);
	}
}

void ScriptSceneTicker::RemoveFromList()
{
	// If we have no owner, nop - must have an owner to be in a list.
	if (!m_pOwner)
	{
		// Sanity check that all our other variables are nullptr.
		SEOUL_ASSERT(!m_pNext.IsValid());
		SEOUL_ASSERT(!m_pPrev.IsValid());
		return;
	}

	// If we have a next pointer, update its previous pointer.
	if (m_pNext)
	{
		m_pNext->m_pPrev = m_pPrev;
	}

	// If we have a previous pointer, update its next pointer.
	if (m_pPrev)
	{
		m_pPrev->m_pNext = m_pNext;
	}

	// Update our owner's head pointer, if we are currently
	// the head.
	if (this == m_pOwner->m_pHead)
	{
		m_pOwner->m_pHead = m_pNext;
	}

	// Clear our list pointers.
	m_pPrev.Reset();
	m_pNext.Reset();
	m_pOwner.Reset();
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE
