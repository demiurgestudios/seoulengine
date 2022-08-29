/**
 * \file SteamAchievementManager.h
 * \brief Global singleton manager for Steam achievements
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STEAM_ACHIEVEMENT_MANAGER_H
#define STEAM_ACHIEVEMENT_MANAGER_H

#include "AchievementManager.h"

#if SEOUL_WITH_STEAM

namespace Seoul
{

/**
 * Global singleton manager for Steam achievements
 */
class SteamAchievementManager SEOUL_SEALED : public AchievementManager
{
public:
	SteamAchievementManager();
	~SteamAchievementManager();

private:
	virtual void InternalAwardAchievements(const AchievementQueue& achievements) SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(SteamAchievementManager);
}; // class SteamAchievementManager

} // namespace Seoul

#endif // /#if SEOUL_WITH_STEAM

#endif // include guard
