/**
 * \file SeoulWildcard.h
 * \brief Class for applying filename style wilcards.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_WILDCARD_H
#define SEOUL_WILDCARD_H

#include "Prereqs.h"
#include "SeoulRegex.h"

namespace Seoul
{

/**
 * Specialized usage of Regex to minimic filename wildcard patterns. 
 */
class Wildcard SEOUL_SEALED
{
public:
	Wildcard(const String& sPattern)
		: m_Regex(ConvertPatternToRegex(sPattern))
	{
	}

	~Wildcard()
	{
	}

	Bool IsExactMatch(Byte const* sInput) const { return m_Regex.IsExactMatch(sInput); }
	Bool IsExactMatch(const String& sInput) const { return m_Regex.IsExactMatch(sInput); }

	Bool IsMatch(Byte const* sInput) const { return m_Regex.IsMatch(sInput); }
	Bool IsMatch(const String& sInput) const { return m_Regex.IsMatch(sInput); }

private:
	SEOUL_DISABLE_COPY(Wildcard);

	Regex m_Regex;

	static String ConvertPatternToRegex(const String& sPattern);
}; // class Wildcard

} // namespace Seoul

#endif // include guard
