/*
	ScriptEngineAnalyticsManager.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptEngineAnalyticsManager
	{
		public abstract void Flush();
		[Pure] public abstract bool GetAnalyticsSandboxed();
		public abstract SlimCS.Table GetStateProperties();
		public abstract void TrackEvent(string sName, SlimCS.Table tData = null);
		public abstract void UpdateProfile(object eOp, SlimCS.Table tData);
		public abstract int GetSessionCount();
	}
}
