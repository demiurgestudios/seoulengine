/**
 * \file ScriptEngineCore.h
 * \brief Binder instance for exposing Core
 * functionality into script.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_CORE_H
#define SCRIPT_ENGINE_CORE_H

#include "BuildDistro.h"
#include "Logger.h"
#include "Prereqs.h"
namespace Seoul { namespace Script { class FunctionInterface; } }
namespace Seoul { class String; }

namespace Seoul
{

class ScriptEngineCore SEOUL_SEALED
{
public:
	ScriptEngineCore();
	~ScriptEngineCore();

	void JsonStringToTable(Script::FunctionInterface* pInterface) const;
	void TableToJsonString(Script::FunctionInterface* pInterface) const;
	void StringCompare(Script::FunctionInterface* pInterface) const;
	void StringReplace(Script::FunctionInterface* pInterface) const;
	String StringSub(const String& s, Int startIndex, UInt32 len) const;

	void GzipCompress(Script::FunctionInterface* pInterface) const;
	void LZ4Compress(Script::FunctionInterface* pInterface) const;
	void LZ4Decompress(Script::FunctionInterface* pInterface) const;
	void ZSTDCompress(Script::FunctionInterface* pInterface) const;
	void ZSTDDecompress(Script::FunctionInterface* pInterface) const;

	void SetGameLogChannelNames(Script::FunctionInterface* pInterface);

	String StringToUpper(const String& stringIn) const;
	String StringToUpperHtmlAware(const String& s) const;
	String StringToLower(const String& stringIn) const;
	String StringToUpperASCII(const String& stringIn) const;
	String StringToLowerASCII(const String& stringIn) const;

	void Base64Encode(Script::FunctionInterface* pInterface) const;
	void Base64Decode(Script::FunctionInterface* pInterface) const;

	void Log(Script::FunctionInterface* pInterface) const;
	void LogChannel(Script::FunctionInterface* pInterface) const;
	Bool IsLogChannelEnabled(LoggerChannel eChannel) const;
	void Warn(Script::FunctionInterface* pInterface) const;

	void SendErrorReportingMessageWithoutStackTrace(const String& stringIn);

	String NewUUID() const;

	// TODO: Make this a qualified method of SeoulTime that returns a script-friendly version of a FixedArray
	void GetWorldTimeYearMonthDay(Script::FunctionInterface* pInterface) const;

	void WriteTableToFile(Script::FunctionInterface* pInterface) const;

	UInt32 CaseInsensitiveStringHash(const String& string) const;

	// Native crash testing.
#if !SEOUL_BUILD_FOR_DISTRIBUTION
	void TestNativeCrash();
#endif // /#if !SEOUL_BUILD_FOR_DISTRIBUTION

private:
	SEOUL_DISABLE_COPY(ScriptEngineCore);

	typedef HashTable<HString, LoggerChannel, MemoryBudgets::Scripting> GameChannels;
	GameChannels m_tGameChannels;
}; // class ScriptEngineCore

} // namespace Seoul

#endif // include guard
