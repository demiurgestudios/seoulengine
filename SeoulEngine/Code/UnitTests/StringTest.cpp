/**
 * \file StringTest.cpp
 * \brief Unit test implementation for String class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Core.h"
#include "FromString.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulString.h"
#include "StringTest.h"
#include "StringUtil.h"
#include "ToString.h"
#include "UnitTesting.h"
#include "Vector.h"
#include <wchar.h>

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(StringTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestIsValidUnicodeChar)
	SEOUL_METHOD(TestIsValidUTF8String)
	SEOUL_METHOD(TestUTF8Strlen)
	SEOUL_METHOD(TestUTF8BytesPerChar)
	SEOUL_METHOD(TestUTF8EncodeChar)
	SEOUL_METHOD(TestUTF8DecodeChar)
	SEOUL_METHOD(TestUTF8ToISO88591)
	SEOUL_METHOD(TestUTF8ToWindows1252)
	SEOUL_METHOD(TestUTF8ToWCharT)
	SEOUL_METHOD(TestISO88591ToUTF8)
	SEOUL_METHOD(TestWindows1252ToUTF8)
	SEOUL_METHOD(TestWCharTToUTF8)
	SEOUL_METHOD(TestTranslateStringToUTF8)
	SEOUL_METHOD(TestBase64Encode)
	SEOUL_METHOD(TestBase64Decode)
	SEOUL_METHOD(TestURLDecode)
	SEOUL_METHOD(TestSplitString)
	SEOUL_METHOD(TestStrNCopy)
	SEOUL_METHOD(TestStrNCat)
	SEOUL_METHOD(TestToString)
	SEOUL_METHOD(TestFromString)
	SEOUL_METHOD(TestToFromString)
	SEOUL_METHOD(TestHexParseBytes)
	SEOUL_METHOD(TestBasicEmptyStrings)
	SEOUL_METHOD(TestCharacterConstructor)
	SEOUL_METHOD(TestAssign)
	SEOUL_METHOD(TestAppend)
	SEOUL_METHOD(TestComparisons)
	SEOUL_METHOD(TestUTF8Strings)
	SEOUL_METHOD(TestReserve)
	SEOUL_METHOD(TestIterators)
	SEOUL_METHOD(TestFindMethods)
	SEOUL_METHOD(TestSubstring)
	SEOUL_METHOD(TestReplaceAll)
	SEOUL_METHOD(TestReverse)
	SEOUL_METHOD(TestTakeOwnership)
	SEOUL_METHOD(TestToUpper)
	SEOUL_METHOD(TestToLower)
	SEOUL_METHOD(TestToUpperASCII)
	SEOUL_METHOD(TestToLowerASCII)
	SEOUL_METHOD(TestIsASCII)
	SEOUL_METHOD(TestWStr)
	SEOUL_METHOD(TestSwap)
	SEOUL_METHOD(TestPrintf)
	SEOUL_METHOD(TestPopBack)
	SEOUL_METHOD(TestMove)
	SEOUL_METHOD(TestRelinquishBuffer)
	SEOUL_METHOD(TestToStringVector)
SEOUL_END_TYPE()

/**
 * Tests the functionality of the IsValidUnicodeChar() function
 */
void StringTest::TestIsValidUnicodeChar(void)
{
	SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(-1));
	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar(0));
	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar('A'));
	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar(0x1000));
	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar(0xD7FF));
	SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(0xD800));
	SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(0xDBFF));
	SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(0xDC00));
	SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(0xDFFF));
	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar(0xE000));
	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar(0xFDCF));

	for(UniChar c = 0xFDD0; c < 0xFDEF; c++)
	{
		SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(c));
	}

	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar(0xFDF0));
	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar(0xFFFD));
	SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(0xFFFE));
	SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(0xFFFF));
	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar(0x10000));
	SEOUL_UNITTESTING_ASSERT( IsValidUnicodeChar(0x10FFFF));
	SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(0x110000));
	SEOUL_UNITTESTING_ASSERT(!IsValidUnicodeChar(0x7FFFFFFF));
}

/**
 * Tests the functionality of the IsValidUTF8String() function
 */
void StringTest::TestIsValidUTF8String(void)
{
	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String(""));
	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String("wxyz"));
	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String("wx\xC3\xA9yz"));  // 2-byte char
	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String("wx\xE2\x80\x93yz"));  // 3-byte char
	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String("wx\xF0\x9D\x84\xA0yz"));  // 4-byte char

	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String(nullptr));
	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\x83yz"));
	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\xC3yz"));
	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\xE3\x81yz"));
	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\xF3\x81\x81yz"));
	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\xF9\xAA\xAA\xAA\xAAyz"));
	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\xFD\xAA\xAA\xAA\xAA\xAAyz"));
	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\xFE\x83\xAA\xAA\xAA\xAA\xAAyz"));

	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String("", 5));
	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String("wxyz", 0));
	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String("wx\xC3\xA9yz", 5));  // 2-byte char
	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String("wx\xE2\x80\x93yz", 6));  // 3-byte char
	SEOUL_UNITTESTING_ASSERT( IsValidUTF8String("wx\xF0\x9D\x84\xA0yz", 7));  // 4-byte char

	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\xC3\xA9yz", 3));  // 2-byte char
	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\xE2\x80\x93yz", 4));  // 3-byte char
	SEOUL_UNITTESTING_ASSERT(!IsValidUTF8String("wx\xF0\x9D\x84\xA0yz", 5));  // 4-byte char
}

/**
 * Tests the functionality of the UTF8Strlen() function
 */
void StringTest::TestUTF8Strlen(void)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, UTF8Strlen(""));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, UTF8Strlen("wxyz"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, UTF8Strlen("wx\xC3\xA9yz"));  // 2-byte char
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, UTF8Strlen("wx\xE2\x80\x93yz"));  // 3-byte char
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, UTF8Strlen("wx\xF0\x9D\x84\xA0yz"));  // 4-byte char
}

/**
 * Tests the functionality of the UTF8BytesPerChar() function
 */
void StringTest::TestUTF8BytesPerChar(void)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((Byte)1, UTF8BytesPerChar(0));
	SEOUL_UNITTESTING_ASSERT_EQUAL((Byte)1, UTF8BytesPerChar('A'));
	SEOUL_UNITTESTING_ASSERT_EQUAL((Byte)1, UTF8BytesPerChar(0x7F));
	SEOUL_UNITTESTING_ASSERT_EQUAL((Byte)2, UTF8BytesPerChar(0x80));
	SEOUL_UNITTESTING_ASSERT_EQUAL((Byte)2, UTF8BytesPerChar(0x7FF));
	SEOUL_UNITTESTING_ASSERT_EQUAL((Byte)3, UTF8BytesPerChar(0x800));
	SEOUL_UNITTESTING_ASSERT_EQUAL((Byte)3, UTF8BytesPerChar(0xFFFF));
	SEOUL_UNITTESTING_ASSERT_EQUAL((Byte)4, UTF8BytesPerChar(0x10000));
	SEOUL_UNITTESTING_ASSERT_EQUAL((Byte)4, UTF8BytesPerChar(0x10FFFF));
}

/**
 * Tests the functionality of the UTF8EncodeChar() function
 */
void StringTest::TestUTF8EncodeChar(void)
{
	Byte sBuffer[5] = {(Byte)0xFF, (Byte)0xFF, (Byte)0xFF, (Byte)0xFF, (Byte)0xFF};

	SEOUL_UNITTESTING_ASSERT(UTF8EncodeChar(      0, sBuffer) == 1 && memcmp(sBuffer, "\x00\xFF", 2) == 0);
	SEOUL_UNITTESTING_ASSERT(UTF8EncodeChar(    'A', sBuffer) == 1 && memcmp(sBuffer, "A\xFF", 2) == 0);
	SEOUL_UNITTESTING_ASSERT(UTF8EncodeChar(   0xE9, sBuffer) == 2 && memcmp(sBuffer, "\xC3\xA9\xFF", 3) == 0);
	SEOUL_UNITTESTING_ASSERT(UTF8EncodeChar( 0x2013, sBuffer) == 3 && memcmp(sBuffer, "\xE2\x80\x93\xFF", 4) == 0);
	SEOUL_UNITTESTING_ASSERT(UTF8EncodeChar(0x1D120, sBuffer) == 4 && memcmp(sBuffer, "\xF0\x9D\x84\xA0\xFF", 5) == 0);
}

/**
 * Tests the functionality of the UTF8DecodeChar() function
 */
void StringTest::TestUTF8DecodeChar(void)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL((UniChar)      0, UTF8DecodeChar("\x00"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UniChar)    'A', UTF8DecodeChar("A"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UniChar)   0xE9, UTF8DecodeChar("\xC3\xA9"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UniChar) 0x2013, UTF8DecodeChar("\xE2\x80\x93"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UniChar)0x1D120, UTF8DecodeChar("\xF0\x9D\x84\xA0"));
}

/**
 * Tests the functionality of the UTF8ToISO88591() function
 */
void StringTest::TestUTF8ToISO88591(void)
{
	Byte sBuffer[512], sExpectedResult[512];
	String s1;

	for(UniChar chChar = 0x0001; chChar <= 0x0100; chChar++)
	{
		s1.Append(chChar);
		sExpectedResult[chChar - 1] = (chChar <= 0xFF) ? (Byte)chChar : '?';
	}

	s1.Append(0x2013);
	s1.Append(0x1D120);
	s1.Append('A');

	sExpectedResult[256] = '?';
	sExpectedResult[257] = '?';
	sExpectedResult[258] = 'A';
	sExpectedResult[259] = '\0';

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)260, UTF8ToISO88591(s1.CStr(), sBuffer, 512, '?'));
	SEOUL_UNITTESTING_ASSERT(strcmp(sExpectedResult, sBuffer) == 0);

	sExpectedResult[15] = '\0';
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)260, UTF8ToISO88591(s1.CStr(), sBuffer, 16, '?'));
	SEOUL_UNITTESTING_ASSERT(strcmp(sExpectedResult, sBuffer) == 0);
}

/**
 * Tests the functionality of the UTF8ToWindows1252() function
 */
void StringTest::TestUTF8ToWindows1252(void)
{
	Byte sBuffer[512], sExpectedResult[512];
	String s1;

	for(UniChar chChar = 0x0001; chChar <= 0x0100; chChar++)
	{
		s1.Append(chChar);
	}

	s1.Append(0x1D120);

	s1.Append(0x20AC).Append(0x201A).Append(0x0192).Append(0x201E)
	  .Append(0x2026).Append(0x2020).Append(0x2021).Append(0x02C6)
	  .Append(0x2030).Append(0x0160).Append(0x2039).Append(0x0152)
	  .Append(0x017D).Append(0x2018).Append(0x2019).Append(0x201C)
	  .Append(0x201D).Append(0x2022).Append(0x2013).Append(0x2014)
	  .Append(0x02DC).Append(0x2122).Append(0x0161).Append(0x203A)
	  .Append(0x0153).Append(0x017E).Append(0x0178);

	for(UInt i = 0x01; i <= 0x100; i++)
	{
		if(i < 0x80 || (i >= 0xA0 && i <= 0xFF) || i == 0x81 || i == 0x8D || i == 0x8F || i == 0x90 || i == 0x9D)
		{
			sExpectedResult[i - 1] = (Byte)i;
		}
		else
		{
			sExpectedResult[i - 1] = '?';
		}
	}

	memcpy(sExpectedResult + 256, "?\x80\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8E\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9E\x9F\x00", 29);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)285, UTF8ToWindows1252(s1.CStr(), sBuffer, 512, '?'));
	SEOUL_UNITTESTING_ASSERT(strcmp(sExpectedResult, sBuffer) == 0);

	sExpectedResult[15] = '\0';
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)285, UTF8ToWindows1252(s1.CStr(), sBuffer, 16, '?'));
	SEOUL_UNITTESTING_ASSERT(strcmp(sExpectedResult, sBuffer) == 0);
}

/**
 * Tests the functionality of the UTF8ToWCharT() function
 */
