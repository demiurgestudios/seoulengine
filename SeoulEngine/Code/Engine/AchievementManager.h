/**
 * \file AchievementManager.h
 * \brief Global singleton manager for game achievements. AchievementManager
 * handles Tick()ing AchievementSets each frame, and handles submitting
 * awarded achievements to platform specific functions. AchievementManager
 * also provides methods for displaying platform specific Achievement UI.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ACHIEVEMENT_MANAGER_H
#define ACHIEVEMENT_MANAGER_H

#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "Singleton.h"
#include "Vector.h"

namespace Seoul
{

/**
 * Global singleton manager for game achievements. AchievementManager handles
 * submitting awarded achievements to platform-specific
 * functions. AchievementManager also provides methods for displaying
 * platform-specific Achievement UI.
 */
class AchievementManager SEOUL_ABSTRACT : public Singleton<AchievementManager>
{
public:
	AchievementManager();
	virtual ~AchievementManager();

	/**
	 * Display the platform dependent achievement UI.
	 *
	 * There is no way to detect when the achievement screen
	 * has closed - do not trigger the achievement UI unless the game
	 * is paused and unpausing the game requires a button press
	 * (the achievement UI will capture all button presses).
	 */
	virtual void DisplayAchievementUI() { }

	// Queues up the given achievement to be unlocked
	void UnlockAchievement(HString sAchievementID);

	// Queues up the given achievement to be unlocked (run on main thread)
	void UnlockAchievementOnMainThread(HString sAchievementID);

	/** Ticks the achievement manager */
	virtual void Tick();

#if SEOUL_ENABLE_CHEATS
	/** Resets all stats and achievements - use with caution */
	virtual void ResetAchievements() {}
#endif // /SEOUL_ENABLE_CHEATS

	/**
	 * Structure representing one achievement
	 */
	struct Achievement SEOUL_SEALED
	{
		Achievement()
			: m_uPlatformIDUInt(UIntMax)
		{
		}

		/** Platform-agnostic achievement ID */
		HString m_sID;

		/** Platform-specific achievement ID for platforms using integer IDs */
		UInt32 m_uPlatformIDUInt;

		/** Platform-specific achievement ID for platforms using string IDs */
		HString m_sPlatformIDString;
	};

	/**
	 * Structure representing an achievement to be awarded to a user
	 */
	struct UnlockedAchievement SEOUL_SEALED
	{
		UnlockedAchievement()
			: m_pAchievement(nullptr)
		{
		}

		UnlockedAchievement(const Achievement *pAchievement)
			: m_pAchievement(pAchievement)
		{
		}

		/** Achievement which is to be awarded */
		const Achievement *m_pAchievement;

		Bool operator==(HString sID) const
		{
			return (nullptr != m_pAchievement && m_pAchievement->m_sID == sID);
		}

		Bool operator!=(HString sID) const
		{
			return !(*this == sID);
		}
	};

	typedef Vector<UnlockedAchievement> AchievementQueue;
	typedef Vector<Achievement> Achievements;

protected:
	/**
	 * Override to implement platform-specific achievement/trophy unlocking
	 */
	virtual void InternalAwardAchievements(const AchievementQueue& achievements) = 0;

private:
	SEOUL_DISABLE_COPY(AchievementManager);
	SEOUL_REFLECTION_FRIENDSHIP(AchievementManager);
	friend struct Reflection::TypeOfDetail<Achievement>;

	/**
	 * List of all achievements.  We just use a flat list instead of a hash
	 * table since the total number of achievements is expected to be small,
	 * so searching it by achievement ID is not expensive.
	 */
	Achievements m_vAchievements;

	/**
	 * Internal queue of achievements to be awarded
	 */
	AchievementQueue m_vAchievementsToBeAwarded;
}; // class AchievementManager

/**
 * Default AchievementManager implementation.  Does nothing.
 */
class NullAchievementManager SEOUL_SEALED : public AchievementManager
{
private:
	/**
	 * No achievements awarded
	 */
	virtual void InternalAwardAchievements(const AchievementQueue& achievements) SEOUL_OVERRIDE
	{
	}
}; // class NullAchievementManager

} // namespace Seoul

#endif // include guard
