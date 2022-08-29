/**
 * \file AndroidAchievementManager.cpp
 * \brief Specialization of AchievementManager for the Android platform
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidAchievementManager.h"
#include "AndroidEngine.h"
#include "AndroidPrereqs.h"
#include "Logger.h"

namespace Seoul
{

AndroidAchievementManager::AndroidAchievementManager()
{
}

AndroidAchievementManager::~AndroidAchievementManager()
{
}

void AndroidAchievementManager::DisplayAchievementUI()
{
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"DisplayAchievementUI",
		"()V");
}

void AndroidAchievementManager::InternalAwardAchievements(const AchievementQueue& achievements)
{
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	for (UInt i = 0; i < achievements.GetSize(); i++)
	{
		HString sAchievementId = achievements[i].m_pAchievement->m_sPlatformIDString;
		if (sAchievementId.IsEmpty())
		{
			SEOUL_WARN("AndroidAchievementManager::InternalAwardAchievements: No platform id string defined for %s\n", 
				achievements[i].m_pAchievement->m_sID.CStr());
		}
		else
		{
			Java::Invoke<void>(
				pEnvironment,
				pActivity->clazz,
				"UnlockAchievement",
				"(Ljava/lang/String;)V",
				sAchievementId);
		}
	}
}

#if SEOUL_ENABLE_CHEATS
/**
 * Resets all stats and achievements - use with caution
 */
void AndroidAchievementManager::ResetAchievements()
{
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"ResetAchievements",
		"()V");
}

#endif // /SEOUL_ENABLE_CHEATS

} // namespace Seoul
