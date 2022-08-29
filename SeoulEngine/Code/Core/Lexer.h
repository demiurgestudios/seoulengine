/**
 * \file Lexer.h
 * \brief Shared structures and functions for lexer implementations in SeoulEngine.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef LEXER_H
#define LEXER_H

#include "FilePath.h"
#include "Prereqs.h"
#include "StringUtil.h"

namespace Seoul
{

/** UTF8 byte order mark - can appear at the start of UTF8 text input. */
static const UByte kaUTF8Bom[3] = { 239u /* 0xEF */, 187u /* 0xBB */, 191u /* 0xBF */ };

/**
 * Structure used to track and manipulate the input stream used while lexing.
 */
class LexerContext SEOUL_SEALED
{
public:
	// TODO: Tricky - tab width of 4 may not match a user's
	// editor, but we have no way of detecting or specifying it.
	static const Int32 knTabWidth = 4;

	LexerContext()
		: m_iLine(1)
		, m_iColumn(1)
		, m_sStream(nullptr)
		, m_sStreamBegin(nullptr)
		, m_sStreamEnd(nullptr)
		, m_cCurrent(0)
	{
	}

	/**
	 * Advance the stream by 1 character, updates the current
	 * line/column if a newline is encountered.
	 */
	UniChar Advance()
	{
		UniChar const c = m_cCurrent;
		UInt32 const nAdvance = UTF8BytesPerChar(c);

		m_sStream += nAdvance;

		// Advance column by knTabWidth if '\t' == c.
		if ('\t' == c)
		{
			m_iColumn += knTabWidth;
		}
		// Otherwise advance by 1 character.
		else
		{
			++m_iColumn;
		}

		// NOTE: This is invalid if a file uses just '\r' for line terminators,
		// but that's unlikely anymore, since it was only used by old versions
		// of Mac OS.
		if ('\n' == c)
		{
			++m_iLine;
			m_iColumn = 1;
		}

		// Read the next character.
		if (SEOUL_LIKELY(IsStreamValid()))
		{
			m_cCurrent = UTF8DecodeChar(m_sStream);
		}
		else
		{
			m_cCurrent = (UniChar)'\0';
		}

		return m_cCurrent;
	}

	/**
	 * @return The character at the current stream position.
	 */
	UniChar GetCurrent() const
	{
		return m_cCurrent;
	}

	/**
	 * @return the current (0-based) column index of the lexer context.
	 */
	Int32 GetColumn() const
	{
		return m_iColumn;
	}

	/**
	 * @return the current (0-based) line index of the lexer context.
	 */
	Int32 GetLine() const
	{
		return m_iLine;
	}

	/**
	 * Advance the stream by zSizeInBytes.
	 *
	 * \pre zSizeInBytes must leave the stream pointer at at exactly the stream
	 * end, or must leave the stream pointer within the stream at the start of
	 * a valid UniCode character, or the stream will not be advanced by zSizeInBytes.
	 */
	void AdvanceInBytes(UInt32 zSizeInBytes)
	{
		Byte const* const sEnd = (m_sStream + zSizeInBytes);
		while (IsStreamValid() && (m_sStream < sEnd))
		{
			Advance();
		}
	}

	/**
	 * Given a stream position assumed to be within the stream of this LexerContext,
	 * on the same line as this LexerContext, updates the internal stream position
	 * and column index so that it points at s.
	 */
	void AdjustSameLine(Byte const* s)
	{
		SEOUL_ASSERT(s >= m_sStreamBegin && s < m_sStreamEnd);

		if (s > m_sStream)
		{
			m_iColumn += UTF8Strlen(m_sStream, (UInt32)(s - m_sStream));
		}
		else if (s < m_sStream)
		{
			m_iColumn -= UTF8Strlen(s, (UInt32)(m_sStream - s));
		}

		m_sStream = s;

		// Read the next character.
		if (SEOUL_LIKELY(IsStreamValid()))
		{
			m_cCurrent = UTF8DecodeChar(m_sStream);
		}
		else
		{
			m_cCurrent = (UniChar)'\0';
		}
	}

	/**
	 * @return True if the stream of this LexerContext is not at the end of
	 * the stream, and is not pointing at a null terminator '\0'.
	 */
	Bool IsStreamValid() const
	{
		return m_sStream < m_sStreamEnd && *m_sStream;
	}

