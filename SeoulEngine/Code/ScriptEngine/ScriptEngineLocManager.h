/**
 * \file ScriptEngineLocManager.h
 * \brief Binder instance for exposing the LocManager Singleton into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_ENGINE_LOC_MANAGER_H
#define SCRIPT_ENGINE_LOC_MANAGER_H

#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

/** Binder, wraps a Camera instance and exposes functionality to a script VM. */
class ScriptEngineLocManager SEOUL_SEALED
{
public:
	ScriptEngineLocManager();
	~ScriptEngineLocManager();

	String FormatNumber(Double iNumber, Int decimals) const;
	String GetCurrentLanguage() const;
	String GetCurrentLanguageCode() const;
	Bool IsValidToken(const String& s) const;
	String Localize(const String& s) const;
	Int TokenOrder(const String& sTokenA, const String& sTokenB) const;
	// Formats a time interval in seconds into abbreviated days, hours, minutes, and seconds.
	String TimeToString(Float seconds,
		HString sDaysAbbreviation,
		HString sHoursAbbreviation,
		HString sMinutesAbbreviation,
		HString sSecondsAbbreviation);
	Bool ValidateTokens() const;

private:
	SEOUL_DISABLE_COPY(ScriptEngineLocManager);
}; // class ScriptEngineLocManager

} // namespace Seoul

#endif // include guard
