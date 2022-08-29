/**
 * \file StringUtil.cpp
 * \brief Miscellaneous conversion, generation, and cleaning functions
 * for Seoul::String.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "SeoulString.h"
#include "StringUtil.h"
#include "Vector.h"

#include <string.h>
#include <wchar.h>

namespace Seoul
{

/**
 * Calculates the length in characters in the given UTF-8 string
 *
 * Calculates the length in characters in the given UTF-8 string.  The given
 * string must be a valid non-null UTF-8 string; if not, the results are
 * undefined.
 *
 * @param[in] sStr UTF-8 string
 * @param[in] zSizeInBytes Length of the input UTF-8 string in bytes.
 *
 * @return Length in characters of the given UTF-8 string
 */
UInt UTF8Strlen(const Byte *sStr, UInt zSizeInBytes)
{
	Byte const* const sEnd = (sStr + zSizeInBytes);
	UInt zLength = 0;

	while(sStr < sEnd)
	{
		Byte const chChar = *sStr++;
		if ((chChar & 0x80) == 0x00)  // 1-byte character (0xxxxxxx)
		{
			zLength++;
		}
		else if ((chChar & 0xE0) == 0xC0)  // 2-byte character (110xxxxx)
		{
			zLength++;
			sStr++;
		}
		else if ((chChar & 0xF0) == 0xE0)  // 3-byte character (1110xxxx)
		{
			zLength++;
			sStr += 2;
		}
		else if ((chChar & 0xF8) == 0xF0)  // 4-byte character (11110xxx)
		{
			zLength++;
			sStr += 3;
		}
		else
		{
			SEOUL_WARN("UTF8Strlen(): Invalid byte: 0x%02x\n", (UByte)chChar);
		}
	}

	return zLength;
}

/**
 * Converts a UTF-8 string to an ISO 8859-1 encoded string
 *
 * Converts a UTF-8 string to an ISO 8859-1 encoded string.  Any characters
 * which cannot be encoded in ISO 8859-1 are replaced by chReplacementChar.
 * If the input string is not a valid UTF-8 string, the results are undefined.
 * If the resulting ISO 8859-1 string cannot fit in the output buffer, the
 * output is truncated, and the number of bytes needed is returned.  The output
 * is always null-terminated.
 *
 * @param[in]  sInStr            Input UTF-8 string
 * @param[out] sOutStr           Output buffer, receives ISO 8859-1 string
 * @param[in]  zOutSize          Size of the output buffer, in bytes
 * @param[in]  chReplacementChar Character used to replace characters in the
 *                               input string which cannot be encoded in ISO
 *                               8859-1
 *
 * @return The number of bytes in the output string, or the number of bytes that
 *         would be needed if the output buffer was too small
 */
UInt UTF8ToISO88591(const Byte *sInStr, Byte *sOutStr, UInt zOutSize, Byte chReplacementChar /* = '?' */)
{
	UniChar chChar;
	UInt zLength = 0;

	while((chChar = UTF8DecodeChar(sInStr)) != '\0')
	{
		if (zLength < zOutSize - 1)
		{
			// ISO 8859-1 maps directly to the first 256 code points of Unicode
			if (chChar >= 0 && chChar <= 0xFF)
			{
				sOutStr[zLength] = (Byte)chChar;
			}
			else
			{
				sOutStr[zLength] = chReplacementChar;
			}
		}

		sInStr += UTF8BytesPerChar(chChar);
		zLength++;
	}

	if (zLength < zOutSize)
	{
		sOutStr[zLength] = '\0';
	}
	else
	{
		sOutStr[zOutSize - 1] = '\0';
	}

	zLength++;

	return zLength;
}

/**
 * Map from Windows-1252 code point to Unicode code point for the characters in
 * the range 0x80-0x9F.
 */
const UniChar g_aWindows1252CodePoints_80_9F[32] =
{
	0x20AC, // 80 ==> Euro sign
	0x0081, // 81 ==> High octet present (C1 control code)
	0x201A, // 82 ==> Single low-9 quotation mark
	0x0192, // 83 ==> Latin small letter f with hook
	0x201E, // 84 ==> Double low-9 quotation mark
	0x2026, // 85 ==> Horizontal ellipsis
	0x2020, // 86 ==> Dagger
	0x2021, // 87 ==> Double dagger
	0x02C6, // 88 ==> Modifier letter circumflex accent
	0x2030, // 89 ==> Per mille sign
	0x0160, // 8A ==> Latin capital letter S with caron
	0x2039, // 8B ==> Single left-pointing angle quotation mark
	0x0152, // 8C ==> Latin capital ligature OE
	0x008D, // 8D ==> Reverse line feed (C1 control code)
	0x017D, // 8E ==> Latin capital letter Z with caron
	0x008F, // 8F ==> Single shift 3 (C1 control code)
	0x0090, // 90 ==> Device control string (C1 control code)
	0x2018, // 91 ==> Left single quotation mark
	0x2019, // 92 ==> Right single quotation mark
	0x201C, // 93 ==> Left double quotation mark
	0x201D, // 94 ==> Right double quotation mark
	0x2022, // 95 ==> Bullet
	0x2013, // 96 ==> En dash
	0x2014, // 97 ==> Em dash
	0x02DC, // 98 ==> Small tilde
	0x2122, // 99 ==> Trademark sign
	0x0161, // 9A ==> Latin small letter S with caron
	0x203A, // 9B ==> Single right-pointing angle quotation mark
	0x0153, // 9C ==> Latin small ligature OE
	0x009D, // 9D ==> Operating system command (C1 control code)
	0x017E, // 9E ==> Latin small letter Z with caron
	0x0178  // 9F ==> Latin capital letter Y with diaeresis
};