	/**
	 * @return A point to the head of the stream currently set to this LexerContext.
	 */
	Byte const* GetStreamBegin() const
	{
		return m_sStreamBegin;
	}

	/**
	 * @return A pointer to the current stream position of this LexerContext.
	 */
	Byte const* GetStream() const
	{
		return m_sStream;
	}

	/** @return Size in bytes of the entire stream, from GetStreamBegin() to end. */
	UInt32 GetStreamSizeInBytes() const
	{
		return (UInt32)(m_sStreamEnd - m_sStreamBegin);
	}

	/**
	 * @return A pointer to the end stream position of this LexerContext.
	 */
	Byte const* GetStreamEnd() const
	{
		return m_sStreamEnd;
	}

	/**
	 * Set the stream associated with this LexerContext - assumes that
	 * sStream points at the 0 column and 0 line of the input data.
	 */
	void SetStream(Byte const* sStream, UInt32 zStreamLengthInBytes)
	{
		m_sStream = sStream;
		m_sStreamBegin = m_sStream;
		m_sStreamEnd = (m_sStream + zStreamLengthInBytes);
		m_iLine = 1;
		m_iColumn = 1;

		// Skip the UTF8 BOM if present - the UTF8 byte order mark is the 3-byte sequence 0xEF 0xBB 0xBF
		if (m_sStreamEnd - m_sStream >= 3)
		{
			UByte const c0 = *(m_sStream + 0u);
			UByte const c1 = *(m_sStream + 1u);
			UByte const c2 = *(m_sStream + 2u);

			if (c0 == kaUTF8Bom[0] &&
				c1 == kaUTF8Bom[1] &&
				c2 == kaUTF8Bom[2])
			{
				m_sStream += 3u;
			}
		}

		if (SEOUL_LIKELY(IsStreamValid()))
		{
			m_cCurrent = UTF8DecodeChar(m_sStream);
		}
		else
		{
			m_cCurrent = (UniChar)'\0';
		}
	}

	/**
	 * Forces the column and line indices of this LexerContext to nColumn
	 * and nLine without modifying the stream position.
	 *
	 * Typically you will only use this method when you are creating a
	 * "sub" LexerContext from a parent LexerContext. It will not be used
	 * during normal byte-by-byte lexing.
	 */
	void SetColumnAndLine(Int nColumn, Int nLine)
	{
		m_iColumn = nColumn;
		m_iLine = nLine;
	}

private:
	/** Current (0 based) line number of the lexer. */
	Int32 m_iLine;

	/** Current (0 based) column number of the lexer. */
	Int32 m_iColumn;

	/** Current position of the lexer stream. */
	Byte const* m_sStream;

	/** Begin position of the lexer stream, used for sanity checking. */
	Byte const* m_sStreamBegin;

	/** End position of the lexer stream. */
	Byte const* m_sStreamEnd;

	/** Current character. */
	UniChar m_cCurrent;
}; // class LexerContext

/**
 * Write the numeric value of a single hex digit in ch to
 * rOut.
 *
 * @return True if ch is a valid hex digit, false otherwise. If this
 * function returns false, rOut is left unmodified, otherwise it will
 * contain the numeric value of the hex digit.
 */
inline Bool HexCharToUInt32(Byte ch, UInt32& rOut)
{
	switch (ch)
	{
	case '0': rOut = 0; return true;
	case '1': rOut = 1; return true;
	case '2': rOut = 2; return true;
	case '3': rOut = 3; return true;
	case '4': rOut = 4; return true;
	case '5': rOut = 5; return true;
	case '6': rOut = 6; return true;
	case '7': rOut = 7; return true;
	case '8': rOut = 8; return true;
	case '9': rOut = 9; return true;
	case 'A': rOut = 10; return true;
	case 'B': rOut = 11; return true;
	case 'C': rOut = 12; return true;
	case 'D': rOut = 13; return true;
	case 'E': rOut = 14; return true;
	case 'F': rOut = 15; return true;
	case 'a': rOut = 10; return true;
	case 'b': rOut = 11; return true;
	case 'c': rOut = 12; return true;
	case 'd': rOut = 13; return true;
	case 'e': rOut = 14; return true;
	case 'f': rOut = 15; return true;
	default:
		return false;
	};
}

