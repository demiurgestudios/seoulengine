/*
	Game_ScriptMain.cs
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
	public abstract class Game_ScriptMain
	{
		[Pure] public abstract int GetConfigUpdateCl();
		[Pure] public abstract string GetServerBaseURL();
		public abstract bool ManulUpdateRefreshData(SlimCS.Table a0);
		public abstract void SetAutomationValue(string sKey, object oValue);
	}
}
