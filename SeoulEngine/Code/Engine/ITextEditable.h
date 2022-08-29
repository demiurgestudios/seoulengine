/**
 * \file ITextEditable.h
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

#pragma once
#ifndef ITEXT_EDITABLE_H
#define ITEXT_EDITABLE_H

#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

/**
 * Typical string specify a maximum number of characters and a filter
 * pattern (regular expression).
 */
struct StringConstraints
{
	StringConstraints()
		: m_sRestrict()
		, m_iMaxCharacters(0)
	{
	}

	// Restriction filter - uses the ActionScript 3 TextField.restrict format.
	String m_sRestrict;
	// maximum number of characters the field allows
	Int m_iMaxCharacters;
}; // struct StringConstraints

class ITextEditable SEOUL_ABSTRACT
{
public:
	static void TextEditableApplyConstraints(const StringConstraints& constraints, String& rs);

	virtual ~ITextEditable()
	{
	}

	virtual void TextEditableApplyChar(UniChar c) = 0;
	virtual void TextEditableApplyText(const String& sText) = 0;
	virtual void TextEditableEnableCursor() = 0;
	virtual void TextEditableStopEditing() = 0;

protected:
	ITextEditable()
	{
	}

private:
	SEOUL_DISABLE_COPY(ITextEditable);
}; // class ITextEditable

} // namespace Seoul

#endif // include guard
