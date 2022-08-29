// Game-specific Engine singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class GameMain
{
	static readonly Native.Game_ScriptMain udGameMain = null;

	static GameMain()
	{
		udGameMain = CoreUtilities.NewNativeUserData<Native.Game_ScriptMain>();
		if (null == udGameMain)
		{
			error("Failed instantiating Game_ScriptMain.");
		}
	}

	public static double GetConfigUpdateCl()
	{
		return udGameMain.GetConfigUpdateCl();
	}

	public static string GetServerBaseURL()
	{
		var sRet = udGameMain.GetServerBaseURL();
		return sRet;
	}

	/// <summary>
	/// Submit an update of auth manager refresh data to the AuthManager.
	/// </summary>
	/// <returns>true if the update data will require a reboot, false otherwise.</returns>
	public static bool ManulUpdateRefreshData(Table tRefreshData)
	{
		return udGameMain.ManulUpdateRefreshData(tRefreshData);
	}

	public static void SetAutomationValue(string sKey, object oValue)
	{
		udGameMain.SetAutomationValue(sKey, oValue);
	}
}
