/**
 * \file AndroidAchievementManager.h
 * \brief Specialization of AchievementManager for the Android platform
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_ACHIEVEMENT_MANAGER_H
#define ANDROID_ACHIEVEMENT_MANAGER_H

#include "AchievementManager.h"

namespace Seoul
{

class AndroidAchievementManager SEOUL_SEALED : public AchievementManager
{
public:
	AndroidAchievementManager();
	~AndroidAchievementManager();

	virtual void DisplayAchievementUI() SEOUL_OVERRIDE;

#if SEOUL_ENABLE_CHEATS
	// Resets all stats and achievements - use with caution
	virtual void ResetAchievements() SEOUL_OVERRIDE;
#endif // /SEOUL_ENABLE_CHEATS

private:
	virtual void InternalAwardAchievements(const AchievementQueue& achievements) SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(AndroidAchievementManager);
}; // class AndroidAchievementManager

} // namespace Seoul

#endif // include guard