/**
 * Converts a UTF-8 string to a Windows-1252 encoded string
 *
 * Converts a UTF-8 string to a Windows-1252 encoded string.  Any characters
 * which cannot be encoded in Windows-1252 are replaced by chReplacementChar.
 * If the input string is not a valid UTF-8 string, the results are undefined.
 * If the resulting Windows-1252 string cannot fit in the output buffer, the
 * output is truncated, and the number of bytes needed is returned.  The output
 * is always null-terminated.
 *
 * @param[in]  sInStr            Input UTF-8 string
 * @param[out] sOutStr           Output buffer, receives Windows-1252 string
 * @param[in]  zOutSize          Size of the output buffer, in bytes
 * @param[in]  chReplacementChar Character used to replace characters in the
 *                               input string which cannot be encoded in
 *                               Windows-1252
 *
 * @return The number of bytes in the output string, or the number of bytes that
 *         would be needed if the output buffer was too small
 */
UInt UTF8ToWindows1252(const Byte *sInStr, Byte *sOutStr, UInt zOutSize, Byte chReplacementChar /* = '?' */)
{
	UniChar chChar;
	UInt zLength = 0;

	while((chChar = UTF8DecodeChar(sInStr)) != '\0')
	{
		if (zLength < zOutSize - 1)
		{
			// Code points 0x00-0x7F and 0xA0-0xFF map directly between Unicode
			// and Windows-1252
			if ((chChar >= 0 && chChar <= 0x7F) || (chChar >= 0xA0 && chChar <= 0xFF))
			{
				sOutStr[zLength] = (Byte)chChar;
			}
			else
			{
				// Windows-1252 code points 0x80-0x9F are more complicated, so
				// search for a match
				Bool bFound = false;

				for (Byte i = 0; i < 0x20; i++)
				{
					if (g_aWindows1252CodePoints_80_9F[(UByte)i] == chChar)
					{
						sOutStr[zLength] = (Byte)(0x80 + i);
						bFound = true;
						break;
					}
				}

				// If no match, use replacement character
				if (!bFound)
				{
					sOutStr[zLength] = chReplacementChar;
				}
			}
		}

		sInStr += UTF8BytesPerChar(chChar);
		zLength++;
	}

	if (zLength < zOutSize)
	{
		sOutStr[zLength] = '\0';
	}
	else
	{
		sOutStr[zOutSize - 1] = '\0';
	}

	zLength++;

	return zLength;
}

SEOUL_STATIC_ASSERT(sizeof(WChar16) == 2);

/**
 * Converts a UTF-8 string to a UTF-16 string
 *
 * Converts a UTF-8 string to a UTF-16 string.  The endianness of the resulting
 * string is the native endianness of the target machine.  No byte-order mark is
 * output.  If the input string is not a valid UTF-8 string, the results are
 * undefined.  If the resulting UTF-16 string cannot fit in the output buffer,
 * the output is truncated, and the number of wide characterss needed is
 * returned.  The output is always null-terminated, as long as zOutSize is
 * non-zero.
 *
 * @param[in]  sInStr            Input UTF-8 string
 * @param[out] sOutStr           Output buffer, receives UTF-16 string
 * @param[in]  zOutSize          Size of the output buffer, in wide characters
 *
 * @return The number of wide characters in the output string, or the number of
 *         wide characters that would be needed if the output buffer was too
 *         small
 */
UInt UTF8ToUTF16(const Byte *sInStr, WChar16 *sOutStr, UInt zOutSize)
{
	UniChar chChar;
	UInt zLength = 0;

	while((chChar = UTF8DecodeChar(sInStr)) != '\0')
	{
		// Code points 0x0000-0xFFFF map directly (code point 0xD800-0xDFFF
		// are illegal, and if they appear, the resulting behavior of this
		// function is undefined)
		if (chChar <= 0xFFFF)
		{
			if (zLength + 1 < zOutSize)
			{
				sOutStr[zLength] = (WChar16)chChar;
			}

			zLength++;
		}
		else
		{
			// Encode high code points as surrogate pairs
			// Make sure buffer has room for two characters (don't want to
			// encode half of a surrogate pair)
			if (zLength + 2 < zOutSize)
			{
				sOutStr[zLength]     = (WChar16)(0xD800 | (((chChar - 0x10000) & 0xFFC00) >> 10));
				sOutStr[zLength + 1] = (WChar16)(0xDC00 | (chChar & 0x003FF));
			}

			zLength += 2;
		}

		sInStr += UTF8BytesPerChar(chChar);
	}

	if (zLength < zOutSize)
	{
		sOutStr[zLength] = '\0';
	}
	else if (zOutSize > 0)
	{
		sOutStr[zOutSize - 1] = '\0';
	}

	zLength++;

	return zLength;
}

