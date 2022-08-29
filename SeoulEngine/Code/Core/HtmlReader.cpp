/**
 * \file HtmlReader.cpp
 * \brief HtmlReader is a utility class to extract nodes and attributes
 * from a subset of HTML format text chunks.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HtmlReader.h"
#include "Logger.h"

namespace Seoul
{

static inline HtmlAlign ParseAlignment(
	Byte const* s,
	Byte const* const sEnd,
	HtmlAlign eDefaultValue)
{
	if (s + 1 > sEnd)
	{
		return eDefaultValue;
	}

	switch (s[0])
	{
	case 'c':
	case 'C':
		if (s + 6 == sEnd && 0 == STRNCMP_CASE_INSENSITIVE("center", s, 6))
		{
			return HtmlAlign::kCenter;
		}
		break;
	case 'j':
	case 'J':
		if (s + 7 == sEnd && 0 == STRNCMP_CASE_INSENSITIVE("justify", s, 7))
		{
			return HtmlAlign::kJustify;
		}
		break;
	case 'l':
	case 'L':
		if (s + 4 == sEnd && 0 == STRNCMP_CASE_INSENSITIVE("left", s, 4))
		{
			return HtmlAlign::kLeft;
		}
		break;
	case 'r':
	case 'R':
		if (s + 5 == sEnd && 0 == STRNCMP_CASE_INSENSITIVE("right", s, 5))
		{
			return HtmlAlign::kRight;
		}
		break;
	default:
		break;
	};

	return eDefaultValue;
}

static inline HtmlImageAlign ParseImageAlignment(
	Byte const* s,
	Byte const* const sEnd,
	HtmlImageAlign eDefaultValue)
{
	if (s + 1 > sEnd)
	{
		return eDefaultValue;
	}

	switch (s[0])
	{
	case 't':
	case 'T':
		if (s + 3 == sEnd && 0 == STRNCMP_CASE_INSENSITIVE("top", s, 3))
		{
			return HtmlImageAlign::kTop;
		}
		break;
	case 'm':
	case 'M':
		if (s + 6 == sEnd && 0 == STRNCMP_CASE_INSENSITIVE("middle", s, 6))
		{
			return HtmlImageAlign::kMiddle;
		}
		break;
	case 'b':
	case 'B':
		if (s + 6 == sEnd && 0 == STRNCMP_CASE_INSENSITIVE("bottom", s, 6))
		{
			return HtmlImageAlign::kBottom;
		}
		break;
	case 'l':
	case 'L':
		if (s + 4 == sEnd && 0 == STRNCMP_CASE_INSENSITIVE("left", s, 4))
		{
			return HtmlImageAlign::kLeft;
		}
		break;
	case 'r':
	case 'R':
		if (s + 5 == sEnd && 0 == STRNCMP_CASE_INSENSITIVE("right", s, 5))
		{
			return HtmlImageAlign::kRight;
		}
		break;
	default:
		break;
	};

	return eDefaultValue;
}

static inline RGBA ParseColor(
	Byte const* s,
	Byte const* const sEnd,
	RGBA defaultValue)
{
	if (s < sEnd && '#' == *s)
	{
		++s;
	}

	if (s + 6 > sEnd)
	{
		return defaultValue;
	}

	UInt32 u0;
	UInt32 u1;
	UInt32 u2;
	UInt32 u3;
	UInt32 u4;
	UInt32 u5;
	if (HexCharToUInt32(s[0], u0) &&
		HexCharToUInt32(s[1], u1) &&
		HexCharToUInt32(s[2], u2) &&
		HexCharToUInt32(s[3], u3) &&
		HexCharToUInt32(s[4], u4) &&
		HexCharToUInt32(s[5], u5))
	{
		RGBA ret = RGBA::White();
		ret.m_R = (UInt8)(((u0 & 0xF) << 4) | ((u1 & 0xF) << 0));
		ret.m_G = (UInt8)(((u2 & 0xF) << 4) | ((u3 & 0xF) << 0));
		ret.m_B = (UInt8)(((u4 & 0xF) << 4) | ((u5 & 0xF) << 0));
		return ret;
	}
	else
	{
		return defaultValue;
	}
}

static inline Bool IsSimpleWhiteSpace(UniChar ch)
{
	return (' ' == ch || '\t' == ch || '\n' == ch || '\r' == ch);
}

static inline UniChar AdvanceToNextNonSimpleWhitespace(LexerContext& c)
{
	auto ch = c.GetCurrent();
	while (IsSimpleWhiteSpace(ch))
	{
		ch = c.Advance();
	}

	return ch;
}

static inline UniChar SkipAttributeValue(LexerContext& c)
{
	// Skip the '='.
	auto ch = c.Advance();
	// Skip whitespace.
	ch = AdvanceToNextNonSimpleWhitespace(c);

	// Quotes - we're in a quote string. Otherwise, we're looking for the next white space.
	if ('\'' == ch || '"' == ch)
	{
#if SEOUL_LOGGING_ENABLED
		// Cache for reporting.
		auto const chStart = ch;
#endif

		// Skip the quote.
		ch = c.Advance();
		// Escape tracking.
		Bool bEscaped = false;

		// Iterate until we hit a terminating quote.
		while (bEscaped || ('\'' != ch && '"' != ch && '\0' != ch))
		{
			// Update escaping state.
			if (bEscaped)
			{
				bEscaped = false;
			}
			else if ('\\' == ch)
			{
				bEscaped = true;
			}

			// Advance.
			ch = c.Advance();
		}

		// Failed to find a reasonable terminator, warn.
		if ('\'' != ch && '"' != ch)
		{
			SEOUL_WARN("|%.*s|: end of string without finding close for opening %c",
				(Int)(c.GetStream() - c.GetStreamBegin()),
				c.GetStreamBegin(),
				chStart);
			return ch;
		}

		// Otherwise, skip the terminator, then advance passed white space.
		ch = c.Advance();
		ch = AdvanceToNextNonSimpleWhitespace(c);
	}
	// Otherwise, advance until we hit white space.
	else
	{
		while (
			!IsSimpleWhiteSpace(ch) &&
			'\0' != ch &&
			'/' != ch &&
			'\\' != ch &&
			'>' != ch)
		{
			ch = c.Advance();
		}

		// Finally, skip any trailing whitespace.
		ch = AdvanceToNextNonSimpleWhitespace(c);
	}

	return ch;
}

static inline Bool IsAlwaysSelfTerminating(HtmlTag eTag)
{
	switch (eTag)
	{
	case HtmlTag::kBr:
	case HtmlTag::kImg:
	case HtmlTag::kVerticalCentered:
		return true;
	default:
		return false;
	};
}

static inline HashTable<HString, HtmlTag, MemoryBudgets::Falcon> GetTagLookup()
{
	HashTable<HString, HtmlTag, MemoryBudgets::Falcon> t;
	t.Insert(HString("b", true), HtmlTag::kB);
	t.Insert(HString("br", true), HtmlTag::kBr);
	t.Insert(HString("font", true), HtmlTag::kFont);
	t.Insert(HString("i", true), HtmlTag::kI);
	t.Insert(HString("img", true), HtmlTag::kImg);
	t.Insert(HString("a", true), HtmlTag::kLink);
	t.Insert(HString("p", true), HtmlTag::kP);
	t.Insert(HString("vertical_centered", true), HtmlTag::kVerticalCentered);
	t.Insert(HString("vertically_centered", true), HtmlTag::kVerticalCentered);
	return t;
}

static const HashTable<HString, HtmlTag, MemoryBudgets::Falcon> s_tTagLookup(GetTagLookup());

static inline HtmlTag ToTag(Byte const* sBegin, Byte const* sEnd)
{
	HString tag;
	if (!HString::Get(tag, sBegin, (UInt32)(sEnd - sBegin), true))
	{
		return HtmlTag::kUnknown;
	}

	auto eTag = HtmlTag::kUnknown;
	(void)s_tTagLookup.GetValue(tag, eTag);
	return eTag;
}

static inline UniChar ConsumeTag(
	LexerContext& c,
	HtmlTag& reTag,
	HtmlTagStyle& reStyle)
{
	// Sanity - only valid to call ConsumeTag if at a '<'.
	SEOUL_ASSERT('<' == c.GetCurrent());
	// Skip the '<'
	auto ch = c.Advance();

	// Advance for tag detection.
	ch = AdvanceToNextNonSimpleWhitespace(c);

	// Separate terminator.
	//
	// TODO: interpreting a backslash as a terminator slash is flexibility
	// I hate but necessary given lots of typos.
	if ('/' == ch || '\\' == ch)
	{
		// Track.
		reStyle = HtmlTagStyle::kTerminator;

		// Skip the terminator indicator.
		ch = c.Advance();

		// Advance for tag detection.
		ch = AdvanceToNextNonSimpleWhitespace(c);
	}

	// Find the range of the tag itself.
	auto const sBegin = c.GetStream();
	while (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ('_' == ch))
	{
		ch = c.Advance();
	}
	auto const sEnd = c.GetStream();

	// Record.
	reTag = ToTag(sBegin, sEnd);

	// Advance for tag detection.
	ch = AdvanceToNextNonSimpleWhitespace(c);

	// Self terminating.
	//
	// TODO: interpreting a backslash as a terminator slash is flexibility
	// I hate but necessary given lots of typos.
	if ('/' == ch || '\\' == ch)
	{
		// Track.
		reStyle = HtmlTagStyle::kSelfTerminating;

		// Skip the terminator indicator.
		ch = c.Advance();

		// Advance for tag detection.
		ch = AdvanceToNextNonSimpleWhitespace(c);
	}

	return ch;
}

static inline HashTable<HString, HtmlAttribute, MemoryBudgets::Falcon> GetAttrLookup()
{
	HashTable<HString, HtmlAttribute, MemoryBudgets::Falcon> t;
	t.Insert(HString("align", true), HtmlAttribute::kAlign);
	t.Insert(HString("color", true), HtmlAttribute::kColor);
	t.Insert(HString("color_bottom", true), HtmlAttribute::kColorBottom);
	t.Insert(HString("color_top", true), HtmlAttribute::kColorTop);
	t.Insert(HString("effect", true), HtmlAttribute::kEffect);
	t.Insert(HString("face", true), HtmlAttribute::kFace);
	t.Insert(HString("height", true), HtmlAttribute::kHeight);
	t.Insert(HString("hoffset", true), HtmlAttribute::kHoffset);
	t.Insert(HString("hspace", true), HtmlAttribute::kHspace);
	t.Insert(HString("href", true), HtmlAttribute::kHref);
	t.Insert(HString("letterSpacing", true), HtmlAttribute::kLetterSpacing);
	t.Insert(HString("size", true), HtmlAttribute::kSize);
	t.Insert(HString("src", true), HtmlAttribute::kSrc);
	t.Insert(HString("type", true), HtmlAttribute::kType);
	t.Insert(HString("voffset", true), HtmlAttribute::kVoffset);
	t.Insert(HString("vspace", true), HtmlAttribute::kVspace);
	t.Insert(HString("width", true), HtmlAttribute::kWidth);
	return t;
}

static const HashTable<HString, HtmlAttribute, MemoryBudgets::Falcon> s_tAttrLookup(GetAttrLookup());

static inline HtmlAttribute ToAttribute(Byte const* sBegin, Byte const* sEnd)
{
	HString attr;
	if (!HString::Get(attr, sBegin, (UInt32)(sEnd - sBegin), true))
	{
		return HtmlAttribute::kUnknown;
	}

	auto eAttr = HtmlAttribute::kUnknown;
	(void)s_tAttrLookup.GetValue(attr, eAttr);
	return eAttr;
}

static inline UniChar UTF16ToUniChar(UInt32 lo, UInt32 hi)
{
	if (lo < 0xD800 || lo >= 0xE000)
	{
		return (UniChar)lo;
	}
	else
	{
		return (UniChar)(0x10000 + ((lo & 0x03FF) << 10) + (hi & 0x03FF));
	}
}

static inline HashTable<HString, UniChar, MemoryBudgets::Falcon> GetCharRefLookup()
{
	HashTable<HString, UniChar, MemoryBudgets::Falcon> t;

#define SEOUL_CHAR_REF_ACTION(str, lo, hi) \
SEOUL_VERIFY(t.Insert(HString(#str), UTF16ToUniChar((lo), (hi))).Second);

#include "HtmlCharRefInternal.h"

#undef SEOUL_CHAR_REF_ACTION

	return t;
}

static const HashTable<HString, UniChar, MemoryBudgets::Falcon> s_tCharRefLookup(GetCharRefLookup());

static inline Bool FromHtmlCharRef(Byte const* sBegin, Byte const* sEnd, UniChar& rc)
{
	// NOTE: Unlike other similar cases, car ref is case sensitive - this is required.
	// There are symbolic cases (e.g. &Downarrow; and &DoubleDownArrow; that differ
	// only by case but resolve to unique character values).
	HString ref;
	if (!HString::Get(ref, sBegin, (UInt32)(sEnd - sBegin)))
	{
		return false;
	}

	return s_tCharRefLookup.GetValue(ref, rc);
}

Bool HtmlReader::ReadAttribute(HtmlAttribute& re, HtmlTagStyle& reStyle)
{
	// Sanity, mis-use of the API.
	SEOUL_ASSERT(m_bInTag);

	// If we're at a '=', it means the previous attribute was skipped,
	// so skip the value.
	auto ch = m_Context.GetCurrent();
	if ('=' == ch)
	{
		ch = SkipAttributeValue(m_Context);
	}

	// If we're at a '/' or a '\\', we're at a self-terminating node end.
	//
	// TODO: interpreting a backslash as a terminator slash is flexibility
	// I hate but necessary given lots of typos.
	if ('/' == ch || '\\' == ch)
	{
		// Update.
		reStyle = HtmlTagStyle::kSelfTerminating;

		// Advance to next.
		ch = m_Context.Advance();
		ch = AdvanceToNextNonSimpleWhitespace(m_Context);

		// Warn if not what we expect.
		if ('>' != ch)
		{
			SEOUL_WARN("|%.*s|: unexpected character '%c' encountered at expected tag end (expected a '>').",
				(Int)(m_Context.GetStream() - m_Context.GetStreamBegin()) + 1, // Include the unexpected character.
				m_Context.GetStreamBegin(),
				ch);

			// No longer in a tag.
			m_bInTag = false;

			// Done.
			return false;
		}
	}

	// Check if we're at a tag terminator, if so, skip and return false.
	if ('>' == ch || '\0' == ch)
	{
		ch = m_Context.Advance();

		// No longer in a tag.
		m_bInTag = false;

		// Done.
		return false;
	}

	// Get the attribute name.
	auto const sBegin = m_Context.GetStream();
	while ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || '_' == ch)
	{
		ch = m_Context.Advance();
	}
	auto const sEnd = m_Context.GetStream();

	// Failed to advance, so fail the attribute parse attempt.
	if (sBegin == sEnd)
	{
		SEOUL_WARN("|%.*s|: unexpected character '%c', expected attribute name.",
			(Int)(m_Context.GetStream() - m_Context.GetStreamBegin()) + 1, // include the char that triggered the termination.
			m_Context.GetStreamBegin(),
			ch);

		// Done - no other state mutation since we're not entirely sure what's going on.
		return false;
	}

	// Get the attribute type.
	re = ToAttribute(sBegin, sEnd);

	// Advance to next delimiter.
	AdvanceToNextNonSimpleWhitespace(m_Context);

	return true;
}

void HtmlReader::ReadAttributeValue(HtmlAlign& re, HtmlAlign eDefaultValue /*= Align::kLeft*/)
{
	Byte const* sBegin = nullptr, *sEnd = nullptr;
	ReadAttributeRawValue(sBegin, sEnd);

	re = ParseAlignment(sBegin, sEnd, eDefaultValue);
}

