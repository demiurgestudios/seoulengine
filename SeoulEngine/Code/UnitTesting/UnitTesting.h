/**
 * \file UnitTesting.h
 * \brief Classes and utilities for implementing unit testing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UNIT_TESTING_H
#define UNIT_TESTING_H

#include "List.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ReflectionWeakAny.h"
#include "SeoulMath.h"
#include "SeoulString.h"
#include "ToString.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

static const UInt32 kMaxUnitTestPrintLength = 100u;

// Functions for manipulating temporary files for testing.
void DeleteAllTempFiles();

// Function that generates a path which is not writable. Useful
// for failure testing on write.
String GetNotWritableTempFileAbsoluteFilename();

// Floating point 0.0, hidden, to suppress warnings about divide by
// zero in an explicit indeterminate test.
extern Float const kfUnitTestZeroConstant;
extern Float const kfUnitTestMaxConstant;

// Compare two files on disk, guarantee that they are byte-for-byte equal.
Bool FilesAreEqual(const String& sFilenameA, const String& sFilenameB);

// Copy a File from source to destination.
Bool CopyFile(const String& sSourceFilename, const String& sDestinationFilename);

// Utility, returns the number of files in the given directly
// with the specified extension.
Bool TestDirCountFiles(
	const String& sDir,
	const String& sExtension,
	Int32& riCount);

// Utility, returns true if directory A is identical to directory B, recursively.
// Meaning, both hierarchies have the same number of (regular) files with
// the given extension, and the contents of these files are binary identical.
Bool TestDirIdenticalRecursive(
	const String& sA,
	const String& sB,
	const String& sExtension,
	Int32 iExpectedFiles);

// General purpose converted for output unit test values.
String GenericUnitTestingToString(const Reflection::WeakAny& p);

template <typename T>
struct ToStringUtil
{
	static String ToString(const T& v)
	{
		return GenericUnitTestingToString(&v);
	}
};
template <>
struct ToStringUtil<char>
{
	static String ToString(char ch) { return String::Printf("'%c'", ch); }
};
template <typename T>
struct ToStringUtil<T const*>
{
	static String ToString(T const* p) { return String::Printf("%p", p); }
};
template <typename T>
struct ToStringUtil<T*>
{
	static String ToString(T* p) { return String::Printf("%p", p); }
};
template <typename T>
struct ToStringUtil< SharedPtr<T> >
{
	static String ToString(const SharedPtr<T>& p) { return String::Printf("%p", p.GetPtr()); }
};
template <>
struct ToStringUtil<Byte const*>
{
	static String ToString(Byte const*);
};

template <typename T>
static inline String UnitTestingToString(T v)
{
	return ToStringUtil<T>::ToString(v);
}

// TODO: Because we only know about
// iterators as (e.g.) List<>::Iterator, it
// is difficult for the compiler to resolve
// this as the proper specialization implicitly,
// so I'm cheating and just type defining based
// on the compiler platform.
#if !defined(__ANDROID__) && (defined (_MSC_VER) || defined(__linux__) || defined(__MINGW32__))
template <typename T>
static inline String UnitTestingToString(const std::_List_iterator<T>& i)
#else
template <typename T, typename U>
static inline String UnitTestingToString(const std::__list_iterator<T, U>& i)
#endif
{
	return String::Printf("%p", std::addressof(*i));
}

/**
* Assert a condition, and fail the test if it isn't true.
*
* Use this to actually test a value. If it fails, it will be logged by the unit test runner.
*
* @param[in] bAssertion  The value to test
*/
#define SEOUL_UNITTESTING_ASSERT(bAssertion) (void)( (!!(bAssertion)) || (SEOUL_LOG_ASSERTION("Assertion: %s(%d): %s", __FILE__, __LINE__, #bAssertion), ((*((unsigned int volatile*)nullptr)) = 1), 0))

