-- Class for formatting large numbers--mostly taken from Project Madrid.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





local BigNumberFormatter = class 'BigNumberFormatter'

local BIG_NUMBER_DIGITS_FORMAT_STEP = 3
BigNumberFormatter.MIN_NUMBER_DIGITS_BEFORE_FORMATTING_BIG_NUMBER = 7
local NUMBER_OF_DECIMALS_IN_FORMATTED_BIG_NUMBER = 2




function BigNumberFormatter:Constructor() self.m_ExtraNumberSuffixMap = {}; self.m_NumberOfDigitsConfiguredInLocalization = 0; end

local TotalNumberWholeDigits;local QuantizePowerForBigNumber;local GetNumberSuffix;function BigNumberFormatter:FormatLargeNumber(
	numberValue,
	_,
	fractionalDigitsIfNotEmbiggened,
	wholeNumber,
	minNumDigBeforeFormatting)

	local outputNumber = 0

	local totalDigits = TotalNumberWholeDigits(self, numberValue)
	local power = QuantizePowerForBigNumber(self, totalDigits)
	if totalDigits > self.m_NumberOfDigitsConfiguredInLocalization then

		power = __i32narrow__(totalDigits - 1)
	end

	--- <summary>
	--- Do we need to format as a big number?
	--- </summary>
	if totalDigits >= minNumDigBeforeFormatting then

		outputNumber = numberValue / math.pow(10, power)
		--[[if (cutDecimalsForLargeNumbers)
			{
				return udLocManager.FormatNumber(outputNumber, NUMBER_OF_DECIMALS_IN_FORMATTED_BIG_NUMBER).Split(new char[] { DECIMAL_SEP })[0] + GetNumberSuffix(totalDigits);
			}]]
		return tostring(LocManager.FormatNumber(outputNumber, 2--[[NUMBER_OF_DECIMALS_IN_FORMATTED_BIG_NUMBER]])) ..tostring(GetNumberSuffix(self, totalDigits))

	else

		--- <summary>
		--- else number is small, return as a formatted number.
		--- </summary>
		outputNumber = numberValue
		if wholeNumber then

			return LocManager.FormatNumber(numberValue, 0)

		else

			return LocManager.FormatNumber(numberValue, fractionalDigitsIfNotEmbiggened)
		end
	end
end

--- <summary>
--- Uses our LocTokens to get the Big Number suffix for the number of digits provided.
--- </summary>
--- <param name="digits"></param>
--- <returns></returns>
local PopulateNumberMapIfNeeded;function GetNumberSuffix(self, digits)

	PopulateNumberMapIfNeeded(self)
	if self.m_NumberSuffixMap[digits] ~= nil then

		return self.m_NumberSuffixMap[digits]
	end

	digits = __i32narrow__((digits) - (1)) -- Scientific notiation will be offset by one less.

	-- lets not constantly instantiate new strings...
	if self.m_ExtraNumberSuffixMap[digits] == nil then

		self.m_ExtraNumberSuffixMap[digits] = 'e' .. digits
	end
	return self.m_ExtraNumberSuffixMap[digits]
end

--- <summary>
--- Quantize our power in a stepped fashion (i.e. 100=0, 1000=3, 10,000=3, 100,000=3, 1,000,000=6...)
--- </summary>
--- <param name="digits"></param>
--- <returns></returns>
function QuantizePowerForBigNumber(self, digits)

	return __castint__(((math.ceil(digits / 3--[[BIG_NUMBER_DIGITS_FORMAT_STEP]]) - 1) * 3--[[BIG_NUMBER_DIGITS_FORMAT_STEP]]))
end

--- <summary>
--- Returns the total number of whole number digits. Does not care about decimal places
--- </summary>
--- <param name="number"></param>
--- <returns></returns>
function TotalNumberWholeDigits(self, number)

	if number <= 1 then

		return 1
	end
	local numberDigitPlaces = __castint__(math.floor(math.log10(number) + 1))
	return numberDigitPlaces
end

function PopulateNumberMapIfNeeded(self)

	--- <summary>
	--- if we are empty, populate map.
	--- </summary>
	if self.m_NumberSuffixMap == nil then

		self.m_NumberSuffixMap = {}
		local foundNonEmptySuffix = false

		local chunkStep = 5 -- How many more we'll check if we found a loc string.
		local lastFoundSuffix = ''

		--- <summary>
		--- check for a single chunk, counting upward.
		--- </summary>
		local chunkLeft = chunkStep
		local step = 0
		while chunkLeft > 0 do

			chunkLeft = chunkLeft - 1

			--- <summary>
			--- our incremental loc token to check.
			--- </summary>
			local token = 'BigNumberFormatSuffix_DigitsThreshold_' .. step
			if LocManager.IsValidToken(token) then

				lastFoundSuffix = LocManager.Localize(token)
				self.m_NumberOfDigitsConfiguredInLocalization = step

				-- found one, check for <ChunkStep> more
				chunkLeft = chunkStep
			end
			if lastFoundSuffix ~= ''--[[String.Empty]] then

				foundNonEmptySuffix = true
			end

			--- <summary>
			--- continue to use the last found threshold
			--- </summary>
			if not foundNonEmptySuffix or step < __i32narrow__(self.m_NumberOfDigitsConfiguredInLocalization + 3--[[BIG_NUMBER_DIGITS_FORMAT_STEP]]) then

				self.m_NumberSuffixMap[step] = lastFoundSuffix
			end
			step = step + 1
		end

		self.m_NumberOfDigitsConfiguredInLocalization = __i32narrow__((self.m_NumberOfDigitsConfiguredInLocalization) +(3--[[BIG_NUMBER_DIGITS_FORMAT_STEP]]))
	end
end
return BigNumberFormatter
