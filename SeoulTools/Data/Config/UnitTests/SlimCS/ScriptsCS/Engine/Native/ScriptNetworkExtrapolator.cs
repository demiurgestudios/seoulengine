/*
	ScriptNetworkExtrapolator.cs
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
	public abstract class ScriptNetworkExtrapolator
	{
		[Pure] public abstract int ExtrapolateAt(int a0, SlimCS.Table a1);
		[Pure] public abstract SlimCS.Table GetSettings();
		public abstract void SetServerTickNow(int a0, int a1);
		public abstract void SetSettings(SlimCS.Table a0);
	}
}
