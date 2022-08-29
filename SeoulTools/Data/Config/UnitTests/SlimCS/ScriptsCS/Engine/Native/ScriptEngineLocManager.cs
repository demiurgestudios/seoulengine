/*
	ScriptEngineLocManager.cs
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
	public abstract class ScriptEngineLocManager
	{
		[Pure] public abstract string FormatNumber(double a0, int a1);
		[Pure] public abstract string GetCurrentLanguage();
		[Pure] public abstract string GetCurrentLanguageCode();
		[Pure] public abstract bool IsValidToken(string a0);
		[Pure] public abstract string Localize(string a0);
		[Pure] public abstract int TokenOrder(string a0, string a1);

		public abstract string TimeToString(
			double fSeconds,
			string sDaysAbbreviation,
			string sHoursAbbreviation,
			string sMinutesAbbreviation,
			string sSecondsAbbreviation);

		[Pure] public abstract bool ValidateTokens();
	}
}