/**
* Assert a condition, and fail the test if it isn't true. Output a message on failure.
*
* Use this to actually test a value. If it fails, it will be logged by the unit test runner.
*
* @param[in] bAssertion  The value to test
* @param[in] sMessage  Message to output upon failure
*/
#define SEOUL_UNITTESTING_ASSERT_MESSAGE(bAssertion, sMessage, ...) (void)( (!!(bAssertion)) || (SEOUL_LOG_ASSERTION("Assertion: %s(%d): %s", __FILE__, __LINE__, Seoul::String::Printf((sMessage), ##__VA_ARGS__).CStr()), ((*((unsigned int volatile*)nullptr)) = 1), 0))
#define SEOUL_UNITTESTING_ASSERT_MESSAGE_FILE_LINE(bAssertion, sFile, iLine, sMessage, ...) (void)( (!!(bAssertion)) || (SEOUL_LOG_ASSERTION("Assertion: %s(%d): %s", sFile, iLine, Seoul::String::Printf((sMessage), ##__VA_ARGS__).CStr()), ((*((unsigned int volatile*)nullptr)) = 1), 0))

/**
* Fail the test, and output a specific message.
*
* Manually fail a test without making an assertion
*
* @param[in] sMessage  The message to output
*/
#define SEOUL_UNITTESTING_FAIL(sMessage, ...) SEOUL_UNITTESTING_ASSERT_MESSAGE(false, sMessage, ##__VA_ARGS__)

/**
* Assert two values are equal, not-equal, less-than, less-equal, greater-than, or greater-equal.
*
* These test a value against an expected value. Use the equality operators to test the values.
*
* @param[in] ExpectedValue  The value to test
* @param[in] ActualValue  The expected value
*/
template <typename A, typename B> static inline void UnitTestAssertEqual(A&& e, B&& a, Byte const* sFile, Int iLine) { SEOUL_UNITTESTING_ASSERT_MESSAGE_FILE_LINE((e) == (a), sFile, iLine, "(%s == %s)", UnitTestingToString(e).CStr(), UnitTestingToString(a).CStr()); }
#define SEOUL_UNITTESTING_ASSERT_EQUAL(ExpectedValue, ActualValue) UnitTestAssertEqual((ExpectedValue), (ActualValue), __FILE__, __LINE__)
template <typename A, typename B> static inline void UnitTestAssertNotEqual(A&& e, B&& a, Byte const* sFile, Int iLine) { SEOUL_UNITTESTING_ASSERT_MESSAGE_FILE_LINE((e) != (a), sFile, iLine, "(%s != %s)", UnitTestingToString(e).CStr(), UnitTestingToString(a).CStr()); }
#define SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(ExpectedValue, ActualValue) UnitTestAssertNotEqual((ExpectedValue), (ActualValue), __FILE__, __LINE__)
template <typename A, typename B> static inline void UnitTestAssertLessThan(A&& e, B&& a, Byte const* sFile, Int iLine) { SEOUL_UNITTESTING_ASSERT_MESSAGE_FILE_LINE((e) < (a), sFile, iLine, "(%s < %s)", UnitTestingToString(e).CStr(), UnitTestingToString(a).CStr()); }
#define SEOUL_UNITTESTING_ASSERT_LESS_THAN(ExpectedValue, ActualValue) UnitTestAssertLessThan((ExpectedValue), (ActualValue), __FILE__, __LINE__)
template <typename A, typename B> static inline void UnitTestAssertLessEqual(A&& e, B&& a, Byte const* sFile, Int iLine) { SEOUL_UNITTESTING_ASSERT_MESSAGE_FILE_LINE((e) <= (a), sFile, iLine, "(%s <= %s)", UnitTestingToString(e).CStr(), UnitTestingToString(a).CStr()); }
#define SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(ExpectedValue, ActualValue) UnitTestAssertLessEqual((ExpectedValue), (ActualValue), __FILE__, __LINE__)
template <typename A, typename B> static inline void UnitTestAssertGreaterThan(A&& e, B&& a, Byte const* sFile, Int iLine) { SEOUL_UNITTESTING_ASSERT_MESSAGE_FILE_LINE((e) > (a), sFile, iLine, "(%s > %s)", UnitTestingToString(e).CStr(), UnitTestingToString(a).CStr()); }
#define SEOUL_UNITTESTING_ASSERT_GREATER_THAN(ExpectedValue, ActualValue) UnitTestAssertGreaterThan((ExpectedValue), (ActualValue), __FILE__, __LINE__)
template <typename A, typename B> static inline void UnitTestAssertGreaterEqual(A&& e, B&& a, Byte const* sFile, Int iLine) { SEOUL_UNITTESTING_ASSERT_MESSAGE_FILE_LINE((e) >= (a), sFile, iLine, "(%s >= %s)", UnitTestingToString(e).CStr(), UnitTestingToString(a).CStr()); }
#define SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(ExpectedValue, ActualValue) UnitTestAssertGreaterEqual((ExpectedValue), (ActualValue), __FILE__, __LINE__)

