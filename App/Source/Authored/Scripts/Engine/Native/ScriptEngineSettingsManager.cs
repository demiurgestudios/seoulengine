/*
	ScriptEngineSettingsManager.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptEngineSettingsManager
	{
		[Pure] public abstract bool Exists(object filePathOrFileNameString);
		[Pure] public abstract (SlimCS.Table, FilePath) GetSettings(object filePathOrFileNameString);
		[Pure] public abstract void SetSettings(FilePath path, SlimCS.Table data);
		[Pure] public abstract (string, FilePath) GetSettingsAsJsonString(object filePathOrFileNameString);
		[Pure] public abstract (SlimCS.Table, FilePath) GetSettingsInDirectory(object filePathOrFileNameString, bool bRecursive = false);
		[Pure] public abstract bool ValidateSettings(string sExcludeWildcard, bool bCheckDependencies);
	}
}
