/**
 * \file ScriptEngineCore.cpp
 * \brief Binder instance for exposing Core
 * functionality into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ScriptEngineCore.h"
#include "ScriptFunctionInterface.h"
#include "ScriptVm.h"
#include "SeoulString.h"
#include "SeoulUUID.h"
#include "StringUtil.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineCore, TypeFlags::kDisableCopy)
	SEOUL_METHOD(JsonStringToTable)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "SlimCS.Table", "string sJson, bool bConvertNilToEmptyTable = false")
	SEOUL_METHOD(TableToJsonString)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "SlimCS.Table tData")
	SEOUL_METHOD(StringCompare)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "int", "string s, params string[] asOther")
	SEOUL_METHOD(StringReplace)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sReplace, params string[] asInput")
	SEOUL_METHOD(StringSub)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string s, int startIndex, int len")
	SEOUL_METHOD(GzipCompress)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData, double eCompressionLevel = -1")
	SEOUL_METHOD(LZ4Compress)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData, double eCompressionLevel = 16")
	SEOUL_METHOD(LZ4Decompress)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData")
	SEOUL_METHOD(ZSTDCompress)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData, double eCompressionLevel = 22")
	SEOUL_METHOD(ZSTDDecompress)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData")

	SEOUL_METHOD(StringToUpper)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData")
	SEOUL_METHOD(StringToUpperHtmlAware)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData")
	SEOUL_METHOD(StringToLower)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData")
	SEOUL_METHOD(StringToUpperASCII)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData")
	SEOUL_METHOD(StringToLowerASCII)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "string sData")
	SEOUL_METHOD(CaseInsensitiveStringHash)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "int", "string sData")

	SEOUL_METHOD(Base64Encode)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "object bytesData, bool urlSafe")
	SEOUL_METHOD(Base64Decode)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "object", "string encodedData")

	SEOUL_METHOD(SetGameLogChannelNames)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "params string[] asNames")
	SEOUL_METHOD(Log)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "params string[] asInput")
	SEOUL_METHOD(LogChannel)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "object eChannel, params string[] asInput")
	SEOUL_METHOD(IsLogChannelEnabled)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "string eChannel")
	SEOUL_METHOD(Warn)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "params string[] asInput")
	SEOUL_METHOD(NewUUID)
	SEOUL_METHOD(GetWorldTimeYearMonthDay)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double, double)", "Native.WorldTime t")
	SEOUL_METHOD(SendErrorReportingMessageWithoutStackTrace)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "string stringIn")
	SEOUL_METHOD(WriteTableToFile)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "bool", "object filePathOrStringPath, SlimCS.Table data")

	// Native crash testing.
#if !SEOUL_BUILD_FOR_DISTRIBUTION
	SEOUL_METHOD(TestNativeCrash)
#endif // /#if !SEOUL_BUILD_FOR_DISTRIBUTION
SEOUL_END_TYPE()

ScriptEngineCore::ScriptEngineCore()
{
}

ScriptEngineCore::~ScriptEngineCore()
{
}

void ScriptEngineCore::JsonStringToTable(Script::FunctionInterface* pInterface) const
{
	Byte const* s = nullptr;
	UInt32 u = 0u;
	if (!pInterface->GetString(1, s, u))
	{
		pInterface->RaiseError(1, "invalid string argument.");
		return;
	}

	DataStore dataStore;
	if (!DataStoreParser::FromString(s, u, dataStore))
	{
		// TODO: Tracking down a native crash on iOS. Speculation is that
		// it is due to the body of the JSON data in some cases being passed to Lua
		// as a string and then handled in an error context. So logging it instead.
#if SEOUL_LOGGING_ENABLED
		{
			String sBody(s, u);
			SEOUL_WARN("Invalid JSON string data: |%s|", sBody.CStr());
		}
#endif // /#if SEOUL_LOGGING_ENABLED

		pInterface->RaiseError(1, "failed parsing JSON string - likely syntax error.");
		return;
	}

	Bool bConvertNilToEmptyTable = false;
	if (!pInterface->IsNilOrNone(2))
	{
		if (!pInterface->GetBoolean(2, bConvertNilToEmptyTable))
		{
			pInterface->RaiseError(2, "expected boolean bConvertNilToEmptyTable");
			return;
		}
	}

	if (!pInterface->PushReturnDataNode(dataStore, dataStore.GetRootNode(), bConvertNilToEmptyTable))
	{
		pInterface->RaiseError(-1, "failed returning table, likely syntax error.");
		return;
	}
}

void ScriptEngineCore::TableToJsonString(Script::FunctionInterface* pInterface) const
{
	DataStore dataStore;
	if(!pInterface->GetTable(1, dataStore))
	{
		pInterface->RaiseError(1, "invalid table argument.");
		return;
	}

	String sTableAsJSON;
	dataStore.ToString(dataStore.GetRootNode(), sTableAsJSON, false, 0, false);

	pInterface->PushReturnString(sTableAsJSON);
}

UInt32 ScriptEngineCore::CaseInsensitiveStringHash(const String& string) const
{
	return Seoul::GetCaseInsensitiveHash(string.CStr(), string.GetSize());
}

void ScriptEngineCore::StringCompare(Script::FunctionInterface* pInterface) const
{
	auto const iCount = pInterface->GetArgumentCount();

	Byte const* sLast = nullptr;
	UInt32 uUnused = 0u;
	if (!pInterface->GetString(1, sLast, uUnused))
	{
		pInterface->RaiseError(1, "expected string");
		return;
	}

	for (Int32 i = 2; i < iCount; ++i)
	{
		Byte const* s = nullptr;
		if (!pInterface->GetString(i, s, uUnused))
		{
			pInterface->RaiseError(i, "expected string");
			return;
		}

		auto const iResult = strcmp(sLast, s);
		if (0 != iResult)
		{
			pInterface->PushReturnInteger(iResult);
			return;
		}

		sLast = s;
	}

	pInterface->PushReturnInteger(0);
}

void ScriptEngineCore::StringReplace(Script::FunctionInterface* pInterface) const
{
	using namespace Seoul;

	Int const iCount = pInterface->GetArgumentCount();
	if (4 > iCount)
	{
		pInterface->RaiseError(-1, "Incorrect number of arguments to StringReplace - it needs at least 3: string, pattern, replacement");
		return;
	}

	if (0 != iCount % 2)
	{
		pInterface->RaiseError(-1, "Incorrect number of arguments to SeoulReplace - it needs at least an odd number : string, pattern, replacement, pattern, replacement, ...");
		return;
	}

	String sStr;
	if (!pInterface->GetString(1, sStr))
	{
		pInterface->RaiseError(1, "Incorrect argument type 1 in SeoulReplace - expected string.");
		return;
	}

	for (Int32 i = 2; i < iCount; i += 2)
	{
		String sPattern;
		if (!pInterface->GetString(i, sPattern))
		{
			pInterface->RaiseError(i, "Incorrect pattern argument, expected string.");
			return;
		}
		String sReplacement;
		if (!pInterface->GetString(i+1, sReplacement))
		{
			pInterface->RaiseError(i, "Incorrect replacement argument, expected string.");
			return;
		}

		sStr = sStr.ReplaceAll(sPattern, sReplacement);
	}

	pInterface->PushReturnString(sStr);
}

String ScriptEngineCore::StringSub(const String& s, Int startIndex, UInt32 len) const
{
	if (startIndex < 0)
	{
		auto const uSize = s.GetSize();
		startIndex = (Int)(uSize - (UInt32)Min((Int)uSize, startIndex));
	}
	return s.Substring((UInt)startIndex, len);
}

String ScriptEngineCore::StringToUpper(const String& stringIn) const
{
	return stringIn.ToUpper("en");
}

String ScriptEngineCore::StringToUpperHtmlAware(const String& stringIn) const
{
	String sReturn;
	String sAccum;

	Bool bInTag = false;

	auto const flush = [&]()
	{
		sReturn.Append(sAccum.ToUpper("en"));
		sAccum.Clear();
	};

	for (auto i = stringIn.Begin(), iEnd = stringIn.End(); iEnd != i; ++i)
	{
		switch (*i)
		{
		case '<':
			bInTag = true;
			flush();
			sReturn.Append('<');
			break;
		case '>':
			bInTag = false;
			flush();
			sReturn.Append('>');
			break;
		default:
			if (bInTag)
			{
				sReturn.Append(*i);
			}
			else
			{
				sAccum.Append(*i);
			}
			break;
		}
	}

	flush();
	return sReturn;
}

String ScriptEngineCore::StringToLower(const String& stringIn) const
{
	return stringIn.ToLower("en");
}

String ScriptEngineCore::StringToUpperASCII(const String& stringIn) const
{
	return stringIn.ToUpperASCII();
}

String ScriptEngineCore::StringToLowerASCII(const String& stringIn) const
{
	return stringIn.ToLowerASCII();
}

void ScriptEngineCore::Base64Encode(Script::FunctionInterface* pInterface) const
{
	Script::ByteBuffer buffer;
	if (!pInterface->GetByteBuffer(1, buffer))
	{
		pInterface->RaiseError(1, "expected bytes.");
	}

	Bool urlSafe;
	if (!pInterface->GetBoolean(2, urlSafe))
	{
		pInterface->RaiseError(1, "expected boolean urlSafe.");
	}

	String const s = Seoul::Base64Encode(buffer.m_pData, buffer.m_zDataSizeInBytes, urlSafe);
	pInterface->PushReturnString(s);
}

void ScriptEngineCore::Base64Decode(Script::FunctionInterface* pInterface) const
{
	String s;
	if (!pInterface->GetString(1, s)) {
		pInterface->RaiseError(1, "expected string.");
		return;
	}

	Vector<Byte> buffer;
	auto success = Seoul::Base64Decode(s, buffer);
	if (!success)
	{
		pInterface->RaiseError("Base64 input contained invalid characters");
		return;
	}

	Script::ByteBuffer scriptOut;
	scriptOut.m_pData = buffer.Data();
	scriptOut.m_zDataSizeInBytes = buffer.GetSizeInBytes();

	pInterface->PushReturnByteBuffer(scriptOut);
}

void ScriptEngineCore::GzipCompress(Script::FunctionInterface* pInterface) const
{
	Byte const* s = nullptr;
	UInt32 u = 0u;
	if (!pInterface->GetString(1, s, u))
	{
		pInterface->RaiseError(1, "expected string.");
		return;
	}

	ZlibCompressionLevel eLevel = ZlibCompressionLevel::kDefault;
	if (!pInterface->IsNilOrNone(2))
	{
		if (!pInterface->GetEnum(2, eLevel))
		{
			pInterface->RaiseError(2, "invalid compression level enum.");
			return;
		}
	}

	Script::ByteBuffer buffer;
	if (!Seoul::GzipCompress(
		(void const*)s,
		(UInt32)u,
		buffer.m_pData,
		buffer.m_zDataSizeInBytes,
		eLevel))
	{
		pInterface->RaiseError(-1, "failed compressing data.");
		return;
	}

	pInterface->PushReturnByteBuffer(buffer);
	MemoryManager::Deallocate(buffer.m_pData);
	buffer.m_pData = nullptr;
	buffer.m_zDataSizeInBytes = 0u;
}

void ScriptEngineCore::LZ4Compress(Script::FunctionInterface* pInterface) const
{
	Byte const* s = nullptr;
	UInt32 u = 0u;
	if (!pInterface->GetString(1u, s, u))
	{
		pInterface->RaiseError(1, "expected string.");
		return;
	}

	LZ4CompressionLevel eLevel = LZ4CompressionLevel::kBest;
	if (!pInterface->IsNilOrNone(2u))
	{
		if (!pInterface->GetEnum(2u, eLevel))
		{
			pInterface->RaiseError(2, "invalid compression level enum.");
			return;
		}
	}

	Script::ByteBuffer buffer;
	if (!Seoul::LZ4Compress(
		(void const*)s,
		(UInt32)u,
		buffer.m_pData,
		buffer.m_zDataSizeInBytes,
		eLevel))
	{
		pInterface->RaiseError(-1, "failed compressing data.");
		return;
	}

	pInterface->PushReturnByteBuffer(buffer);
	MemoryManager::Deallocate(buffer.m_pData);
	buffer.m_pData = nullptr;
	buffer.m_zDataSizeInBytes = 0u;
}

void ScriptEngineCore::LZ4Decompress(Script::FunctionInterface* pInterface) const
{
	Byte const* s = nullptr;
	UInt32 u = 0u;
	if (!pInterface->GetString(1u, s, u))
	{
		pInterface->RaiseError(1, "expected string.");
		return;
	}

	Script::ByteBuffer buffer;
	if (!Seoul::LZ4Decompress(
		(void const*)s,
		(UInt32)u,
		buffer.m_pData,
		buffer.m_zDataSizeInBytes))
	{
		pInterface->RaiseError(-1, "failed compressing data.");
		return;
	}

	pInterface->PushReturnByteBuffer(buffer);
	MemoryManager::Deallocate(buffer.m_pData);
	buffer.m_pData = nullptr;
	buffer.m_zDataSizeInBytes = 0u;
}

void ScriptEngineCore::ZSTDCompress(Script::FunctionInterface* pInterface) const
{
	Byte const* s = nullptr;
	UInt32 u = 0u;
	if (!pInterface->GetString(1u, s, u))
	{
		pInterface->RaiseError(1, "expected string.");
		return;
	}

	ZSTDCompressionLevel eLevel = ZSTDCompressionLevel::kBest;
	if (!pInterface->IsNilOrNone(2u))
	{
		if (!pInterface->GetEnum(2u, eLevel))
		{
			pInterface->RaiseError(2, "invalid compression level enum.");
			return;
		}
	}

	Script::ByteBuffer buffer;
	if (!Seoul::ZSTDCompress(
		(void const*)s,
		(UInt32)u,
		buffer.m_pData,
		buffer.m_zDataSizeInBytes,
		eLevel))
	{
		pInterface->RaiseError(-1, "failed compressing data.");
		return;
	}

	pInterface->PushReturnByteBuffer(buffer);
	MemoryManager::Deallocate(buffer.m_pData);
	buffer.m_pData = nullptr;
	buffer.m_zDataSizeInBytes = 0u;
}

void ScriptEngineCore::ZSTDDecompress(Script::FunctionInterface* pInterface) const
{
	Byte const* s = nullptr;
	UInt32 u = 0u;
	if (!pInterface->GetString(1u, s, u))
	{
		pInterface->RaiseError(1, "expected string.");
		return;
	}

	Script::ByteBuffer buffer;
	if (!Seoul::ZSTDDecompress(
		(void const*)s,
		(UInt32)u,
		buffer.m_pData,
		buffer.m_zDataSizeInBytes))
	{
		pInterface->RaiseError(-1, "failed compressing data.");
		return;
	}

	pInterface->PushReturnByteBuffer(buffer);
	MemoryManager::Deallocate(buffer.m_pData);
	buffer.m_pData = nullptr;
	buffer.m_zDataSizeInBytes = 0u;
}

void ScriptEngineCore::SetGameLogChannelNames(Script::FunctionInterface* pInterface)
{
#if SEOUL_LOGGING_ENABLED
	auto const iCount = pInterface->GetArgumentCount();
	if (iCount <= 1)
	{
		return;
	}

	Vector<String, MemoryBudgets::Game> vsNames;
	vsNames.Reserve((UInt32)(iCount - 1));
	for (Int32 i = 1; i < iCount; ++i)
	{
		String sName;
		if (!pInterface->GetString(i, sName))
		{
			pInterface->RaiseError(i, "expected string channel name.");
			return;
		}
		vsNames.PushBack(sName);
	}

	// Register channels for future lookup.
	m_tGameChannels.Clear();
	for (UInt32 i = 0u; i < vsNames.GetSize(); ++i)
	{
		auto const& s = vsNames[i];
		(void)m_tGameChannels.Insert(HString(s), (LoggerChannel)((UInt32)LoggerChannel::MinGameChannel + i));
	}

	Logger::GetSingleton().SetGameChannelNames(vsNames.Data(), vsNames.GetSize());

	// Need to reload the configuration to enable game log channels
	Logger::GetSingleton().LoadConfiguration();
#endif // /#if SEOUL_LOGGING_ENABLED
}

void ScriptEngineCore::Log(Script::FunctionInterface* pInterface) const
{
#if SEOUL_LOGGING_ENABLED
	Int const iCount = pInterface->GetArgumentCount();
	Byte const* s = nullptr;
	UInt32 u = 0u;
	for (Int i = 1; i < iCount; ++i)
	{
		if (!pInterface->GetString(i, s, u))
		{
			pInterface->RaiseError(i, "invalid non-string argument to Log.");
			return;
		}

		Seoul::LogMessage(LoggerChannel::Default, "%s", s);
	}
#endif // /#if SEOUL_LOGGING_ENABLED
}

void ScriptEngineCore::LogChannel(Script::FunctionInterface* pInterface) const
{
#if SEOUL_LOGGING_ENABLED
	Int const iCount = pInterface->GetArgumentCount();

	LoggerChannel eChannel = LoggerChannel::Default;
	if (!pInterface->GetEnum(1u, eChannel))
	{
		// Handle game specific channels, which won't be part of
		// the reflection information.
		HString name;
		if (!pInterface->GetString(1u, name) ||
			!m_tGameChannels.GetValue(name, eChannel))
		{
			pInterface->RaiseError(1, "invalid logger channel to LogChannel.");
			return;
		}
	}

	Byte const* s = nullptr;
	UInt32 u = 0u;
	for (Int i = 2; i < iCount; ++i)
	{
		if (!pInterface->GetString(i, s, u))
		{
			pInterface->RaiseError(i, "invalid non-string argument to Log.");
			return;
		}

		Seoul::LogMessage(eChannel, "%s", s);
	}
#endif // /#if SEOUL_LOGGING_ENABLED
}

Bool ScriptEngineCore::IsLogChannelEnabled(LoggerChannel eChannel) const
{
#if SEOUL_LOGGING_ENABLED
	return Logger::GetSingleton().IsChannelEnabled(eChannel);
#else
	return false;
#endif
}

void ScriptEngineCore::Warn(Script::FunctionInterface* pInterface) const
{
#if SEOUL_LOGGING_ENABLED
	Int const iCount = pInterface->GetArgumentCount();
	Byte const* s = nullptr;
	UInt32 u = 0u;
	for (Int i = 1; i < iCount; ++i)
	{
		if (!pInterface->GetString(i, s, u))
		{
			pInterface->RaiseError(i, "invalid non-string argument to Warn.");
			return;
		}

		Seoul::LogMessage(LoggerChannel::Warning, "%s", s);
	}
#endif // /#if SEOUL_LOGGING_ENABLED
}

String ScriptEngineCore::NewUUID() const
{
	return UUID::GenerateV4().ToString();
}

void ScriptEngineCore::GetWorldTimeYearMonthDay(Script::FunctionInterface* pInterface) const
{
	WorldTime time;
	if (!pInterface->GetWorldTime(1, time))
	{
		pInterface->RaiseError(1, "missing Native.WorldTime");
		return;
	}

	Int32 year;
	UInt32 month;
	UInt32 day;
	time.GetYearMonthDay(year, month, day);

	pInterface->PushReturnInteger(year);
	pInterface->PushReturnUInt32(month);
	pInterface->PushReturnUInt32(day);
}

void ScriptEngineCore::SendErrorReportingMessageWithoutStackTrace(const String& stringIn)
{
	if (CrashManager::Get())
	{
		CustomCrashErrorState state;
		CustomCrashErrorState::Frame frame;
		frame.m_sFilename = __FILE__;
		frame.m_iLine = __LINE__;
		frame.m_sFunction = __FUNCTION__;
		state.m_vStack.PushBack(frame);
		state.m_sReason = stringIn;
		CrashManager::Get()->SendCustomCrash(state);
	}
}

void ScriptEngineCore::WriteTableToFile(Script::FunctionInterface* pInterface) const
{
	// Support argument as string or as a raw FilePath.
	String sFilename;
	FilePath filePath;
	if (pInterface->GetString(1u, sFilename))
	{
		filePath = FilePath::CreateConfigFilePath(sFilename);
	}
	// Error if argument one is not a FilePath after not being a string.
	else if (!pInterface->GetFilePath(1u, filePath))
	{
		SEOUL_WARN("Failed saving data to file. Invalid filepath.");
		pInterface->PushReturnBoolean(false);
		return;
	}

	DataStore dataStore;
	if (!pInterface->GetTable(2, dataStore))
	{
		SEOUL_WARN("Failed saving data to file. Invalid data.");
		pInterface->PushReturnBoolean(false);
		return;
	}

	if (!Reflection::SaveDataStore(dataStore, dataStore.GetRootNode(), filePath))
	{
		SEOUL_WARN("Failed saving data to file. Check that \"%s\" is not read-only "
			"(checked out from source control).",
			filePath.GetAbsoluteFilenameInSource().CStr());
		pInterface->PushReturnBoolean(false);
		return;
	}

	pInterface->PushReturnBoolean(true);
}

// Native crash testing.
#if !SEOUL_BUILD_FOR_DISTRIBUTION
void ScriptEngineCore::TestNativeCrash()
{
	*((volatile Int*)1) = 1;
}
#endif // /#if !SEOUL_BUILD_FOR_DISTRIBUTION

} // namespace Seoul
