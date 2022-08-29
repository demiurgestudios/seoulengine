// Access to the native SoundManager singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class SoundManager
{
	static readonly Native.ScriptEngineSoundManager udSoundManager = null;

	// Note: These values from Engine\SoundManager.h
	/// <summary>
	/// Standard category for the overall mix
	/// </summary>
	public static string ksSoundCategoryMaster = "bus:/";

	/// <summary>
	/// Standard category for the music sub mix
	/// </summary>
	public static string ksSoundCategoryMusic = "bus:/music";

	/// <summary>
	/// Standard category for the sound sub mix
	/// </summary>
	public static string ksSoundCategorySFX = "bus:/SFX";

	static SoundManager()
	{
		// Before further action, capture any methods that are not safe to be called
		// from off the main thread (static initialization occurs on a worker thread).
		CoreUtilities.MainThreadOnly(
			astable(typeof(SoundManager)),
			nameof(IsCategoryPlaying),
			nameof(SetCategoryMute),
			nameof(SetCategoryVolume));

		udSoundManager = CoreUtilities.NewNativeUserData<Native.ScriptEngineSoundManager>();
		if (null == udSoundManager)
		{
			error("Failed instantiating ScriptEngineSoundManager.");
		}
	}

	public static bool IsCategoryPlaying(string sCategoryName, bool bIncludeLoopingSounds)
	{
		var bRet = udSoundManager.IsCategoryPlaying(sCategoryName, bIncludeLoopingSounds);
		return bRet;
	}

	public static bool SetCategoryMute(string sCategoryName, bool bMute, bool bAllowPending, bool bSuppressLogging)
	{
		var bRet = udSoundManager.SetCategoryMute(sCategoryName, bMute, bAllowPending, bSuppressLogging);
		return bRet;
	}

	public static bool SetCategoryVolume(string sCategoryName, double fVolume, double fFadeTimeInSeconds, bool bAllowPending, bool bSuppressLogging)
	{
		var bRet = udSoundManager.SetCategoryVolume(sCategoryName, fVolume, fFadeTimeInSeconds, bAllowPending, bSuppressLogging);
		return bRet;
	}
}
