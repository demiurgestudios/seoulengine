/*
	Game_ScriptManagerProxy.cs
	AUTO GENERATED - DO NOT MODIFY
	API FOR NATIVE CLASS INSTANCE

	Run GenerateScriptBindings.bat in the Utilities folder to re-generate bindings.

	Copyright (c) 2018-2022 Demiurge Studios Inc.  All rights reserved.
*/

using System.Diagnostics.Contracts;

namespace Native
{
	public abstract class Game_ScriptManagerProxy
	{
		[Pure] public abstract void ReceiveState(SlimCS.Table tState, SlimCS.Table tMetatableState);
		[Pure] public abstract (SlimCS.Table, SlimCS.Table) RestoreState();
	}
}