/**
 * Convert the UTF8 string sInStr to a wide character string sOutStr.
 *
 * @param[in]  sInStr            Input UTF-8 string
 * @param[out] sOutStr           Output buffer, receives wide char string
 * @param[in]  zOutSize          Size of the output buffer, in wide characters
 *
 * @return The number of wide characters in the output string, or the number of
 *         wide characters that would be needed if the output buffer was too
 *         small.  The result includes the null terminator in the count.
 */
UInt UTF8ToWCharT(const Byte *sInStr, wchar_t *sOutStr, UInt zOutSize)
{
	// If wide characters are 2 bytes on the current platform, we can
	// just use UTF8ToUTF16 for the conversion.
#if SEOUL_WCHAR_T_IS_2_BYTES
	return UTF8ToUTF16(sInStr, sOutStr, zOutSize);
	// Otherwise wide char is 4 bytes, so we construct a String object
	// and iterate over the wide characters.
#else
	String s(sInStr);
	UInt i = 0;
	for (auto iter = s.Begin(); iter != s.End(); ++iter)
	{
		// If we've hit the end of our output buffer, null terminate.
		if (i + 1 == zOutSize)
		{
			sOutStr[i] = L'\0';
		}
		// Otherwise, if we still have space, write out the next Unicode character.
		else if (i + 1 < zOutSize)
		{
			sOutStr[i] = *iter;
		}

		++i;
	}

	// We need to null terminate if we did not already hit the end of the output buffer.
	if (i < zOutSize)
	{
		sOutStr[i] = L'\0';
	}

	return (i + 1u);
#endif
}

/**
 * Converts an ISO 8859-1 string to a UTF-8 encoded string
 *
 * Converts an ISO 8859-1 string to a UTF-8 encoded string.
 *
 * @param[in] sInStr Input ISO 8859-1 string
 *
 * @return The UTF-8 encoded string
 */
String ISO88591ToUTF8(const Byte *sInStr)
{
	String sResult;
	sResult.Reserve((UInt)strlen(sInStr) + 8);

	UByte chChar;
	while((chChar = *sInStr++) != '\0')
	{
		// ISO 8859-1 code points map directly to the corresponding Unicode code
		// points
		sResult.Append((UniChar)chChar);
	}

	return sResult;
}

/**
 * Converts a Windows-1252 string to a UTF-8 encoded string
 *
 * Converts a Windows-1252 string to a UTF-8 encoded string.
 *
 * @param[in] sInStr Input Windows-1252 string
 *
 * @return The UTF-8 encoded string
 */
String Windows1252ToUTF8(const Byte *sInStr)
{
	String sResult;
	sResult.Reserve((UInt)strlen(sInStr) + 8);

	UByte chChar;
	while((chChar = *sInStr++) != '\0')
	{
		// Code points 0x00-0x7F and 0xA0-0xFF map directly between Windows-1252
		// and Unicode
		if (chChar <= 0x7F || chChar >= 0xA0 )
		{
			sResult.Append((UniChar)chChar);
		}
		else
		{
			// Code points 0x80-0x9F require a table lookup
			sResult.Append(g_aWindows1252CodePoints_80_9F[chChar - 0x80]);
		}
	}

	return sResult;
}

/**
 * Converts a UTF-16 string to a UTF-8 encoded string
 *
 * Converts a UTF-16 string to a UTF-8 encoded string.  The endianness of the
 * input string must be the native endianness of the system.  If the input
 * string is not a valid UTF-16 string, the results are undefined.
 *
 * @param[in] sInStr Input UTF-16 string
 *
 * @return The UTF-8 encoded string
 */
String UTF16ToUTF8(const WChar16 *sInStr)
{
	String sResult;

	WChar16 wchChar;
	while((wchChar = *sInStr++) != '\0')
	{
		if (wchChar < 0xD800 || wchChar >= 0xE000)
		{
			sResult.Append((UniChar)wchChar);
		}
		else
		{
			WChar16 wchHighSurrogate = *sInStr++;
			SEOUL_ASSERT(wchChar >= 0xD800 && wchChar < 0xDC00 && wchHighSurrogate >= 0xDC00 && wchHighSurrogate < 0xE000);

			sResult.Append((UniChar)(0x10000 + ((wchChar & 0x03FF) << 10) + (wchHighSurrogate & 0x03FF)));
		}
	}

	return sResult;
}

/**
 * Converts a wide character string to a UTF-8 encoded string
 *
 * @param[in] sInStr Input wide character string
 *
 * @return The UTF-8 encoded String object.
 */
