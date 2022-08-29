/**
 * \file ReflectionUnitTestRunner.h
 * \brief Defines a runner that executes unit tests using the Reflection system
 * to enumerate available unit tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef REFLECTION_UNIT_TEST_RUNNER_H
#define REFLECTION_UNIT_TEST_RUNNER_H

#include "Prereqs.h"
#include "Platform.h"
#include "ReflectionMethod.h"
#include "ReflectionAny.h"
namespace Seoul { class String; }

namespace Seoul::UnitTesting
{

#if SEOUL_BENCHMARK_TESTS
void RunBenchmarks(const String& sOptionalTestName);
#endif // /SEOUL_BENCHMARK_TESTS

#if SEOUL_UNIT_TESTS
Bool RunUnitTests(const String& sOptionalTestName);

#ifdef _MSC_VER
Int UnitTestsExceptionFilter(
	DWORD uExceptionCode,
	LPEXCEPTION_POINTERS pExceptionInfo);
#else
void SignalHandler(Int /*iSigNum*/);
#endif

Bool TestMethodWrapper(Reflection::Any& rReturnValue, const Reflection::Method& method, const Reflection::WeakAny& weakThis);

void TestMethod(const Reflection::Type& rootType, const Reflection::Method& method, const Reflection::WeakAny& weakThis);

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul::UnitTesting

#endif // include guard
