/**
 * \file SeoulUtil.cpp
 * \brief Unit test implementation for Seoul utility functions
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulMD5.h"
#include "SeoulUtil.h"
#include "SeoulUtilTest.h"
#include "SeoulUUID.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SeoulUtilTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestCompareVersionStrings)
	SEOUL_METHOD(TestMD5)
	SEOUL_METHOD(TestUUID)
SEOUL_END_TYPE()

/**
 * Helper function to get a useful failure message if a test case in
 * TestCompareVersionStrings fails
 */
String GetTestCompareVersionStringsFailMessage(const String& s1, const String& s2, const char *sExpectedResult)
{
	return String::Printf("CompareVersionStrings(\"%s\", \"%s\") failed: expected %s, got %d", s1.CStr(), s2.CStr(), sExpectedResult, CompareVersionStrings(s1, s2));
}

/**
 * Tests the functionality of the CompareVersionStrings() function
 */
void SeoulUtilTest::TestCompareVersionStrings(void)
{
	// Array of test cases.  The "smaller" version string must come first if
	// they are unequal.
	const struct
	{
		String s1;
		String s2;
		Bool bExpectedEqual;
	} kTestCases[] = {

		{"0",  "1",  false},
		{"9",  "10", false},
		{"99", "99", true},

		{"9.0.2", "9.0.2",  true},
		{"9.0.2", "9.0.3",  false},
		{"9.0.3", "9.1.0",  false},
		{"9.1.1", "10.0.0", false},

		{"",   "",  true},
		{"0",  "",  false},
		{"",   "x", false},
		{"99", "x", false},

		{"1.9",   "1.9x",  false},
		{"1.9x",  "1.9xx", false},
		{"1.9xx", "1.9y",  false},
		{"1.9y",  "1.10",  false},
		{"1.9y",  "1.10a", false},
		{"1.10a", "1.10a", true},

		{"1.x", "1.x", true},
		{"1.x", "1.y", false},

		{"1",   "1.0", false},
		{"1.0", "2",   false}
	};

	// For each test case, make sure the comparison works properly in both
	// directions
	for (UInt i = 0; i < SEOUL_ARRAY_COUNT(kTestCases); i++)
	{
		String s1 = kTestCases[i].s1;
		String s2 = kTestCases[i].s2;

		if(kTestCases[i].bExpectedEqual)
		{
			SEOUL_UNITTESTING_ASSERT_MESSAGE(CompareVersionStrings(s1, s2) == 0, "%s", GetTestCompareVersionStringsFailMessage(s1, s2, "0").CStr());
			SEOUL_UNITTESTING_ASSERT_MESSAGE(CompareVersionStrings(s2, s1) == 0, "%s", GetTestCompareVersionStringsFailMessage(s1, s2, "0").CStr());
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_MESSAGE(CompareVersionStrings(s1, s2) < 0, "%s", GetTestCompareVersionStringsFailMessage(s1, s2, "<0").CStr());
			SEOUL_UNITTESTING_ASSERT_MESSAGE(CompareVersionStrings(s2, s1) > 0, "%s", GetTestCompareVersionStringsFailMessage(s1, s2, ">0").CStr());
		}
	}
}

struct TestMD5TestPair
{
	Byte const* m_sExpectedMD5;
	Byte const* m_sData;
};