/**
 * Parse a JSON style unicode escape sequence (\uXXXX) at sString
 * into rOut and output the number of consumed bytes from sString into rzConsumedBytes
 *
 * \pre sString must be non-NULL.
 *
 * @return True if sString contains a valid unicode escape sequence, false otherwise.
 */
inline Bool JSONUnicodeEscapeToUniChar(Byte const* sString, UniChar& rOut, UInt32& rzConsumedBytes)
{
	// Sanity check.
	SEOUL_ASSERT(nullptr != sString);

	// Get the first UTF-16 value from the string - if any byte is an unexpected value, return false.
	UInt u0 = 0;
	UInt u1 = 0;
	UInt u2 = 0;
	UInt u3 = 0;
	if ('\\' != sString[0] ||
		'u' != sString[1] ||
		!HexCharToUInt32(sString[2], u0) ||
		!HexCharToUInt32(sString[3], u1) ||
		!HexCharToUInt32(sString[4], u2) ||
		!HexCharToUInt32(sString[5], u3))
	{
		return false;
	}

	// Construct the value.
	UInt32 const ch0 = (
		((u0 & 0xF) << 12) |
		((u1 & 0xF) <<  8) |
		((u2 & 0xF) <<  4) |
		((u3 & 0xF) <<  0));

	// If the value is in the Basic Multilingual Plane, return as-is.
	if (ch0 <= 0xD7FF || ch0 >= 0xE000)
	{
		rzConsumedBytes = 6u;
		rOut = (UniChar)ch0;
		return true;
	}
	// Otherwise, consume the next Unicode escape character, as it forms the
	// surrogate pair against the first value. If any error occurs while consuming
	// the second escape code, return false.
	else if (
		'\\' != sString[6] ||
		'u' != sString[7] ||
		!HexCharToUInt32(sString[8], u0) ||
		!HexCharToUInt32(sString[9], u1) ||
		!HexCharToUInt32(sString[10], u2) ||
		!HexCharToUInt32(sString[11], u3))
	{
		return false;
	}
	// Construct the surrogate pair and then construct the full UTF-32 value.
	else
	{
		// Low surrogate.
		UInt32 const ch1 = (
			((u0 & 0xF) << 12) |
			((u1 & 0xF) <<  8) |
			((u2 & 0xF) <<  4) |
			((u3 & 0xF) <<  0));

		rzConsumedBytes = 12u;

		// Combine the low and high surrogates into the full UTF-32 value.
		rOut = (UniChar)(0x10000 + ((ch0 & 0x03FF) << 10) + (ch1 & 0x03FF));
		return true;
	}
}

/**
 * Compute the length of the string after unescaping contained in rContext and
 * set it to rzUnescapedStringLengthInBytes, or return false and leave rzUnescapedStringLengthInBytes
 * unassigned if an error occurs.
 *
 * rzUnescapedStringLengthInBytes is the length of the unescaped string EXCLUDING the chTerminator
 * argument.
 */
template <UniChar chTerminator>
inline Bool JSONUnescapedLength(LexerContext& rContext, UInt32& rzUnescapedStringLengthInBytes)
{
	UInt32 zUnescapedLength = 0u;
	Bool bEscaped = false;

	// Consume the stream.
	while (rContext.IsStreamValid())
	{
		// Cache the current character and its size in bytes.
		UniChar const c = rContext.GetCurrent();
		UInt const zBytesPerChar = UTF8BytesPerChar(c);

		// If the previous character was an escape character, handle the current character
		// as possible escape completion.
		if (bEscaped)
		{
			switch (c)
			{
				// 'u' starts a Unicode escape, so the unescaped size varies. We need to actually
				// compute the Unicode character to determine the unescaped size.
			case 'u':
				{
					UniChar chOut = 0;
					UInt32 zConsumedBytes = 0u;
					if (!JSONUnicodeEscapeToUniChar(rContext.GetStream() - 1, chOut, zConsumedBytes))
					{
						return false;
					}

					rContext.AdvanceInBytes(zConsumedBytes - 2);
					zUnescapedLength += UTF8BytesPerChar(chOut);
				}
				break;

				// For all other values, the unescaped size is equal to the single character size - either
				// the unescaped value is a control code (\n, \t, \r, etc.) and its size (1 byte) is equal
				// to the size of its escaped placeholder ('n', 't', 'r', etc.) or the escaped value is
				// exactly equal to the unescaped value, with the '\' character removed.
			default:
				zUnescapedLength += zBytesPerChar;
				break;
			};

			// After processing an escaped character, we are no longer escaping.
			bEscaped = false;
		}
		// Handle situations when not escaping.
		else
		{
			switch (c)
			{
				// If we hit the specified terminator when not escaping, we're done. Return the output length.
			case chTerminator:
				rzUnescapedStringLengthInBytes = zUnescapedLength;
				return true;
				// If we hit a backslash character, we ignore it and enable escaping for the next
				// character.
			case '\\':
				bEscaped = true;
				break;
				// All other characters are consumed exactly.
			default:
				zUnescapedLength += zBytesPerChar;
				break;
			};
		}

		// Consume the processed character from thes tream.
		rContext.Advance();
	}

	// Return the final unescaped string length.
	rzUnescapedStringLengthInBytes = zUnescapedLength;
	return true;
}

