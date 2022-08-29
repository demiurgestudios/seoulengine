/*
	ScriptEngineCore.cs
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
	public abstract class ScriptEngineCore
	{
		[Pure] public abstract SlimCS.Table JsonStringToTable(string sJson, bool bConvertNilToEmptyTable = false);
		[Pure] public abstract string TableToJsonString(SlimCS.Table tData);
		[Pure] public abstract int StringCompare(string s, params string[] asOther);
		[Pure] public abstract string StringReplace(string sReplace, params string[] asInput);
		[Pure] public abstract string StringSub(string s, int startIndex, int len);
		[Pure] public abstract string GzipCompress(string sData, double eCompressionLevel = -1);
		[Pure] public abstract string LZ4Compress(string sData, double eCompressionLevel = 16);
		[Pure] public abstract string LZ4Decompress(string sData);
		[Pure] public abstract string ZSTDCompress(string sData, double eCompressionLevel = 22);
		[Pure] public abstract string ZSTDDecompress(string sData);
		[Pure] public abstract string StringToUpper(string sData);
		[Pure] public abstract string StringToUpperHtmlAware(string sData);
		[Pure] public abstract string StringToLower(string sData);
		[Pure] public abstract string StringToUpperASCII(string sData);
		[Pure] public abstract string StringToLowerASCII(string sData);
		[Pure] public abstract int CaseInsensitiveStringHash(string sData);
		[Pure] public abstract string Base64Encode(object bytesData, bool urlSafe);
		[Pure] public abstract object Base64Decode(string encodedData);
		public abstract void SetGameLogChannelNames(params string[] asNames);
		[Pure] public abstract void Log(params string[] asInput);
		[Pure] public abstract void LogChannel(object eChannel, params string[] asInput);
		[Pure] public abstract bool IsLogChannelEnabled(string eChannel);
		[Pure] public abstract void Warn(params string[] asInput);
		[Pure] public abstract string NewUUID();
		[Pure] public abstract (double, double, double) GetWorldTimeYearMonthDay(Native.WorldTime t);
		public abstract void SendErrorReportingMessageWithoutStackTrace(string stringIn);
		[Pure] public abstract bool WriteTableToFile(object filePathOrStringPath, SlimCS.Table data);
		public abstract void TestNativeCrash();
	}
}
