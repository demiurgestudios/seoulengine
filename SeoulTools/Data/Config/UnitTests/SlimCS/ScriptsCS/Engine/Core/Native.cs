// SeoulEngine core utilities.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using System.Diagnostics;
using System.Diagnostics.Contracts;

using static SlimCS;

public static class CoreNative
{
	static readonly Native.ScriptEngineCore udCore = null;

	static CoreNative()
	{
		udCore = CoreUtilities.NewNativeUserData<Native.ScriptEngineCore>();
		if (null == udCore)
		{
			error("Failed instantiating ScriptEngineCore.");
		}
	}

	[Pure]
	public static Table JsonStringToTable(string sData, bool bOptionalConvertNilToEmptyTable = false)
	{
		var ret = udCore.JsonStringToTable(sData, bOptionalConvertNilToEmptyTable);
		return ret;
	}

	[Pure]
	public static string TableToJsonString(Table tData)
	{
		var sRet = udCore.TableToJsonString(tData);
		return sRet;
	}

	[Pure]
	public static string StringReplace(string sReplace, params string[] aArgs)
	{
		var sRet = udCore.StringReplace(sReplace, aArgs);
		return sRet;
	}

	[Pure]
	public static string StringSub(string s, int startIndex, int len)
	{
		var sRet = udCore.StringSub(s, startIndex, len);
		return sRet;
	}

	[Pure]
	public static string GzipCompress(string sData)
	{
		var sRet = udCore.GzipCompress(sData);
		return sRet;
	}

	[Pure]
	public static string LZ4Compress(string sData)
	{
		var sRet = udCore.LZ4Compress(sData);
		return sRet;
	}

	[Pure]
	public static string LZ4Decompress(string sData)
	{
		var sRet = udCore.LZ4Decompress(sData);
		return sRet;
	}

	[Pure]
	public static string ZSTDCompresss(string sData)
	{
		var sRet = udCore.ZSTDCompress(sData);
		return sRet;
	}

	[Pure]
	public static string ZSTDDecompress(string sData)
	{
		var sRet = udCore.ZSTDDecompress(sData);
		return sRet;
	}

	[Conditional("DEBUG")]
	public static void SetGameLogChannelNames(params string[] aArgs)
	{
		udCore.SetGameLogChannelNames(aArgs);
	}

	[Conditional("DEBUG")]
	[Pure]
	public static void Log(params string[] aArgs)
	{
		udCore.Log(aArgs);
	}

	[Conditional("DEBUG")]
	[Pure]
	public static void LogChannel(
		string sChannel,
		params string[] aArgs)
	{
		udCore.LogChannel(sChannel, aArgs);
	}

	[Pure]
	public static bool IsLogChannelEnabled(string eChannel)
	{
		return udCore.IsLogChannelEnabled(eChannel);
	}

	[Conditional("DEBUG")]
	[Pure]
	public static void Warn(params string[] aArgs)
	{
		udCore.Warn(aArgs);
	}

	public static string NewUUID()
	{
		return udCore.NewUUID();
	}

	public static (double, double, double) GetWorldTimeYearMonthDay(Native.WorldTime time)
	{
		return udCore.GetWorldTimeYearMonthDay(time);
	}

	public static int StringCompare(string s, params string[] asOther)
	{
		return udCore.StringCompare(s, asOther);
	}

	public static string StringToUpper(string sData)
	{
		return udCore.StringToUpper(sData);
	}

	public static string StringToUpperHtmlAware(string sData)
	{
		return udCore.StringToUpperHtmlAware(sData);
	}

	public static string StringToLower(string sData)
	{
		return udCore.StringToLower(sData);
	}

	public static string StringToUpperASCII(string sData)
	{
		return udCore.StringToUpperASCII(sData);
	}

	public static string StringToLowerASCII(string sData)
	{
		return udCore.StringToLowerASCII(sData);
	}

	public static int CaseInsensitiveStringHash(string data)
	{
		return udCore.CaseInsensitiveStringHash(data);
	}

	public static string Base64Encode(object bytesData, bool urlSafe = false)
	{
		return udCore.Base64Encode(bytesData, urlSafe);
	}

	public static object Base64Decode(string encodedData)
	{
		return udCore.Base64Decode(encodedData);
	}

	public static void SendErrorReportingMessageWithoutStackTrace(string stringIn)
	{
		udCore.SendErrorReportingMessageWithoutStackTrace(stringIn);
	}

#if DEBUG
	public static bool WriteTableToFile(object filePathOrStringPath, Table data)
	{
		return udCore.WriteTableToFile(filePathOrStringPath, data);
	}
#endif

#if SEOUL_BUILD_NOT_FOR_DISTRIBUTION
	public static void TestNativeCrash()
	{
		udCore.TestNativeCrash();
	}
#endif
}
