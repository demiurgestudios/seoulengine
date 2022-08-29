/**
 * \file ScriptEngineAnalyticsManager.h
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
#ifndef SCRIPT_ENGINE_ANALYTICS_MANAGER_H
#define SCRIPT_ENGINE_ANALYTICS_MANAGER_H

#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Binder, wraps the global AnalyticsManager Singleton into Script. */
class ScriptEngineAnalyticsManager SEOUL_SEALED
{
public:
	ScriptEngineAnalyticsManager();
	~ScriptEngineAnalyticsManager();

	void Flush();
	Bool GetAnalyticsSandboxed() const;
	void GetStateProperties(Script::FunctionInterface* pInterface);
	void TrackEvent(Script::FunctionInterface* pInterface);
	void UpdateProfile(Script::FunctionInterface* pInterface);
	Int64 GetSessionCount();

private:
	SEOUL_DISABLE_COPY(ScriptEngineAnalyticsManager);
}; // class ScriptEngineAnalyticsManager

} // namespace Seoul

#endif // include guard
