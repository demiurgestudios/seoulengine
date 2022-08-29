/**
 * \file ScriptEngineLocManager.cpp
 * \brief Binder instance for exposing the LocManager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "LocManager.h"
#include "ReflectionDefine.h"
#include "ScriptEngineLocManager.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineLocManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(FormatNumber)
	SEOUL_METHOD(GetCurrentLanguage)
	SEOUL_METHOD(GetCurrentLanguageCode)
	SEOUL_METHOD(IsValidToken)
	SEOUL_METHOD(Localize)
	SEOUL_METHOD(TokenOrder)
	SEOUL_METHOD_N("TimeToString", TimeToString)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "string", "double fSeconds, string sDaysAbbreviation, string sHoursAbbreviation, string sMinutesAbbreviation, string sSecondsAbbreviation")
	SEOUL_METHOD(ValidateTokens)
SEOUL_END_TYPE()

ScriptEngineLocManager::ScriptEngineLocManager()
{
}

ScriptEngineLocManager::~ScriptEngineLocManager()
{
}

String ScriptEngineLocManager::FormatNumber(Double dNumber, Int decimals = 2) const
{
	return LocManager::Get()->FormatNumber(dNumber, decimals);
}

String ScriptEngineLocManager::GetCurrentLanguage() const
{
	return LocManager::Get()->GetCurrentLanguage();
}

String ScriptEngineLocManager::GetCurrentLanguageCode() const
{
	return LocManager::Get()->GetCurrentLanguageCode();
}

Bool ScriptEngineLocManager::IsValidToken(const String& s) const
{
	return LocManager::Get()->IsValidToken(s);
}

String ScriptEngineLocManager::Localize(const String& s) const
{
	return LocManager::Get()->Localize(s);
}

Int ScriptEngineLocManager::TokenOrder(const String& sTokenA, const String& sTokenB) const
{
	return LocManager::Get()->Localize(sTokenA).Compare(LocManager::Get()->Localize(sTokenB));
}

String ScriptEngineLocManager::TimeToString(Float seconds,
		HString sDaysAbbreviation,
		HString sHoursAbbreviation,
		HString sMinutesAbbreviation,
		HString sSecondsAbbreviation)
{
	return LocManager::Get()->TimeToString(seconds,
		sDaysAbbreviation,
		sHoursAbbreviation,
		sMinutesAbbreviation,
		sSecondsAbbreviation);
}

Bool ScriptEngineLocManager::ValidateTokens() const
{
#if !SEOUL_SHIP
	UInt32 uUnusedNumChecked = 0u;
	return LocManager::Get()->ValidateTokens(uUnusedNumChecked);
#else
	return false;
#endif // /#if !SEOUL_SHIP
}

} // namespace Seoul