void HtmlReader::ReadAttributeValue(Float32& rf, Float32 fDefaultValue /*= 0.0f*/)
{
	Byte const* sBegin = nullptr, *sEnd = nullptr;
	ReadAttributeRawValue(sBegin, sEnd);

	if (sBegin == sEnd)
	{
		rf = fDefaultValue;
		return;
	}

	Byte* sEndPtr = nullptr;
	Double const f = STRTOD(sBegin, &sEndPtr);
	if (sEndPtr && sEndPtr <= sEnd)
	{
		rf = (Float32)f;
	}
	else
	{
		rf = fDefaultValue;
	}
}

void HtmlReader::ReadAttributeValue(HString& rh)
{
	Byte const* sBegin = nullptr, *sEnd = nullptr;
	ReadAttributeRawValue(sBegin, sEnd);

	rh = HString(sBegin, (UInt32)(sEnd - sBegin));
}

void HtmlReader::ReadAttributeValue(HtmlImageAlign& re, HtmlImageAlign eDefaultValue /*= ImageAlign::kMiddle*/)
{
	Byte const* sBegin = nullptr, *sEnd = nullptr;
	ReadAttributeRawValue(sBegin, sEnd);

	re = ParseImageAlignment(sBegin, sEnd, eDefaultValue);
}

