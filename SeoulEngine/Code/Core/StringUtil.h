/**
 * \file StringUtil.h
 * \brief Miscellaneous conversion, generation, and cleaning functions
 * for Seoul::String.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "SeoulMath.h"
#include "SeoulString.h"
#include "SeoulTypes.h"
#include "Vector.h"
#include <wctype.h>

namespace Seoul
{

/**
 * Character encoding constants
 */
enum CharacterEncoding
{
	EncodingISO88591,
	EncodingWindows1252,
	EncodingUTF8,
	EncodingUTF16,
	EncodingUTF16LE,
	EncodingUTF16BE
};
// XXX: Should we add support for other encodings such as UTF-32?

extern const UniChar g_aWindows1252CodePoints_80_9F[];


/**
 * Tests if the given code point is a valid Unicode code point
 *
 * Tests if the given code point is a valid Unicode code point.  This is NOT an
 * exhaustive test - it only rejects certain ranges of characters which are
 * guaranteed not to be valid code points.  This may return true for code points
 * which may in the future become valid code points.  If false is returned, the
 * code point is definitely invalid.
 *
 * @param[in] chChar Code point to test
 *
 * @return True if chChar is is a valid Unicode code point, or false
 *         otherwise
 *
 * \sa http://unicode.org/faq/utf_bom.html#40
 */
inline Bool IsValidUnicodeChar(UniChar chChar)
{
	if (chChar < 0 || chChar > 0x10FFFF)
	{
		return false;
	}

	if (chChar >= 0xD800 && chChar <= 0xDFFF)
	{
		return false;
	}

	if (chChar == 0xFFFE || chChar == 0xFFFF)
	{
		return false;
	}

	if (chChar >= 0xFDD0 && chChar <= 0xFDEF)
	{
		return false;
	}

	return true;
}

/**
 * Tests if the given byte sequence is a valid UTF-8 string
 *
 * Tests if the given byte sequence is a valid UTF-8 string.  This does NOT
 * validate the individual code points.
 *
 * @param[in] sStr  Byte sequence to test
 * @param[in] zSize Length of byte sequence to test
 *
 * @return True if the given byte sequence represents a valid UTF-8 string,
 *         or false otherwise.  If sStr is nullptr, then false is
 *         returned.
 */
