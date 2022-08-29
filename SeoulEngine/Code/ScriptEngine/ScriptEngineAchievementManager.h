/**
 * \file ScriptEngineAchievementManager.h
 * \brief Binder, wraps the global AchievementManager Singleton into Script
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_ACHIEVEMENT_MANAGER_H
#define SCRIPT_ENGINE_ACHIEVEMENT_MANAGER_H

#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul
{

/** Binder, wraps the global AchievementManager Singleton into Script. */
class ScriptEngineAchievementManager SEOUL_SEALED
{
public:
	ScriptEngineAchievementManager();
	~ScriptEngineAchievementManager();

	// Queues up the given achievement to be unlocked
	void UnlockAchievement(HString sAchievementID);

private:
	SEOUL_DISABLE_COPY(ScriptEngineAchievementManager);
}; // class ScriptEngineAchievementManager

} // namespace Seoul

#endif // include guard