void HtmlReader::ReadAttributeValue(Int32& ri, Int32 iDefaultValue /*= 0*/)
{
	Byte const* sBegin = nullptr, *sEnd = nullptr;
	ReadAttributeRawValue(sBegin, sEnd);

	if (sBegin == sEnd)
	{
		ri = iDefaultValue;
		return;
	}

	Byte* sEndPtr = nullptr;
	Int64 const i = STRTOINT64(sBegin, &sEndPtr, 0);
	if (sEndPtr && sEndPtr <= sEnd)
	{
		ri = (Int32)i;
	}
	else
	{
		ri = iDefaultValue;
	}
}

void HtmlReader::ReadAttributeValue(RGBA& rc, RGBA defaultValue /*= RGBA::Black()*/)
{
	Byte const* sBegin = nullptr, *sEnd = nullptr;
	ReadAttributeRawValue(sBegin, sEnd);

	rc = ParseColor(sBegin, sEnd, defaultValue);
}

void HtmlReader::ReadAttributeValue(String& rs)
{
	Byte const* sBegin = nullptr, *sEnd = nullptr;
	ReadAttributeRawValue(sBegin, sEnd);

	rs.Assign(sBegin, (UInt32)(sEnd - sBegin));
}

void HtmlReader::ReadAttributeRawValue(Byte const*& rsBegin, Byte const*& rsEnd)
{
	auto ch = m_Context.GetCurrent();
	// Unexpected, not at a '='.
	if ('=' != ch)
	{
		return;
	}

	// Advance, now find start.
	ch = m_Context.Advance();
	ch = AdvanceToNextNonSimpleWhitespace(m_Context);

	// Quotes - we're in a quote string. Otherwise, we're looking for the next white space.
	if ('\'' == ch || '"' == ch)
	{
#if SEOUL_LOGGING_ENABLED
		// Cache for reporting.
		auto const chStart = ch;
#endif

		// Skip the quote.
		ch = m_Context.Advance();
		// Mark beginning.
		rsBegin = m_Context.GetStream();
		// Escape tracking.
		Bool bEscaped = false;

		// Iterate until we hit a terminating quote.
		while (bEscaped || ('\'' != ch && '"' != ch && '\0' != ch))
		{
			// Update escaping state.
			if (bEscaped)
			{
				bEscaped = false;
			}
			else if ('\\' == ch)
			{
				bEscaped = true;
			}

			// Advance.
			ch = m_Context.Advance();
		}

		// Mark end.
		rsEnd = m_Context.GetStream();

		// Failed to find a reasonable terminator, warn.
		if ('\'' != ch && '"' != ch)
		{
			SEOUL_WARN("|%.*s|: end of string without finding close for opening %c",
				(Int)(m_Context.GetStream() - m_Context.GetStreamBegin()),
				m_Context.GetStreamBegin(),
				chStart);
		}
		// Otherwise, skip the terminator, then advance passed white space.
		else
		{
			ch = m_Context.Advance();
			ch = AdvanceToNextNonSimpleWhitespace(m_Context);
		}
	}
	// Otherwise, advance until we hit white space.
	else
	{
		// Mark the beginning.
		rsBegin = m_Context.GetStream();
		while (
			!IsSimpleWhiteSpace(ch) &&
			'\0' != ch &&
			'/' != ch &&
			'\\' != ch &&
			'>' != ch)
		{
			ch = m_Context.Advance();
		}

		// Mark the end.
		rsEnd = m_Context.GetStream();

		// Finally, skip any trailing whitespace.
		ch = AdvanceToNextNonSimpleWhitespace(m_Context);
	}
}

