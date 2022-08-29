/**
 * \file AchievementManager.cpp
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

#include "AchievementManager.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SettingsManager.h"

namespace Seoul
{

#if SEOUL_PLATFORM_WINDOWS || SEOUL_PLATFORM_ANDROID
namespace
{

String IgnoredGet(const AchievementManager::Achievement&) { return String(); }
void IgnoredSet(AchievementManager::Achievement&, String) {}

} // namespace anonymous
#endif

SEOUL_SPEC_TEMPLATE_TYPE(Vector<AchievementManager::Achievement, 48>)
SEOUL_BEGIN_TYPE(AchievementManager::Achievement)
	SEOUL_PROPERTY_N("ID", m_sID)
#if SEOUL_PLATFORM_WINDOWS
	SEOUL_PROPERTY_N("SteamID", m_sPlatformIDString)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_PAIR_N_Q("AndroidID", IgnoredGet, IgnoredSet)
		SEOUL_ATTRIBUTE(NotRequired)
#elif SEOUL_PLATFORM_ANDROID
	SEOUL_PROPERTY_PAIR_N_Q("SteamID", IgnoredGet, IgnoredSet)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("AndroidID", m_sPlatformIDString)
		SEOUL_ATTRIBUTE(NotRequired)
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_LINUX
	// Nothing for now
#else
#error "Define for this platform"
#endif
SEOUL_END_TYPE()


AchievementManager::AchievementManager()
{
	SEOUL_ASSERT(IsMainThread());

	// Load out configuration.
	auto const filePath(FilePath::CreateConfigFilePath("achievements.json"));
	if (!SettingsManager::Get()->DeserializeObject(filePath, &m_vAchievements))
	{
		SEOUL_WARN("%s: failed loading achievements configuration file.", filePath.CStr());
	}
}

AchievementManager::~AchievementManager()
{
}

/**
 * Ticks the AchievementManager.  Awards any achievements which have been
 * unlocked up since the last tick, possibly doing so on a worker thread,
 * depending on the current platform.
 */
void AchievementManager::Tick()
{
	SEOUL_ASSERT(IsMainThread());

	// Early out if no achievements to award.
	if (m_vAchievementsToBeAwarded.IsEmpty())
	{
		return;
	}

	// Processing.

#if SEOUL_LOGGING_ENABLED
	static const Double kfExpectedMaxDiff = 0.5;

	auto const iStart = SeoulTime::GetGameTimeInTicks(); // Debug timing.
#endif // /#if SEOUL_LOGGING_ENABLED

	// Swap in and process.
	AchievementQueue v;
	v.Swap(m_vAchievementsToBeAwarded);
	InternalAwardAchievements(v);

#if SEOUL_LOGGING_ENABLED
	auto const iEnd = SeoulTime::GetGameTimeInTicks(); // Debug timing.

	// If greater than 0.5 milliseconds, warn - implementation
	// should move the body of InternalAwardAchievements to a secondary
	// thread.
	auto const fDiff = SeoulTime::ConvertTicksToMilliseconds(iEnd - iStart);
	if (fDiff > kfExpectedMaxDiff)
	{
		SEOUL_WARN("AchievementManager::InternalAwardAchievements() took %.2f ms, greater than %.2f ms.",
			fDiff,
			kfExpectedMaxDiff);
	}
#endif // /#if SEOUL_LOGGING_ENABLED
}

static void GlobalUnlockAchievementOnMainThread(HString sAchievementID)
{
	if (nullptr != AchievementManager::Get())
	{
		AchievementManager::Get()->UnlockAchievementOnMainThread(sAchievementID);
	}
}

/**
 * Queues up the given achievement to be unlocked
 */
void AchievementManager::UnlockAchievement(HString sAchievementID)
{
	// Off main thread handling.
	if (!IsMainThread())
	{
		// unlocking achievements must run on main thread, so use a delegate
		Jobs::AsyncFunction(
			GetMainThreadId(),
			&GlobalUnlockAchievementOnMainThread,
			sAchievementID);
		return;
	}

	UnlockAchievementOnMainThread(sAchievementID);
}

void AchievementManager::UnlockAchievementOnMainThread(HString sAchievementID)
{
	SEOUL_ASSERT(IsMainThread());

	for (auto const& e : m_vAchievements)
	{
		if (e.m_sID == sAchievementID)
		{
			if (!m_vAchievementsToBeAwarded.Contains(sAchievementID))
			{
				m_vAchievementsToBeAwarded.PushBack(
					UnlockedAchievement(&e));
			}
			return;
		}
	}

	SEOUL_WARN("Failed to unlock achievement: %s, no entry in achievements.json\n", sAchievementID.CStr());
}

} // namespace Seoul
