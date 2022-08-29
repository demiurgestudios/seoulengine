/**
 * \file SeoulAssert.h
 * \brief SeoulEngine rough equivalent to the standard <cassert>
 * or <assert.h> headers, specifically, the assert() functionality.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_ASSERT_H
#define SEOUL_ASSERT_H

#include "StandardPlatformIncludes.h"
#include "SeoulTypes.h"
#include "StaticAssert.h"

namespace Seoul
{

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

// Assertions are enabled in Debug and Developer builds.  Assertions are disabled
// in Profiling and Ship builds, as well as in Developer tools builds
// (editor and command-line tools), since for tools, Developer is effectively Ship.

#if SEOUL_SHIP || (!SEOUL_DEBUG && SEOUL_EDITOR_AND_TOOLS)
#undef SEOUL_ASSERTIONS_DISABLED
#define SEOUL_ASSERTIONS_DISABLED 1
#else
#ifndef SEOUL_ASSERTIONS_DISABLED
#define SEOUL_ASSERTIONS_DISABLED 0
#endif
#endif

// Set to 1 to enable slow assertions
#define SEOUL_ENABLE_ASSERT_SLOW 0

// Break into a debugger
#if SEOUL_PLATFORM_WINDOWS
#define SEOUL_DEBUG_BREAK() __debugbreak()
#elif SEOUL_PLATFORM_IOS || SEOUL_PLATFORM_ANDROID || SEOUL_PLATFORM_LINUX
#define SEOUL_DEBUG_BREAK() __builtin_trap()
#else
#error "Define for this platform."
#endif

#if !SEOUL_ASSERTIONS_DISABLED

/**
 * Asserts that the given expression is true
 *
 * If expression is true, nothing happens.  Otherwise, calls
 * AssertionFailed() with bLog set to true.  In a Ship build, the
 * expression is not evaluated at all, and thus nothing happens whether it is
 * true or false.
 *
 * @param[in] expression The expression to evaluate
 */
// This is defined using the ternary operator instead of an if () statement so
// that the result is an expression of type void, so we can do things like
// foo ? SEOUL_ASSERT(bar) : baz
#define SEOUL_ASSERT(expression) (void)( (!!(expression)) || (Seoul::AssertionFailed(#expression, "", __FILE__, __PRETTY_FUNCTION__, __LINE__, true), SEOUL_DEBUG_BREAK(), 0))

/**
 * Breaks execution.
 *
 * We've been using SEOUL_ASSERT_MESSAGE(0, "Foo") to notify developers when a
 * section of code is reached when it souldn't be. SEOUL_FAIL("Foo") is a more
 * concise way of expressing this.
 *
 * @param[in] sMessage   Message to display
 */
#define SEOUL_FAIL(sMessage) (Seoul::AssertionFailed("FAILED", sMessage, __FILE__, __PRETTY_FUNCTION__, __LINE__, true), SEOUL_DEBUG_BREAK())

/**
 * Asserts that the given expression is true
 *
 * If expression is true, nothing happens.  Otherwise, calls
 * AssertionFailed() with bLog set to true.  In a Ship build, the
 * expression is not evaluated at all, and thus nothing happens whether it is
 * true or false.
 *
 * @param[in] expression The expression to evaluate
 * @param[in] sMessage   Message to display if assertion fails
 */
#define SEOUL_ASSERT_MESSAGE(expression, sMessage) (void)( (!!(expression)) || (Seoul::AssertionFailed(#expression, sMessage, __FILE__, __PRETTY_FUNCTION__, __LINE__, true), SEOUL_DEBUG_BREAK(), 0))

/**
 * Asserts that the given expression is true
 *
 * If expression is true, nothing happens.  Otherwise, calls
 * AssertionFailed() with bLog set to false.  This macro should only
 * be used by the Logger class; all other code should use SEOUL_ASSERT().  In a
 * Ship build, the expression is not evaluated at all, and thus nothing happens
 * whether it is true or false.
 *
 * @param[in] expression The expression to evaluate
 */
#define SEOUL_ASSERT_NO_LOG(expression) (void)( (!!(expression)) || (Seoul::AssertionFailed(#expression, "", __FILE__, __PRETTY_FUNCTION__, __LINE__, false), SEOUL_DEBUG_BREAK(), 0))

/**
 * Asserts that the given expression is true
 *
 * If expression is true, nothing happens.  Otherwise, calls
 * AssertionFailed() with bLog set to false.  This macro should only
 * be used by the Logger class; all other code should use SEOUL_ASSERT().  In a
 * Ship build, the expression is not evaluated at all, and thus nothing happens
 * whether it is true or false.
 *
 * @param[in] expression The expression to evaluate
 * @param[in] sMessage   Message to display if the assertion fails
 */