void StringTest::TestUTF8ToWCharT(void)
{
	wchar_t sBuffer[64];
	String s;
	s.Append(0x0001).Append('A').Append(0x00E9).Append(0x2013).Append(0x1D120);

#if SEOUL_WCHAR_T_IS_2_BYTES
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)7, UTF8ToWCharT(s.CStr(), sBuffer, 64));
	SEOUL_UNITTESTING_ASSERT(wcscmp(L"\x0001\x0041\x00E9\x2013\xD834\xDD20", sBuffer) == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)7, UTF8ToWCharT(s.CStr(), sBuffer, 5));
	SEOUL_UNITTESTING_ASSERT(wcscmp(L"\x0001\x0041\x00E9\x2013", sBuffer) == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)7, UTF8ToWCharT(s.CStr(), sBuffer, 6));
	SEOUL_UNITTESTING_ASSERT(wcscmp(L"\x0001\x0041\x00E9\x2013", sBuffer) == 0);
#else
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)6, UTF8ToWCharT(s.CStr(), sBuffer, 64));
	SEOUL_UNITTESTING_ASSERT(wcscmp(L"\x0001\x0041\x00E9\x2013\x1D120", sBuffer) == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)6, UTF8ToWCharT(s.CStr(), sBuffer, 5));
	SEOUL_UNITTESTING_ASSERT(wcscmp(L"\x0001\x0041\x00E9\x2013", sBuffer) == 0);
#endif
}

/**
 * Tests the functionality of the ISO88591ToUTF8() function
 */
void StringTest::TestISO88591ToUTF8(void)
{
	Byte s1[256], sExpected[384], s3[256];

	for(UInt i = 0x01; i <= 0xFF; i++)
	{
		s1[i - 1] = (Byte)i;
	}

	s1[255] = '\0';

	for(UInt i = 0x01; i <= 0x7F; i++)
	{
		sExpected[i - 1] = (Byte)i;
	}

	for(UInt i = 0x80; i <= 0xFF; i++)
	{
		sExpected[2 * i - 0x81] = (Byte)(0xC0 | ((i >> 6) & 0x1F));
		sExpected[2 * i - 0x80] = (Byte)(0x80 | (i & 0x3F));
	}

	sExpected[383] = '\0';

	String s2 = ISO88591ToUTF8(s1);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)383, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)255, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(sExpected, s2.CStr()) == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)256, UTF8ToISO88591(s2.CStr(), s3, 256));
	SEOUL_UNITTESTING_ASSERT(strcmp(s1, s3) == 0);
}

//extern const UniChar g_aWindows1252CodePoints_80_9F[];

/**
 * Tests the functionality of the Windows1252ToUTF8() function
 */
void StringTest::TestWindows1252ToUTF8(void)
{
	Byte s1[256], sExpected[512], s3[256];
	UInt zExpectedSize = 0;

	for(UInt i = 0x01; i <= 0xFF; i++)
	{
		s1[i - 1] = (Byte)i;
	}

	s1[255] = '\0';

	for(UInt i = 0x01; i <= 0xFF; i++)
	{
		if(i < 0x80 || i >= 0xA0)
		{
			zExpectedSize += UTF8EncodeChar((UniChar)i, sExpected + zExpectedSize);
		}
		else
		{
			zExpectedSize += UTF8EncodeChar(g_aWindows1252CodePoints_80_9F[i - 0x80], sExpected + zExpectedSize);
		}
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)400, zExpectedSize);
	sExpected[400] = '\0';

	String s2 = Windows1252ToUTF8(s1);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)400, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)255, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(sExpected, s2.CStr()) == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)256, UTF8ToWindows1252(s2.CStr(), s3, 256));
	SEOUL_UNITTESTING_ASSERT(strcmp(s1, s3) == 0);
}

/**
 * Tests the functionality of the WCharTToUTF8() function
 */
void StringTest::TestWCharTToUTF8(void)
{
#if SEOUL_WCHAR_T_IS_2_BYTES
	const wchar_t *s1 = L"\x0001\x0041\x00E9\x2013\xD834\xDD20";
#else
	const wchar_t *s1 = L"\x0001\x0041\x00E9\x2013\x1D120";
#endif

	String s2 = WCharTToUTF8(s1);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)11, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "\x01\x41\xC3\xA9\xE2\x80\x93\xF0\x9D\x84\xA0") == 0);

	wchar_t s3[16];
#if SEOUL_WCHAR_T_IS_2_BYTES
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)7, UTF8ToWCharT(s2.CStr(), s3, 16));
#else
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)6, UTF8ToWCharT(s2.CStr(), s3, 16));
#endif
	SEOUL_UNITTESTING_ASSERT(wcscmp(s1, s3) == 0);

	memset(s3, 'x', sizeof(s3));
#if SEOUL_WCHAR_T_IS_2_BYTES
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)7, UTF8ToWCharT(s2.CStr(), s3+4, 0));
	SEOUL_UNITTESTING_ASSERT(memcmp(s3, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 16 * sizeof(wchar_t)) == 0);
#else
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)6, UTF8ToWCharT(s2.CStr(), s3+4, 0));
	SEOUL_UNITTESTING_ASSERT(memcmp(s3, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 16 * sizeof(wchar_t)) == 0);
#endif
}

/**
 * Tests the functionality of the TranslateStringToUTF8() function
 */
void StringTest::TestTranslateStringToUTF8(void)
{
	Byte sInBuffer[1024], sOutBuffer[1024], sExpected[1024];
	UInt zBytesConsumed, zBytesTranslated;
	Int i;

	Byte *pExpected = sExpected;

	//////////////////////////////////////////////
	// Case 1: ISO 8859-1, no CRLF translations //
	//////////////////////////////////////////////

	for(i = 0; i < 256; i++)
	{
		sInBuffer[i] = (Byte)i;
		pExpected += UTF8EncodeChar((UniChar)i, pExpected);
	}

	memcpy(sInBuffer + 256, "\r\n", 2);
	memcpy(pExpected, "\r\n", 2);

	zBytesTranslated = TranslateStringToUTF8(sInBuffer, 258, sOutBuffer, 1024, EncodingISO88591, false, &zBytesConsumed);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)258, zBytesConsumed);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)386, zBytesTranslated);

	SEOUL_UNITTESTING_ASSERT(memcmp(sOutBuffer, sExpected, 386) == 0);

	////////////////////////////////////////////////
	// Case 2: ISO 8859-1, with CRLF translations //
	////////////////////////////////////////////////

	*pExpected = '\n';
	zBytesTranslated = TranslateStringToUTF8(sInBuffer, 258, sOutBuffer, 1024, EncodingISO88591, true, &zBytesConsumed);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)258, zBytesConsumed);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)385, zBytesTranslated);

	SEOUL_UNITTESTING_ASSERT(memcmp(sOutBuffer, sExpected, 385) == 0);

	////////////////////////////////////////////////
	// Case 3: Windows-1252, no CRLF translations //
	////////////////////////////////////////////////

	pExpected = sExpected + 128;
	for(i = 0x80; i <= 0x9F; i++)
	{
		pExpected += UTF8EncodeChar(g_aWindows1252CodePoints_80_9F[i - 0x80], pExpected);
	}

	for( ; i < 256; i++)
	{
		pExpected += UTF8EncodeChar((UniChar)i, pExpected);
	}

	memcpy(pExpected, "\r\n", 2);

	zBytesTranslated = TranslateStringToUTF8(sInBuffer, 258, sOutBuffer, 1024, EncodingWindows1252, false, &zBytesConsumed);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)258, zBytesConsumed);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)403, zBytesTranslated);

	SEOUL_UNITTESTING_ASSERT(memcmp(sOutBuffer, sExpected, 403) == 0);

	//////////////////////////////////////////////////
	// Case 4: Windows-1252, with CRLF translations //
	//////////////////////////////////////////////////

	*pExpected = '\n';

	zBytesTranslated = TranslateStringToUTF8(sInBuffer, 258, sOutBuffer, 1024, EncodingWindows1252, true, &zBytesConsumed);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)258, zBytesConsumed);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)402, zBytesTranslated);

	SEOUL_UNITTESTING_ASSERT(memcmp(sOutBuffer, sExpected, 402) == 0);

	/////////////////////////////////////////
	// Case 5: UTF-8, no CRLF translations //
	/////////////////////////////////////////

	memcpy(sInBuffer, "ABCD\rEFGH\n\xC3\xA9\r\n\xE2\x80\x93\0\xF0\x9D\x84\xA0", 22);
	memcpy(sExpected, sInBuffer, 22);

	zBytesTranslated = TranslateStringToUTF8(sInBuffer, 22, sOutBuffer, 1024, EncodingUTF8, false, &zBytesConsumed);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)22, zBytesConsumed);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)22, zBytesTranslated);

	SEOUL_UNITTESTING_ASSERT(memcmp(sOutBuffer, sExpected, 22) == 0);

	///////////////////////////////////////////
	// Case 6: UTF-8, with CRLF translations //
	///////////////////////////////////////////

	memcpy(sExpected, "ABCD\rEFGH\n\xC3\xA9\n\xE2\x80\x93\0\xF0\x9D\x84\xA0", 21);

	zBytesTranslated = TranslateStringToUTF8(sInBuffer, 22, sOutBuffer, 1024, EncodingUTF8, true, &zBytesConsumed);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)22, zBytesConsumed);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)21, zBytesTranslated);

	SEOUL_UNITTESTING_ASSERT(memcmp(sOutBuffer, sExpected, 21) == 0);

	////////////////////////////////////////////////////////////////////////////////////
	// Cases 7-12: UTF-16, UTF-16LE, and UTF-16BE, with and without CRLF translations //
	////////////////////////////////////////////////////////////////////////////////////

	CharacterEncoding utf16Encodings[3] = {EncodingUTF16, EncodingUTF16LE, EncodingUTF16BE};

	for(i = 0; i < 3; i++)
	{
		Bool bSwapBytes = (i == 2) || (i == 0 && IsSystemBigEndian());

		memcpy(sInBuffer, "A\x00""B\x00\r\x00""C\x00\n\x00""D\x00\r\x00\n\x00\xE9\x00\x00\x00\x13\x20\x34\xD8\x20\xDD", 26);
		memcpy(sExpected, "AB\rC\nD\r\n\xC3\xA9\x00\xE2\x80\x93\xF0\x9D\x84\xA0", 18);

		if(bSwapBytes)
		{
			for(Int j = 0; j < 26; j += 2)
			{
				*(UInt16 *)(sInBuffer + j) = EndianSwap16(*(UInt16 *)(sInBuffer + j));
			}
		}

		zBytesTranslated = TranslateStringToUTF8(sInBuffer, 26, sOutBuffer, 1024, utf16Encodings[i], false, &zBytesConsumed);

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)26, zBytesConsumed);
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)18, zBytesTranslated);

		SEOUL_UNITTESTING_ASSERT(memcmp(sOutBuffer, sExpected, 18) == 0);

		memmove(sExpected + 6, sExpected + 7, 11);

		zBytesTranslated = TranslateStringToUTF8(sInBuffer, 26, sOutBuffer, 1024, utf16Encodings[i], true, &zBytesConsumed);

		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)26, zBytesConsumed);
		SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)17, zBytesTranslated);

		SEOUL_UNITTESTING_ASSERT(memcmp(sOutBuffer, sExpected, 17) == 0);
	}
}

/**
 * Tests the Base64Encode() function
 */
void StringTest::TestBase64Encode(void)
{
	// Test input which gives each char in output
	const char aInput[] = "\x00\x10\x83\x10\x51\x87\x20\x92\x8b\x30\xd3\x8f\x41\x14\x93\x51\x55\x97\x61\x96\x9b\x71\xd7\x9f\x82\x18\xa3\x92\x59\xa7\xa2\x9a\xab\xb2\xdb\xaf\xc3\x1c\xb3\xd3\x5d\xb7\xe3\x9e\xbb\xf3\xdf\xbf";
	String s = Base64Encode(aInput, sizeof(aInput)-1);
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"), s);

	// Test empty string
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(""), Base64Encode("", 0));

	// Test padding
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("YQ=="), Base64Encode("a", 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("YWI="), Base64Encode("ab", 2));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("YWJj"), Base64Encode("abc", 3));
}

/**
 * Tests the Base64Decode() function
 */
