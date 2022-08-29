/*
	ScriptEngine.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) Demiurge Studios, Inc.
	
	This source code is licensed under the MIT license.
	Full license details can be found in the LICENSE file
	in the root directory of this source tree.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptEngine
	{
		[Pure] public abstract int GetGameTimeInTicks();
		[Pure] public abstract double GetGameTimeInSeconds();
		[Pure] public abstract double GetTimeInSecondsSinceFrameStart();
		[Pure] public abstract string GetPlatformUUID();
		[Pure] public abstract SlimCS.Table GetPlatformData();
		[Pure] public abstract double GetSecondsInTick();
		[Pure] public abstract double? GetThisProcessId();
		[Pure] public abstract bool HasNativeBackButtonHandling();
		public abstract bool NetworkPrefetch(FilePath a0);
		public abstract bool OpenURL(string a0);
		public abstract bool PostNativeQuitMessage();
		public abstract bool HasEnabledRemoteNotifications();
		public abstract bool CanRequestRemoteNotificationsWithoutPrompt();
		public abstract void SetCanRequestRemoteNotificationsWithoutPrompt(bool a0);
		[Pure] public abstract string URLEncode(string a0);
		[Pure] public abstract bool IsAutomationOrUnitTestRunning();
		public abstract void ScheduleLocalNotification(double iNotificationID, WorldTime fireDate, string sLocalizedMessage, string sLocalizedAction);
		public abstract void CancelLocalNotification(int a0);
		public abstract void CancelAllLocalNotifications();
		[Pure] public abstract WorldTime GetCurrentServerTime();
		public abstract void ShowAppStoreToRateThisApp();
		[Pure] public abstract bool DoesSupportInAppRateMe();
		[Pure] public abstract bool WriteToClipboard(string a0);
		[Pure] public abstract double GetGameSecondsSinceStartup();
	}
}
