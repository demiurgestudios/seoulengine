/**
 * \file HtmlReader.h
 * \brief HtmlReader is a utility class to extract nodes and attributes
 * from a subset of HTML format text chunks.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HTML_READER_H
#define HTML_READER_H

#include "Color.h"
#include "HtmlTypes.h"
#include "Lexer.h"
#include "Prereqs.h"
#include "SeoulString.h"

namespace Seoul
{

class HtmlReader SEOUL_SEALED
{
public:
	HtmlReader(
		const String::Iterator& i,
		const String::Iterator& iEnd,
		String& rsPlainTextOut)
		: m_Context()
		, m_rsOut(rsPlainTextOut)
	{
		m_Context.SetStream(i.GetPtr(), iEnd.GetIndexInBytes());
	}

	UInt32 GetColumn() const { return m_Context.GetColumn(); }

	Bool ReadAttribute(HtmlAttribute& re, HtmlTagStyle& reStyle);

	void ReadAttributeValue(HtmlAlign& re, HtmlAlign eDefaultValue = HtmlAlign::kLeft);
	void ReadAttributeValue(Float32& rf, Float32 fDefaultValue = 0.0f);
	void ReadAttributeValue(HString& rh);
	void ReadAttributeValue(HtmlImageAlign& re, HtmlImageAlign eDefaultValue = HtmlImageAlign::kMiddle);
	void ReadAttributeValue(Int32& ri, Int32 iDefaultValue = 0);
	void ReadAttributeValue(RGBA& rc, RGBA defaultValue = RGBA::Black());
	void ReadAttributeValue(String& rs);

	void ReadTag(HtmlTag& re, HtmlTagStyle& reStyle);

	Bool ReadTextChunk(String::Iterator& riBegin, String::Iterator& riEnd);

private:
	SEOUL_DISABLE_COPY(HtmlReader);

	void ReadAttributeRawValue(Byte const*& rsBegin, Byte const*& rsEnd);

	LexerContext m_Context;
	String& m_rsOut;
	Bool m_bInTag = false;
}; // class HtmlReader

} // namespace Seoul

#endif // include guard