void HtmlReader::ReadTag(HtmlTag& reTag, HtmlTagStyle& reStyle)
{
	// Skip attributes as needed until we reach tag end.
	while (m_bInTag)
	{
		HtmlAttribute eUnused;
		HtmlTagStyle eStyleUnused;
		if (!ReadAttribute(eUnused, eStyleUnused))
		{
			m_bInTag = false;
			break;
		}
	}

	// Must be at an opening '<' to find a real tag, otherwise this is considered a
	// text chunk.
	reStyle = HtmlTagStyle::kNone; // Initial value.
	if ('<' != m_Context.GetCurrent())
	{
		reTag = HtmlTag::kTextChunk;
	}
	else
	{
		// (Robustness) - handle cases like (e.g.) "<1m" - basically, try to parse
		// the tag name - if this results in an unknown tag, treat the next block as
		// a text chunk instead. So we need to roll back the context if that happens.
		auto const backup = m_Context;

		// Now inside of a tag.
		m_bInTag = true;

		// Get the tag data.
		auto ch = ConsumeTag(m_Context, reTag, reStyle);

		// If tag is unknown, treat this as
		// a text chunk instead.
		if (HtmlTag::kUnknown == reTag)
		{
			reTag = HtmlTag::kTextChunk;
			reStyle = HtmlTagStyle::kNone;
			m_Context = backup;
			m_bInTag = false;
			return;
		}

		if (HtmlTagStyle::kTerminator == reStyle)
		{
			// Exit the tag if we've arrived at the end (no attributes).
			// This is only done for the terminator case - self terminating
			// and regular are openers and will be further processed by
			// a read attributes loop.
			if ('>' == ch)
			{
				// Don't skip white space, we want that as part of the next
				// text chunk, if there is one.
				ch = m_Context.Advance();

				// No longer in a tag.
				m_bInTag = false;
			}
		}

		// Final support - certain tags are self terminating if not
		// otherwise marked. This allows for (e.g.) <img> instead of <img/>
		if (HtmlTagStyle::kNone == reStyle &&
			IsAlwaysSelfTerminating(reTag))
		{
			reStyle = HtmlTagStyle::kSelfTerminating;
		}
	}
}

