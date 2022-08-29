-- Access to the native LocManager singleton.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.





local LocManager = class_static 'LocManager'; local s_BigNumberFormatter

local udLocManager = nil


function LocManager.cctor() s_BigNumberFormatter = BigNumberFormatter:New();

	udLocManager = CoreUtilities.NewNativeUserData(ScriptEngineLocManager)
	if nil == udLocManager then

		error 'Failed instantiating ScriptEngineLocManager.'
	end
end

function LocManager.FormatNumber(
	fNumber, decimals)

	local sRet = udLocManager:FormatNumber(fNumber, decimals)
	return sRet
end

function LocManager.FormatLargeNumber(
	numberValue,
	fractionalDigits,
	fractionalDigitsIfNotEmbiggened,
	wholeNumber,
	minNumDigBeforeFormatting)

	return s_BigNumberFormatter:FormatLargeNumber(numberValue, fractionalDigits, fractionalDigitsIfNotEmbiggened, wholeNumber, minNumDigBeforeFormatting)
end

function LocManager.GetCurrentLanguage()


	local sRet = udLocManager:GetCurrentLanguage()
	return sRet
end

function LocManager.GetCurrentLanguageCode()


	local sRet = udLocManager:GetCurrentLanguageCode()
	return sRet
end

function LocManager.IsValidToken(
	sToken)

	local bRet = udLocManager:IsValidToken(sToken)
	return bRet
end

function LocManager.Localize(
	sToken)

	local sRet = udLocManager:Localize(sToken)
	return sRet
end

function LocManager.IsTokenValue(sToken)

	return udLocManager:IsValidToken(sToken)
end

function LocManager.LocalizeAndReplace(sToken, ...)

	if ({...}) == nil or #({...}) == 0 then

		CoreNative.Warn('Trying to LocalizeAndReplace with no replacements. Token=' ..tostring(sToken))
	end
	local sRet = CoreNative.StringReplace(udLocManager:Localize(sToken), ...)
	return sRet
end

--- <summary>
--- Defines a lexographical order of the string data of two localizable
--- tokens.
--- </summary>
--- <param name="sTokenA">First localization token to compare.</param>
--- <param name="sTokenB">Second localization token to compare.</param>
--- <returns>Integer value defining the order of the tokens.</returns>
function LocManager.TokenOrder(
	sTokenA, sTokenB)

	return udLocManager:TokenOrder(sTokenA, sTokenB)
end

function LocManager.Replace(
	sString, ...)

	local sRet = CoreNative.StringReplace(sString, ...)
	return sRet
end

function LocManager.LocalizeAndReplaceWithLocTokensFromTable(sToken, replacements)

	local count = __castint__(#replacements)
	local args = arraycreate(count, nil)

	for index, replacement in ipairs(replacements) do

		if math.fmod(index, 2) == 0 then

			args[(__i32narrow__(__castint__(index) - 1))+1] = (LocManager.Localize(replacement) or false)

		else

			args[(__i32narrow__(__castint__(index) - 1))+1] = (replacement or false)
		end
	end

	return LocManager.LocalizeAndReplace(sToken,table.unpack(args))
end

function LocManager.TimeToString(
	fSeconds,
	sDaysAbbreviation,
	sHoursAbbreviation,
	sMinutesAbbreviation,
	sSecondsAbbreviation)

	return udLocManager:TimeToString(
		fSeconds,
		sDaysAbbreviation,
		sHoursAbbreviation,
		sMinutesAbbreviation,
		sSecondsAbbreviation)
end

function LocManager.ValidateTokens()


	return udLocManager:ValidateTokens()
end
return LocManager