String WCharTToUTF8(const wchar_t *sInStr)
{
	// If wide characters are 16-bits on the current platform, we can
	// just use UTF16ToUTF8.
#if SEOUL_WCHAR_T_IS_2_BYTES
	return UTF16ToUTF8(sInStr);
	// Otherwise, reserve a worse case string (4 * the number of characters in
	// the source) and then append each Unicode character.
#else
	String sReturn;
	size_t const zLength = WSTRLEN(sInStr);
	sReturn.Reserve((UInt)(zLength * 4u));

	for (size_t i = 0u; i < zLength; ++i)
	{
		sReturn.Append(sInStr[i]);
	}

	return sReturn;
#endif
}

UInt TranslateStringToUTF8(const Byte *sInStr, UInt zInSize, Byte *sOutStr, UInt zOutSize, CharacterEncoding eFromEncoding, Bool bTranslateCRLFs, UInt *pzBytesConsumed)
{
	const Byte *pInStr = sInStr;
	const Byte *pInEnd = sInStr + zInSize;
	Byte *pOutStr = sOutStr;
	Byte *pOutEnd = sOutStr + zOutSize;

	while(pInStr < pInEnd)
	{
		UniChar chChar = 0;
		UInt zInputBytes = 0;
		Byte byLeadByte;

		switch(eFromEncoding)
		{
		case EncodingISO88591:
			chChar = (UniChar)(UByte)*pInStr;
			zInputBytes = 1;
			break;

		case EncodingWindows1252:
			chChar = (UniChar)(UByte)*pInStr;
			zInputBytes = 1;

			if (chChar >= 0x80 && chChar <= 0x9F)
			{
				chChar = g_aWindows1252CodePoints_80_9F[chChar - 0x80];
			}

			break;

		case EncodingUTF8:
			byLeadByte = *pInStr;

			if ((byLeadByte & 0x80) == 0x00)  // 1-byte character
			{
				chChar = (UniChar)(UByte)byLeadByte;
				zInputBytes = 1;
			}
			else if ((byLeadByte & 0xE0) == 0xC0)  // 2-byte character
			{
				if (pInStr + 1 >= pInEnd)  // Not enough bytes in input?  Don't consume the lead byte
				{
					goto Done;
				}

				if ((pInStr[1] & 0xC0) != 0x80)  // Illegal UTF-8 sequence - discard
				{
					pInStr += 2;
					continue;
				}

				chChar = (UniChar)(((byLeadByte & 0x1F) << 6) | (pInStr[1] & 0x3F));
				zInputBytes = 2;
			}
			else if ((byLeadByte & 0xF0) == 0xE0)  // 3-byte character
			{
				if (pInStr + 2 >= pInEnd)  // Not enough bytes in input?  Don't consume the lead bytes
				{
					goto Done;
				}

				if (((pInStr[1] & 0xC0) != 0x80) || ((pInStr[2] & 0xC0) != 0x80))  // Illegal UTF-8 sequence - discard
				{
					pInStr += 3;
					continue;
				}

				chChar = (UniChar)(((byLeadByte & 0x0F) << 12) | ((pInStr[1] & 0x3F) << 6) | (pInStr[2] & 0x3F));
				zInputBytes = 3;
			}
			else if ((byLeadByte & 0xF8) == 0xF0)  // 4-byte character
			{
				if (pInStr + 3 >= pInEnd)  // Not enough bytes in input?  Don't consume the lead bytes
				{
					goto Done;
				}

				if (((pInStr[1] & 0xC0) != 0x80) || ((pInStr[2] & 0xC0) != 0x80) || (pInStr[3] & 0xC0) != 0x80)  // Illegal UTF-8 sequence - discard
				{
					pInStr += 4;
					continue;
				}

				chChar = (UniChar)(((byLeadByte & 0x07) << 18) | ((pInStr[1] & 0x3F) << 12) | ((pInStr[2] & 0x3F) << 6) | (pInStr[3] & 0x3F));
				zInputBytes = 4;
			}
			else  // Illegal lead byte - discard
			{
				pInStr += 1;
				continue;
			}

			break;

		case EncodingUTF16:
		case EncodingUTF16LE:
		case EncodingUTF16BE:
			if (pInStr + 1 >= pInEnd)  // Not enough bytes in input?  Don't consume the lead byte
			{
				goto Done;
			}

			WChar16 wchChar;

			if (eFromEncoding == EncodingUTF16)
			{
				wchChar = *(WChar16 *)pInStr;
			}
			else if (eFromEncoding == EncodingUTF16LE)
			{
				wchChar = (WChar16)((UByte)pInStr[0] | ((UByte)pInStr[1] << 8));
			}
			else  // EncodingUTF16BE
			{
				wchChar = (WChar16)(((UByte)pInStr[0] << 8) | (UByte)pInStr[1]);
			}

			if (wchChar < 0xD800 || wchChar >= 0xE000)  // Regular wide character
			{
				chChar = (UniChar)wchChar;
				zInputBytes = 2;
			}
			else if (wchChar >= 0xD800 && wchChar <= 0xDBFF)  // Low surrogate - look for a high surrogate
			{
				if (pInStr + 3 >= pInEnd)  // Not enough bytes in input?  Don't consume the lead bytes
				{
					goto Done;
				}

				WChar16 wchHighSurrogate;

				if (eFromEncoding == EncodingUTF16)
				{
					wchHighSurrogate = *(WChar16 *)(pInStr + 2);
				}
				else if (eFromEncoding == EncodingUTF16LE)
				{
					wchHighSurrogate = (WChar16)((UByte)pInStr[2] | ((UByte)pInStr[3] << 8));
				}
				else
				{
					wchHighSurrogate = (WChar16)(((UByte)pInStr[2] << 8) | ((UByte)pInStr[3]));
				}

				if (wchHighSurrogate >= 0xDC00 && wchHighSurrogate <= 0xDFFF)  // Alright, we got a pair of matching surrogates
				{
					chChar = (UniChar)(0x10000 + ((wchChar & 0x03FF) << 10) + (wchHighSurrogate & 0x03FF));
					zInputBytes = 4;
				}
				else  // Illegal lone low surrogate - discard
				{
					pInStr += 2;
					continue;
				}
			}
			else  // (Illegal) high surrogate - discard
			{
				pInStr += 2;
				continue;
			}

			break;

		default:
			SEOUL_FAIL("Illegal character encoding");
			break;
		}

		// Ok, we've now decoded one character from the input stream and gotten
		// its Unicode code point.  Now it's time for CRLF conversions, and then
		// it's off to UTF-8.

		if (bTranslateCRLFs && chChar == '\r')
		{
			if (eFromEncoding == EncodingISO88591 || eFromEncoding == EncodingWindows1252 || eFromEncoding == EncodingUTF8)
			{
				// Encodings in which CRs and LFs are 1 byte
				if (pInStr + 1 >= pInEnd)  // There might be a newline afterwards, we don't know, so don't consume the carriage return
				{
					goto Done;
				}

				if (pInStr[1] == '\n')
				{
					// Next char is a newline - consume it discard the carriage return
					chChar = '\n';
					zInputBytes += 1;
				}
				// else use the bare CR
			}
			else  // eFromEncoding == EncodingUTF16 || eFromEncoding == EncodingUTF16LE || eFromEncoding == EncodingUTF16BE
			{
				// Encodings in which CRs and LFs are 2 bytes
				if (pInStr + 3 >= pInEnd)  // There might be a newline afterwards, we don't know, so don't consume the carriage return
				{
					goto Done;
				}

				if ((eFromEncoding == EncodingUTF16   && *(WChar16 *)(pInStr + 2) == '\n') ||
				   (eFromEncoding == EncodingUTF16LE && pInStr[2] == '\n' && pInStr[3] == 0x00) ||
				   (eFromEncoding == EncodingUTF16BE && pInStr[2] == 0x00 && pInStr[3] == '\n'))
				{
					// Next char is a newline - consume it and discard the carriage return
					chChar = '\n';
					zInputBytes += 2;
				}
				// else use the bare CR
			}
		}

		// Finally, convert character to UTF-8.
		UInt zBytes = UTF8BytesPerChar(chChar);

		if (pOutStr + zBytes > pOutEnd)
		{
			// Not enough bytes in the output buffer - don't consume current character
			goto Done;
		}

		// Consume the input bytes
		pInStr += zInputBytes;

		UTF8EncodeChar(chChar, pOutStr);
		pOutStr += zBytes;
	}  // end while

Done:
	// Store number of bytes consumed from the input buffer
	*pzBytesConsumed = (UInt)(pInStr - sInStr);

	// Return number of bytes written to the output buffer
	return (UInt)(pOutStr - sOutStr);
}

