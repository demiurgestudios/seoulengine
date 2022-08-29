// Wrapper and access to JSON files from script.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class SettingsManager
{
	#region Private members
	static readonly Native.ScriptEngineSettingsManager s_udSettings = CoreUtilities.NewNativeUserData<Native.ScriptEngineSettingsManager>();
	static readonly Table<string, Table> s_tCache = dyncast<Table<string, Table>>(CoreUtilities.MakeWeakTable());
	#endregion

	public static bool Exists(string sPath)
	{
		var bRet = s_udSettings.Exists(sPath);
		return bRet;
	}

	public static Table GetSettings(string sPath)
	{
		// Get from cache.
		var t = s_tCache[sPath];
		if (null != t)
		{
			return t;
		}

		// Otherwise, try to get settings from the native user data.
		(t, _) = s_udSettings.GetSettings(sPath);

		// If successful, cache it in the s_tCache table.
		if (null != t)
		{
			s_tCache[sPath] = t;
		}

		// Either way, return the value of t.
		return t;
	}

	public static bool IsQAOrLocal()
	{
		var clientSettings = GetSettings("ClientSettings");
		var env = clientSettings["ServerType"];

		return env == null || env == "LOCAL" || env == "QA";
	}

	public static (string, Native.FilePath) GetSettingsAsJsonString(string sPath)
	{
		return s_udSettings.GetSettingsAsJsonString(sPath);
	}

	public static (Table, Native.FilePath) GetSettingsInDirectory(string sPath, bool bRecursive = false)
	{
		return s_udSettings.GetSettingsInDirectory(sPath, bRecursive);
	}

	public static void SetSettings(Native.FilePath fPath, Table data)
	{
		s_udSettings.SetSettings(fPath, data);
	}

	public static bool ValidateSettings(string sWildcard, bool bCheckDependencies)
	{
		return s_udSettings.ValidateSettings(sWildcard, bCheckDependencies);
	}
}
