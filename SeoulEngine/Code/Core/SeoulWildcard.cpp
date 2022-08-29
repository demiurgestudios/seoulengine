/**
 * \file SeoulWildcard.cpp
 * \brief Class for applying filename style wilcards.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Path.h"
#include "SeoulWildcard.h"

namespace Seoul
{

/** Escape control characters in a regular expression string. */
static inline String RegexEscape(const String& s)
{
	String sReturn;
	sReturn.Reserve(s.GetSize());

	auto const iBegin = s.Begin();
	auto const iEnd = s.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		switch (*i)
		{
			// Control characters.
		case '^':
		case '$':
		case '|':
		case '(':
		case ')':
		case '?':
		case '*':
		case '+':
		case '.':
		case '[':
		case ']':
		case '{':
		case '}':
		case '\\':
			sReturn.Append('\\');
			// fall-through
		default:
			sReturn.Append(*i);
		};
	}

	return sReturn;
}

String Wildcard::ConvertPatternToRegex(const String& sPattern)
{
	return RegexEscape(sPattern.ReplaceAll("\\", "/")) // Normalize directory separators to /, then escape all characters.
		.ReplaceAll("/", "[/\\\\]") // Support either / or \ as a directory separator
		.ReplaceAll("\\*", ".*")   // Handle the asterisk wildcard character.
		.ReplaceAll("\\?", ".");   // Handle the question mark wildcard character.
}

} // namespace Seoul
