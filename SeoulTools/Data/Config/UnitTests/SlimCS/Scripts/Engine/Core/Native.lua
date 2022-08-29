-- SeoulEngine core utilities.
--
-- Copyright (c) Demiurge Studios, Inc.
-- 
-- This source code is licensed under the MIT license.
-- Full license details can be found in the LICENSE file
-- in the root directory of this source tree.






local CoreNative = class_static 'CoreNative'

local udCore = nil

function CoreNative.cctor()

	udCore = CoreUtilities.NewNativeUserData(ScriptEngineCore)
	if nil == udCore then

		error 'Failed instantiating ScriptEngineCore.'
	end
end

function CoreNative.JsonStringToTable(
	sData, bOptionalConvertNilToEmptyTable)

	local ret = udCore:JsonStringToTable(sData, bOptionalConvertNilToEmptyTable)
	return ret
end

function CoreNative.TableToJsonString(
	tData)

	local sRet = udCore:TableToJsonString(tData)
	return sRet
end

function CoreNative.StringReplace(
	sReplace, ...)

	local sRet = udCore:StringReplace(sReplace, ...)
	return sRet
end

function CoreNative.StringSub(
	s, startIndex, len)

	local sRet = udCore:StringSub(s, startIndex, len)
	return sRet
end

function CoreNative.GzipCompress(
	sData)

	local sRet = udCore:GzipCompress(sData, -1)
	return sRet
end

function CoreNative.LZ4Compress(
	sData)

	local sRet = udCore:LZ4Compress(sData, 16)
	return sRet
end

function CoreNative.LZ4Decompress(
	sData)

	local sRet = udCore:LZ4Decompress(sData)
	return sRet
end

function CoreNative.ZSTDCompresss(
	sData)

	local sRet = udCore:ZSTDCompress(sData, 22)
	return sRet
end

function CoreNative.ZSTDDecompress(
	sData)

	local sRet = udCore:ZSTDDecompress(sData)
	return sRet
end

function CoreNative.SetGameLogChannelNames(
	...)

	udCore:SetGameLogChannelNames(...)
end

function CoreNative.Log(

	...)

	udCore:Log(...)
end

function CoreNative.LogChannel(


	sChannel,
	...)

	udCore:LogChannel(sChannel, ...)
end

function CoreNative.IsLogChannelEnabled(
	eChannel)

	return udCore:IsLogChannelEnabled(eChannel)
end

function CoreNative.Warn(

	...)

	udCore:Warn(...)
end

function CoreNative.NewUUID()

	return udCore:NewUUID()
end

function CoreNative.GetWorldTimeYearMonthDay(time)

	return udCore:GetWorldTimeYearMonthDay(time)
end

function CoreNative.StringCompare(s, ...)

	return udCore:StringCompare(s, ...)
end

function CoreNative.StringToUpper(sData)

	return udCore:StringToUpper(sData)
end

function CoreNative.StringToUpperHtmlAware(sData)

	return udCore:StringToUpperHtmlAware(sData)
end

function CoreNative.StringToLower(sData)

	return udCore:StringToLower(sData)
end

function CoreNative.StringToUpperASCII(sData)

	return udCore:StringToUpperASCII(sData)
end

function CoreNative.StringToLowerASCII(sData)

	return udCore:StringToLowerASCII(sData)
end

function CoreNative.CaseInsensitiveStringHash(data)

	return udCore:CaseInsensitiveStringHash(data)
end

function CoreNative.Base64Encode(bytesData, urlSafe)

	return udCore:Base64Encode(bytesData, urlSafe)
end

function CoreNative.Base64Decode(encodedData)

	return udCore:Base64Decode(encodedData)
end

function CoreNative.SendErrorReportingMessageWithoutStackTrace(stringIn)

	udCore:SendErrorReportingMessageWithoutStackTrace(stringIn)
end









function CoreNative.TestNativeCrash()

	udCore:TestNativeCrash()
end

return CoreNative