Bool HtmlReader::ReadTextChunk(String::Iterator& riBegin, String::Iterator& riEnd)
{
	// Sanity, mis-use of the API.
	SEOUL_ASSERT(!m_bInTag);

	Bool bNeedsEscaping = false; // false by default.

	// Find begin and end.
	auto const sBegin = m_Context.GetStream();
	auto ch = m_Context.GetCurrent();
	while ('\0' != ch)
	{
		// Robustness special case - we need to determine
		// if the '<' we've encountered opens a valid
		// tag - if not, we keep reading as a text chunk.
		if ('<' == ch)
		{
			auto tester = m_Context;
			HtmlTag eTag = HtmlTag::kUnknown;
			HtmlTagStyle eUnusedStyle;
			(void)ConsumeTag(tester, eTag, eUnusedStyle);

			// Break immediately if we've hit a valid tag, otherwise continue
			// to consume as a text chunk.
			if (HtmlTag::kUnknown != eTag)
			{
				break;
			}
		}

		// Just assume - '&' is a rare character if we're not
		// encountering an escape sequence.
		if ('&' == ch)
		{
			bNeedsEscaping = true;
		}

		ch = m_Context.Advance();
	}
	auto const sEnd = m_Context.GetStream();

	// Early out if no body.
	if (sBegin == sEnd)
	{
		return false;
	}

	// If we don't need to escape, just append the text to plain text output
	// and setup the iterators.
	String::Iterator iBegin;
	String::Iterator iEnd;
	if (!bNeedsEscaping)
	{
		auto const uBegin = m_rsOut.GetSize();
		m_rsOut.Append(sBegin, (UInt32)(sEnd - sBegin));
		auto const uEnd = m_rsOut.GetSize();

		iBegin = String::Iterator(m_rsOut.CStr(), uBegin);
		iEnd = String::Iterator(m_rsOut.CStr(), uEnd);
	}
	// Otherwise, append a char at a time and handle HTML escape sequences.
	else
	{
		LexerContext context;
		context.SetStream(sBegin, (UInt32)(sEnd - sBegin));

		auto const uBegin = m_rsOut.GetSize();
		while (context.IsStreamValid())
		{
			auto const ch = context.GetCurrent();
			if ('&' == ch)
			{
				// Look for an escaped HTML character reference - if we fail to find a matching pattern,
				// this is not an error, the text is just appended verbatim.
				auto const backup = context;

				// Possible unicode lookup.
				Bool bCodepoint = false;
				auto val = context.Advance();
				Byte const* sBegin = nullptr;
				if ('#' == val)
				{
					bCodepoint = true;
					val = context.Advance();

					// Mark begin.
					sBegin = context.GetStream();

					// TODO: Need confirmation (from standards)
					// if this is always a decimal, or if it can also
					// be octal or hex. Currently assuming it can be all
					// three.
					while (
						('0' <= val && val <= '9') || // decimal or hex.
						('a' <= val && val <= 'f') || // hex.
						('A' <= val && val <= 'F') || // hex.
						('x' == val)) // hex.
					{
						val = context.Advance();
					}
				}
				// Otherwise, looking for one of the symbolic constants.
				else
				{
					// Mark begin.
					sBegin = context.GetStream();

					while (('a' <= val && val <= 'z') || ('A' <= val && val <= 'Z'))
					{
						val = context.Advance();
					}
				}

				// If we found a ';', then this is a possible escape.
				if (';' == val)
				{
					// Mark end.
					Byte const* sEnd = context.GetStream();

					// Advance to next.
					val = context.Advance();

					// Codepoint, convert into a number and then UniChar.
					if (bCodepoint)
					{
						Byte* sEndPtr = nullptr;
						Int64 const i = STRTOINT64(sBegin, &sEndPtr, 0);
						if (sEndPtr && sEndPtr <= sEnd)
						{
							auto const ch = (UniChar)i;
							if (IsValidUnicodeChar(ch))
							{
								// Add the character, then continue - we've
								// already advanced to the next real character.
								m_rsOut.Append(ch);
								continue;
							}
						}
					}
					// Otherwise, table lookup.
					else
					{
						UniChar ch = 0;
						if (FromHtmlCharRef(sBegin, sEnd, ch))
						{
							// Add the character, then continue - we've
							// already advanced to the next real character.
							m_rsOut.Append(ch);
							continue;
						}
					}
				}

				// If we get here, it means we didn't successfully parse - restore
				// the backup and fall through. This will append the characters verbatim.
				context = backup;
			}

			m_rsOut.Append(ch);
			context.Advance();
		}
		auto const uEnd = m_rsOut.GetSize();

		iBegin = String::Iterator(m_rsOut.CStr(), uBegin);
		iEnd = String::Iterator(m_rsOut.CStr(), uEnd);
	}

	// Out.
	riBegin = iBegin;
	riEnd = iEnd;

	// Check.
	return (iEnd != iBegin);
}

} // namespace Seoul