#define SEOUL_ASSERT_MESSAGE_NO_LOG(expression, sMessage) (void)( (!!(expression)) || (Seoul::AssertionFailed(#expression, sMessage, __FILE__, __PRETTY_FUNCTION__, __LINE__, false), SEOUL_DEBUG_BREAK(), 0))

/**
 * Verifies that the given expression is true
 *
 * If expression is true, nothing happens.  Otherwise, if building in Debug
 * or Release mode, calls AssertionFailed() with bLog set to true.  If
 * building in Ship mode, no action is taken if the expression is false, but
 * the expression is always evaluated.
 *
 * @param[in] expression The expression to evaluated
 */
#define SEOUL_VERIFY(expression) SEOUL_ASSERT(expression)

/**
 * Verifies that the given expression is true, without
 * issuing a log statement in the event that it is false.
 */
#define SEOUL_VERIFY_NO_LOG(expression) SEOUL_ASSERT_NO_LOG(expression)

/**
 * Verifies that the given expression is true
 *
 * If expression is true, nothing happens.  Otherwise, if building in Debug
 * or Release mode, calls AssertionFailed() with bLog set to true.  If
 * building in Ship mode, no action is taken if the expression is false, but
 * the expression is always evaluated.
 *
 * @param[in] expression The expression to evaluated
 * @param[in] sMessage   Message to display if the expression is false
 */
#define SEOUL_VERIFY_MESSAGE(expression, sMessage) SEOUL_ASSERT_MESSAGE(expression, sMessage)

/**
 * Equivalent to SEOUL_ASSERT(), except only an op in debug builds. Useful
 * for assertions that are too costly for the developer build.
 */
#if SEOUL_DEBUG
#define SEOUL_ASSERT_DEBUG(expression) SEOUL_ASSERT(expression)
#define SEOUL_ASSERT_DEBUG_NO_LOG(expression) SEOUL_ASSERT_NO_LOG(expression)
#else
#define SEOUL_ASSERT_DEBUG(expression) ((void)0)
#define SEOUL_ASSERT_DEBUG_NO_LOG(expression) ((void)0)
#endif

/**
 * Equivalent to SEOUL_ASSERT(), except disabled by default since they're
 * really slow.  Define SEOUL_ENABLE_ASSERT_SLOW to 1 to enable.
 */
#if SEOUL_ENABLE_ASSERT_SLOW
#define SEOUL_ASSERT_SLOW(expression) SEOUL_ASSERT(expression)
#define SEOUL_ASSERT_SLOW_MESSAGE(expression, sMessage) SEOUL_ASSERT_MESSAGE(expression, sMessage)
#else
#define SEOUL_ASSERT_SLOW(expression) ((void)0)
#define SEOUL_ASSERT_SLOW_MESSAGE(expression, sMessage) ((void)0)
#endif

#else  // SEOUL_ASSERTIONS_DISABLED

// If assertions are disabled, set to no-ops that evaluate to void
#define SEOUL_ASSERT(expression) ((void)0)
#define SEOUL_ASSERT_DEBUG(expression) ((void)0)
#define SEOUL_ASSERT_DEBUG_NO_LOG(expression) ((void)0)
#define SEOUL_FAIL(sMessage) ((void)0)
#define SEOUL_ASSERT_MESSAGE(expression, sMessage) ((void)0)
#define SEOUL_ASSERT_NO_LOG(expression) ((void)0)
#define SEOUL_ASSERT_MESSAGE_NO_LOG(expression, sMessage) ((void)0)

#define SEOUL_ASSERT_SLOW(expression) ((void)0)
#define SEOUL_ASSERT_SLOW_MESSAGE(expression, sMessage) ((void)0)

#define SEOUL_VERIFY(expression) ((void)(expression))
#define SEOUL_VERIFY_NO_LOG(expression) ((void)(expression))
#define SEOUL_VERIFY_MESSAGE(expression, sMessage) ((void)(expression))

#endif  // SEOUL_ASSERTIONS_DISABLED

// Function called on assertion failure.  Note that this can actually return in
// some cases, in order to allow the debugger to break on the source line where the
// assertion failed and optionally continue execution, instead of breaking inside
// this function.
void AssertionFailed(const char *sExpression, const char *sMessage, const char *sFile, const char *sFunction, int nLine, bool bLog) SEOUL_ANALYZER_NORETURN;

// Flag to enable/disable showing message boxes on assertion failures
extern Bool g_bShowMessageBoxesOnFailedAssertions;

} // namespace Seoul

#endif // include guard
