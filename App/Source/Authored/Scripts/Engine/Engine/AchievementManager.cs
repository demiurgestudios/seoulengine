// Access to the native AchievementManager singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class AchievementManager
{
	static readonly Native.ScriptEngineAchievementManager udAchievementManager = null;

	static AchievementManager()
	{
		udAchievementManager = CoreUtilities.NewNativeUserData<Native.ScriptEngineAchievementManager>();
		if (null == udAchievementManager)
		{
			error("Failed instantiating ScriptEngineAchievementManager.");
		}
	}

	public static void UnlockAchievement(string sAchievementID)
	{
		udAchievementManager.UnlockAchievement(sAchievementID);
	}
}