/**
 * Encodes binary data into base64 using the standard alphabet
 * ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
 *
 * @param[in] pData Data to encode
 * @param[in] zDataLength Length of data to encode
 * @param[in] bURLSafe If true, use '-' in place of '+' and '_' in place of '/'. The padding character '=' is also replaced with the URL encoding of '=', "%3D".
 *
 * @return The base64-encoded data
 */
String Base64Encode(const void *pData, UInt32 zDataLength, Bool bURLSafe /*=false*/)
{
	static const Byte *kStandardAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	static const Byte *kURLSafeAlphabet  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

	static const Byte *kStandardPadding = "=";
	static const Byte *kURLSafePadding = "%3D"; // From RFC-4648, URL encoded version

	const Byte *const kAlphabet = (bURLSafe ? kURLSafeAlphabet : kStandardAlphabet);
	const Byte *const kPadding = (bURLSafe ? kURLSafePadding : kStandardPadding);

	String sOutput;
	sOutput.Reserve((4 * zDataLength + 2) / 3 + 2);

	// Walk through the string 3 bytes at a time, encoding into 4 characters
	const UInt8 *pByteData = (const UInt8 *)pData;
	while (zDataLength >= 3)
	{
		UInt uBits = (pByteData[0] >> 2);
		sOutput += kAlphabet[uBits];

		uBits = ((pByteData[0] & 0x03) << 4) | ((pByteData[1] & 0xF0) >> 4);
		sOutput += kAlphabet[uBits];

		uBits = ((pByteData[1] & 0x0F) << 2) | ((pByteData[2] & 0xC0) >> 6);
		sOutput += kAlphabet[uBits];

		uBits = (pByteData[2] & 0x3F);
		sOutput += kAlphabet[uBits];

		pByteData += 3;
		zDataLength -= 3;
	}

	// Handle trailing bytes
	if (zDataLength == 1 || zDataLength == 2)
	{
		UInt uBits = (pByteData[0] >> 2);
		sOutput += kAlphabet[uBits];

		uBits = (pByteData[0] & 0x03) << 4;
		if (zDataLength == 1)
		{
			sOutput += kAlphabet[uBits];
			sOutput += kPadding;
			sOutput += kPadding;
		}
		else
		{
			uBits |= ((pByteData[1] & 0xF0) >> 4);
			sOutput += kAlphabet[uBits];

			uBits = ((pByteData[1] & 0x0F) << 2);
			sOutput += kAlphabet[uBits];
			sOutput += kPadding;
		}
	}

	return sOutput;
}

