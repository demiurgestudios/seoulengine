// Access to the native LocManager singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics.Contracts;

using static SlimCS;

public static class LocManager
{
	static Native.ScriptEngineLocManager udLocManager = null;
	static BigNumberFormatter s_BigNumberFormatter = new BigNumberFormatter();

	static LocManager()
	{
		udLocManager = CoreUtilities.NewNativeUserData<Native.ScriptEngineLocManager>();
		if (null == udLocManager)
		{
			error("Failed instantiating ScriptEngineLocManager.");
		}
	}

	[Pure]
	public static string FormatNumber(double fNumber, int decimals = 0)
	{
		var sRet = udLocManager.FormatNumber(fNumber, decimals);
		return sRet;
	}

	public static string FormatLargeNumber(
		double numberValue,
		int fractionalDigits = 2,
		int fractionalDigitsIfNotEmbiggened = 0,
		bool wholeNumber = false,
		int minNumDigBeforeFormatting = BigNumberFormatter.MIN_NUMBER_DIGITS_BEFORE_FORMATTING_BIG_NUMBER)
	{
		return s_BigNumberFormatter.FormatLargeNumber(numberValue, fractionalDigits, fractionalDigitsIfNotEmbiggened, wholeNumber, minNumDigBeforeFormatting);
	}

	[Pure]
	public static string GetCurrentLanguage()
	{
		var sRet = udLocManager.GetCurrentLanguage();
		return sRet;
	}

	[Pure]
	public static string GetCurrentLanguageCode()
	{
		var sRet = udLocManager.GetCurrentLanguageCode();
		return sRet;
	}

	[Pure]
	public static bool IsValidToken(string sToken)
	{
		var bRet = udLocManager.IsValidToken(sToken);
		return bRet;
	}

	[Pure]
	public static string Localize(string sToken)
	{
		var sRet = udLocManager.Localize(sToken);
		return sRet;
	}

	public static bool IsTokenValue(string sToken)
	{
		return udLocManager.IsValidToken(sToken);
	}

	public static string LocalizeAndReplace(string sToken, params string[] aArgs)
	{
		if (aArgs == null || aArgs.Length == 0)
		{
			CoreNative.Warn("Trying to LocalizeAndReplace with no replacements. Token=" + sToken);
		}
		var sRet = CoreNative.StringReplace(udLocManager.Localize(sToken), aArgs);
		return sRet;
	}

	/// <summary>
	/// Defines a lexographical order of the string data of two localizable
	/// tokens.
	/// </summary>
	/// <param name="sTokenA">First localization token to compare.</param>
	/// <param name="sTokenB">Second localization token to compare.</param>
	/// <returns>Integer value defining the order of the tokens.</returns>
	[Pure]
	public static double TokenOrder(string sTokenA, string sTokenB)
	{
		return udLocManager.TokenOrder(sTokenA, sTokenB);
	}

	[Pure]
	public static string Replace(string sString, params string[] aArgs)
	{
		var sRet = CoreNative.StringReplace(sString, aArgs);
		return sRet;
	}

	internal static string LocalizeAndReplaceWithLocTokensFromTable(string sToken, Table replacements)
	{
		int count = (int)lenop(replacements);
		string[] args = new string[count];

		foreach ((double index, string replacement) in ipairs(replacements))
		{
			if (index % 2 == 0)
			{
				args[(int)index - 1] = LocManager.Localize(replacement);
			}
			else
			{
				args[(int)index - 1] = replacement;
			}
		}

		return LocManager.LocalizeAndReplace(sToken, args);
	}

	public static string TimeToString(
		double fSeconds,
		string sDaysAbbreviation,
		string sHoursAbbreviation,
		string sMinutesAbbreviation,
		string sSecondsAbbreviation)
	{
		return udLocManager.TimeToString(
			fSeconds,
			sDaysAbbreviation,
			sHoursAbbreviation,
			sMinutesAbbreviation,
			sSecondsAbbreviation);
	}

	[Pure]
	public static bool ValidateTokens()
	{
		return udLocManager.ValidateTokens();
	}
}