/**
* Assert two values are equal, not-equal, less-than, less-equal, greater-than, or greater-equal. Output a message on failure.
*
* These test a value against an expected value. Use the equality operators to test the values.
*
* @param[in] ExpectedValue  The value to test
* @param[in] ActualValue  The expected value
* @param[in] sMessage  Optional message to output upon failure
*/
#define SEOUL_UNITTESTING_ASSERT_EQUAL_MESSAGE(ExpectedValue, ActualValue, sMessage, ...) SEOUL_UNITTESTING_ASSERT_MESSAGE((ExpectedValue) == (ActualValue), sMessage, ##__VA_ARGS__)
#define SEOUL_UNITTESTING_ASSERT_NOT_EQUAL_MESSAGE(ExpectedValue, ActualValue, sMessage, ...) SEOUL_UNITTESTING_ASSERT_MESSAGE((ExpectedValue) != (ActualValue), sMessage, ##__VA_ARGS__)
#define SEOUL_UNITTESTING_ASSERT_LESS_THAN_MESSAGE(ExpectedValue, ActualValue, sMessage, ...) SEOUL_UNITTESTING_ASSERT_MESSAGE((ExpectedValue) < (ActualValue), sMessage, ##__VA_ARGS__)
#define SEOUL_UNITTESTING_ASSERT_LESS_EQUAL_MESSAGE(ExpectedValue, ActualValue, sMessage, ...) SEOUL_UNITTESTING_ASSERT_MESSAGE((ExpectedValue) <= (ActualValue), sMessage, ##__VA_ARGS__)
#define SEOUL_UNITTESTING_ASSERT_GREATER_THAN_MESSAGE(ExpectedValue, ActualValue, sMessage, ...) SEOUL_UNITTESTING_ASSERT_MESSAGE((ExpectedValue) > (ActualValue), sMessage, ##__VA_ARGS__)
#define SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL_MESSAGE(ExpectedValue, ActualValue, sMessage, ...) SEOUL_UNITTESTING_ASSERT_MESSAGE((ExpectedValue) >= (ActualValue), sMessage, ##__VA_ARGS__)

/**
* Assert two values are equal.
*
* These test a value against an expected value within a maximum delta.
*
* @param[in] fExpectedValue  The value to test
* @param[in] fActualValue  The expected value
* @param[in] fDeltaValue  The delta to test - fabs(expected-actual) <= delta
*/
#define SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fExpectedValue, fActualValue, fDeltaValue) SEOUL_UNITTESTING_ASSERT_MESSAGE(Equals(fExpectedValue, fActualValue, fDeltaValue), "Equals(%s, %s, %s)", UnitTestingToString(fExpectedValue).CStr(), UnitTestingToString(fActualValue).CStr(), UnitTestingToString(fDeltaValue).CStr())

/**
* Assert two values are equal. Output a message on failure.
*
* These test a value against an expected value within a maximum delta.
*
* @param[in] fExpectedValue  The value to test
* @param[in] fActualValue  The expected value
* @param[in] fDeltaValue  The delta to test - fabs(expected-actual) <= delta
* @param[in] sMessage  Optional message to output upon failure
*/
#define SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL_MESSAGE(fExpectedValue, fActualValue, fDeltaValue, sMessage, ...) SEOUL_UNITTESTING_ASSERT_MESSAGE(Equals(fExpectedValue, fActualValue, fDeltaValue), sMessage, ##__VA_ARGS__)

// Convenience, directory to use for in flight unit test files during testing.
String GetUnitTestingSaveDir();

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
