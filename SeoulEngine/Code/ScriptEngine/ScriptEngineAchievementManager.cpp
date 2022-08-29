/**
* \file ScriptEngineAchievementManager.cpp
* \brief Binder, wraps the global AchievementManager Singleton into Script
* a script VM.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#include "AchievementManager.h"
#include "ReflectionDefine.h"
#include "ScriptEngineAchievementManager.h"
#include "ScriptFunctionInterface.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineAchievementManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(UnlockAchievement)
SEOUL_END_TYPE()

	ScriptEngineAchievementManager::ScriptEngineAchievementManager()
{
}

ScriptEngineAchievementManager::~ScriptEngineAchievementManager()
{
}

void ScriptEngineAchievementManager::UnlockAchievement(HString sAchievementID)
{
	AchievementManager::Get()->UnlockAchievement(sAchievementID);
}

} // namespace Seoul
