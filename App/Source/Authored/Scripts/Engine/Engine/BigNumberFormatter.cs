// Class for formatting large numbers--mostly taken from Project Madrid.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics.Contracts;

using static SlimCS;

public class BigNumberFormatter
{
	private const int BIG_NUMBER_DIGITS_FORMAT_STEP = 3;
	public const int MIN_NUMBER_DIGITS_BEFORE_FORMATTING_BIG_NUMBER = 7;
	private const int NUMBER_OF_DECIMALS_IN_FORMATTED_BIG_NUMBER = 2;

	private Table<int, string> m_NumberSuffixMap = null;
	private Table<int, string> m_ExtraNumberSuffixMap = new Table<int, string>();

	private int m_NumberOfDigitsConfiguredInLocalization = 0;

	public string FormatLargeNumber(
		double numberValue,
		int fractionalDigits = 2,
		int fractionalDigitsIfNotEmbiggened = 0,
		bool wholeNumber = false,
		int minNumDigBeforeFormatting = MIN_NUMBER_DIGITS_BEFORE_FORMATTING_BIG_NUMBER)
	{
		double outputNumber = 0;

		int totalDigits = TotalNumberWholeDigits(numberValue);
		int power = QuantizePowerForBigNumber(totalDigits);
		if (totalDigits > m_NumberOfDigitsConfiguredInLocalization)
		{
			power = totalDigits - 1;
		}

		/// <summary>
		/// Do we need to format as a big number?
		/// </summary>
		if (totalDigits >= minNumDigBeforeFormatting)
		{
			outputNumber = numberValue / math.pow(10, power);
			/*if (cutDecimalsForLargeNumbers)
			{
				return udLocManager.FormatNumber(outputNumber, NUMBER_OF_DECIMALS_IN_FORMATTED_BIG_NUMBER).Split(new char[] { DECIMAL_SEP })[0] + GetNumberSuffix(totalDigits);
			}*/
			return LocManager.FormatNumber(outputNumber, NUMBER_OF_DECIMALS_IN_FORMATTED_BIG_NUMBER) + GetNumberSuffix(totalDigits);
		}
		else
		{
			/// <summary>
			/// else number is small, return as a formatted number.
			/// </summary>
			outputNumber = numberValue;
			if (wholeNumber)
			{
				return LocManager.FormatNumber(numberValue, 0);
			}
			else
			{
				return LocManager.FormatNumber(numberValue, fractionalDigitsIfNotEmbiggened);
			}
		}
	}

	/// <summary>
	/// Uses our LocTokens to get the Big Number suffix for the number of digits provided.
	/// </summary>
	/// <param name="digits"></param>
	/// <returns></returns>
	private string GetNumberSuffix(int digits)
	{
		PopulateNumberMapIfNeeded();
		if (m_NumberSuffixMap[digits] != null)
		{
			return m_NumberSuffixMap[digits];
		}

		digits -= 1; // Scientific notiation will be offset by one less.

		// lets not constantly instantiate new strings...
		if (m_ExtraNumberSuffixMap[digits] == null)
		{
			m_ExtraNumberSuffixMap[digits] = "e" + digits;
		}
		return m_ExtraNumberSuffixMap[digits];
	}

	/// <summary>
	/// Quantize our power in a stepped fashion (i.e. 100=0, 1000=3, 10,000=3, 100,000=3, 1,000,000=6...)
	/// </summary>
	/// <param name="digits"></param>
	/// <returns></returns>
	private int QuantizePowerForBigNumber(int digits)
	{
		return (int)((math.ceil(digits / (double)BIG_NUMBER_DIGITS_FORMAT_STEP) - 1) * BIG_NUMBER_DIGITS_FORMAT_STEP);
	}

	/// <summary>
	/// Returns the total number of whole number digits. Does not care about decimal places
	/// </summary>
	/// <param name="number"></param>
	/// <returns></returns>
	private int TotalNumberWholeDigits(double number)
	{
		if (number <= 1)
		{
			return 1;
		}
		int numberDigitPlaces = (int)math.floor(math.log10(number) + 1);
		return numberDigitPlaces;
	}

	private void PopulateNumberMapIfNeeded()
	{
		/// <summary>
		/// if we are empty, populate map.
		/// </summary>
		if (m_NumberSuffixMap == null)
		{
			m_NumberSuffixMap = new Table<int, string>();
			bool foundNonEmptySuffix = false;

			int chunkStep = 5; // How many more we'll check if we found a loc string.
			string lastFoundSuffix = "";

			/// <summary>
			/// check for a single chunk, counting upward.
			/// </summary>
			int chunkLeft = chunkStep;
			int step = 0;
			while (chunkLeft > 0)
			{
				chunkLeft--;

				/// <summary>
				/// our incremental loc token to check.
				/// </summary>
				string token = "BigNumberFormatSuffix_DigitsThreshold_" + step;
				if (LocManager.IsValidToken(token))
				{
					lastFoundSuffix = LocManager.Localize(token);
					m_NumberOfDigitsConfiguredInLocalization = step;

					// found one, check for <ChunkStep> more
					chunkLeft = chunkStep;
				}
				if (lastFoundSuffix != string.Empty)
				{
					foundNonEmptySuffix = true;
				}

				/// <summary>
				/// continue to use the last found threshold
				/// </summary>
				if (!foundNonEmptySuffix || step < m_NumberOfDigitsConfiguredInLocalization + BIG_NUMBER_DIGITS_FORMAT_STEP)
				{
					m_NumberSuffixMap[step] = lastFoundSuffix;
				}
				step++;
			}

			m_NumberOfDigitsConfiguredInLocalization += BIG_NUMBER_DIGITS_FORMAT_STEP;
		}
	}
}
