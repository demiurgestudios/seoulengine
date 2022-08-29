/**
 * \file SeoulRegexTest.cpp
 * \brief Unit tests for the Regex class.
 *
 * \sa https://github.com/mattbucknall/subreg/blob/master/tests/subreg-tests.c
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulRegex.h"
#include "SeoulRegexTest.h"
#include "Thread.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SeoulRegexTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAny)
	SEOUL_METHOD(TestBackspace)
	SEOUL_METHOD(TestCarriageReturn)
	SEOUL_METHOD(TestEmptyFail)
	SEOUL_METHOD(TestEmptyPass)
	SEOUL_METHOD(TestFormFeed)
	SEOUL_METHOD(TestHorizontalTab)
	SEOUL_METHOD(TestLiteral)
	SEOUL_METHOD(TestMultiThreaded)
	SEOUL_METHOD(TestNewLine)
	SEOUL_METHOD(TestOneOrMoreFail)
	SEOUL_METHOD(TestOneOrMorePass)
	SEOUL_METHOD(TestOptionalNone)
	SEOUL_METHOD(TestOptionalOne)
	SEOUL_METHOD(TestOrNoneOfThree)
	SEOUL_METHOD(TestOrNoneOfTwo)
	SEOUL_METHOD(TestOrOneOfThree)
	SEOUL_METHOD(TestOrOneOfTwo)
	SEOUL_METHOD(TestOrThreeOfThree)
	SEOUL_METHOD(TestOrTwoOfThree)
	SEOUL_METHOD(TestOrTwoOfTwo)
	SEOUL_METHOD(TestSimpleFail)
	SEOUL_METHOD(TestSimplePass)
	SEOUL_METHOD(TestVerticalTab)
	SEOUL_METHOD(TestZeroOrMore)
SEOUL_END_TYPE()

static Byte const kAny[] =
{
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23,       0x25, 0x26, 0x27,
	0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5A, 0x5B,       0x5D,       0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7A, 0x7B,       0x7D, 0x7E,

	0x00
};

static Byte const kLiteral[] =
{
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23,       0x25, 0x26, 0x27,
	0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5A,             0x5D,       0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7A,             0x7D, 0x7E,

	0x00
};

void SeoulRegexTest::TestAny()
{
	Byte aBuffer[2] = { 0 };
	aBuffer[1] = '\0';

	for (auto p = kAny; '\0' != *p; ++p)
	{
		aBuffer[0] = *p;
		SEOUL_UNITTESTING_ASSERT(Regex(".").IsMatch(aBuffer));
		SEOUL_UNITTESTING_ASSERT(Regex(".").IsExactMatch(aBuffer));
	}
}

void SeoulRegexTest::TestBackspace()
{
	SEOUL_UNITTESTING_ASSERT(Regex("\\b").IsMatch("\b"));
	SEOUL_UNITTESTING_ASSERT(Regex("\\b").IsExactMatch("\b"));
}

void SeoulRegexTest::TestCarriageReturn()
{
	SEOUL_UNITTESTING_ASSERT(Regex("\\r").IsMatch("\r"));
	SEOUL_UNITTESTING_ASSERT(Regex("\\r").IsExactMatch("\r"));
}

void SeoulRegexTest::TestEmptyFail()
{
	SEOUL_UNITTESTING_ASSERT(!Regex("x").IsMatch(""));
	SEOUL_UNITTESTING_ASSERT(!Regex("x").IsExactMatch(""));
}

void SeoulRegexTest::TestEmptyPass()
{
	SEOUL_UNITTESTING_ASSERT(Regex("\x09").IsMatch("\x09"));
	SEOUL_UNITTESTING_ASSERT(Regex("\x09").IsExactMatch("\x09"));
}

void SeoulRegexTest::TestFormFeed()
{
	SEOUL_UNITTESTING_ASSERT(Regex("\\f").IsMatch("\f"));
	SEOUL_UNITTESTING_ASSERT(Regex("\\f").IsExactMatch("\f"));
}

void SeoulRegexTest::TestHorizontalTab()
{
	SEOUL_UNITTESTING_ASSERT(Regex("\\t").IsMatch("\t"));
	SEOUL_UNITTESTING_ASSERT(Regex("\\t").IsExactMatch("\t"));
}

void SeoulRegexTest::TestLiteral()
{
	Byte aBuffer[2] = { 0 };
	aBuffer[1] = '\0';

	for (auto p = kLiteral; '\0' != *p; ++p)
	{
		aBuffer[0] = *p;
		SEOUL_UNITTESTING_ASSERT(Regex(aBuffer).IsMatch(aBuffer));
		SEOUL_UNITTESTING_ASSERT(Regex(aBuffer).IsExactMatch(aBuffer));
	}
}

// Regression test - rapidjson's regex was not thread safe
// in an old version.
void SeoulRegexTest::TestMultiThreaded()
{
	struct Tester
	{
		SEOUL_DELEGATE_TARGET(Tester);

		Regex const m_Regex;

		Tester()
			: m_Regex(".*")
		{
		}

		Int Test(const Thread& thread)
		{
			Byte aBuffer[256];
			for (UInt32 i = 0u; i < 128; ++i)
			{
				for (Int32 i = 0; i < 255; ++i)
				{
					aBuffer[i] = '\0';
					m_Regex.IsMatch(aBuffer);
					m_Regex.IsExactMatch(aBuffer);
					aBuffer[i] = 'x';
				}
			}

			return 0;
		}
	};

	Tester tester;
	Vector<Thread*, MemoryBudgets::Developer> vThreads;
	vThreads.Resize(Thread::GetProcessorCount());
	for (auto& e : vThreads)
	{
		e = SEOUL_NEW(MemoryBudgets::Developer) Thread(SEOUL_BIND_DELEGATE(&Tester::Test, &tester), false);
	}

	for (auto& e : vThreads)
	{
		e->Start();
	}

	for (auto& e : vThreads)
	{
		e->WaitUntilThreadIsNotRunning();
	}

	SafeDeleteVector(vThreads);
}

void SeoulRegexTest::TestNewLine()
{
	SEOUL_UNITTESTING_ASSERT(Regex("\\n").IsMatch("\n"));
	SEOUL_UNITTESTING_ASSERT(Regex("\\n").IsExactMatch("\n"));
}

void SeoulRegexTest::TestOneOrMoreFail()
{
	SEOUL_UNITTESTING_ASSERT(!Regex(".+").IsMatch(""));
	SEOUL_UNITTESTING_ASSERT(!Regex(".+").IsExactMatch(""));
}

void SeoulRegexTest::TestOneOrMorePass()
{
	Byte aBuffer[256];
	aBuffer[0] = 'x';

	for (Int32 i = 1; i < 255; ++i)
	{
		aBuffer[i] = '\0';
		SEOUL_UNITTESTING_ASSERT(Regex(".+").IsMatch(aBuffer));
		SEOUL_UNITTESTING_ASSERT(Regex(".+").IsExactMatch(aBuffer));
		aBuffer[i] = 'x';
	}
}

void SeoulRegexTest::TestOptionalNone()
{
	SEOUL_UNITTESTING_ASSERT(Regex("x?").IsMatch(""));
	SEOUL_UNITTESTING_ASSERT(Regex("x?").IsExactMatch(""));
}

void SeoulRegexTest::TestOptionalOne()
{
	SEOUL_UNITTESTING_ASSERT(Regex("x?").IsMatch("x"));
	SEOUL_UNITTESTING_ASSERT(Regex("x?").IsExactMatch("x"));
}

void SeoulRegexTest::TestOrNoneOfThree()
{
	SEOUL_UNITTESTING_ASSERT(!Regex("a|b|c").IsMatch("d"));
	SEOUL_UNITTESTING_ASSERT(!Regex("a|b|c").IsExactMatch("d"));
}

void SeoulRegexTest::TestOrNoneOfTwo()
{
	SEOUL_UNITTESTING_ASSERT(!Regex("a|b").IsMatch("c"));
	SEOUL_UNITTESTING_ASSERT(!Regex("a|b").IsExactMatch("c"));
}

void SeoulRegexTest::TestOrOneOfThree()
{
	SEOUL_UNITTESTING_ASSERT(Regex("a|b|c").IsMatch("a"));
	SEOUL_UNITTESTING_ASSERT(Regex("a|b|c").IsExactMatch("a"));
}

void SeoulRegexTest::TestOrOneOfTwo()
{
	SEOUL_UNITTESTING_ASSERT(Regex("a|b").IsMatch("a"));
	SEOUL_UNITTESTING_ASSERT(Regex("a|b").IsExactMatch("a"));
}

void SeoulRegexTest::TestOrThreeOfThree()
{
	SEOUL_UNITTESTING_ASSERT(Regex("a|b|c").IsMatch("c"));
	SEOUL_UNITTESTING_ASSERT(Regex("a|b|c").IsExactMatch("c"));
}

void SeoulRegexTest::TestOrTwoOfThree()
{
	SEOUL_UNITTESTING_ASSERT(Regex("a|b|c").IsMatch("b"));
	SEOUL_UNITTESTING_ASSERT(Regex("a|b|c").IsExactMatch("b"));
}

void SeoulRegexTest::TestOrTwoOfTwo()
{
	SEOUL_UNITTESTING_ASSERT(Regex("a|b").IsMatch("b"));
	SEOUL_UNITTESTING_ASSERT(Regex("a|b").IsExactMatch("b"));
}

void SeoulRegexTest::TestSimpleFail()
{
	SEOUL_UNITTESTING_ASSERT(!Regex("hello").IsMatch("goodbye"));
	SEOUL_UNITTESTING_ASSERT(!Regex("hello").IsExactMatch("goodbye"));
}

void SeoulRegexTest::TestSimplePass()
{
	SEOUL_UNITTESTING_ASSERT(Regex("hello").IsMatch("hello"));
	SEOUL_UNITTESTING_ASSERT(Regex("hello").IsExactMatch("hello"));
}

void SeoulRegexTest::TestVerticalTab()
{
	SEOUL_UNITTESTING_ASSERT(Regex("\\v").IsMatch("\v"));
	SEOUL_UNITTESTING_ASSERT(Regex("\\v").IsExactMatch("\v"));
}

void SeoulRegexTest::TestZeroOrMore()
{
	Byte aBuffer[256];

	for (Int32 i = 0; i < 255; ++i)
	{
		aBuffer[i] = '\0';
		SEOUL_UNITTESTING_ASSERT(Regex(".*").IsMatch(aBuffer));
		SEOUL_UNITTESTING_ASSERT(Regex(".*").IsExactMatch(aBuffer));
		aBuffer[i] = 'x';
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
