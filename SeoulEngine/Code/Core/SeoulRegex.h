/**
 * \file SeoulRegex.h
 * \brief Class for applying regular expressions.
 *
 * Implementation is in DataStoreParser.cpp - see note at the bottom
 * of that file.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_REGEX_H
#define SEOUL_REGEX_H

#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

/**
 * Implemented with the internal regex parser in rapidjson, which has the following syntax:
 * - ab     Concatenation
 * - a|b    Alternation
 * - a?     Zero or one
 * - a*     Zero or more
 * - a+     One or more
 * - a{3}   Exactly 3 times
 * - a{3,}  At least 3 times
 * - a{3,5} 3 to 5 times
 * - (ab)   Grouping
 * - ^a     At the beginning
 * - a$     At the end
 * - .      Any character
 * - [abc]  Character classes
 * - [a-c]  Character class range
 * - [a-z0-9_] Character class combination
 * - [^abc] Negated character classes
 * - [^a-c] Negated character class range
 * - [\b]   Backspace (U+0008)
 * - \\| \\\\ ...  Escape characters
 * - \\f Form feed (U+000C)
 * - \\n Line feed (U+000A)
 * - \\r Carriage return (U+000D)
 * - \\t Tab (U+0009)
 * - \\v Vertical tab (U+000B)
 *
 * This is a Thompson NFA engine, implemented with reference to
 * Cox, Russ. "Regular Expression Matching Can Be Simple And Fast (but is slow in Java, Perl, PHP, Python, Ruby,...).",
 * https://swtch.com/~rsc/regexp/regexp1.html
 */
class Regex SEOUL_SEALED
{
public:
	Regex(const String& sRegex);
	~Regex();

	// Return true if the input string is a match, and is
	// entirely consumed by the regular expression.
	Bool IsExactMatch(Byte const* sInput) const;
	Bool IsExactMatch(const String& sInput) const
	{
		return IsExactMatch(sInput.CStr());
	}

	// Return true if the input string is a partial match
	// to the regular expression (the regular expression
	// matches at least one part or subset of the input
	// string).
	Bool IsMatch(Byte const* sInput) const;
	Bool IsMatch(const String& sInput) const
	{
		return IsMatch(sInput.CStr());
	}

private:
	SEOUL_DISABLE_COPY(Regex);

	void* m_p;
}; // class Regex

} // namespace Seoul

#endif // include guard