void StringTest::TestBase64Decode(void)
{
	// Test input of all chars
	Vector<Byte> vOutput;
	SEOUL_UNITTESTING_ASSERT(Base64Decode("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/", vOutput));
	const char aExpectedOutput[] = "\x00\x10\x83\x10\x51\x87\x20\x92\x8b\x30\xd3\x8f\x41\x14\x93\x51\x55\x97\x61\x96\x9b\x71\xd7\x9f\x82\x18\xa3\x92\x59\xa7\xa2\x9a\xab\xb2\xdb\xaf\xc3\x1c\xb3\xd3\x5d\xb7\xe3\x9e\xbb\xf3\xdf\xbf";
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(aExpectedOutput)-1, (size_t)vOutput.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vOutput.Get(0), aExpectedOutput, sizeof(aExpectedOutput)-1));

	// Test empty string
	SEOUL_UNITTESTING_ASSERT(Base64Decode("", vOutput));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, vOutput.GetSize());

	// Test padding
	SEOUL_UNITTESTING_ASSERT(Base64Decode("YQ==", vOutput));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1u, vOutput.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vOutput.Get(0), "a", 1));

	SEOUL_UNITTESTING_ASSERT(Base64Decode("YWI=", vOutput));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2u, vOutput.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vOutput.Get(0), "ab", 2));

	SEOUL_UNITTESTING_ASSERT(Base64Decode("YWJj", vOutput));
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, vOutput.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, memcmp(vOutput.Get(0), "abc", 3));

	// Test invalid chars/padding/etc.
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("AB$=", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("AB=$", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("ABC$", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("=", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("==", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("===", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("====", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("A", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("AB", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("ABC", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("A=", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("A==", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("A===", vOutput));
	SEOUL_UNITTESTING_ASSERT(!Base64Decode("AB=", vOutput));
}

/**
 * Tests the URLDecode() function
 */
void StringTest::TestURLDecode(void)
{
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(""), URLDecode(""));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("foo"), URLDecode("foo"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx\xC3\xA9yz"), URLDecode("wx\xC3\xA9yz"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx\xC3\xA9yz"), URLDecode("wx\xC3\xA9yz"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("abc"), URLDecode("a%62c"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("ABCDEFG"), URLDecode("ABC%44EFG"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("\n%:|\\"), URLDecode("%0a%25%3a%7C%5c"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("a%b%x%%"), URLDecode("a%b%x%%"));
}

/**
 * Tests the SplitString() function
 */
void StringTest::TestSplitString(void)
{
	Vector<String> tokens;

	SplitString("this is a  test", ' ', tokens);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, tokens.GetSize());
	SEOUL_UNITTESTING_ASSERT(tokens[0] == "this");
	SEOUL_UNITTESTING_ASSERT(tokens[1] == "is");
	SEOUL_UNITTESTING_ASSERT(tokens[2] == "a");
	SEOUL_UNITTESTING_ASSERT(tokens[3] == "");
	SEOUL_UNITTESTING_ASSERT(tokens[4] == "test");

	SplitString("", ' ', tokens);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, tokens.GetSize());
	SEOUL_UNITTESTING_ASSERT(tokens[0] == "");

	SplitString("Hello\xC3\xA9world", 0xE9, tokens);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)2, tokens.GetSize());
	SEOUL_UNITTESTING_ASSERT(tokens[0] == "Hello");
	SEOUL_UNITTESTING_ASSERT(tokens[1] == "world");
}

/**
 * Tests the StrNCopy() function
 */
void StringTest::TestStrNCopy(void)
{
	Byte sBuffer[8], sReference[8];
	memset(sBuffer, 'a', sizeof(sBuffer));
	memset(sReference, 'a', sizeof(sReference));

	// Test 0 size & return value
	SEOUL_UNITTESTING_ASSERT(StrNCopy(sBuffer, "foo", 0) == sBuffer);
	SEOUL_UNITTESTING_ASSERT(memcmp(sBuffer, sReference, sizeof(sReference)) == 0);

	// Test other cases
	SEOUL_UNITTESTING_ASSERT(StrNCopy(sBuffer, "foo", 1) == sBuffer);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "") == 0);

	SEOUL_UNITTESTING_ASSERT(StrNCopy(sBuffer, "foo", 2) == sBuffer);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "f") == 0);

	StrNCopy(sBuffer, "foo", 4);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "foo") == 0);

	StrNCopy(sBuffer, "foo", 8);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "foo") == 0);
}

/**
 * Tests the StrNCat() function
 */
void StringTest::TestStrNCat(void)
{
	Byte sBuffer[32], sReference[32];
	memset(sBuffer, 'a', sizeof(sBuffer));
	memset(sReference, 'a', sizeof(sReference));

	// Test 0 size & return value
	SEOUL_UNITTESTING_ASSERT(StrNCat(sBuffer, "foo", 0) == sBuffer);
	SEOUL_UNITTESTING_ASSERT(memcmp(sBuffer, sReference, sizeof(sReference)) == 0);
	// Test other cases
	sBuffer[0] = 0;
	SEOUL_UNITTESTING_ASSERT(StrNCat(sBuffer, "foo", 1) == sBuffer);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "") == 0);

	SEOUL_UNITTESTING_ASSERT(StrNCat(sBuffer, "foo", 2) == sBuffer);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "f") == 0);

	StrNCat(sBuffer, "foo", 4);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "ffo") == 0);

	StrNCat(sBuffer, "foo", 7);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "ffofoo") == 0);

	StrNCat(sBuffer, "bar", 32);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "ffofoobar") == 0);

	StrNCat(sBuffer, "bar", 10);
	SEOUL_UNITTESTING_ASSERT(strcmp(sBuffer, "ffofoobar") == 0);
}

void StringTest::TestToString(void)
{
	// Bool
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("true"), ToString(true));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("false"), ToString(false));
	}

	// Int8
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("-128"), ToString(Int8Min));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("127"), ToString(Int8Max));
	}

	// Int16
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("-32768"), ToString(Int16Min));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("32767"), ToString(Int16Max));
	}

	// Int32
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("-2147483648"), ToString(IntMin));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("2147483647"), ToString(IntMax));
	}

	// Int64
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("-9223372036854775808"), ToString(Int64Min));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("9223372036854775807"), ToString(Int64Max));
	}

	// UInt8
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("0"), ToString(0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("255"), ToString(UInt8Max));
	}

	// UInt16
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("65535"), ToString(UInt16Max));
	}

	// UInt32
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("4294967295"), ToString(UIntMax));
	}

	// UInt64
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("18446744073709551615"), ToString(UInt64Max));
	}

	// Float32
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("0"), ToString(0.0f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("1"), ToString(1.0f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("1.5"), ToString(1.5f));
	}

	// Float64
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("0"), ToString(0.0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("1"), ToString(1.0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(String("1.5"), ToString(1.5));
	}
}

void StringTest::TestFromString(void)
{
	// Bool
	{
		Bool bTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("true"), bTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("false"), bTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, bTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("True"), bTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("False"), bTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, bTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("TRUE"), bTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("FALSE"), bTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, bTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), bTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   true  "), bTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("0"), bTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("true     "), bTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("falseasdflkjasdf"), bTest));
	}

	// Int8
	{
		Int8 iTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("-128"), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int8Min, iTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("127"), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int8Max, iTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   123  "), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("128"), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("127     "), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("-128asdf;lkjs"), iTest));
	}

	// Int16
	{
		Int16 iTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("-32768"), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int16Min, iTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("32767"), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int16Max, iTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   16324  "), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("32768"), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("32767     "), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("-32768asdf;lkjs"), iTest));
	}

	// Int32
	{
		Int32 iTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("-2147483648"), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, iTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("2147483647"), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, iTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   12312411  "), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("2147483648"), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("2147483647     "), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("-2147483648asdf;lkjs"), iTest));
	}

	// Int64
	{
		Int64 iTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("-9223372036854775808"), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, iTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("9223372036854775807"), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, iTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   512351235551235  "), iTest));
		// TODO: Need to use something other than _strtoi64 internally to detect this
		// case. Unfortunately, it consumes the entire value but truncates it to the range of
		// an Int64.
		// SEOUL_UNITTESTING_ASSERT(!FromString(String("9223372036854775808"), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("9223372036854775807     "), iTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("-9223372036854775808asdf;lkjs"), iTest));
	}

	// UInt8
	{
		UInt8 uTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("0"), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, uTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("255"), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt8Max, uTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("-1"), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   123  "), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("256"), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("255     "), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("255asdf;lkjs"), uTest));
	}

	// UInt16
	{
		UInt16 uTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("65535"), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16Max, uTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("-1"), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   16324  "), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("65536"), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("65535     "), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("65535asdf;lkjs"), uTest));
	}

	// UInt32
	{
		UInt32 uTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("4294967295"), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, uTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("-1"), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   12312411  "), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("4294967296"), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("4294967295     "), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("4294967295asdf;lkjs"), uTest));
	}

	// UInt64
	{
		UInt64 uTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("18446744073709551615"), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt64Max, uTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("-1"), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   512351235551235  "), uTest));
		// TODO: Need to use something other than _strtou64 internally to detect this
		// case. Unfortunately, it consumes the entire value but truncates it to the range of
		// a UInt64.
		// SEOUL_UNITTESTING_ASSERT(!FromString(String("18446744073709551616"), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("18446744073709551615     "), uTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("18446744073709551615asdf;lkjs"), uTest));
	}

	// Float32
	{
		Float32 fTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("0"), fTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, fTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("1.75"), fTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.75f, fTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), fTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   123  "), fTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("123     "), fTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("123asdf;lkjs"), fTest));
	}

	// Float64
	{
		Float64 fTest;

		// Successful
		SEOUL_UNITTESTING_ASSERT(FromString(String("0"), fTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, fTest);
		SEOUL_UNITTESTING_ASSERT(FromString(String("1.75"), fTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.75, fTest);

		// Failures
		SEOUL_UNITTESTING_ASSERT(!FromString(String(), fTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("   123  "), fTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("123     "), fTest));
		SEOUL_UNITTESTING_ASSERT(!FromString(String("123asdf;lkjs"), fTest));
	}
}

void StringTest::TestToFromString(void)
{
	// Bool
	{
		Bool bTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(true), bTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, bTest);
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(false), bTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, bTest);
	}

	// Int8
	{
		Int8 iTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(Int8Min), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int8Min, iTest);
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(Int8Max), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int8Max, iTest);
	}

	// Int16
	{
		Int16 iTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(Int16Min), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int16Min, iTest);
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(Int16Max), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int16Max, iTest);
	}

	// Int32
	{
		Int32 iTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(IntMin), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, iTest);
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(IntMax), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, iTest);
	}

	// Int64
	{
		Int64 iTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(Int64Min), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, iTest);
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(Int64Max), iTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, iTest);
	}

	// UInt8
	{
		UInt8 uTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(0), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, uTest);
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(UInt8Max), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt8Max, uTest);
	}

	// UInt16
	{
		UInt16 uTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(0), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, uTest);
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(UInt16Max), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16Max, uTest);
	}

	// UInt32
	{
		UInt32 uTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(0), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, uTest);
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(UIntMax), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, uTest);
	}

	// UInt64
	{
		UInt64 uTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(0), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, uTest);
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(UInt64Max), uTest));
		SEOUL_UNITTESTING_ASSERT_EQUAL(UInt64Max, uTest);
	}

	// Float32
	{
		Float32 fTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(-FloatMax), fTest));
		SEOUL_UNITTESTING_ASSERT(Equals(-FloatMax, fTest, 1e33f));
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(FloatMax), fTest));
		SEOUL_UNITTESTING_ASSERT(Equals(FloatMax, fTest, 1e33f));
	}

	// Float64
	{
		Float64 fTest;
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(-DoubleMax), fTest));
		SEOUL_UNITTESTING_ASSERT(Equals(-DoubleMax, fTest, 1e303));
		SEOUL_UNITTESTING_ASSERT(FromString(ToString(DoubleMax), fTest));
		SEOUL_UNITTESTING_ASSERT(Equals(DoubleMax, fTest, 1e303));
	}
}

