/**
 * \file ScriptEngineAnalyticsManager.cpp
 * \brief Binder instance for exposing a Camera instance into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "ReflectionDefine.h"
#include "ScriptEngineAnalyticsManager.h"
#include "ScriptFunctionInterface.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineAnalyticsManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(Flush)
	SEOUL_METHOD(GetAnalyticsSandboxed)
	SEOUL_METHOD(GetStateProperties)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "SlimCS.Table")
	SEOUL_METHOD(TrackEvent)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "string sName, SlimCS.Table tData = null")
	SEOUL_METHOD(UpdateProfile)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "object eOp, SlimCS.Table tData")
	SEOUL_METHOD(GetSessionCount)
SEOUL_END_TYPE()

ScriptEngineAnalyticsManager::ScriptEngineAnalyticsManager()
{
}

ScriptEngineAnalyticsManager::~ScriptEngineAnalyticsManager()
{
}

/** Ask the AnalyticsManager to immediately submit events to the backend. */
void ScriptEngineAnalyticsManager::Flush()
{
	AnalyticsManager::Get()->Flush();
}

/** @return Whether analytics thinks we're in the sandbox or not (are a cheater). */
Bool ScriptEngineAnalyticsManager::GetAnalyticsSandboxed() const
{
	return AnalyticsManager::Get()->GetAnalyticsSandboxed();
}

/** Grab state properties, which are internal properties tracked by analytics manager and added to every analytics event, automatically. */
void ScriptEngineAnalyticsManager::GetStateProperties(Script::FunctionInterface* pInterface)
{
	DataStore dataStore;
	dataStore.MakeTable();
	AnalyticsManager::Get()->AddStateProperties(dataStore, dataStore.GetRootNode());

	pInterface->PushReturnDataNode(dataStore, dataStore.GetRootNode());
}

/** Submit an event to the analytics system for tracking. */
void ScriptEngineAnalyticsManager::TrackEvent(Script::FunctionInterface* pInterface)
{
	String sName;
	if (!pInterface->GetString(1, sName))
	{
		pInterface->RaiseError(1, "expected string event name for tracking");
		return;
	}

	// Attributes are optional.
	AnalyticsEvent evt(sName);
	if (!pInterface->GetTable(2, evt.GetProperties()))
	{
		pInterface->RaiseError(2, "expected table of properties.");
		return;
	}

	// Submit the event.
	AnalyticsManager::Get()->TrackEvent(evt);
}

/** Submit an event to the analytics system for tracking. */
void ScriptEngineAnalyticsManager::UpdateProfile(Script::FunctionInterface* pInterface)
{
	AnalyticsProfileUpdateOp eOp = AnalyticsProfileUpdateOp::kUnknown;
	if (!pInterface->GetEnum(1, eOp))
	{
		pInterface->RaiseError(1, "expected profile update op");
		return;
	}

	AnalyticsProfileUpdate update(eOp);
	if (!pInterface->GetTable(2, update.GetUpdates()))
	{
		pInterface->RaiseError(2, "expected table of updates.");
		return;
	}

	// Submit the update.
	AnalyticsManager::Get()->UpdateProfile(update);
}

Int64 ScriptEngineAnalyticsManager::GetSessionCount()
{
	return AnalyticsManager::Get()->GetSessionCount();
}

} // namespace Seoul