/**
 * Escape the string in sString, writing the result to sOut up to zUnescapedStringBufferSizeInBytes - 1.
 * The last character is always the NULL terminator.
 *
 * \warning NOTE that zUnescapedStringBufferSizeInBytes is the buffer size, NOT the output string size.
 * This value is the output of JSONUnescapedLength + 1.
 *
 * \pre zUnescapedStringBufferSizeInBytes must be >= 1u.
 * \pre sString must be non-NULL.
 * \pre sOut must be non-NULL.
 * \pre sString != sOut
 *
 * This function assumes sString is valid and fits exactly into the buffer
 * zUnescapedStringBufferSizeInBytes. To fulfill this, you should call JSONUnescapedLength on
 * sString and pass the result rzUnescapedStringLengthInBytes + 1 to this function.
 */
inline void JSONUnescape(Byte const* sString, Byte* sOut, UInt32 zUnescapedStringBufferSizeInBytes)
{
	SEOUL_ASSERT(zUnescapedStringBufferSizeInBytes > 0u);
	SEOUL_ASSERT(nullptr != sString);
	SEOUL_ASSERT(nullptr != sOut);

	// Mark our stopping point.
	Byte const* const sOutEnd = (sOut + zUnescapedStringBufferSizeInBytes - 1u);
	Bool bEscaped = false;

	// Keep consuming until we hit the end.
	while (sOut < sOutEnd)
	{
		// Cache the character and its size than advance the input.
		UniChar const c = UTF8DecodeChar(sString);
		UInt const zBytesPerChar = UTF8BytesPerChar(c);
		sString += zBytesPerChar;

		// If this character is escaped, handle all escape sequences.
		if (bEscaped)
		{
			switch (c)
			{
				// Any of the below are special cases and map to a control character.
			case '\\': *sOut = '\\'; ++sOut; break;
			case 'b':  *sOut = '\b'; ++sOut; break;
			case 'f':  *sOut = '\f'; ++sOut; break;
			case 'n':  *sOut = '\n'; ++sOut; break;
			case 'r':  *sOut = '\r'; ++sOut; break;
			case 't':  *sOut = '\t'; ++sOut; break;
				// The 'u' character defines a Unicode code point.
			case 'u':
				{
					UniChar chOut = 0;
					UInt32 zConsumedBytes = 0u;
					SEOUL_VERIFY(JSONUnicodeEscapeToUniChar(sString - 2, chOut, zConsumedBytes));

					sString += (zConsumedBytes - 2);
					sOut += UTF8EncodeChar(chOut, sOut);
				}
				break;
				// All other characters are consumed exactly.
			default:
				sOut += UTF8EncodeChar(c, sOut);
				break;
			};

			// After consuming an escaped character, we're done escaping.
			bEscaped = false;
		}
		// If not escaping, all characters are consumed exactly, except for the '\' character,
		// which is skipped but enables escaping of the next character.
		else
		{
			switch (c)
			{
			case '\\':
				bEscaped = true;
				break;
			default:
				sOut += UTF8EncodeChar(c, sOut);
				break;
			};
		}

		// Sanity check - if this fails, someone passed an output buffer that is too small for the
		// input range.
		SEOUL_ASSERT(sOut <= sOutEnd);
	}

	// null terminate
	*sOut = '\0';
}

} // namespace Seoul

#endif // include guard