void StringTest::TestHexParseBytes(void)
{
	{
		String sIn = "000000";
		Vector<UByte> vOut;
		HexParseBytes(sIn, vOut);

		SEOUL_UNITTESTING_ASSERT_EQUAL(3, vOut.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, vOut[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, vOut[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, vOut[2]);
	}

	{
		String sIn = "0190c5FF";
		Vector<UByte> vOut;
		HexParseBytes(sIn, vOut);

		SEOUL_UNITTESTING_ASSERT_EQUAL(4, vOut.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL(1, vOut[0]);   // 01
		SEOUL_UNITTESTING_ASSERT_EQUAL(144, vOut[1]); // 90
		SEOUL_UNITTESTING_ASSERT_EQUAL(197, vOut[2]); // c5
		SEOUL_UNITTESTING_ASSERT_EQUAL(255, vOut[3]); // FF (uppercase)
	}
}

/**
 * Tests basic functionality with empty strings
 */
void StringTest::TestBasicEmptyStrings(void)
{
	// Test default constructor
	String s1;

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);

	// Test copy constructor
	String s2 = s1;

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "") == 0);

	// Test Assign() method
	String s3;
	s3.Assign(s1);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s3.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s3.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s3.CStr(), "") == 0);

	// Test operator=
	String s4;
	s4 = s1;

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s4.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s4.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s4.CStr(), "") == 0);

	// Test assigning to empty string
	s1 = "";

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);
}

/**
 * Tests the String(UniChar) and String(UniChar, UInt) constructors for the
 * String class.
 */
void StringTest::TestCharacterConstructor(void)
{
	String s1('A');

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 2);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "A") == 0);

	String s2('B', 5);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s2.GetCapacity() >= 6);
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "BBBBB") == 0);

	String s3('C', 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s3.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s3.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s3.GetCapacity() >= 1);
	SEOUL_UNITTESTING_ASSERT(strcmp(s3.CStr(), "") == 0);
}

/**
 * Tests the functionality of all of the different forms of the Assign() method
 * of the String class.
 */
void StringTest::TestAssign(void)
{
	String s1 = "Hello";

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 6);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	// Test self-assignment
	s1 = s1;

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 6);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	// Test assigning to shorter string
	s1 = "Hell";

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 6);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hell") == 0);

	// Test assigning to longer string
	s1.Assign("Hello!");

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)6, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 7);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello!") == 0);

	// Test Assign(const Byte *, UInt) version
	s1.Assign("Helloasdfghjkl", 5);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 7);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	// Edge case - size too big
	s1.Assign("Hello", 10);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 7);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	// Edge case - size 0
	s1.Assign("Hello", 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 7);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);

	// Assign(UniChar)
	s1.Assign('A');

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 7);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "A") == 0);

	// Assign(UniChar, UInt)
	s1.Assign('B', 5);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 7);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "BBBBB") == 0);

	// Edge case
	s1.Assign('C', 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 7);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);
}

/**
 * Tests the functionality of all of the different forms of the Append() method
 * of the String class.
 */
void StringTest::TestAppend(void)
{
	String s1, s2 = "C";

	s1.Append("A");
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, s1.GetSize());

	s1.Append("Bxyz", 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)2, s1.GetSize());

	s1.Append(s2);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, s1.GetSize());

	s1.Append('D');
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, s1.GetSize());

	s1.Append('E', 5);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)9, s1.GetSize());

	s1.Append('X', 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)9, s1.GetSize());

	// Test all 3 forms of operator+=
	s1 += "F";
	s1 += s2;
	s1 += 'G';

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)12, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)12, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 13);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "ABCDEEEEEFCG") == 0);

	// Test all 3 forms of operator+, make sure they don't modify arguments
	String s3 = s2 + s2;
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "C") == 0);
	SEOUL_UNITTESTING_ASSERT(strcmp(s3.CStr(), "CC") == 0);

	s3 = s2 + "howder";
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "C") == 0);
	SEOUL_UNITTESTING_ASSERT(strcmp(s3.CStr(), "Chowder") == 0);

	s3 = "Vitamin " + s2;
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "C") == 0);
	SEOUL_UNITTESTING_ASSERT(strcmp(s3.CStr(), "Vitamin C") == 0);
}

/**
 * Tests the functionality of all of the forms of the Compare() method and the
 * 6 comparison operators of the String class.
 */
void StringTest::TestComparisons(void)
{
	String s1 = "Hello";
	String s2 = "Hello";
	String s3 = "Goodbye";

	// Make sure they got different buffers
	SEOUL_UNITTESTING_ASSERT(s1.CStr() != s2.CStr());

	// Test self-comparisons
	SEOUL_UNITTESTING_ASSERT(s1.Compare(s1) == 0);
	SEOUL_UNITTESTING_ASSERT( (s1 == s1));
	SEOUL_UNITTESTING_ASSERT(!(s1 != s1));
	SEOUL_UNITTESTING_ASSERT(!(s1 <  s1));
	SEOUL_UNITTESTING_ASSERT( (s1 <= s1));
	SEOUL_UNITTESTING_ASSERT(!(s1 >  s1));
	SEOUL_UNITTESTING_ASSERT( (s1 >= s1));

	// Test all comparisons of two equal Strings
	SEOUL_UNITTESTING_ASSERT(s1.Compare(s2) == 0);
	SEOUL_UNITTESTING_ASSERT(s2.Compare(s1) == 0);
	SEOUL_UNITTESTING_ASSERT( (s1 == s2));
	SEOUL_UNITTESTING_ASSERT(!(s1 != s2));
	SEOUL_UNITTESTING_ASSERT(!(s1 <  s2));
	SEOUL_UNITTESTING_ASSERT( (s1 <= s2));
	SEOUL_UNITTESTING_ASSERT(!(s1 >  s2));
	SEOUL_UNITTESTING_ASSERT( (s1 >= s2));

	SEOUL_UNITTESTING_ASSERT( (s2 == s1));
	SEOUL_UNITTESTING_ASSERT(!(s2 != s1));
	SEOUL_UNITTESTING_ASSERT(!(s2 <  s1));
	SEOUL_UNITTESTING_ASSERT( (s2 <= s1));
	SEOUL_UNITTESTING_ASSERT(!(s2 >  s1));
	SEOUL_UNITTESTING_ASSERT( (s2 >= s1));

	// Test all comparisons of an equal String and const Byte*
	SEOUL_UNITTESTING_ASSERT(s1.Compare("Hello") == 0);
	SEOUL_UNITTESTING_ASSERT( (s1 == "Hello"));
	SEOUL_UNITTESTING_ASSERT(!(s1 != "Hello"));
	SEOUL_UNITTESTING_ASSERT(!(s1 <  "Hello"));
	SEOUL_UNITTESTING_ASSERT( (s1 <= "Hello"));
	SEOUL_UNITTESTING_ASSERT(!(s1 >  "Hello"));
	SEOUL_UNITTESTING_ASSERT( (s1 >= "Hello"));

	SEOUL_UNITTESTING_ASSERT( ("Hello" == s1));
	SEOUL_UNITTESTING_ASSERT(!("Hello" != s1));
	SEOUL_UNITTESTING_ASSERT(!("Hello" <  s1));
	SEOUL_UNITTESTING_ASSERT( ("Hello" <= s1));
	SEOUL_UNITTESTING_ASSERT(!("Hello" >  s1));
	SEOUL_UNITTESTING_ASSERT( ("Hello" >= s1));

	// Test all comparisons of two unequal Strings
	SEOUL_UNITTESTING_ASSERT(s1.Compare(s3) > 0);
	SEOUL_UNITTESTING_ASSERT(s3.Compare(s1) < 0);
	SEOUL_UNITTESTING_ASSERT(!(s1 == s3));
	SEOUL_UNITTESTING_ASSERT( (s1 != s3));
	SEOUL_UNITTESTING_ASSERT(!(s1 <  s3));
	SEOUL_UNITTESTING_ASSERT(!(s1 <= s3));
	SEOUL_UNITTESTING_ASSERT( (s1 >  s3));
	SEOUL_UNITTESTING_ASSERT( (s1 >= s3));

	SEOUL_UNITTESTING_ASSERT(!(s3 == s1));
	SEOUL_UNITTESTING_ASSERT( (s3 != s1));
	SEOUL_UNITTESTING_ASSERT( (s3 <  s1));
	SEOUL_UNITTESTING_ASSERT( (s3 <= s1));
	SEOUL_UNITTESTING_ASSERT(!(s3 >  s1));
	SEOUL_UNITTESTING_ASSERT(!(s3 >= s1));

	// Test all comparisons of an unequal String and const Byte* (String is
	// larger)
	SEOUL_UNITTESTING_ASSERT(s1.Compare("Goodbye") > 0);
	SEOUL_UNITTESTING_ASSERT(!(s1 == "Goodbye"));
	SEOUL_UNITTESTING_ASSERT( (s1 != "Goodbye"));
	SEOUL_UNITTESTING_ASSERT(!(s1 <  "Goodbye"));
	SEOUL_UNITTESTING_ASSERT(!(s1 <= "Goodbye"));
	SEOUL_UNITTESTING_ASSERT( (s1 >  "Goodbye"));
	SEOUL_UNITTESTING_ASSERT( (s1 >= "Goodbye"));

	SEOUL_UNITTESTING_ASSERT(!("Goodbye" == s1));
	SEOUL_UNITTESTING_ASSERT( ("Goodbye" != s1));
	SEOUL_UNITTESTING_ASSERT( ("Goodbye" <  s1));
	SEOUL_UNITTESTING_ASSERT( ("Goodbye" <= s1));
	SEOUL_UNITTESTING_ASSERT(!("Goodbye" >  s1));
	SEOUL_UNITTESTING_ASSERT(!("Goodbye" >= s1));

	// Test all comparisons of an unequal String and const Byte* (String is
	// smaller)
	SEOUL_UNITTESTING_ASSERT(s1.Compare("Welcome") < 0);
	SEOUL_UNITTESTING_ASSERT(!(s1 == "Welcome"));
	SEOUL_UNITTESTING_ASSERT( (s1 != "Welcome"));
	SEOUL_UNITTESTING_ASSERT( (s1 <  "Welcome"));
	SEOUL_UNITTESTING_ASSERT( (s1 <= "Welcome"));
	SEOUL_UNITTESTING_ASSERT(!(s1 >  "Welcome"));
	SEOUL_UNITTESTING_ASSERT(!(s1 >= "Welcome"));

	SEOUL_UNITTESTING_ASSERT(!("Welcome" == s1));
	SEOUL_UNITTESTING_ASSERT( ("Welcome" != s1));
	SEOUL_UNITTESTING_ASSERT(!("Welcome" <  s1));
	SEOUL_UNITTESTING_ASSERT(!("Welcome" <= s1));
	SEOUL_UNITTESTING_ASSERT( ("Welcome" >  s1));
	SEOUL_UNITTESTING_ASSERT( ("Welcome" >= s1));

	// Test case-insensitive comparisons
	s1 = "hello";
	s2 = "HEllO";
	s3 = "WoRlD";
	String s4 = "_begins_with_an_underscore";

	SEOUL_UNITTESTING_ASSERT(s1.CompareASCIICaseInsensitive(s1) == 0);
	SEOUL_UNITTESTING_ASSERT(s1.CompareASCIICaseInsensitive(s2) == 0);
	SEOUL_UNITTESTING_ASSERT(s1.CompareASCIICaseInsensitive(s3) <  0);
	SEOUL_UNITTESTING_ASSERT(s1.CompareASCIICaseInsensitive(s4) >  0);

	SEOUL_UNITTESTING_ASSERT(s2.CompareASCIICaseInsensitive(s1) == 0);
	SEOUL_UNITTESTING_ASSERT(s2.CompareASCIICaseInsensitive(s2) == 0);
	SEOUL_UNITTESTING_ASSERT(s2.CompareASCIICaseInsensitive(s3) <  0);
	SEOUL_UNITTESTING_ASSERT(s2.CompareASCIICaseInsensitive(s4) >  0);

	SEOUL_UNITTESTING_ASSERT(s3.CompareASCIICaseInsensitive(s1) >  0);
	SEOUL_UNITTESTING_ASSERT(s3.CompareASCIICaseInsensitive(s2) >  0);
	SEOUL_UNITTESTING_ASSERT(s3.CompareASCIICaseInsensitive(s3) == 0);
	SEOUL_UNITTESTING_ASSERT(s3.CompareASCIICaseInsensitive(s4) >  0);

	SEOUL_UNITTESTING_ASSERT(s4.CompareASCIICaseInsensitive(s1) <  0);
	SEOUL_UNITTESTING_ASSERT(s4.CompareASCIICaseInsensitive(s2) <  0);
	SEOUL_UNITTESTING_ASSERT(s4.CompareASCIICaseInsensitive(s3) <  0);
	SEOUL_UNITTESTING_ASSERT(s4.CompareASCIICaseInsensitive(s4) == 0);

	// Test case-insensitive comparisons for String versus C string
	SEOUL_UNITTESTING_ASSERT(s1.CompareASCIICaseInsensitive(s1.CStr()) == 0);
	SEOUL_UNITTESTING_ASSERT(s1.CompareASCIICaseInsensitive(s2.CStr()) == 0);
	SEOUL_UNITTESTING_ASSERT(s1.CompareASCIICaseInsensitive(s3.CStr()) <  0);
	SEOUL_UNITTESTING_ASSERT(s1.CompareASCIICaseInsensitive(s4.CStr()) >  0);

	SEOUL_UNITTESTING_ASSERT(s2.CompareASCIICaseInsensitive(s1.CStr()) == 0);
	SEOUL_UNITTESTING_ASSERT(s2.CompareASCIICaseInsensitive(s2.CStr()) == 0);
	SEOUL_UNITTESTING_ASSERT(s2.CompareASCIICaseInsensitive(s3.CStr()) <  0);
	SEOUL_UNITTESTING_ASSERT(s2.CompareASCIICaseInsensitive(s4.CStr()) >  0);

	SEOUL_UNITTESTING_ASSERT(s3.CompareASCIICaseInsensitive(s1.CStr()) >  0);
	SEOUL_UNITTESTING_ASSERT(s3.CompareASCIICaseInsensitive(s2.CStr()) >  0);
	SEOUL_UNITTESTING_ASSERT(s3.CompareASCIICaseInsensitive(s3.CStr()) == 0);
	SEOUL_UNITTESTING_ASSERT(s3.CompareASCIICaseInsensitive(s4.CStr()) >  0);

	SEOUL_UNITTESTING_ASSERT(s4.CompareASCIICaseInsensitive(s1.CStr()) <  0);
	SEOUL_UNITTESTING_ASSERT(s4.CompareASCIICaseInsensitive(s2.CStr()) <  0);
	SEOUL_UNITTESTING_ASSERT(s4.CompareASCIICaseInsensitive(s3.CStr()) <  0);
	SEOUL_UNITTESTING_ASSERT(s4.CompareASCIICaseInsensitive(s4.CStr()) == 0);
}

