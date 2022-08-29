/**
 * \file ScriptSceneTicker.h
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

#pragma once
#ifndef SCRIPT_SCENE_TICKER_H
#define SCRIPT_SCENE_TICKER_H

#include "CheckedPtr.h"
#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class ScriptSceneTickerList; }
namespace Seoul { namespace Scene { class Interface; } }

#if SEOUL_WITH_SCENE

namespace Seoul
{

/** Base class of tickers that can be tracked by a ScriptScene. */
class ScriptSceneTicker SEOUL_ABSTRACT
{
public:
	virtual ~ScriptSceneTicker();

	virtual void Tick(Scene::Interface& rInterface, Float fDeltaTimeInSeconds) = 0;

	/** @return The next entry in this ScriptSceneTicker's list. */
	CheckedPtr<ScriptSceneTicker> GetNext() const
	{
		return m_pNext;
	}

	/** @return The current list owner of this node or not valid if no current owner. */
	CheckedPtr<ScriptSceneTickerList> GetOwner() const
	{
		return m_pOwner;
	}

	/** @return The previous entry in this Nodes list. */
	CheckedPtr<ScriptSceneTicker> GetPrev() const
	{
		return m_pPrev;
	}

	void InsertInList(ScriptSceneTickerList& rList);
	void InsertInScene(Script::FunctionInterface* pInterface);
	void RemoveFromList();

protected:
	ScriptSceneTicker();

private:
	CheckedPtr<ScriptSceneTickerList> m_pOwner;
	CheckedPtr<ScriptSceneTicker> m_pNext;
	CheckedPtr<ScriptSceneTicker> m_pPrev;

	SEOUL_DISABLE_COPY(ScriptSceneTicker);
}; // class ScriptSceneTicker

/** Simplified List<> like utility structure to allow clients to track ScriptSceneTicker instances. */
class ScriptSceneTickerList SEOUL_SEALED
{
public:
	ScriptSceneTickerList()
		: m_pHead()
	{
	}

	/** @return True if no entries are contained in this list, false otherwise. */
	Bool IsEmpty() const
	{
		return (!m_pHead);
	}

	/** @return The head entry of this ScriptSceneTickerList. */
	CheckedPtr<ScriptSceneTicker> GetHead() const
	{
		return m_pHead;
	}

private:
	SEOUL_DISABLE_COPY(ScriptSceneTickerList);

	friend class ScriptSceneTicker;
	CheckedPtr<ScriptSceneTicker> m_pHead;
}; // class ScriptSceneTickerList

} // namespace Seoul

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
