// Access to the native Engine singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics.Contracts;

using static SlimCS;

public static class Engine
{
	static readonly Native.ScriptEngine udEngine = null;

	static Engine()
	{
		udEngine = CoreUtilities.NewNativeUserData<Native.ScriptEngine>();
		if (null == udEngine)
		{
			error("Failed instantiating ScriptEngine.");
		}
	}

	[Pure]
	public static double GetGameTimeInTicks()
	{
		var iRet = udEngine.GetGameTimeInTicks();
		return iRet;
	}

	[Pure]
	public static double GetGameTimeInSeconds()
	{
		var iRet = udEngine.GetGameTimeInSeconds();
		return iRet;
	}

	[Pure]
	public static double GetTimeInSecondsSinceFrameStart()
	{
		var iRet = udEngine.GetTimeInSecondsSinceFrameStart();
		return iRet;
	}


	public static string GetPlatformUUID()
	{
		var sRet = udEngine.GetPlatformUUID();
		return sRet;
	}

	public static Table GetPlatformData()
	{
		var sRet = udEngine.GetPlatformData();
		return sRet;
	}

	public static double GetSecondsInTick()
	{
		var fRet = udEngine.GetSecondsInTick();
		return fRet;
	}

	public static bool IsAutomationOrUnitTestRunning()
	{
		var bRet = udEngine.IsAutomationOrUnitTestRunning();
		return bRet;
	}

	public static double? GetThisProcessId()
	{
		var iRet = udEngine.GetThisProcessId();
		return iRet;
	}

	public static bool HasNativeBackButtonHandling()
	{
		var bRet = udEngine.HasNativeBackButtonHandling();
		return bRet;
	}

	public static bool OpenURL(string sURL)
	{
		var bRet = udEngine.OpenURL(sURL);
		return bRet;
	}

	public static bool NetworkPrefetch(Native.FilePath filePath)
	{
		var bRet = udEngine.NetworkPrefetch(filePath);
		return bRet;
	}

	public static bool PostNativeQuitMessage()
	{
		var bRet = udEngine.PostNativeQuitMessage();
		return bRet;
	}

	public static void ScheduleLocalNotification(int iNotificationID, Native.WorldTime fireDate, string sLocalizedMessage, string sLocalizedAction)
	{
		udEngine.ScheduleLocalNotification(iNotificationID, fireDate, sLocalizedMessage, sLocalizedAction);
	}

	public static void CancelLocalNotification(int iNotificationID)
	{
		udEngine.CancelLocalNotification(iNotificationID);
	}

	public static void CancelAllLocalNotifications()
	{
		udEngine.CancelAllLocalNotifications();
	}

	public static bool HasEnabledRemoteNotifications()
	{
		return udEngine.HasEnabledRemoteNotifications();
	}

	public static bool CanRequestRemoteNotificationsWithoutPrompt()
	{
		return udEngine.CanRequestRemoteNotificationsWithoutPrompt();
	}

	public static void SetCanRequestRemoteNotificationsWithoutPrompt(bool canRequest)
	{
		udEngine.SetCanRequestRemoteNotificationsWithoutPrompt(canRequest);
	}

	public static Native.WorldTime GetCurrentServerTime()
	{
		return udEngine.GetCurrentServerTime();
	}

	public static void ShowAppStoreToRateThisApp()
	{
		udEngine.ShowAppStoreToRateThisApp();
	}

	public static bool DoesSupportInAppRateMe()
	{
		return udEngine.DoesSupportInAppRateMe();
	}

	public static string URLEncode(string sURL)
	{
		var sRet = udEngine.URLEncode(sURL);
		return sRet;
	}

	public static bool WriteToClipboard(string toWrite)
	{
		var bRet = udEngine.WriteToClipboard(toWrite);
		return bRet;
	}

	public static double GetGameSecondsSinceStartup()
	{
		return udEngine.GetGameSecondsSinceStartup();
	}
}