/**
 * Tests the functionality of various forms of the Assign() and Append()
 * methods of the String class when used with non-ANSI strings.
 */
void StringTest::TestUTF8Strings(void)
{
	String s1 = String(0xE9);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)2, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 3);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "\xC3\xA9") == 0);

	s1 = String(0x2013, 4);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)12, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 13);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "\xE2\x80\x93\xE2\x80\x93\xE2\x80\x93\xE2\x80\x93") == 0);

	s1 = "wx\xF0\x9D\x84\xA0yz";

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 13);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "wx\xF0\x9D\x84\xA0yz") == 0);

	s1.Assign(0xE9, 2);
	s1.Append(0x2013);
	s1.Append("\xF0\x9D\x84\xA0");

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)11, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)4, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 13);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "\xC3\xA9\xC3\xA9\xE2\x80\x93\xF0\x9D\x84\xA0") == 0);
}

/**
 * Tests the Reserve() method of the String class.
 */
void StringTest::TestReserve(void)
{
	String s1;

	s1.Reserve(16);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)16, s1.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);

	s1 = "Hello";

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)16, s1.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	// Reserving less than current capacity should not change
	s1.Reserve(2);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)16, s1.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	String s2 = s1;

	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), s2.CStr()) == 0);

	// Reserving on a string should copy it, and all other instances should
	// refer to the old copy
	s1.Reserve(17);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)17, s1.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s2.GetCapacity() >= 6);
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "Hello") == 0);

	SEOUL_UNITTESTING_ASSERT(s1.CStr() != s2.CStr());
}

void StringTest::TestTrim(void)
{
	// Test string smaller than 16 characters (the small string optimization)
	String s1 = "Hello";
	s1.Reserve(32);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)32, s1.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	s1.Trim();

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)16, s1.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	// Nothing should change here
	s1.Trim();

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)16, s1.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello") == 0);

	// Test string larger than 16 characters
	String s2 = String('a', 18);
	s2.Reserve(32);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)18, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)18, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)32, s2.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "aaaaaaaaaaaaaaaaaa") == 0);

	s2.Trim();

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)18, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)18, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)19, s2.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "aaaaaaaaaaaaaaaaaa") == 0);

	// Test empty string
	String s3;
	s3.Trim();
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);
}