void SeoulUtilTest::TestMD5()
{
	static TestMD5TestPair const kaTests[] =
	{
		// Standard MD5 test quite
		{ "d41d8cd98f00b204e9800998ecf8427e", "" }, // empty string
		{ "0cc175b9c0f1b6a831c399e269772661", "a" }, // empty string
		{ "900150983cd24fb0d6963f7d28e17f72", "abc" }, // empty string
		{ "f96b697d7cb7938d525a2f31aaf161d0", "message digest" }, // empty string
		{ "c3fcd3d76192e4007dfb496cca67e13b", "abcdefghijklmnopqrstuvwxyz" }, // empty string
		{ "d174ab98d277d9f5a5611c2c9f419d9f", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" }, // empty string
		{ "57edf4a22be3c955ac49da2e2107b67a", "12345678901234567890123456789012345678901234567890123456789012345678901234567890" },

		// small number of letters
		{ "858e42116e8aad6573a29eb32f45657c", "Shoot Many Robots" },
		// exactly 64 characters
		{ "7af21eac4b0c483745db4bb2b0e47467", "0123456789012345678901234567890123456789012345678901234567891234" },
		// exactly 65 characters
		{ "c1b3d88956f8adc0569e2483f8158f7b", "01234567890123456789012345678901234567890123456789012345678912341" },
	};

	FixedArray<UInt8, MD5::kResultSize> aResult;

	// Standard test.
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(kaTests); ++i)
	{
		{
			MD5 md5(aResult);
			md5.AppendData(kaTests[i].m_sData);
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(String(kaTests[i].m_sExpectedMD5), HexDump(aResult.Data(), aResult.GetSize()).ToLowerASCII());
	}

	// HString test.
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(kaTests); ++i)
	{
		{
			MD5 md5(aResult);
			md5.AppendData(HString(kaTests[i].m_sData));
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(String(kaTests[i].m_sExpectedMD5), HexDump(aResult.Data(), aResult.GetSize()).ToLowerASCII());
	}

	// One at a time test.
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(kaTests); ++i)
	{
		{
			MD5 md5(aResult);

			size_t const zSize = StrLen(kaTests[i].m_sData);
			for (size_t j = 0u; j < zSize; ++j)
			{
				md5.AppendData(&(kaTests[i].m_sData[j]), 1);
			}
		}
		SEOUL_UNITTESTING_ASSERT_EQUAL(String(kaTests[i].m_sExpectedMD5), HexDump(aResult.Data(), aResult.GetSize()).ToLowerASCII());
	}
}

static void TestUUIDUtil(const UUID& uuid)
{
	// Self equality.
	SEOUL_UNITTESTING_ASSERT_EQUAL(uuid, uuid);

	auto const sUUID(uuid.ToString());
	SEOUL_UNITTESTING_ASSERT_EQUAL(36, sUUID.GetSize());

	// Test the variant and version portions.
	auto const& auBytes = uuid.GetBytes();
	SEOUL_UNITTESTING_ASSERT_EQUAL(0x80, (auBytes[8u] & 0xC0)); // Variant is the masked portion of byte 8u.
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, (auBytes[6u] >> 4u)); // Version is the shifted value of byte 6u.

	// All other characters must be a hex digital
	// (0-9a-f).
	for (UInt32 i = 0u; i < sUUID.GetSize(); ++i)
	{
		// These should be a '-'.
		if (8u == i || 13u == i || 18u == i || 23u == i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL('-', sUUID[i]);
		}
		else
		{
			Byte const ch = sUUID[i];
			SEOUL_UNITTESTING_ASSERT((ch >= 'a' && ch <= 'f') || (ch >= '0' && ch <= '9'));
		}
	}

	// Verify that we get an equal UUID back out.
	UUID const uuidB = UUID::FromString(sUUID);
	SEOUL_UNITTESTING_ASSERT_EQUAL(uuid, uuidB);
}

void SeoulUtilTest::TestUUID()
{
	auto uuid = UUID::GenerateV4();
	TestUUIDUtil(uuid);

	UUID::GenerateV4(uuid);
	TestUUIDUtil(uuid);

	// Verify some bad UUIDs.
	SEOUL_UNITTESTING_ASSERT_EQUAL(UUID::Zero(), UUID::FromString(String()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(UUID::Zero(), UUID::FromString("asdf"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(UUID::Zero(), UUID::FromString("888a3b6f-cdc5-469a-9dba-26212f57772")); // Too short.
	SEOUL_UNITTESTING_ASSERT_EQUAL(UUID::Zero(), UUID::FromString("888a3b6f-cdc5-469a-9dba-26212f57772f3")); // Too long.
	SEOUL_UNITTESTING_ASSERT_EQUAL(UUID::Zero(), UUID::FromString("888a3b6f-cdc5-469a-9dba-26212f57772f-")); // Too long.
	SEOUL_UNITTESTING_ASSERT_EQUAL(UUID::Zero(), UUID::FromString("888a3b6f-cdc5-469a-9dba--26212f57772")); // Too many hyphens
	SEOUL_UNITTESTING_ASSERT_EQUAL(UUID::Zero(), UUID::FromString("888a3b6f-cdc5-469a-9dba-26212f5-7772")); // Too many hyphens
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
