/*
	ScriptEngineSoundManager.cs
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
	public abstract class ScriptEngineSoundManager
	{
		[Pure] public abstract bool IsCategoryPlaying(string a0, bool a1);
		public abstract bool SetCategoryMute(string a0, bool a1, bool a2, bool a3);
		public abstract bool SetCategoryVolume(string a0, double a1, double a2, bool a3, bool a4);
		[Pure] public abstract double GetCategoryVolume(string a0);
	}
}