void StringTest::TestIterators(void)
{
	String s1 = "abc\xC3\xA9\xE2\x80\x93\xF0\x9D\x84\xA0";
	const UniChar achChars[6] = {'a', 'b', 'c', 0xE9, 0x2013, 0x1D120};

	String::Iterator iter;

	// Test preincrement
	Int nIndex;
	for(iter = s1.Begin(), nIndex = 0; iter != s1.End() && nIndex < 6; ++iter, ++nIndex)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[nIndex], *iter);
	}

	SEOUL_UNITTESTING_ASSERT(iter == s1.End());

	// Test predecrement
	for(--iter, --nIndex; nIndex >= 0; --iter, --nIndex)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[nIndex], *iter);

		if(nIndex == 0)
		{
			SEOUL_UNITTESTING_ASSERT(iter == s1.Begin());
			break;  // Don't decrement iter beyond the front
		}
	}

	// Test postincrement
	for(nIndex = 0; nIndex < 6; nIndex++)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[nIndex], *(iter++));
	}

	SEOUL_UNITTESTING_ASSERT(iter == s1.End());

	// Test postdecrement
	iter--;
	for(nIndex = 5; nIndex > 0; nIndex--)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[nIndex], *(iter--));
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[0], *iter);
	SEOUL_UNITTESTING_ASSERT(iter == s1.Begin());

	// Test operator+= and operator-=
	iter += 4;
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[4], *iter);
	iter += -1;
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[3], *iter);

	iter -= 2;
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[1], *iter);
	iter -= -1;
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[2], *iter);

	// Test operator+ and operator-
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[5], *(iter + 3));
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[1], *(iter + -1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[2], *(iter + 0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[1], *(iter - 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[3], *(iter - -1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(achChars[2], *(iter - 0));

	// Test iterators on empty string
	String s2;
	SEOUL_UNITTESTING_ASSERT(s2.Begin() == s2.End());
}

/**
 * Tests the Find(UniChar), Find(String), FindLast(UniChar), FindLast(String),
 * FindFirstOf, FindFirstNotOf, FindLast, FindLastOf, and FindLastNotOf
 * methods.
 */
void StringTest::TestFindMethods(void)
{
	String s1 = "I like jalape\xC3\xB1os";
	String s2 = "";

	// Test Find(UniChar)
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.Find('I'));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)1, s1.Find(' '));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)13, s1.Find(0xF1));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)15, s1.Find('o'));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.Find('x'));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.Find(0xC3));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.Find('a', 9));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.Find('a', 10));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.Find('a', 11));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.Find('a'));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.Find(0xF1));

	// Test Find(String)
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.Find(""));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, s1.Find("ike"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)12, s1.Find("e\xC3\xB1"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)13, s1.Find("\xC3\xB1o"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)15, s1.Find("os"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.Find("lie"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.Find("a", 9));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.Find("a", 10));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.Find("a", 11));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.Find("a"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.Find("\xC3\xB1"));

	// Test FindLast()
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.FindLast('I'));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)16, s1.FindLast('s'));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)6, s1.FindLast(' '));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)13, s1.FindLast(0xF1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindLast('x'));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindLast(0xC3));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindLast('a'));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindLast('a', 10));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindLast('a', 9));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindLast('a', 8));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindLast('a', 7));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindLast('a'));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindLast(0xF1));

	// Test FindLast(String)
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)17, s1.FindLast(""));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, s1.FindLast("ike"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)12, s1.FindLast("e\xC3\xB1"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)13, s1.FindLast("\xC3\xB1o"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)15, s1.FindLast("os"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindLast("lie"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindLast("a"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindLast("a", 10));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindLast("a", 9));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindLast("a", 8));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindLast("a", 7));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindLast("a"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindLast("\xC3\xB1"));

	// Test FindFirstOf()
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindFirstOf("abcd"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindFirstOf("dbca"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindFirstOf("ab\xC3\xB1"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)13, s1.FindFirstOf("s\xC3\xB1o"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindFirstOf("bcdfghmnqrtuvwxyzABCDEFGHJKLMNOPQRSTUVWXYZ01234567890`~!@#$%^&*()-_=+[{]}\\|;:'\",<.>/?"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindFirstOf("abci", 9));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindFirstOf("abci", 10));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindFirstOf("abci", 11));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindFirstOf("abci"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindFirstOf("abc\xC3\xB1"));

	// Test FindFirstNotOf()
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)5, s1.FindFirstNotOf(" abcdilkI"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.FindFirstNotOf(" abcdilk"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)16, s1.FindFirstNotOf(" abcdeijklopI\xC3\xB1"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindFirstNotOf(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\xC3\xB1\xC3\xB2\xC3\xB3" "01234567890`~!@#$%^&*()-_=+[{]}\\|;:'\",<.>/?"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindFirstNotOf("lpe\xC3\xB1os", 9));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindFirstNotOf("lpe\xC3\xB1os", 10));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindFirstNotOf("lpe\xC3\xB1os", 11));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindFirstNotOf("abci"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindFirstNotOf("abc\xC3\xB1"));

	// Test FindLastOf()
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindLastOf("abcd"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)10, s1.FindLastOf("dbca"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)13, s1.FindLastOf("ab\xC3\xB1"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)16, s1.FindLastOf("s\xC3\xB1o"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindLastOf("bcdfghmnqrtuvwxyzABCDEFGHJKLMNOPQRSTUVWXYZ01234567890`~!@#$%^&*()-_=+[{]}\\|;:'\",<.>/?"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindLastOf("apoi", 9));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindLastOf("apo", 8));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindLastOf("apo", 7));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindLastOf("abci"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindLastOf("abc\xC3\xB1"));

	// Test FindLastNotOf()
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)13, s1.FindLastNotOf("os"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)16, s1.FindLastNotOf(" abcdilk\xC3\xB1"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.FindLastNotOf(" abcdeijklops\xC3\xB1"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindLastNotOf(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\xC3\xB1\xC3\xB2\xC3\xB3" "01234567890`~!@#$%^&*()-_=+[{]}\\|;:'\",<.>/?"));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindLastNotOf("I likej", 9));
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)8, s1.FindLastNotOf("I likej", 8));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s1.FindLastNotOf("I likej", 7));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindLastNotOf("abci"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::NPos, s2.FindLastNotOf("abc\xC3\xB1"));

	// Test StartsWith
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, s1.StartsWith(""));
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, s1.StartsWith("I"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, s1.StartsWith("i"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, s1.StartsWith("I like"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, s1.StartsWith("I like jal"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, s1.StartsWith("I like jalo"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, s1.StartsWith(s1));

	// Test EndsWith
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, s1.EndsWith(""));
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, s1.EndsWith("I"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, s1.EndsWith("os"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, s1.EndsWith(s1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(true, s1.EndsWith("\xC3\xB1os"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, s1.EndsWith("like"));
}

/**
 * Tests the functionality of the Substring() method
 */
void StringTest::TestSubstring(void)
{
	String s1 = "Hello, world!";
	String s2 = s1.Substring(0);

	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), s2.CStr()) == 0);

	s2 = s2.Substring(2);

	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Hello, world!") == 0);
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "llo, world!") == 0);

	s2 = s2.Substring(2, 25);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)9, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)9, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, s2.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "o, world!") == 0);

	s2 = s2.Substring(2, 3);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)3, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), s2.GetCapacity());
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), " wo") == 0);

	s2 = s2.Substring(3, 1);
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s2.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(strcmp(s2.CStr(), "") == 0);
}

/**
 * Tests the String::ReplaceAll() method.
 */
void StringTest::TestReplaceAll(void)
{
	// Test replacing strings of equal length
	String s1 = "this is a test";
	String s2 = s1.ReplaceAll(" ", "_");
	SEOUL_UNITTESTING_ASSERT(s1 == "this is a test");
	SEOUL_UNITTESTING_ASSERT(s2 == "this_is_a_test");

	// Test replacing shorter string with longer
	s2 = s1.ReplaceAll(" ", "    ");
	SEOUL_UNITTESTING_ASSERT(s2 == "this    is    a    test");

	// Test replacing longer string with shorter
	s2 = s1.ReplaceAll("is ", " p");
	SEOUL_UNITTESTING_ASSERT(s2 == "th p pa test");
}

/**
 * Tests the String::Reverse() method.
 */
void StringTest::TestReverse(void)
{
	// Test const String
	const String s1 = "FooBar.";
	SEOUL_UNITTESTING_ASSERT(s1.Reverse() == ".raBooF");

	// Test empty String
	String s2 = "";
	SEOUL_UNITTESTING_ASSERT(s2.Reverse().IsEmpty());

	// Test non-ASCII
	s2 = "jalape\xC3\xB1o";
	SEOUL_UNITTESTING_ASSERT(s2.Reverse() == "o\xC3\xB1""epalaj");  // Extra quotes to prevent compiler from reading a 3-digit hex sequence of \xB1e

	// Test larger String
	s2 = "abcdefghijklmnopqrstuvwxyz";
	SEOUL_UNITTESTING_ASSERT(s2.Reverse() == "zyxwvutsrqponmlkjihgfedcba");

	// Test larger String with all different length Unicode characters
	s2 = "Hello \xC2\xA9 How are you? \xE2\x80\x93 I am well \xF0\x9D\x84\xA0";
	SEOUL_UNITTESTING_ASSERT(s2.Reverse() == "\xF0\x9D\x84\xA0 llew ma I \xE2\x80\x93 ?uoy era woH \xC2\xA9 olleH");
}

static void TestTakeOwnershipUtilUtil(Byte const* sValue, Byte const* sInitialValue)
{
	auto u = StrLen(sValue);
	void* p = MemoryManager::Allocate(u, MemoryBudgets::Strings);
	memcpy(p, sValue, u);

	String s(sInitialValue);
	SEOUL_UNITTESTING_ASSERT_EQUAL(StrLen(sInitialValue), s.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, strcmp(sInitialValue, s.CStr()));

	s.TakeOwnership(p, u);

	SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, p);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, u);
	SEOUL_UNITTESTING_ASSERT_EQUAL(sValue, s);
}

static void TestTakeOwnershipUtil(Byte const* sValue)
{
	TestTakeOwnershipUtilUtil(sValue, "");
	TestTakeOwnershipUtilUtil(sValue, "asd");
	TestTakeOwnershipUtilUtil(sValue, "asdf");
	TestTakeOwnershipUtilUtil(sValue, "asdffds");
	TestTakeOwnershipUtilUtil(sValue, "asdffdsa");
	TestTakeOwnershipUtilUtil(sValue, " Hello World - Goodbye World");
}

void StringTest::TestTakeOwnership(void)
{
	// Short strings.
	TestTakeOwnershipUtil("");
	TestTakeOwnershipUtil("abc");
	TestTakeOwnershipUtil("abcd");
	TestTakeOwnershipUtil("abcde");
	TestTakeOwnershipUtil("abcdefgh");

	// Regular string.
	TestTakeOwnershipUtil(" Hello World");
	TestTakeOwnershipUtil(
		"\xf0\x90\x90\xa8\xf0\x90\x90\xa9\xf0\x90\x90\xaa\xf0\x90\x90\xab\xf0\x90\x90\xac\xf0\x90\x90\xad\xf0\x90\x90\xae\xf0\x90\x90\xaf\xf0\x90\x90\xb0\xf0\x90\x90\xb1\xf0\x90\x90\xb2\xf0\x90\x90\xb3\xf0\x90\x90\xb4\xf0\x90\x90\xb5\xf0\x90\x90\xb6\xf0\x90\x90\xb7\xf0\x90\x90\xb8\xf0\x90\x90\xb9\xf0\x90\x90\xba\xf0\x90\x90\xbb\xf0\x90\x90\xbc\xf0\x90\x90\xbd\xf0\x90\x90\xbe\xf0\x90\x90\xbf\xf0\x90\x91\x80\xf0\x90\x91\x81\xf0\x90\x91\x82\xf0\x90\x91\x83\xf0\x90\x91\x84\xf0\x90\x91\x85\xf0\x90\x91\x86\xf0\x90\x91\x87\xf0\x90\x91\x88\xf0\x90\x91\x89\xf0\x90\x91\x8a\xf0\x90\x91\x8b\xf0\x90\x91\x8c\xf0\x90\x91\x8d\xf0\x90\x91\x8e\xf0\x90\x91\x8f"
		"\xf0\x90\x90\x80\xf0\x90\x90\x81\xf0\x90\x90\x82\xf0\x90\x90\x83\xf0\x90\x90\x84\xf0\x90\x90\x85\xf0\x90\x90\x86\xf0\x90\x90\x87\xf0\x90\x90\x88\xf0\x90\x90\x89\xf0\x90\x90\x8a\xf0\x90\x90\x8b\xf0\x90\x90\x8c\xf0\x90\x90\x8d\xf0\x90\x90\x8e\xf0\x90\x90\x8f\xf0\x90\x90\x90\xf0\x90\x90\x91\xf0\x90\x90\x92\xf0\x90\x90\x93\xf0\x90\x90\x94\xf0\x90\x90\x95\xf0\x90\x90\x96\xf0\x90\x90\x97\xf0\x90\x90\x98\xf0\x90\x90\x99\xf0\x90\x90\x9a\xf0\x90\x90\x9b\xf0\x90\x90\x9c\xf0\x90\x90\x9d\xf0\x90\x90\x9e\xf0\x90\x90\x9f\xf0\x90\x90\xa0\xf0\x90\x90\xa1\xf0\x90\x90\xa2\xf0\x90\x90\xa3\xf0\x90\x90\xa4\xf0\x90\x90\xa5\xf0\x90\x90\xa6\xf0\x90\x90\xa7"
		"");
}

/**
 * Tests the String::ToUpper() method.
 */
void StringTest::TestToUpper()
{
	static const struct TestCase
	{
		String m_sStr;
		String m_sExpected;
		String m_sLocale;
	} kaTestCases[] =
		  {
			  // Basic ASCII tests (U+0020 through U+007E)
			  {"", "", ""},
			  {"ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", ""},
			  {"abcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "en"},
			  {"0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c", "0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c", ""},
			  {"abc123ABC", "ABC123ABC", ""},

			  // Latin-1 test (U+00E0 through U+00FF)
			  {"\xc3\xa0\xc3\xa1\xc3\xa2\xc3\xa3\xc3\xa4\xc3\xa5\xc3\xa6\xc3\xa7\xc3\xa8\xc3\xa9\xc3\xaa\xc3\xab\xc3\xac\xc3\xad\xc3\xae\xc3\xaf\xc3\xb0\xc3\xb1\xc3\xb2\xc3\xb3\xc3\xb4\xc3\xb5\xc3\xb6\xc3\xb7\xc3\xb8\xc3\xb9\xc3\xba\xc3\xbb\xc3\xbc\xc3\xbd\xc3\xbe\xc3\xbf",
			   "\xc3\x80\xc3\x81\xc3\x82\xc3\x83\xc3\x84\xc3\x85\xc3\x86\xc3\x87\xc3\x88\xc3\x89\xc3\x8a\xc3\x8b\xc3\x8c\xc3\x8d\xc3\x8e\xc3\x8f\xc3\x90\xc3\x91\xc3\x92\xc3\x93\xc3\x94\xc3\x95\xc3\x96\xc3\xb7\xc3\x98\xc3\x99\xc3\x9a\xc3\x9b\xc3\x9c\xc3\x9d\xc3\x9e\xc5\xb8",
			   ""},

			  // German test (U+00DF)
			  {"Stra\xc3\x9f""e", "STRASSE", ""},

			  // Ligature test (U+FB00 through U+FB06)
			  {"\xef\xac\x80\xef\xac\x81\xef\xac\x82\xef\xac\x83\xef\xac\x84\xef\xac\x85\xef\xac\x86",
			   "FFFIFLFFIFFLSTST",
			   ""},
			  // Armenian ligature test (U+0587, U+FB13 through U+FB17)
			  {"\xd6\x87\xef\xac\x93\xef\xac\x94\xef\xac\x95\xef\xac\x96\xef\xac\x97",
			   "\xd4\xb5\xd5\x92\xd5\x84\xd5\x86\xd5\x84\xd4\xb5\xd5\x84\xd4\xbb\xd5\x8e\xd5\x86\xd5\x84\xd4\xbd",
			   ""},

			  // Turkish/Azeri dotless/dotted i tests (U+0049, U+0069, U+0130,
			  // U+0131)
			  {"Ii\xc4\xb0\xc4\xb1", "II\xc4\xb0I", "en"},
			  {"Ii\xc4\xb0\xc4\xb1", "I\xc4\xb0\xc4\xb0I", "tr"},
			  {"Ii\xc4\xb0\xc4\xb1", "I\xc4\xb0\xc4\xb0I", "az"},

			  // Deseret tests (4-byte UTF-8 chars)
			  {"\xf0\x90\x90\xa8\xf0\x90\x90\xa9\xf0\x90\x90\xaa\xf0\x90\x90\xab\xf0\x90\x90\xac\xf0\x90\x90\xad\xf0\x90\x90\xae\xf0\x90\x90\xaf\xf0\x90\x90\xb0\xf0\x90\x90\xb1\xf0\x90\x90\xb2\xf0\x90\x90\xb3\xf0\x90\x90\xb4\xf0\x90\x90\xb5\xf0\x90\x90\xb6\xf0\x90\x90\xb7\xf0\x90\x90\xb8\xf0\x90\x90\xb9\xf0\x90\x90\xba\xf0\x90\x90\xbb\xf0\x90\x90\xbc\xf0\x90\x90\xbd\xf0\x90\x90\xbe\xf0\x90\x90\xbf\xf0\x90\x91\x80\xf0\x90\x91\x81\xf0\x90\x91\x82\xf0\x90\x91\x83\xf0\x90\x91\x84\xf0\x90\x91\x85\xf0\x90\x91\x86\xf0\x90\x91\x87\xf0\x90\x91\x88\xf0\x90\x91\x89\xf0\x90\x91\x8a\xf0\x90\x91\x8b\xf0\x90\x91\x8c\xf0\x90\x91\x8d\xf0\x90\x91\x8e\xf0\x90\x91\x8f",
			   "\xf0\x90\x90\x80\xf0\x90\x90\x81\xf0\x90\x90\x82\xf0\x90\x90\x83\xf0\x90\x90\x84\xf0\x90\x90\x85\xf0\x90\x90\x86\xf0\x90\x90\x87\xf0\x90\x90\x88\xf0\x90\x90\x89\xf0\x90\x90\x8a\xf0\x90\x90\x8b\xf0\x90\x90\x8c\xf0\x90\x90\x8d\xf0\x90\x90\x8e\xf0\x90\x90\x8f\xf0\x90\x90\x90\xf0\x90\x90\x91\xf0\x90\x90\x92\xf0\x90\x90\x93\xf0\x90\x90\x94\xf0\x90\x90\x95\xf0\x90\x90\x96\xf0\x90\x90\x97\xf0\x90\x90\x98\xf0\x90\x90\x99\xf0\x90\x90\x9a\xf0\x90\x90\x9b\xf0\x90\x90\x9c\xf0\x90\x90\x9d\xf0\x90\x90\x9e\xf0\x90\x90\x9f\xf0\x90\x90\xa0\xf0\x90\x90\xa1\xf0\x90\x90\xa2\xf0\x90\x90\xa3\xf0\x90\x90\xa4\xf0\x90\x90\xa5\xf0\x90\x90\xa6\xf0\x90\x90\xa7",
			   ""},
		  };

	const String kasLocales[] = {"en", "tr", "az", "lt"};

	for (UInt i = 0; i < SEOUL_ARRAY_COUNT(kaTestCases); i++)
	{
		// If the test case is locale-independent, test it against each locale.
		// Otherwise, only test it against its specific locale
		const String *pasLocales;
		size_t nLocales;
		if (kaTestCases[i].m_sLocale.IsEmpty())
		{
			pasLocales = kasLocales;
			nLocales = SEOUL_ARRAY_COUNT(kasLocales);
		}
		else
		{
			pasLocales = &kaTestCases[i].m_sLocale;
			nLocales = 1;
		}

		for (size_t j = 0; j < nLocales; j++)
		{
			String sUpper = kaTestCases[i].m_sStr.ToUpper(pasLocales[j]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(kaTestCases[i].m_sExpected, sUpper);

			// Make sure the uppercasing is idempotent
			String sUpper2 = sUpper.ToUpper(pasLocales[j]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(sUpper, sUpper2);
		}
	}
}

/**
 * Tests the String::ToLower() method.
 */
void StringTest::TestToLower()
{
	static const char *ksLatin1TestInput = "\xc3\x80\xc3\x81\xc3\x82\xc3\x83\xc3\x84\xc3\x85\xc3\x86\xc3\x87\xc3\x88\xc3\x89\xc3\x8a\xc3\x8b\xc3\x8c\xc3\x8d\xc3\x8e\xc3\x8f\xc3\x90\xc3\x91\xc3\x92\xc3\x93\xc3\x94\xc3\x95\xc3\x96\xc3\x97\xc3\x98\xc3\x99\xc3\x9a\xc3\x9b\xc3\x9c\xc3\x9d\xc3\x9e\xc3\x9f";
	static const char *ksLatin1TestOutputEnTrAz = "\xc3\xa0\xc3\xa1\xc3\xa2\xc3\xa3\xc3\xa4\xc3\xa5\xc3\xa6\xc3\xa7\xc3\xa8\xc3\xa9\xc3\xaa\xc3\xab\xc3\xac\xc3\xad\xc3\xae\xc3\xaf\xc3\xb0\xc3\xb1\xc3\xb2\xc3\xb3\xc3\xb4\xc3\xb5\xc3\xb6\xc3\x97\xc3\xb8\xc3\xb9\xc3\xba\xc3\xbb\xc3\xbc\xc3\xbd\xc3\xbe\xc3\x9f";

	static const struct TestCase
	{
		String m_sStr;
		String m_sExpected;
		String m_sLocale;
	} kaTestCases[] =
		  {
			  // Basic ASCII tests (U+0020 through U+007E)
			  {"", "", ""},
			  {"abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz", ""},
			  {"ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz", "en"},
			  {"0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c", "0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c", ""},
			  {"abc123ABC", "abc123abc", ""},

			  // Latin-1 test (U+00C0 through U+00DF).  Lithuanian has special
			  // handling for the dots on accented I's.
			  {ksLatin1TestInput, ksLatin1TestOutputEnTrAz, "en"},
			  {ksLatin1TestInput, ksLatin1TestOutputEnTrAz, "tr"},
			  {ksLatin1TestInput, ksLatin1TestOutputEnTrAz, "az"},
			  {ksLatin1TestInput,
			   "\xc3\xa0\xc3\xa1\xc3\xa2\xc3\xa3\xc3\xa4\xc3\xa5\xc3\xa6\xc3\xa7\xc3\xa8\xc3\xa9\xc3\xaa\xc3\xab\x69\xcc\x87\xcc\x80\x69\xcc\x87\xcc\x81\xc3\xae\xc3\xaf\xc3\xb0\xc3\xb1\xc3\xb2\xc3\xb3\xc3\xb4\xc3\xb5\xc3\xb6\xc3\x97\xc3\xb8\xc3\xb9\xc3\xba\xc3\xbb\xc3\xbc\xc3\xbd\xc3\xbe\xc3\x9f",
			   "lt"},

			  // Turkish/Azeri dotless/dotted i tests (U+0049, U+0069, U+0130,
			  // U+0131)
			  {"Ii\xc4\xb0\xc4\xb1I\xcc\x87", "iii\xcc\x87\xc4\xb1i\xcc\x87", "en"},
			  {"Ii\xc4\xb0\xc4\xb1I\xcc\x87", "\xc4\xb1ii\xc4\xb1i", "tr"},
			  {"Ii\xc4\xb0\xc4\xb1I\xcc\x87", "\xc4\xb1ii\xc4\xb1i", "az"},

			  // Deseret tests (4-byte UTF-8 chars)
			  {"\xf0\x90\x90\x80\xf0\x90\x90\x81\xf0\x90\x90\x82\xf0\x90\x90\x83\xf0\x90\x90\x84\xf0\x90\x90\x85\xf0\x90\x90\x86\xf0\x90\x90\x87\xf0\x90\x90\x88\xf0\x90\x90\x89\xf0\x90\x90\x8a\xf0\x90\x90\x8b\xf0\x90\x90\x8c\xf0\x90\x90\x8d\xf0\x90\x90\x8e\xf0\x90\x90\x8f\xf0\x90\x90\x90\xf0\x90\x90\x91\xf0\x90\x90\x92\xf0\x90\x90\x93\xf0\x90\x90\x94\xf0\x90\x90\x95\xf0\x90\x90\x96\xf0\x90\x90\x97\xf0\x90\x90\x98\xf0\x90\x90\x99\xf0\x90\x90\x9a\xf0\x90\x90\x9b\xf0\x90\x90\x9c\xf0\x90\x90\x9d\xf0\x90\x90\x9e\xf0\x90\x90\x9f\xf0\x90\x90\xa0\xf0\x90\x90\xa1\xf0\x90\x90\xa2\xf0\x90\x90\xa3\xf0\x90\x90\xa4\xf0\x90\x90\xa5\xf0\x90\x90\xa6\xf0\x90\x90\xa7",
			   "\xf0\x90\x90\xa8\xf0\x90\x90\xa9\xf0\x90\x90\xaa\xf0\x90\x90\xab\xf0\x90\x90\xac\xf0\x90\x90\xad\xf0\x90\x90\xae\xf0\x90\x90\xaf\xf0\x90\x90\xb0\xf0\x90\x90\xb1\xf0\x90\x90\xb2\xf0\x90\x90\xb3\xf0\x90\x90\xb4\xf0\x90\x90\xb5\xf0\x90\x90\xb6\xf0\x90\x90\xb7\xf0\x90\x90\xb8\xf0\x90\x90\xb9\xf0\x90\x90\xba\xf0\x90\x90\xbb\xf0\x90\x90\xbc\xf0\x90\x90\xbd\xf0\x90\x90\xbe\xf0\x90\x90\xbf\xf0\x90\x91\x80\xf0\x90\x91\x81\xf0\x90\x91\x82\xf0\x90\x91\x83\xf0\x90\x91\x84\xf0\x90\x91\x85\xf0\x90\x91\x86\xf0\x90\x91\x87\xf0\x90\x91\x88\xf0\x90\x91\x89\xf0\x90\x91\x8a\xf0\x90\x91\x8b\xf0\x90\x91\x8c\xf0\x90\x91\x8d\xf0\x90\x91\x8e\xf0\x90\x91\x8f",
			   ""},
		  };

	const String kasLocales[] = {"en", "tr", "az", "lt"};

	for (UInt i = 0; i < SEOUL_ARRAY_COUNT(kaTestCases); i++)
	{
		// If the test case is locale-independent, test it against each locale.
		// Otherwise, only test it against its specific locale
		const String *pasLocales;
		size_t nLocales;
		if (kaTestCases[i].m_sLocale.IsEmpty())
		{
			pasLocales = kasLocales;
			nLocales = SEOUL_ARRAY_COUNT(kasLocales);
		}
		else
		{
			pasLocales = &kaTestCases[i].m_sLocale;
			nLocales = 1;
		}

		for (size_t j = 0; j < nLocales; j++)
		{
			String sLower = kaTestCases[i].m_sStr.ToLower(pasLocales[j]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(kaTestCases[i].m_sExpected, sLower);

			// Make sure the lowercasing is idempotent
			String sLower2 = sLower.ToLower(pasLocales[j]);
			SEOUL_UNITTESTING_ASSERT_EQUAL(sLower, sLower2);
		}
	}
}

/**
 * Tests the String::ToUpperASCII() method.
 */
void StringTest::TestToUpperASCII()
{
	static const struct TestCase
	{
		String m_sStr;
		String m_sExpected;
	} kaTestCases[] =
		  {
			  {"", ""},
			  {"abcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
			  {"0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c", "0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c"},
			  {"abc123ABC", "ABC123ABC"},
			  {"i\xc3\xa9\xc3\x89\xc4\xb0\xc4\xb1", "I\xc3\xa9\xc3\x89\xc4\xb0\xc4\xb1"}
		  };

	for (UInt i = 0; i < SEOUL_ARRAY_COUNT(kaTestCases); i++)
	{
		String sUpper = kaTestCases[i].m_sStr.ToUpperASCII();
		SEOUL_UNITTESTING_ASSERT_EQUAL(kaTestCases[i].m_sExpected, sUpper);

		// Make sure the uppercasing is idempotent
		String sUpper2 = sUpper.ToUpperASCII();
		SEOUL_UNITTESTING_ASSERT_EQUAL(sUpper, sUpper2);
	}
}

/**
 * Tests the String::ToLowerASCII() method.
 */
void StringTest::TestToLowerASCII()
{
	static const struct TestCase
	{
		String m_sStr;
		String m_sExpected;
	} kaTestCases[] =
		  {
			  {"", ""},
			  {"ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz"},
			  {"0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c", "0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\x0b\x0c"},
			  {"abc123ABC", "abc123abc"},
			  {"I\xc3\xa9\xc3\x89\xc4\xb0\xc4\xb1", "i\xc3\xa9\xc3\x89\xc4\xb0\xc4\xb1"}
		  };

	for (UInt i = 0; i < SEOUL_ARRAY_COUNT(kaTestCases); i++)
	{
		String sLower = kaTestCases[i].m_sStr.ToLowerASCII();
		SEOUL_UNITTESTING_ASSERT_EQUAL(kaTestCases[i].m_sExpected, sLower);

		// Make sure the lowercasing is idempotent
		String sLower2 = sLower.ToLowerASCII();
		SEOUL_UNITTESTING_ASSERT_EQUAL(sLower, sLower2);
	}
}

/**
 * Tests the String::IsASCII() method.
 */
void StringTest::TestIsASCII(void)
{
	SEOUL_UNITTESTING_ASSERT( String().IsASCII());
	SEOUL_UNITTESTING_ASSERT( String("").IsASCII());
	SEOUL_UNITTESTING_ASSERT( String("Hello, world!").IsASCII());
	SEOUL_UNITTESTING_ASSERT(!String("Here is an accent: \xC3\xA9").IsASCII());
	SEOUL_UNITTESTING_ASSERT(!String("Here is a 3-byte character: \xE2\x80\x93").IsASCII());
	SEOUL_UNITTESTING_ASSERT(!String("Here is a 4-byte character: \xF0\x9D\x84\xA0").IsASCII());

	String s1;

	// Add every ASCII character
	for(UniChar c = 0x01; c <= 0x7F; c++)
	{
		s1.Append(c);
	}

	SEOUL_UNITTESTING_ASSERT(s1.IsASCII());

	// Add a non-ASCII character
	s1.Append(0x80);
	SEOUL_UNITTESTING_ASSERT(!s1.IsASCII());
}

/**
 * Tests the String::WStr() method.
 */
void StringTest::TestWStr(void)
{
	String s1;
	String s2 = "foo";
	String s3 = "I like jalape\xC3\xB1os";
	String s4 = "Treble clef: \xF0\x9D\x84\xA0";

	WString w0;
	SEOUL_UNITTESTING_ASSERT((const wchar_t*)w0 != nullptr);
	SEOUL_UNITTESTING_ASSERT(wcscmp(w0, L"") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, w0.GetLengthInChars());

	WString w1 = s1.WStr();
	SEOUL_UNITTESTING_ASSERT(wcscmp(w1, L"") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, w1.GetLengthInChars());

	WString w2 = s2.WStr();
	SEOUL_UNITTESTING_ASSERT(wcscmp(w2, L"foo") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(3u, w2.GetLengthInChars());

	WString w3 = s3.WStr();
	SEOUL_UNITTESTING_ASSERT(wcscmp(w3, L"I like jalape\u00F1os") == 0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(16u, w3.GetLengthInChars());

	WString w4 = w3;
	SEOUL_UNITTESTING_ASSERT(wcscmp(w4, L"I like jalape\u00F1os") == 0);
	SEOUL_UNITTESTING_ASSERT((const wchar_t*)w3 != (const wchar_t*)w4);
	SEOUL_UNITTESTING_ASSERT_EQUAL(16u, w4.GetLengthInChars());

	WString w5; w5 = w3;
	SEOUL_UNITTESTING_ASSERT(wcscmp(w5, L"I like jalape\u00F1os") == 0);
	SEOUL_UNITTESTING_ASSERT((const wchar_t*)w3 != (const wchar_t*)w5);
	SEOUL_UNITTESTING_ASSERT_EQUAL(16u, w5.GetLengthInChars());

	WString w6 = s4.WStr();
	SEOUL_UNITTESTING_ASSERT(wcscmp(w6, L"Treble clef: \U0001D120") == 0);

#if SEOUL_WCHAR_T_IS_2_BYTES
	SEOUL_UNITTESTING_ASSERT_EQUAL(15u, w6.GetLengthInChars());
#else
	SEOUL_UNITTESTING_ASSERT_EQUAL(14u, w6.GetLengthInChars());
#endif
}

/**
 * Tests the String::Swap() method.
 */
void StringTest::TestSwap(void)
{
	String s1 = "abcd";
	String s2 = "efghijklm";
	String s3 = "12345678901234567890";
	String s4 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	// Test all 4 internal cases of String::Swap (short+short, short+long,
	// long+short, and long+long)
	s1.Swap(s2);
	SEOUL_UNITTESTING_ASSERT(s1 == "efghijklm");
	SEOUL_UNITTESTING_ASSERT(s2 == "abcd");
	SEOUL_UNITTESTING_ASSERT_EQUAL(9u, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, s2.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(10, s1.GetCapacity());
	if (sizeof(void*) >= 8)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), s2.GetCapacity());
	}
	else
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, s2.GetCapacity());
	}

	s2.Swap(s1);
	SEOUL_UNITTESTING_ASSERT(s1 == "abcd");
	SEOUL_UNITTESTING_ASSERT(s2 == "efghijklm");
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(9u, s2.GetSize());
	if (sizeof(void*) >= 8)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), s1.GetCapacity());
	}
	else
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, s1.GetCapacity());
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(10u, s2.GetCapacity());

	s1.Swap(s3);
	SEOUL_UNITTESTING_ASSERT(s1 == "12345678901234567890");
	SEOUL_UNITTESTING_ASSERT(s3 == "abcd");
	SEOUL_UNITTESTING_ASSERT_EQUAL(20u, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL( 4u, s3.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(21u, s1.GetCapacity());
	if (sizeof(void*) >= 8)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), s3.GetCapacity());
	}
	else
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, s3.GetCapacity());
	}

	s3.Swap(s1);
	SEOUL_UNITTESTING_ASSERT(s1 == "abcd");
	SEOUL_UNITTESTING_ASSERT(s3 == "12345678901234567890");
	SEOUL_UNITTESTING_ASSERT_EQUAL( 4u, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(20u, s3.GetSize());
	if (sizeof(void*) >= 8)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), s1.GetCapacity());
	}
	else
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(5, s1.GetCapacity());
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(21u, s3.GetCapacity());

	s3.Swap(s4);
	SEOUL_UNITTESTING_ASSERT(s3 == "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	SEOUL_UNITTESTING_ASSERT(s4 == "12345678901234567890");
	SEOUL_UNITTESTING_ASSERT_EQUAL(26u, s3.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(20u, s4.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(27u, s3.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(21u, s4.GetCapacity());

	s4.Swap(s3);
	SEOUL_UNITTESTING_ASSERT(s3 == "12345678901234567890");
	SEOUL_UNITTESTING_ASSERT(s4 == "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	SEOUL_UNITTESTING_ASSERT_EQUAL(20u, s3.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(26u, s4.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(21u, s3.GetCapacity());
	SEOUL_UNITTESTING_ASSERT_EQUAL(27u, s4.GetCapacity());
}

/**
 * Tests the String::Printf() static method.
 */
void StringTest::TestPrintf(void)
{
	const Byte *sEmpty = "";
	String s1 = String::Printf("%s", sEmpty);

	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)0, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 1);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "") == 0);

	s1 = String::Printf("H\xC3\xA9llo, world!");
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)14, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)13, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 15);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "H\xC3\xA9llo, world!") == 0);

	s1 = String::Printf("Int: %d  Float: %g  Char: %c  String: %s", 34, 3.5f, 'A', "Hi!");
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)41, s1.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt)41, s1.GetUnicodeLength());
	SEOUL_UNITTESTING_ASSERT(s1.GetCapacity() >= 42);
	SEOUL_UNITTESTING_ASSERT(strcmp(s1.CStr(), "Int: 34  Float: 3.5  Char: A  String: Hi!") == 0);
}

/**
 * Tests String::PopBack() member function.
 */
void StringTest::TestPopBack()
{
	String s;
	s.Assign("wxyz");
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wxy"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("w"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String(""), s);

	s.Assign("wx\xC3\xA9yz");
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx\xC3\xA9y"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx\xC3\xA9"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("w"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String(""), s);

	s.Assign("wx\xE2\x80\x93yz");
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx\xE2\x80\x93y"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx\xE2\x80\x93"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("w"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String(""), s);

	s.Assign("wx\xF0\x9D\x84\xA0yz");
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx\xF0\x9D\x84\xA0y"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx\xF0\x9D\x84\xA0"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("wx"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String("w"), s);
	s.PopBack(); SEOUL_UNITTESTING_ASSERT_EQUAL(String(""), s);
}

/**
 * Tests String move constructor and assignment.
 */
void StringTest::TestMove()
{
	// Short assignment.
	{
		String sA;
		String sB("asd");

		sA = RvalRef(sB);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), sA.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, sA.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("asd", sA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), sB.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, sB.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("", sB);
	}

	// Short assignment (existing short).
	{
		String sA("a");
		String sB("asd");

		sA = RvalRef(sB);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), sA.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, sA.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("asd", sA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), sB.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, sB.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("", sB);
	}

	// Short assignment (existing long).
	{
		String sA("alsjdsdfhlalfkjhsdf");
		String sB("asd");

		sA = RvalRef(sB);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), sA.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(3, sA.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("asd", sA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), sB.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, sB.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("", sB);
	}

	// Long assignment.
	{
		String sA;
		String sB("asfasdljlaksdjflaksjdlkfj");

		Byte const* sTest = sB.CStr();
		sA = RvalRef(sB);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sTest, sA.CStr());
		SEOUL_UNITTESTING_ASSERT_EQUAL(26, sA.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(25, sA.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("asfasdljlaksdjflaksjdlkfj", sA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), sB.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, sB.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("", sB);
	}

	// Long assignment (short existing).
	{
		String sA("b");
		String sB("asfasdljlaksdjflaksjdlkfj");

		Byte const* sTest = sB.CStr();
		sA = RvalRef(sB);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sTest, sA.CStr());
		SEOUL_UNITTESTING_ASSERT_EQUAL(26, sA.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(25, sA.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("asfasdljlaksdjflaksjdlkfj", sA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), sB.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, sB.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("", sB);
	}

	// Long assignment (long existing).
	{
		String sA("asdfakjsdhflakjs");
		String sB("asfasdljlaksdjflaksjdlkfj");

		Byte const* sTest = sB.CStr();
		sA = RvalRef(sB);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sTest, sA.CStr());
		SEOUL_UNITTESTING_ASSERT_EQUAL(26, sA.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(25, sA.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("asfasdljlaksdjflaksjdlkfj", sA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(sizeof(void*), sB.GetCapacity());
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, sB.GetSize());
		SEOUL_UNITTESTING_ASSERT_EQUAL("", sB);
	}
}

/**
 * Tests String move constructor and assignment.
 */
void StringTest::TestRelinquishBuffer()
{
	// Small buffer
	{
		String s("TES");

		UInt32 const u = s.GetSize();

		void* pU = nullptr;
		UInt32 uU = 0u;
		s.RelinquishBuffer(pU, uU);

		SEOUL_UNITTESTING_ASSERT_EQUAL(u, uU);
		SEOUL_UNITTESTING_ASSERT(0 == strcmp((Byte const*)pU, "TES"));

		MemoryManager::Deallocate(pU);
	}

	// Large buffer
	{
		String s("TEST TEST TEST A");

		UInt32 const u = s.GetSize();

		void* pU = nullptr;
		UInt32 uU = 0u;
		s.RelinquishBuffer(pU, uU);

		SEOUL_UNITTESTING_ASSERT_EQUAL(u, uU);
		SEOUL_UNITTESTING_ASSERT(0 == strcmp((Byte const*)pU, "TEST TEST TEST A"));

		MemoryManager::Deallocate(pU);
	}
}

/**
 * Tests String::Join().
 */
void StringTest::TestToStringVector()
{
	// String String
	{
		Vector<String> vs;

		vs.PushBack("");
		SEOUL_UNITTESTING_ASSERT_EQUAL("", ToString(vs, ""));

		vs.PushBack("");
		SEOUL_UNITTESTING_ASSERT_EQUAL("", ToString(vs, ""));

		vs.Clear();
		vs.PushBack("a");
		SEOUL_UNITTESTING_ASSERT_EQUAL("a", ToString(vs, ","));
		vs.PushBack("b");
		SEOUL_UNITTESTING_ASSERT_EQUAL("a,b", ToString(vs, ","));

		vs.PushBack("c");
		SEOUL_UNITTESTING_ASSERT_EQUAL("a,b,c", ToString(vs, ","));
		SEOUL_UNITTESTING_ASSERT_EQUAL("a, b, c", ToString(vs, ", "));
		SEOUL_UNITTESTING_ASSERT_EQUAL("a b c", ToString(vs, " "));

		vs.Clear();
		vs.PushBack("aa");
		vs.PushBack("bb");
		SEOUL_UNITTESTING_ASSERT_EQUAL("aa,bb", ToString(vs, ","));
		vs.Clear();
		vs.PushBack("a");
		vs.PushBack("bb");
		SEOUL_UNITTESTING_ASSERT_EQUAL("a,bb", ToString(vs, ","));
		vs.Clear();
		vs.PushBack("aa");
		vs.PushBack("b");
		SEOUL_UNITTESTING_ASSERT_EQUAL("aa,b", ToString(vs, ","));
	}

	// Int.
	{
		Vector<Int> vs;

		vs.PushBack(0);
		SEOUL_UNITTESTING_ASSERT_EQUAL("0", ToString(vs, ""));

		vs.PushBack(1);
		SEOUL_UNITTESTING_ASSERT_EQUAL("01", ToString(vs, ""));

		vs.Clear();
		vs.PushBack(1);
		SEOUL_UNITTESTING_ASSERT_EQUAL("1", ToString(vs, ","));
		vs.PushBack(2);
		SEOUL_UNITTESTING_ASSERT_EQUAL("1,2", ToString(vs, ","));

		vs.PushBack(3);
		SEOUL_UNITTESTING_ASSERT_EQUAL("1,2,3", ToString(vs, ","));
		SEOUL_UNITTESTING_ASSERT_EQUAL("1, 2, 3", ToString(vs));
		SEOUL_UNITTESTING_ASSERT_EQUAL("1 2 3", ToString(vs, " "));

		vs.Clear();
		vs.PushBack(11);
		vs.PushBack(22);
		SEOUL_UNITTESTING_ASSERT_EQUAL("11,22", ToString(vs, ","));
		vs.Clear();
		vs.PushBack(1);
		vs.PushBack(22);
		SEOUL_UNITTESTING_ASSERT_EQUAL("1,22", ToString(vs, ","));
		vs.Clear();
		vs.PushBack(11);
		vs.PushBack(2);
		SEOUL_UNITTESTING_ASSERT_EQUAL("11,2", ToString(vs, ","));
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
