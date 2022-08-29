/*
	ScriptEngineTrackingManager.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptEngineTrackingManager
	{
		[Pure] public abstract string GetExternalTrackingUserID();
		public abstract void TrackEvent(string sName);
	}
}
