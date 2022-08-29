/*
	Game_AutomationScriptObject.cs
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
	public abstract class Game_AutomationScriptObject
	{
		[Pure] public abstract bool BroadcastEvent(string sEvent, params object[] aArgs);
		public abstract bool BroadcastEventTo(string sEvent, string sTarget, params object[] aArgs);
		public abstract void EnableServerTimeChecking(bool a0);
		[Pure] public abstract double GetUptimeInSeconds();
		[Pure] public abstract bool IsSaveLoadManagerFirstTimeTestingComplete();
		[Pure] public abstract void LogAllHStrings();
		[Pure] public abstract SlimCS.Table GetHStringStats();
		public abstract SlimCS.Table<double, SlimCS.Table> GetHitPoints(int a0);
		[Pure] public abstract string GetHitPointLongName(SlimCS.Table a0);
		public abstract SlimCS.TableV<string, int> GetRequestedMemoryUsageBuckets();
		public abstract SlimCS.TableV<string, int> GetScriptMemoryUsageBuckets();
		[Pure] public abstract int GetTotalMemoryUsageInBytes();
		[Pure] public abstract SlimCS.Table GetVmStats();
		[Pure] public abstract int GetCurrentClientWorldTimeInMilliseconds();
		[Pure] public abstract int GetCurrentServerWorldTimeInMilliseconds();
		[Pure] public abstract string GetCurrentISO8601DateTimeUTCString();
		public abstract bool GetUICondition(string a0);
		[Pure] public abstract SlimCS.TableV<string, bool> GetUIConditions();
		[Pure] public abstract SlimCS.Table<double, string> GetUIInputWhitelist();
		public abstract void GotoUIState(string a0, string a1);
		[Pure] public abstract void Log(string a0);
		[Pure] public abstract void LogGlobalUIScriptNodes(bool a0);
		[Pure] public abstract void LogInstanceCountsPerMovie();
		public abstract void LogMemoryDetails(object a0);
		[Pure] public abstract void ManuallyInjectBindingEvent(string a0);
		[Pure] public abstract void QueueLeftMouseButtonEvent(bool a0);
		[Pure] public abstract void QueueMouseMoveEvent(int a0, int a1);
		public abstract void RunIntegrationTests(string a0);
		[Pure] public abstract void SendUITrigger(string a0);
		[Pure] public abstract void SetEnablePerfTesting(bool a0);
		[Pure] public abstract void SetUICondition(string a0, bool a1);
		[Pure] public abstract void Warn(string a0);
	}
}
