/**
 * \file SteamAchievementManager.cpp
 * \brief Global singleton manager for Steam achievements
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "SteamAchievementManager.h"
#include "SteamPrivateApi.h"

#if SEOUL_WITH_STEAM

namespace Seoul
{

SteamAchievementManager::SteamAchievementManager()
{
}

SteamAchievementManager::~SteamAchievementManager()
{
}

/** Unlock an achievement */
void SteamAchievementManager::InternalAwardAchievements(const AchievementQueue& achievements)
{
	if (!SteamUserStats())
	{
		SEOUL_LOG_ENGINE("Unable to award %d achievement(s), Steam client is not active!", achievements.GetSize());
		return;
	}

	for (auto const& achievement : achievements)
	{
		// Get & verify the achievement ID
		auto const achievementId(achievement.m_pAchievement->m_sPlatformIDString);
		if (achievementId.IsEmpty())
		{
			continue;
		}

		// Award it
		SEOUL_LOG_ENGINE("[Steam]: Awarding achievement: %s\n", achievementId.CStr());
		SteamUserStats()->SetAchievement(achievementId.CStr());
	}
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_STEAM