inline Bool IsValidUTF8String(const Byte *sStr, UInt zSize = (UInt)-1)
{
	if (sStr == nullptr)
	{
		return false;
	}

	const Byte *sEnd = (zSize == UIntMax) ? (const Byte *)-1 : (sStr + zSize);

	Byte chChar;
	while((sStr < sEnd) && ((chChar = *sStr) != '\0'))  // Intentional single '='
	{
		sStr++;

		if ((chChar & 0x80) == 0x00)  // 1-byte character (0xxxxxxxx)
		{
			// Do nothing (this test first for optimization for the common case)
		}
		else if ((chChar & 0xE0) == 0xC0)  // 2-byte character (110xxxxx)
		{
			if (sStr >= sEnd)
			{
				return false;
			}

			if ((*sStr++ & 0xC0) != 0x80)
			{
				return false;
			}
		}
		else if ((chChar & 0xF0) == 0xE0)  // 3-byte character (1110xxxx)
		{
			if (sStr + 1 >= sEnd)
			{
				return false;
			}

			if ((*sStr++ & 0xC0) != 0x80)
			{
				return false;
			}

			if ((*sStr++ & 0xC0) != 0x80)
			{
				return false;
			}
		}
		else if ((chChar & 0xF8) == 0xF0)  // 4-byte character (11110xxx)
		{
			if (sStr + 2 >= sEnd)
			{
				return false;
			}

			if ((*sStr++ & 0xC0) != 0x80)
			{
				return false;
			}

			if ((*sStr++ & 0xC0) != 0x80)
			{
				return false;
			}

			if ((*sStr++ & 0xC0) != 0x80)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	return true;
}

// Return the number of Unicode characters in sStr, where
// the input size in bytes is zSizeInBytes.
UInt UTF8Strlen(const Byte *sStr, UInt zSizeInBytes);

/** Convenience version of UTF8Strlen that derives the length of sStr. */
inline UInt UTF8Strlen(const Byte *sStr)
{
	return UTF8Strlen(sStr, StrLen(sStr));
}

/**
 * Calculates the number of bytes needed to encode the given character
 *        in UTF-8.
 *
 * Calculates the number of bytes needed to encode the given character in UTF-8.
 * If the given character is not a valid Unicode character, the results are
 * undefined.
 *
 * @param[in] chChar Unicode character to be encoded
 *
 * @return The number of bytes needed to encode chChar in UTF-8
 */
inline Byte UTF8BytesPerChar(UniChar chChar)
{
	if (chChar < 0x0080)
	{
		return 1;
	}
	else if (chChar < 0x0800)
	{
		return 2;
	}
	else if (chChar < 0x10000)
	{
		return 3;
	}
	else
	{
		return 4;
	}
}

UInt UTF8ToISO88591(const Byte *sInStr, Byte *sOutStr, UInt zOutSize, Byte chReplacementChar = '?');
UInt UTF8ToWindows1252(const Byte *sInStr, Byte *sOutStr, UInt zOutSize, Byte chReplacementChar = '?');
UInt UTF8ToUTF16(const Byte *sInStr, WChar16 *sOutStr, UInt zOutSize);
UInt UTF8ToWCharT(const Byte *sInStr, wchar_t *sOutStr, UInt zOutSize);

String ISO88591ToUTF8(const Byte *sInStr);
String Windows1252ToUTF8(const Byte *sInStr);
String UTF16ToUTF8(const WChar16 *sInStr);
String WCharTToUTF8(const wchar_t *sInStr);

UInt TranslateStringToUTF8(const Byte *sInStr, UInt zInSize, Byte *sOutStr, UInt zOutSize, CharacterEncoding eFromEncoding, Bool bTranslateCRLFs, UInt *pzBytesConsumed);

// Encode/decode base64 data
String Base64Encode(const void *pData, UInt32 zDataLength, Bool bURLSafe = false);
Bool Base64Decode(const String& sString, Vector<Byte>& vOutData);

// Encode a vector of Byte/UByte as base64
template <int MEMORY_BUDGETS>
inline String Base64Encode(const Vector<Byte, MEMORY_BUDGETS>& vData, Bool bURLSafe = false)
{
	return vData.IsEmpty() ? String() : Base64Encode(vData.Get(0), vData.GetSize(), bURLSafe);
}

template <int MEMORY_BUDGETS>
inline String Base64Encode(const Vector<UByte, MEMORY_BUDGETS>& vData, Bool bURLSafe = false)
{
	return vData.IsEmpty() ? String() : Base64Encode(vData.Get(0), vData.GetSize(), bURLSafe);
}

inline String Base64Encode(const String& sData, Bool bURLSafe = false)
{
	return sData.IsEmpty() ? String() : Base64Encode(sData.CStr(), sData.GetSize(), bURLSafe);
}

// Converts a charcter in [0-9a-fA-F] to a uint8 from 0 to 15
UInt8 DecodeHexChar(Byte cChar);

// Decode %XX hex URL escapes
String URLDecode(const Byte *sInStr);

String TrimWhiteSpace(const String & sString);

Bool IsIPAddress(const String & sString);

/**
 * @return True if the Unicode character c is a space character,
 * false otherwise.
 */
inline Bool IsSpace(UniChar c)
{
	// TODO: This is not necessarily correct on platforms with 16-bit wchar_t types.
	return (0 != iswspace((wchar_t)c));
}

// Counts the number of occurrences of the given character in the string
Int CountOccurrences(const String& sString, UniChar c);

/**
 * Splits a string on a delimeter
 *
 * Splits a string on a delimeter into an array of strings.
 *
 * @param[in] sString String to split
 * @param[in] chDelim Character to split the string on
 * @param[out] rvsTokens Receives array of tokens that the string is split into
 * @param[in] bExcludeEmpty (Optional) Exclude entries that are the empty string.
 */

template <int MEMORY_BUDGETS>
void SplitString(
	const String& sString,
	UniChar chDelim,
	Vector<String, MEMORY_BUDGETS>& rvsTokens,
	Bool bExcludeEmpty = false)
{
	UInt nDelimPos = 0, nNextDelimPos;
	UInt nDelimLen = UTF8BytesPerChar(chDelim);

	rvsTokens.Clear();

	while (true)
	{
		nNextDelimPos = sString.Find(chDelim, nDelimPos);

		if (nNextDelimPos == String::NPos)
		{
			rvsTokens.PushBack(sString.Substring(nDelimPos));
			if (bExcludeEmpty && rvsTokens.Back().IsEmpty())
			{
				rvsTokens.PopBack();
			}
			break;
		}
		else
		{
			rvsTokens.PushBack(sString.Substring(nDelimPos, nNextDelimPos - nDelimPos));
			if (bExcludeEmpty && rvsTokens.Back().IsEmpty())
			{
				rvsTokens.PopBack();
			}
			nDelimPos = nNextDelimPos + nDelimLen;
		}
	}
}

// Replacements for strncpy and strncat which are guaranteed to null-terminate
// the output
Byte *StrNCopy(Byte *sDest, const Byte *sSrc, size_t zSize);
Byte *StrNCat(Byte *sDest, const Byte *sSrc, size_t zSize);

// Returns a value on [0, 1] indicating "how closely" sStringA matches sStringB.
// This can be used to determine, for example, how likely a key in input data
// matches an unmatched key in target data.
//
// From: Boer, J. 2006. "Closest-String Matching Algorithm".
//           Game Programming Gems 6.
Float ComputeStringMatchFactor(
	Byte const* sStringA, UInt32 zStringALengthInBytes,
	Byte const* sStringB, UInt32 zStringBLengthInBytes);

inline Float ComputeStringMatchFactor(const String& sStringA, const String& sStringB)
{
	return ComputeStringMatchFactor(
		sStringA.CStr(), sStringA.GetSize(),
		sStringB.CStr(), sStringB.GetSize());
}

inline Float ComputeStringMatchFactor(Byte const* sStringA, Byte const* sStringB)
{
	return ComputeStringMatchFactor(
		sStringA, StrLen(sStringA),
		sStringB, StrLen(sStringB));
}

/**
 * @return The length of sString after escaping, according to JSON escaping rules.
 *
 * \pre sString must be non-null.
 */
inline UInt32 JSONEscapedLength(Byte const* sString, UInt32 zUnescapedStringLengthInBytes)
{
	SEOUL_ASSERT(nullptr != sString);

	// Mark our end point.
	Byte const* const sEnd = (sString + zUnescapedStringLengthInBytes);
	UInt32 zEscapedLength = 0u;
	while (sString < sEnd)
	{
		// Cache the current character and its size.
		UniChar const c = UTF8DecodeChar(sString);
		UInt const zBytesPerChar = UTF8BytesPerChar(c);
		sString += zBytesPerChar;
		SEOUL_ASSERT(sString <= sEnd);

		switch (c)
		{
			// Any characters that must be escaped will output 2 bytes for
			// each input bytes (and all special cases are 1 byte).
		case '"': // fall-through
		case '\\': // fall-through
		case '\b': // fall-through
		case '\f': // fall-through
		case '\n': // fall-through
		case '\r': // fall-through
		case '\t':
			zEscapedLength += 2u;
			break;

		default:
			// Values less than 0x20 (a space) are converted into a \u escape.
			if (c < 0x20)
			{
				zEscapedLength += 6u;
			}
			else
			{
				// All other characters just output exactly.
				zEscapedLength += zBytesPerChar;
			}
			break;
		};
	}

	// Return the total escaped length.
	return zEscapedLength;
}

/**
 * @return Escape the string contained in sString to sOut, up to
 * zEscapedStringBufferSizeInBytes - 1u bytes.
 *
 * \warning NOTE that zUnescapedStringBufferSizeInBytes is the buffer size, NOT the output string size.
 * This value is the output of JSONEscapedLength() + 1.
 *
 * \pre zEscapedStringBufferSizeInBytes >= 1u
 * \pre sString non-null
 * \pre sOut non-null
 * \pre sString != sOut
 *
 * \warning sOut must be exactly large enough to contain the escaped output of sString.
 * zEscapedStringBufferSizeInBytes should be the output of JSONEscapedLength() + 1.
 */
inline void JSONEscape(Byte const* sString, Byte* sOut, UInt32 zEscapedStringBufferSizeInBytes)
{
	// Sanity checks.
	SEOUL_ASSERT(zEscapedStringBufferSizeInBytes > 0u);
	SEOUL_ASSERT(nullptr != sString);
	SEOUL_ASSERT(nullptr != sOut);

	//Mark the end point.
	Byte const* const sOutEnd = (sOut + zEscapedStringBufferSizeInBytes - 1u);
	while (sOut < sOutEnd)
	{
		// Consume the next character.
		UniChar const c = UTF8DecodeChar(sString);
		UInt const zBytesPerChar = UTF8BytesPerChar(c);
		sString += zBytesPerChar;

		switch (c)
		{
			// Special values write out 2 bytes, the backslash and the actual character associated
			// with the special code.
		case '"':  *sOut++ = '\\'; *sOut++ = '"'; break;
		case '\\': *sOut++ = '\\'; *sOut++ = '\\'; break;
		case '\b':  *sOut++ = '\\'; *sOut++ = 'b'; break;
		case '\f':  *sOut++ = '\\'; *sOut++ = 'f'; break;
		case '\n':  *sOut++ = '\\'; *sOut++ = 'n'; break;
		case '\r':  *sOut++ = '\\'; *sOut++ = 'r'; break;
		case '\t':  *sOut++ = '\\'; *sOut++ = 't'; break;

			// All other characters are just appended as-is to the output.
		default:
			// Values less than 0x20 (a space) are converted into a \u escape.
			if (c < 0x20)
			{
#				define SEOUL_TO_HEX_CHAR(u) (((u) <= 9) ? ('0' + (u)) : ('a' - 10 + (u)))

				*sOut = '\\'; ++sOut;
				*sOut = 'u'; ++sOut;
				*sOut = '0'; ++sOut;
				*sOut = '0'; ++sOut;
				*sOut = SEOUL_TO_HEX_CHAR((c & 0x00F0) >> 4u); ++sOut;
				*sOut = SEOUL_TO_HEX_CHAR((c & 0x000F) >> 0u); ++sOut;

#				undef SEOUL_TO_HEX_CHAR
			}
			else
			{
				// All other characters just output exactly.
				sOut += UTF8EncodeChar(c, sOut);
			}
			break;
		};
	}

	// Null terminate.
	*sOut = '\0';
}

/**
 * Escape control characters in an input string into a returned output string
 * using HTML escape sequences.
 */
inline static String HTMLEscape(const String& s)
{
	auto const iBegin = s.Begin();
	auto const iEnd = s.End();

	String sReturn;

	// Reserve the typical case size (&#123;).
	sReturn.Reserve(6 * s.GetSize());
	for (auto i = iBegin; iEnd != i; ++i)
	{
		switch (*i)
		{
			// HTML control characters and others likely
			// to be control characters in other contexts.
		case '&': // fall-through
		case '<': // fall-through
		case '>': // fall-through
		case '"': // fall-through
		case '\'': // fall-through
		case '/': // fall-through
		case '\\': // fall-through
		case ';':
			sReturn.Append(String::Printf("&#%u;", *i));
			break;

			// All other characters, copy through.
		default:
			sReturn.Append(*i);
			break;
		};
	}

	return sReturn;
}

// Converts a data buffer into its hexadecimal representation
String HexDump(const void *pData, UInt32 zDataSize, Bool bUppercase = true);

// Converts a hex string of bytes to a byte vector. Must be an even number of characters.
void HexParseBytes(String const& sHex, Vector<UByte>& rvOut);

// Converts a vector of Bytes into its hexadecimal representation
template <int MEMORY_BUDGETS>
inline String HexDump(const Vector<Byte, MEMORY_BUDGETS>& vData, Bool bUppercase = true)
{
	return vData.IsEmpty() ? String() : HexDump(vData.Get(0), vData.GetSize(), bUppercase);
}

// Converts a vector of UBytes into its hexadecimal representation
template <int MEMORY_BUDGETS>
inline String HexDump(const Vector<UByte, MEMORY_BUDGETS>& vData, Bool bUppercase = true)
{
	return vData.IsEmpty() ? String() : HexDump(vData.Get(0), vData.GetSize(), bUppercase);
}

} // namespace Seoul

#endif // include guard
