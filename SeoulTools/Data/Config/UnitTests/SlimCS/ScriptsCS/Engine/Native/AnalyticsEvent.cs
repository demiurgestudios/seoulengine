/*
	AnalyticsEvent.cs
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
	public abstract class AnalyticsEvent
	{
		public abstract bool DeserializeProperties(SlimCS.Table a0, SlimCS.Table a1, SlimCS.Table a2);
		[Pure] public abstract bool SerializeProperties(SlimCS.Table a0, string a1, SlimCS.Table a2, SlimCS.Table a3);
	}
}
