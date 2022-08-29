/**
 * \file ITextEditable.cpp
 * \brief Interface. Implement to receive text edit notification events
 * from Engine for the current platform.
 *
 * To abstract the details of text entry, Engine implements a platform
 * dependent text input system that reports text state to an active
 * editable via this interface.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ITextEditable.h"

namespace Seoul
{

/** Utility function used by IsAllowedCharater. */
static inline UniChar InternalStaticParseCharacter(
	String::Iterator& i,
	const String::Iterator iEnd)
{
	UniChar chReturn = 0;
	if (iEnd == i)
	{
		chReturn = 0;
	}
	else
	{
		if ('\\' == *i)
		{
			++i;
			if (iEnd == i)
			{
				chReturn = 0;
			}
			else
			{
				chReturn = *i;
				++i;
			}
		}
		else
		{
			chReturn = *i;
			++i;
		}
	}

	return chReturn;
}

/**
 * @return True if c can be inserted into the TextEntry text field,
 * based on any restriction rules. This method exactly
 * matches the is_allowed() method in the iggy codebase, which
 * implements handling of the TextField.restrict field of ActionScript.
 */
static Bool IsAllowedCharacter(const String& sRestrict, UniChar c)
{
	// Empty m_sRestriction string, return true immediately.
	if (sRestrict.IsEmpty())
	{
		return true;
	}

	// Exact match to is_allowed() in the iggy codebase, implementation
	// of the Action Script text_field.restricted field.
	Bool bAllowed = false;
	Bool bInAllowList = true;

	// If the first character is '^', the default allowed state is set to true
	// (the string is a "disallowed" list.
	if (sRestrict[0] == '^')
	{
		bAllowed = true;
	}

	// '\\' is a special character which escapes the next character.
	// '^' is a special character which changes the meaning of bInAllowList.
	auto const iBegin = sRestrict.Begin();
	auto const iEnd = sRestrict.End();
	for (auto i = iBegin; iEnd != i; )
	{
		// Swap the allowed/disallowed mode.
		if ('^' == *i)
		{
			bInAllowList = !bInAllowList;
			++i;
		}
		else
		{
			// Get the first character.
			UniChar const chFirst = InternalStaticParseCharacter(i, iEnd);

			// Range of characters.
			if (iEnd != i && '-' == *i)
			{
				// Skip the hyphen.
				++i;

				// Get the next character.
				UniChar const chLast = (iEnd != i ? InternalStaticParseCharacter(i, iEnd) : 0);

				// If in range, or exactly, set the allowed/disallowed state.
				if ((0 != chLast ? (c >= chFirst && c <= chLast) : (c == chFirst)))
				{
					bAllowed = bInAllowList;
				}
			}
			// Individual character check.
			else
			{
				if (c == chFirst)
				{
					bAllowed = bInAllowList;
				}
			}
		}
	}

	return bAllowed;
}

void ITextEditable::TextEditableApplyConstraints(const StringConstraints& constraints, String& rs)
{
	// Limit length.
	if (constraints.m_iMaxCharacters > 0)
	{
		UInt const uUnicodeLength = rs.GetUnicodeLength();
		if (uUnicodeLength > (UInt)constraints.m_iMaxCharacters)
		{
			UInt const uDifference = (UInt)(uUnicodeLength - (UInt)constraints.m_iMaxCharacters);
			for (UInt i = 0; i < uDifference; ++i)
			{
				rs.PopBack();
			}
		}
	}

	// Filter characters.
	if (!constraints.m_sRestrict.IsEmpty())
	{
		String sNewString;

		{
			auto const iBegin = rs.Begin();
			auto const iEnd = rs.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (IsAllowedCharacter(constraints.m_sRestrict, *i))
				{
					sNewString.Append(*i);
				}
			}
		}

		rs.Swap(sNewString);
	}
}

} // namespace Seoul
