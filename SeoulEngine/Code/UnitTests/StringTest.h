/**
 * \file StringTest.h
 * \brief Unit test header file for String class
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STRING_TEST_H
#define STRING_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class StringTest SEOUL_SEALED
{
public:
	// StringUtil tests
	void TestIsValidUnicodeChar(void);
	void TestIsValidUTF8String(void);
	void TestUTF8Strlen(void);
	void TestUTF8BytesPerChar(void);
	void TestUTF8EncodeChar(void);
	void TestUTF8DecodeChar(void);
	void TestUTF8ToISO88591(void);
	void TestUTF8ToWindows1252(void);
	void TestUTF8ToWCharT(void);
	void TestISO88591ToUTF8(void);
	void TestWindows1252ToUTF8(void);
	void TestWCharTToUTF8(void);
	void TestTranslateStringToUTF8(void);
	void TestBase64Encode(void);
	void TestBase64Decode(void);
	void TestURLDecode(void);
	void TestSplitString(void);
	void TestStrNCopy(void);
	void TestStrNCat(void);
	void TestToString(void);
	void TestFromString(void);
	void TestToFromString(void);
	void TestHexParseBytes(void);

	// String class tests
	void TestBasicEmptyStrings(void);
	void TestCharacterConstructor(void);
	void TestAssign(void);
	void TestAppend(void);
	void TestComparisons(void);
	void TestUTF8Strings(void);
	void TestReserve(void);
	void TestTrim(void);
	void TestIterators(void);
	void TestFindMethods(void);
	void TestSubstring(void);
	void TestReplaceAll(void);
	void TestReverse(void);
	void TestTakeOwnership(void);
	void TestToUpper(void);
	void TestToLower(void);
	void TestToUpperASCII(void);
	void TestToLowerASCII(void);
	void TestIsASCII(void);
	void TestWStr(void);
	void TestSwap(void);
	void TestPrintf(void);
	void TestPopBack(void);
	void TestMove(void);
	void TestRelinquishBuffer(void);
	
	void TestToStringVector(void);
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
