/*
	ScriptSceneStateBinder.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class ScriptSceneStateBinder
	{
		[Pure] public abstract void AsyncAddPrefab(SlimCS.Table a0);
		[Pure] public abstract void GetCamera(SlimCS.Table a0);
		[Pure] public abstract void GetObjectById(SlimCS.Table a0);
		[Pure] public abstract void GetObjectIdFromHandle(SlimCS.Table a0);
		public abstract void SetScriptInterface(SlimCS.Table a0);
	}
}
