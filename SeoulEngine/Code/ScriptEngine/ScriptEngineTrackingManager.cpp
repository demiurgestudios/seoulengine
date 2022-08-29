/**
 * \file ScriptEngineTrackingManager.cpp
 * \brief Binder instance for exposing a Camera instance into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "ScriptEngineTrackingManager.h"
#include "ScriptFunctionInterface.h"
#include "TrackingManager.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineTrackingManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(GetExternalTrackingUserID)
	SEOUL_METHOD(TrackEvent)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "string sName")
SEOUL_END_TYPE()

ScriptEngineTrackingManager::ScriptEngineTrackingManager()
{
}

ScriptEngineTrackingManager::~ScriptEngineTrackingManager()
{
}

/** Get the UUID unique to our external tracking middleware. */
String ScriptEngineTrackingManager::GetExternalTrackingUserID() const
{
	return TrackingManager::Get()->GetExternalTrackingUserID();
}

/** Submit an event to the tracking system for tracking. */
void ScriptEngineTrackingManager::TrackEvent(const String& sName)
{
	// Submit the event.
	TrackingManager::Get()->TrackEvent(sName);
}

} // namespace Seoul