/**
 * Decodes a single base64 character into a number 0-63
 *
 * @param[in] uBase64 base64 character in [A-Za-z0-9+/] to decode
 * @param[out] uOutBits Receives character index 0-63
 *
 * @return True if the input is a valid base64 character, or False otherwise
 */
Bool Base64DecodeChar(UInt8 uBase64, UInt8& uOutBits)
{
	static const Int8 s_DecodeTable[128] =
	{
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
		52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
		15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
		-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
		41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
	};

	if (uBase64 >= sizeof(s_DecodeTable))
	{
		return false;
	}

	Int8 iIndex = s_DecodeTable[uBase64];
	if (iIndex != -1)
	{
		uOutBits = (UInt8)iIndex;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Decodes binary data from base64 using the standard alphabet
 * ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
 *
 * @param[in] sString base64-encoded data to decode
 * @param[out] vOutData Receives decoded binary data
 *
 * @return True if decoding succeeded, or false if an invalid character was
 *         encountered
 */
Bool Base64Decode(const String& sString, Vector<Byte>& vOutData)
{
	// Quick sanity check
	UInt uLength = sString.GetSize();
	if (uLength % 4 != 0)
	{
		return false;
	}

	vOutData.Clear();
	vOutData.Reserve((uLength * 3 + 3) / 4);

	// Empty string is empty
	if (uLength == 0)
	{
		return true;
	}

	for (UInt i = 0; i < uLength; i += 4)
	{
		// Decode 4 bytes at a time into 6 bits each.  If an invalid byte is
		// encountered, return failure, unless it's a padding byte at the end
		UInt8 uIndex0, uIndex1, uIndex2, uIndex3;
		if (!Base64DecodeChar(sString[i],   uIndex0) ||
			!Base64DecodeChar(sString[i+1], uIndex1))
		{
			return false;
		}

		vOutData.PushBack((uIndex0 << 2) | ((uIndex1 & 0x30) >> 4));

		if (!Base64DecodeChar(sString[i+2], uIndex2))
		{
			// If the 3rd char didn't decode, return success if we're at the
			// end and have two padding bytes, otherwise return failure
			return (i == uLength - 4 &&
					sString[i+2] == '=' &&
					sString[i+3] == '=');
		}

		vOutData.PushBack(((uIndex1 & 0x0F) << 4) | ((uIndex2 & 0x3C) >> 2));

		if (!Base64DecodeChar(sString[i+3], uIndex3))
		{
			// If the 4th char didn't decode, return success if we're at the
			// end and have a padding byte, otherwise return failure
			return (i == uLength - 4 &&
					sString[i+3] == '=');
		}

		vOutData.PushBack(((uIndex2 & 0x03) << 6) | uIndex3);
	}

	// We're done
	return true;
}

/**
 * Decodes a hexadecimal character into a number 0-15.  If the character is not
 * in [0-9A-Fa-f], the results are undefined.
 */
UInt8 DecodeHexChar(Byte cChar)
{
	if (cChar >= '0' && cChar <= '9')
	{
		return (cChar - '0');
	}
	else if (cChar >= 'A' && cChar <= 'F')
	{
		return (10 + cChar - 'A');
	}
	else if (cChar >= 'a' && cChar <= 'f')
	{
		return (10 + cChar - 'a');
	}
	else
	{
		SEOUL_FAIL(String::Printf("DecodeHexChar: Invalid character: %c", cChar).CStr());
		return 0;
	}
}

/**
 * Decodes a URL-encoded string by replacing %XX escape codes with their
 * character values.  If an invalid escape code is encountered, it is left
 * unchanged.
 */
String URLDecode(const Byte *sInStr)
{
	String sResult;
	sResult.Reserve(StrLen(sInStr) + 1);

	while (*sInStr)
	{
		// Find a run of non-% characters and append them all at once so that
		// we don't ever get into a state of having only part of a multi-byte
		// character in the string
		const Byte *pStart = sInStr;
		while (*sInStr != '%' && *sInStr)
		{
			sInStr++;
		}

		if (sInStr != pStart)
		{
			sResult.Append(pStart, (UInt32)(sInStr - pStart));
		}

		// We should now be pointing at a '%' or the end of the string
		if (*sInStr == '%')
		{
			if (isxdigit(sInStr[1]) && isxdigit(sInStr[2]))
			{
				UInt8 uDecodedChar = (DecodeHexChar(sInStr[1]) << 4) | DecodeHexChar(sInStr[2]);
				sResult.Append((Byte)uDecodedChar);
				sInStr += 3;
			}
			else
			{
				// Bad escape sequence, append the literal %
				sResult.Append('%');
				sInStr++;
			}
		}
	}

	return sResult;
}

/**
 * Trims leading and trailing white space from the given string
 *
 * Trims leading and trailing white space from the given string.  White space is
 * defined as the space character (' '), horizontal tab ('\\t'), carriage return
 * ('\\r'), line feed ('\\n'), and form feed ('\\f').  If the string consists
 * entirely of white space, the empty string is returned.
 *
 * @param[in] sString The string to trim
 *
 * @return The string, with leading and trailing white space removed
 */
String TrimWhiteSpace(const String & sString)
{
	UInt ixFirstNonSpace = sString.FindFirstNotOf(" \t\r\n\f");
	UInt ixLastNonSpace  = sString.FindLastNotOf(" \t\r\n\f");

	if (ixFirstNonSpace == String::NPos)
	{
		return String();
	}
	else
	{
		SEOUL_ASSERT(ixLastNonSpace >= ixFirstNonSpace);
		// Make sure we aren't cutting off additional UTF8 bytes with the substring call
		UInt uByteCount = UTF8BytesPerChar(sString.CharAtByteIndex(ixLastNonSpace));
		return sString.Substring(ixFirstNonSpace, ixLastNonSpace - ixFirstNonSpace + uByteCount);
	}
}

/**
 * Returns true if the specified string is a valid IP address.
 */
Bool IsIPAddress(const String& sString)
{
	UInt uPeriodCount = 0;

	for (UInt i = 0; i < sString.GetSize(); ++i)
	{
		if (sString[i] == '.')
		{
			++uPeriodCount;
		}
		else if (sString[i] < '0' || sString[i] > '9')
		{
			return false;
		}
	}

	return uPeriodCount == 3;
}

/**
 * Counts the number of occurrences of the given character in the string
 *
 * @param[in] sString Input string
 * @param[in] c Character to count
 *
 * @return The number of instances of the character c in the string
 *         sString
 */
Int CountOccurrences(const String& sString, UniChar c)
{
	Int nCount = 0;
	for (auto iter = sString.Begin(); iter != sString.End(); ++iter)
	{
		if (*iter == c)
		{
			nCount++;
		}
	}

	return nCount;
}

/**
 * Replacement for strncpy that is guaranteed to null-terminate the
 *     destination
 *
 * Copies the string sSrc into sDest, making sure not to write more than
 * zSize characters into sDest.  sDest is always null-terminated,
 * except when zSize is 0, in which case sDest is not modified.
 *
 * @param[out] sDest Destination buffer to copy the string into
 * @param[in]  sSrc Source buffer to copy the string from
 * @param[in]  zSize Size of sDest, in bytes
 *
 * @return A copy of sDest
 */
Byte *StrNCopy(Byte *sDest, const Byte *sSrc, size_t zSize)
{
	if (zSize == 0)
		return sDest;

	Byte *sOrigDest = sDest;

	while(*sSrc != 0 && zSize > 1)
	{
		*sDest++ = *sSrc++;
		zSize--;
	}

	*sDest = 0;

	return sOrigDest;
}

/**
 * Replacement for strncat that is guaranteed to null-terminate the
 *     destination
 *
 * Concatenates the string sSrc onto the end of sDest, making sure not to
 * write beyond the end of sDest.  sDest is always null-terminated,
 * except when zSize is 0, in which case sDest is not modified.
 *
 * @param[in,out] sDest Destination buffer to concatenate the string onto.
 *     Must contain a null-terminated string of length less than zSize.
 * @param[in]  sSrc Source buffer to copy the string from
 * @param[in]  zSize Size of sDest, in bytes
 *
 * @return A copy of sDest
 */
Byte *StrNCat(Byte *sDest, const Byte *sSrc, size_t zSize)
{
	if (zSize == 0)
		return sDest;

	Byte *sOrigDest = sDest;

	while(*sDest != 0 && zSize > 0)
	{
		sDest++;
		zSize--;
	}

	while(*sSrc != 0 && zSize > 1)
	{
		*sDest++ = *sSrc++;
		zSize--;
	}

	*sDest = 0;

	return sOrigDest;
}

/**
@return A value on [0, 1] indicating "how closely" sStringA matches sStringB.
* This can be used to determine, for example, how likely a key in input data
* matches an unmatched key in target data.
*
* From: Boer, J. 2006. "Closest-String Matching Algorithm".
*           Game Programming Gems 6.
*/
Float ComputeStringMatchFactor(
	Byte const* sStringA, UInt32 zStringALengthInBytes,
	Byte const* sStringB, UInt32 zStringBLengthInBytes)
{
	// Factor that a case insensitive match is worth of
	// a case sensitive match
	static const Float kfCaseInsensitiveMatchFactor = 0.9f;

	UInt32 const zLargerSize = Max(zStringALengthInBytes, zStringBLengthInBytes);
	Float const fMatchContribution = (1.0f / zLargerSize);

	Byte const* const sEndA = (sStringA + zStringALengthInBytes);
	Byte const* const sEndB = (sStringB + zStringBLengthInBytes);

	Float fReturnValue = 0.0f;

	while (sStringA < sEndA && sStringB < sEndB)
	{
		// First, check for a simple left/right match
		if (*sStringA == *sStringB)
		{
			// If it matches, add a proportional character's value
			// to the match total.
			fReturnValue += fMatchContribution;

			// Advance both pointers
			++sStringA;
			++sStringB;
		}
		// If the simple match fails,
		// try a match ignoring capitalization
		else if (::tolower(*sStringA) == ::tolower(*sStringB))
		{
			// We'll consider a capitalization mismatch worth 90%
			// of a normal match
			fReturnValue += (kfCaseInsensitiveMatchFactor * fMatchContribution);

			// Advance both pointers
			++sStringA;
			++sStringB;
		}
		else
		{
			Byte const* sBestA = sEndA;
			Byte const* sBestB = sEndB;

			UInt32 nTotalCount = 0u;
			UInt32 nBestCount = UIntMax;
			UInt32 nLeftCount = 0u;
			UInt32 nRightCount = 0u;

			// Here we loop through the left word in an outer loop,
			// but we also check for an early out by ensuring we
			// don't exceed our best current count
			for (Byte const* sA = sStringA; sA < sEndA && (nLeftCount + nRightCount) < nBestCount; ++sA)
			{
				// Inner loop counting
				for (Byte const* sB = sStringB; sB < sEndB && (nLeftCount + nRightCount) < nBestCount; ++sB)
				{
					// At this point, we don't care about case
					if (::tolower(*sStringA) == ::tolower(*sStringB))
					{
						// This is the fitness measurement
						nTotalCount = (nLeftCount + nRightCount);
						if (nTotalCount < nBestCount)
						{
							nBestCount = nTotalCount;
							sBestA = sA;
							sBestB = sB;
						}
					}

					++nRightCount;
				}

				++nLeftCount;
				nRightCount = 0;
			}

			sStringA = sBestA;
			sStringB = sBestB;
		}
	}

	// Clamp the value in case of floating-point error
	if (fReturnValue > 0.99f)
	{
		fReturnValue = 1.0f;
	}
	else if (fReturnValue < 0.01f)
	{
		fReturnValue = 0.0f;
	}

	return fReturnValue;
}

/**
 * Dumps a data buffer to its hexadecimal representation.  The returned string
 * will be 2*zDataSize characters long, consisting of only the characters
 * [0-9A-Fa-f].
 *
 * @param[in] pData Data buffer to dump
 * @param[in] zDataSize Size of data buffer to dump
 * @param[in] bUpperCase True to dump as uppercase hexadecimal (A-F), false to
 *              dump as lowercase hexadecimal (a-f)
 *
 * @return The hexadecimal representation of the given data buffer
 */
String HexDump(const void *pData, UInt32 zDataSize, Bool bUppercase /* = true */)
{
	static const char kaUpperHex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	static const char kaLowerHex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	String sResult;
	sResult.Reserve(2 * zDataSize);

	for (size_t i = 0; i < zDataSize; i++)
	{
		UByte uCodePoint = ((const UByte *)pData)[i];
		if (bUppercase)
		{
			sResult.Append(kaUpperHex[uCodePoint >> 4]);
			sResult.Append(kaUpperHex[uCodePoint & 0x0F]);
		}
		else
		{
			sResult.Append(kaLowerHex[uCodePoint >> 4]);
			sResult.Append(kaLowerHex[uCodePoint & 0x0F]);
		}
	}

	return sResult;
}

void HexParseBytes(String const& sHex, Vector<UByte>& rvOut)
{
	if (sHex.GetSize() % 2 != 0)
	{
		SEOUL_WARN("Can't parse hex string with length %u", sHex.GetSize());
		return;
	}

	for (UInt i = 0; i < sHex.GetSize(); i += 2)
	{
		auto ch1 = DecodeHexChar(sHex[i]);
		auto ch2 = DecodeHexChar(sHex[i + 1]);

		rvOut.PushBack(Byte((ch1 << 4) + ch2));
	}
}


} // namespace Seoul
