/**
 * \file HStringTest.cpp
 * \brief Unit tests for the Seoul::HString class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HashFunctions.h"
#include "HStringTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulHString.h"
#include "Thread.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(HStringTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestCanonical)
	SEOUL_METHOD(TestEmpty)
	SEOUL_METHOD(TestIntConvenience)
	SEOUL_METHOD(TestMultithreaded)
	SEOUL_METHOD(TestSubString)
SEOUL_END_TYPE()

void HStringTest::TestBasic()
{
	HString helloWorld("Hello World");
	Byte const* sEmpty = "";
	Byte const* sHelloWorld = "Hello World";

	SEOUL_UNITTESTING_ASSERT(!helloWorld.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(11u, helloWorld.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, strcmp("Hello World", helloWorld.CStr()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Seoul::GetCaseInsensitiveHash("Hello World"), helloWorld.GetHash());

	SEOUL_UNITTESTING_ASSERT(helloWorld != "");
	SEOUL_UNITTESTING_ASSERT(helloWorld != sEmpty);
	SEOUL_UNITTESTING_ASSERT("" != helloWorld);
	SEOUL_UNITTESTING_ASSERT(sEmpty != helloWorld);

	SEOUL_UNITTESTING_ASSERT(helloWorld != HString(""));
	SEOUL_UNITTESTING_ASSERT(helloWorld != HString(sEmpty));
	SEOUL_UNITTESTING_ASSERT(helloWorld != HString("", true));
	SEOUL_UNITTESTING_ASSERT(helloWorld != HString(sEmpty, 0u, true));

	SEOUL_UNITTESTING_ASSERT(helloWorld == "Hello World");
	SEOUL_UNITTESTING_ASSERT(helloWorld == sHelloWorld);
	SEOUL_UNITTESTING_ASSERT("Hello World" == helloWorld);
	SEOUL_UNITTESTING_ASSERT(sHelloWorld == helloWorld);

	SEOUL_UNITTESTING_ASSERT_EQUAL(helloWorld, HString("Hello World"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(helloWorld, HString(sHelloWorld));
	SEOUL_UNITTESTING_ASSERT_EQUAL(helloWorld, HString("Hello World", true));
	SEOUL_UNITTESTING_ASSERT_EQUAL(helloWorld, HString(sHelloWorld, 11u, true));
}

void HStringTest::TestCanonical()
{
	Byte const* s = "This string must not exist anywhere else in the test app for this function call to succeed.";
	SEOUL_UNITTESTING_ASSERT(HString::SetCanonicalString("This string must not exist anywhere else in the test app for this function call to succeed."));
	SEOUL_UNITTESTING_ASSERT(!HString::SetCanonicalString(String(s).ToLowerASCII()));

	HString caseSensitiveHString(s);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, strcmp(s, caseSensitiveHString.CStr()));

	HString caseInsensitiveHString(s, StrLen(s), true);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, strcmp(s, caseInsensitiveHString.CStr()));

	SEOUL_UNITTESTING_ASSERT_EQUAL(caseSensitiveHString, caseInsensitiveHString);
}

void HStringTest::TestEmpty()
{
	HString empty;
	Byte const* sEmpty = "";
	Byte const* sHelloWorld = "Hello World";

	SEOUL_UNITTESTING_ASSERT(empty.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, empty.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, strcmp("", empty.CStr()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, empty.GetHash());

	SEOUL_UNITTESTING_ASSERT(empty == "");
	SEOUL_UNITTESTING_ASSERT(empty == sEmpty);
	SEOUL_UNITTESTING_ASSERT("" == empty);
	SEOUL_UNITTESTING_ASSERT(sEmpty == empty);

	SEOUL_UNITTESTING_ASSERT_EQUAL(empty, HString(""));
	SEOUL_UNITTESTING_ASSERT_EQUAL(empty, HString(sEmpty));
	SEOUL_UNITTESTING_ASSERT_EQUAL(empty, HString("", true));
	SEOUL_UNITTESTING_ASSERT_EQUAL(empty, HString(sEmpty, 0u, true));

	SEOUL_UNITTESTING_ASSERT(empty != "Hello World");
	SEOUL_UNITTESTING_ASSERT(empty != sHelloWorld);
	SEOUL_UNITTESTING_ASSERT("Hello World" != empty);
	SEOUL_UNITTESTING_ASSERT(sHelloWorld != empty);

	SEOUL_UNITTESTING_ASSERT(empty != HString("Hello World"));
	SEOUL_UNITTESTING_ASSERT(empty != HString(sHelloWorld));
	SEOUL_UNITTESTING_ASSERT(empty != HString("Hello World", true));
	SEOUL_UNITTESTING_ASSERT(empty != HString(sHelloWorld, 11u, true));
}

void HStringTest::TestIntConvenience()
{
	HString empty;
	Int8 i8;
	Int16 i16;
	Int32 i32;
	Int64 i64;
	UInt8 u8;
	UInt16 u16;
	UInt32 u32;
	UInt64 u64;

	// Basic tests.
	static const Int8 kaiTestValues[] = { 10, 37, -2, 123 };
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(kaiTestValues); ++i)
	{
		Int8 const iTestValue = kaiTestValues[i];
		empty = HString(ToString(iTestValue));
		SEOUL_UNITTESTING_ASSERT(empty.ToInt8(i8));
		SEOUL_UNITTESTING_ASSERT_EQUAL(iTestValue, i8);
		SEOUL_UNITTESTING_ASSERT(empty.ToInt16(i16));
		SEOUL_UNITTESTING_ASSERT_EQUAL(iTestValue, i16);
		SEOUL_UNITTESTING_ASSERT(empty.ToInt32(i32));
		SEOUL_UNITTESTING_ASSERT_EQUAL(iTestValue, i32);
		SEOUL_UNITTESTING_ASSERT(empty.ToInt64(i64));
		SEOUL_UNITTESTING_ASSERT_EQUAL(iTestValue, i64);

		if (iTestValue >= 0)
		{
			SEOUL_UNITTESTING_ASSERT(empty.ToUInt8(u8));
			SEOUL_UNITTESTING_ASSERT_EQUAL(iTestValue, u8);
			SEOUL_UNITTESTING_ASSERT(empty.ToUInt16(u16));
			SEOUL_UNITTESTING_ASSERT_EQUAL(iTestValue, u16);
			SEOUL_UNITTESTING_ASSERT(empty.ToUInt32(u32));
			SEOUL_UNITTESTING_ASSERT_EQUAL(iTestValue, u32);
			SEOUL_UNITTESTING_ASSERT(empty.ToUInt64(u64));
			SEOUL_UNITTESTING_ASSERT_EQUAL(iTestValue, u64);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT(!empty.ToUInt8(u8));
			SEOUL_UNITTESTING_ASSERT(!empty.ToUInt16(u16));
			SEOUL_UNITTESTING_ASSERT(!empty.ToUInt32(u32));
			SEOUL_UNITTESTING_ASSERT(!empty.ToUInt64(u64));
		}
	}

	// Specific tests.

	// Int8
	empty = HString("-129");
	SEOUL_UNITTESTING_ASSERT(!empty.ToInt8(i8));
	empty = HString("-128");
	SEOUL_UNITTESTING_ASSERT(empty.ToInt8(i8));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int8Min, i8);
	empty = HString("128");
	SEOUL_UNITTESTING_ASSERT(!empty.ToInt8(i8));
	empty = HString("127");
	SEOUL_UNITTESTING_ASSERT(empty.ToInt8(i8));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int8Max, i8);

	// Int16
	empty = HString("-32769");
	SEOUL_UNITTESTING_ASSERT(!empty.ToInt16(i16));
	empty = HString("-32768");
	SEOUL_UNITTESTING_ASSERT(empty.ToInt16(i16));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int16Min, i16);
	empty = HString("32768");
	SEOUL_UNITTESTING_ASSERT(!empty.ToInt16(i16));
	empty = HString("32767");
	SEOUL_UNITTESTING_ASSERT(empty.ToInt16(i16));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Int16Max, i16);

	// Int32
	empty = HString("-2147483649");
	SEOUL_UNITTESTING_ASSERT(!empty.ToInt32(i32));
	empty = HString("-2147483648");
	SEOUL_UNITTESTING_ASSERT(empty.ToInt32(i32));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, i32);
	empty = HString("2147483648");
	SEOUL_UNITTESTING_ASSERT(!empty.ToInt32(i32));
	empty = HString("2147483647");
	SEOUL_UNITTESTING_ASSERT(empty.ToInt32(i32));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, i32);

	// UInt8
	empty = HString("-1");
	SEOUL_UNITTESTING_ASSERT(!empty.ToUInt8(u8));
	empty = HString("0");
	SEOUL_UNITTESTING_ASSERT(empty.ToUInt8(u8));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, u8);
	empty = HString("256");
	SEOUL_UNITTESTING_ASSERT(!empty.ToUInt8(u8));
	empty = HString("255");
	SEOUL_UNITTESTING_ASSERT(empty.ToUInt8(u8));
	SEOUL_UNITTESTING_ASSERT_EQUAL(UInt8Max, u8);

	// UInt16
	empty = HString("-1");
	SEOUL_UNITTESTING_ASSERT(!empty.ToUInt16(u16));
	empty = HString("0");
	SEOUL_UNITTESTING_ASSERT(empty.ToUInt16(u16));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, u16);
	empty = HString("65536");
	SEOUL_UNITTESTING_ASSERT(!empty.ToUInt16(u16));
	empty = HString("65535");
	SEOUL_UNITTESTING_ASSERT(empty.ToUInt16(u16));
	SEOUL_UNITTESTING_ASSERT_EQUAL(UInt16Max, u16);

	// UInt32
	empty = HString("-1");
	SEOUL_UNITTESTING_ASSERT(!empty.ToUInt32(u32));
	empty = HString("0");
	SEOUL_UNITTESTING_ASSERT(empty.ToUInt32(u32));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, u32);
	empty = HString("4294967296");
	SEOUL_UNITTESTING_ASSERT(!empty.ToUInt32(u32));
	empty = HString("4294967295");
	SEOUL_UNITTESTING_ASSERT(empty.ToUInt32(u32));
	SEOUL_UNITTESTING_ASSERT_EQUAL(UIntMax, u32);
}

Int HStringTest::TestMultithreadedBody(const Thread&)
{
	TestSubString();

	return 237;
}

void HStringTest::TestMultithreaded()
{
	static const Int32 kTestThreadCount = 32;

	ScopedPtr<Thread> apThreads[kTestThreadCount];
	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(SEOUL_BIND_DELEGATE(&HStringTest::TestMultithreadedBody, this), true));
	}

	for (Int32 i = kTestThreadCount - 1; i >= 0; --i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(237, apThreads[i]->WaitUntilThreadIsNotRunning());
	}
}

void HStringTest::TestSubString()
{
	HString helloWorld("Hello World This Is Me", 11u);
	Byte const* sEmpty = "";
	Byte const* sHelloWorld = "Hello World";

	SEOUL_UNITTESTING_ASSERT(!helloWorld.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(11u, helloWorld.GetSizeInBytes());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, strcmp("Hello World", helloWorld.CStr()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(Seoul::GetCaseInsensitiveHash("Hello World"), helloWorld.GetHash());

	SEOUL_UNITTESTING_ASSERT(helloWorld != "");
	SEOUL_UNITTESTING_ASSERT(helloWorld != sEmpty);
	SEOUL_UNITTESTING_ASSERT("" != helloWorld);
	SEOUL_UNITTESTING_ASSERT(sEmpty != helloWorld);

	SEOUL_UNITTESTING_ASSERT(helloWorld != HString(""));
	SEOUL_UNITTESTING_ASSERT(helloWorld != HString(sEmpty));
	SEOUL_UNITTESTING_ASSERT(helloWorld != HString("", true));
	SEOUL_UNITTESTING_ASSERT(helloWorld != HString(sEmpty, 0u, true));

	SEOUL_UNITTESTING_ASSERT(helloWorld == "Hello World");
	SEOUL_UNITTESTING_ASSERT(helloWorld == sHelloWorld);
	SEOUL_UNITTESTING_ASSERT("Hello World" == helloWorld);
	SEOUL_UNITTESTING_ASSERT(sHelloWorld == helloWorld);

	SEOUL_UNITTESTING_ASSERT_EQUAL(helloWorld, HString("Hello World"));
	SEOUL_UNITTESTING_ASSERT_EQUAL(helloWorld, HString(sHelloWorld));
	SEOUL_UNITTESTING_ASSERT_EQUAL(helloWorld, HString("Hello World", true));
	SEOUL_UNITTESTING_ASSERT_EQUAL(helloWorld, HString(sHelloWorld, 11u, true));
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
