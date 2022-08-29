/**
 * \file ScriptEngineTrackingManager.h
 * \brief Binder instance for exposing a Camera instance into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_TRACKING_MANAGER_H
#define SCRIPT_ENGINE_TRACKING_MANAGER_H

#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Binder, wraps the global TrackingManager Singleton into Script. */
class ScriptEngineTrackingManager SEOUL_SEALED
{
public:
	ScriptEngineTrackingManager();
	~ScriptEngineTrackingManager();

	String GetExternalTrackingUserID() const;
	void TrackEvent(const String& sName);

private:
	SEOUL_DISABLE_COPY(ScriptEngineTrackingManager);
}; // class ScriptEngineTrackingManager

} // namespace Seoul

#endif // include guard
