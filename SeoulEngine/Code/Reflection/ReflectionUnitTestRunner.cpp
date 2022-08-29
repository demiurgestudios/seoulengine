/**
 * \file ReflectionUnitTestRunner.cpp
 * \brief Defines a runner that executes unit tests using the Reflection system
 * to enumerate available unit tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FixedArray.h"
#include "Logger.h"
#include "Platform.h"
#include "ReflectionAttributes.h"
#include "ReflectionMethod.h"
#include "ReflectionMethodTypeInfo.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "ReflectionUnitTestRunner.h"
#include "ScopedAction.h"
#include "StringUtil.h"
#include "Thread.h"

#ifdef _MSC_VER
#include <ImageHlp.h>
#else
#include <setjmp.h>
#include <signal.h>
#endif // /#ifdef _MSC_VER

namespace Seoul::UnitTesting
{

#if SEOUL_BENCHMARK_TESTS
using namespace Reflection;
using namespace Reflection::Attributes;

/** Size aware printing of time. */
static inline String GetPerOpString(Int64 iIterations, Double fTimeInSeconds)
{
	auto const fSecondsPerOp = (fTimeInSeconds / (Double)iIterations);
	if (fSecondsPerOp >= 1.0)
	{
		return String::Printf("%f s", fSecondsPerOp);
	}
	else if (fSecondsPerOp * 1000.0 >= 1.0)
	{
		return String::Printf("%f ms", (fSecondsPerOp * 1000.0));
	}
	else if (fSecondsPerOp * 1e+6 >= 1.0)
	{
		return String::Printf("%f us", (fSecondsPerOp * 1e+6));
	}
	else
	{
		return String::Printf("%f ns", (fSecondsPerOp * 1e+9));
	}
}

static void BenchmarkMethod(
	const WeakAny& pThis,
	Method const* pMethod,
	Int64& riTimeInTicks,
	Double& rfTimeInSeconds,
	Int64& riIterations)
{
	// For each method, we change the iteration count until
	// the test takes kfTargetTime second(s) to complete.
	static const Double kfTargetTime = 0.8;
	static const Int64 kiMaxIncrease = 10000;
	static const Int64 kiMaxIterations = 100000;

	MethodArguments a;
	Int64 iTimeInTicks = 0;
	Double fTimeInSeconds = 0.0;
	Int64 iIterations = 1;
	for (; fTimeInSeconds < kfTargetTime && iIterations < kiMaxIterations; )
	{
		// Cache iterations.
		auto const iLast = iIterations;

		// Predict iIterations.
		if (fTimeInSeconds > 0.0)
		{
			iIterations = (Int64)((kfTargetTime / fTimeInSeconds) * iLast);
		}

		// Overshoot and clamp.
		iIterations = Min(Max(iIterations + (iIterations / 5), iLast + 1), kiMaxIterations, (iLast + kiMaxIncrease));

		// Set and go.
		a[0] = iIterations;

		auto const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
		(void)pMethod->TryInvoke(pThis, a);
		auto const iEndTimeInTicks = SeoulTime::GetGameTimeInTicks();
		Logger::GetSingleton().UnitTesting_ClearSuppressedLogging();
		iTimeInTicks = iEndTimeInTicks - iStartTimeInTicks;
		fTimeInSeconds = SeoulTime::ConvertTicksToSeconds(iTimeInTicks);
	}

	// Output.
	riTimeInTicks = iTimeInTicks;
	rfTimeInSeconds = fTimeInSeconds;
	riIterations = iIterations;
}

static void BenchmarkMethods(
	Type const* pRootType,
	Type const* pType,
	const WeakAny& pThis,
	HString sMethod)
{
	// Traverse parents first.
	for (UInt32 i = 0u; i < pType->GetParentCount(); ++i)
	{
		BenchmarkMethods(pRootType, pType->GetParent(i), pThis, sMethod);
	}

	// Current methods.
	auto const uMethods = pType->GetMethodCount();
	for (auto uMethod = 0u; uMethod < uMethods; ++uMethod)
	{
		auto pMethod = pType->GetMethod(uMethod);

		// Skip method not specified.
		if (!sMethod.IsEmpty() && pMethod->GetName() != sMethod)
		{
			continue;
		}

		// We also time the total testing operation time.
		auto const iOverallStart = SeoulTime::GetGameTimeInTicks();

		Int64 iTimeInTicks = 0;
		Double fTimeInSeconds = 0.0;
		Int64 iIterations = 0;
		BenchmarkMethod(pThis, pMethod, iTimeInTicks, fTimeInSeconds, iIterations);

		// We also time the total testing operation time.
		auto const iOverallEnd = SeoulTime::GetGameTimeInTicks();

		// Reporting.

#if SEOUL_LOGGING_ENABLED
		SEOUL_LOG_UNIT_TEST(
			". Benchmark %s::%s: %" PRId64 " %s/op (%f secs)",
			pRootType->GetName().CStr(),
			pMethod->GetName().CStr(),
			iIterations,
			GetPerOpString(iIterations, fTimeInSeconds).CStr(),
			SeoulTime::ConvertTicksToSeconds(iOverallEnd - iOverallStart));
#else
		PlatformPrint::PrintStringFormatted(
			PlatformPrint::Type::kInfo,
			". Benchmark %s::%s: %" PRId64 " %s/op (%f secs)",
			pRootType->GetName().CStr(),
			pMethod->GetName().CStr(),
			iIterations,
			GetPerOpString(iIterations, fTimeInSeconds).CStr(),
			SeoulTime::ConvertTicksToSeconds(iOverallEnd - iOverallStart));
#endif // /#if SEOUL_LOGGING_ENABLED
	}
}

void RunBenchmarks(const String& sOptionalTestName)
{
	auto const uSplit = sOptionalTestName.Find('.');
	auto const sTypeName = HString(sOptionalTestName.Substring(0u, uSplit));
	auto const sMethodName = HString(String::NPos == uSplit ? String() : sOptionalTestName.Substring(uSplit + 1u));

	auto const uCount = Registry::GetRegistry().GetTypeCount();
	for (auto uType = 0u; uType < uCount; ++uType)
	{
		auto pType = Registry::GetRegistry().GetType(uType);

		// Skip type not specified.
		if (!sTypeName.IsEmpty() && pType->GetName() != sTypeName)
		{
			continue;
		}

		// Skip if no attribute.
		if (!pType->HasAttribute<BenchmarkTest>())
		{
			continue;
		}

		// Create an instance of the object.
		WeakAny pThis = pType->New(MemoryBudgets::Developer);
		if (!pThis.IsValid())
		{
			continue;
		}

		BenchmarkMethods(pType, pType, pThis, sMethodName);

		pType->Delete(pThis);
	}
}
#endif // /SEOUL_BENCHMARK_TESTS

#if SEOUL_UNIT_TESTS

using namespace Reflection;
using namespace Reflection::Attributes;

#ifdef _MSC_VER

/** Crash message buffer - don't want to allocate memory, and don't want a buffer this big on the stack. */
static Byte aCrashReasonBuffer[4096];

Int UnitTestsExceptionFilter(
	DWORD uExceptionCode,
	LPEXCEPTION_POINTERS pExceptionInfo)
{
#if SEOUL_PLATFORM_64
	DWORD const uMachineType = IMAGE_FILE_MACHINE_AMD64;
#else
	DWORD const uMachineType = IMAGE_FILE_MACHINE_I386;
#endif

	CONTEXT* pContext = pExceptionInfo->ContextRecord;

	size_t aCallStack[1];
	memset(aCallStack, 0, sizeof(aCallStack));

	STACKFRAME frame;
	memset(&frame, 0, sizeof(frame));
#if SEOUL_PLATFORM_64
	frame.AddrPC.Offset = pContext->Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Offset = pContext->Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = pContext->Rbp;
	frame.AddrFrame.Mode = AddrModeFlat;
#else
	frame.AddrPC.Offset = pContext->Eip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Offset = pContext->Esp;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = pContext->Ebp;
	frame.AddrFrame.Mode = AddrModeFlat;
#endif

	UInt32 uFrameOffset = 0u;
	while (uFrameOffset < SEOUL_ARRAY_COUNT(aCallStack) &&
		FALSE != ::StackWalk(
		uMachineType,
		::GetCurrentProcess(),
		::GetCurrentThread(),
		&frame,
		pContext,
		nullptr,
		::SymFunctionTableAccess,
		::SymGetModuleBase,
		nullptr))
	{
		aCallStack[uFrameOffset++] = (size_t)frame.AddrPC.Offset;
	}

	if (uFrameOffset > 0u)
	{
#if SEOUL_ENABLE_STACK_TRACES
		memset(aCrashReasonBuffer, 0, sizeof(aCrashReasonBuffer));
		Core::PrintStackTraceToBuffer(
			aCrashReasonBuffer,
			sizeof(aCrashReasonBuffer),
			"",
			aCallStack,
			uFrameOffset);

		SEOUL_LOG_ASSERTION("%s", aCrashReasonBuffer);
#else
		SEOUL_LOG_ASSERTION("Unhandled Win32 Exception");
#endif
	}

	// Execute the __except handler
	return EXCEPTION_EXECUTE_HANDLER;
}
#else
static Atomic32Value<Bool> s_bOnSigError(false);
static jmp_buf s_OnSigJump;
void SignalHandler(Int /*iSigNum*/)
{
	s_bOnSigError = true;
	_longjmp(s_OnSigJump, 1);
}
#endif // /!#ifdef _MSC_VER

Bool TestMethodWrapper(Any& rReturnValue, const Method& method, const WeakAny& weakThis)
{
	Bool bSuccess = false;

#ifdef _MSC_VER
	__try
#else
	s_bOnSigError = false;
	memset(&s_OnSigJump, 0, sizeof(s_OnSigJump));
	if (0 == _setjmp(s_OnSigJump))
#endif
	{
		bSuccess = (Bool)method.TryInvoke(rReturnValue, weakThis);
		bSuccess = bSuccess && (
			(rReturnValue.IsOfType<Bool>() && rReturnValue.Cast<Bool>()) ||
			(rReturnValue.IsOfType<Int>() && rReturnValue.Cast<Int>() == 0) ||
			(rReturnValue.IsOfType<void>()));
	}
#ifdef _MSC_VER
	__except (UnitTestsExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
	{
		return false;
	}
#else
	if (s_bOnSigError)
	{
		SEOUL_LOG_ASSERTION("Unhandled SIGSEGV");
	}
	bSuccess = bSuccess && !s_bOnSigError;
#endif

	return bSuccess;
}

void TestMethod(const Type& rootType, const Method& method, const WeakAny& weakThis)
{
	Any rReturnValue;
	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	Bool const bSuccess = TestMethodWrapper(rReturnValue, method, weakThis);
	Int64 const iEndTimeInTicks = SeoulTime::GetGameTimeInTicks();
	Double const fTimeInSeconds = SeoulTime::ConvertTicksToSeconds(iEndTimeInTicks - iStartTimeInTicks);
	(void)fTimeInSeconds;

	if (bSuccess)
	{
		SEOUL_LOG_UNIT_TEST(". Running test %s::%s: %s (%f secs)", rootType.GetName().CStr(), method.GetName().CStr(), "PASS", fTimeInSeconds);
	}
	else
	{
		// On failure, logger may be in a broken state, so call directly into the low-level printing functionality.
		PlatformPrint::PrintStringFormatted(
			PlatformPrint::Type::kError,
			". Running test %s::%s: %s (%f secs)\n", rootType.GetName().CStr(), method.GetName().CStr(), "FAIL", fTimeInSeconds);
	}

	// Log handling.
	if (!bSuccess)
	{
		Logger::GetSingleton().UnitTesting_EmitSuppressedLogging("\t");
		Thread::Sleep(500); // TODO: Eliminate, ugly workaround to give test runner time to consume our output.

		// Abort immediately on failure - attempt to clenaup in this state
		// is just going to produce side effect errors in most cases
		// because of state cleaned up in "impossible" ways.
#ifdef _MSC_VER
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif // /#ifdef _MSC_VER
		abort();
	}
	else
	{
		Logger::GetSingleton().UnitTesting_ClearSuppressedLogging();
	}
}

static Bool TestUnitMethods(
	UnitTest const* pUnitTest,
	Type const* pRootType,
	Type const* pType,
	WeakAny& rpThis,
	UInt32& uTests,
	HString optionalMethodName)
{
	// Traverse parents first.
	for (UInt32 i = 0u; i < pType->GetParentCount(); ++i)
	{
		if (!TestUnitMethods(
			pUnitTest,
			pRootType,
			pType->GetParent(i),
			rpThis,
			uTests,
			optionalMethodName))
		{
			return false;
		}
	}

	// Current methods.
	UInt32 const nMethods = pType->GetMethodCount();
	for (UInt32 iMethod = 0u; iMethod < nMethods; ++iMethod)
	{
		Method const* pMethod = pType->GetMethod(iMethod);
		if (nullptr == pMethod)
		{
			continue;
		}

		// Individual tests if pUnitTests is null.
		if (nullptr == pUnitTest)
		{
			if (!pMethod->GetAttributes().HasAttribute<UnitTest>())
			{
				continue;
			}
		}

		// Skip methods not specified.
		if (!optionalMethodName.IsEmpty() && pMethod->GetName() != optionalMethodName)
		{
			continue;
		}

		// If no this, create it for the individual test.
		if (nullptr == pUnitTest && !pMethod->GetTypeInfo().IsStatic())
		{
			rpThis = pRootType->New(MemoryBudgets::Developer);
			if (!rpThis.IsValid())
			{
				return false;
			}
		}

		++uTests;
		TestMethod(*pRootType, *pMethod, rpThis);

		// Cleanup.
		if (nullptr == pUnitTest)
		{
			pRootType->Delete(rpThis);
		}

		if (nullptr != pUnitTest)
		{
			if (pUnitTest->InstantiateForEach())
			{
				pRootType->Delete(rpThis);
				rpThis = pRootType->New(MemoryBudgets::Developer);
			}
		}
	}

	return true;
}

static Bool RunUnitTestsImpl(Type const* pType, UInt32& uTests, Byte const* sOptionalMethodName = "")
{
	if (nullptr == pType)
	{
		return true;
	}

	HString optionalMethodName(sOptionalMethodName);

	// Full type unit test.
	if (pType->HasAttribute<UnitTest>())
	{
		UnitTest const* pUnitTest = pType->GetAttribute<UnitTest>();
		if (!pUnitTest)
		{
			return true;
		}

		// Create an instance of the object.
		WeakAny pThis = pType->New(MemoryBudgets::Developer);
		if (!pThis.IsValid())
		{
			return true;
		}

		auto const bReturn = TestUnitMethods(
			pUnitTest,
			pType,
			pType,
			pThis,
			uTests,
			optionalMethodName);

		pType->Delete(pThis);

		return bReturn;
	}
	// Individual method test - enumerate all methods and run any that have the UnitTest attribute
	else
	{
		WeakAny pUnused;
		return TestUnitMethods(
			nullptr,
			pType,
			pType,
			pUnused,
			uTests,
			optionalMethodName);
	}
}

Bool RunUnitTests(const String& sOptionalTestName)
{
	UInt32 uTests = 0u;

#ifndef _MSC_VER
	Int32 const aSignals[] = { SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGPIPE, SIGSEGV };
	struct sigaction aPrevActions[SEOUL_ARRAY_COUNT(aSignals)];
	memset(&aPrevActions, 0, sizeof(aPrevActions));
	auto const scoped(MakeScopedAction(
		[&]()
	{
		for (Int32 i = 0; i < (Int32)SEOUL_ARRAY_COUNT(aSignals); ++i)
		{
			struct sigaction action;
			memset(&action, 0, sizeof(action));
			action.sa_handler = SignalHandler;
			sigaction(aSignals[i], &action, &aPrevActions[i]);
		}
	},
		[&]()
	{
		for (Int i = (Int32)SEOUL_ARRAY_COUNT(aSignals) - 1; i >= 0; --i)
		{
			sigaction(aSignals[i], &aPrevActions[i], nullptr);
		}
	}));
#endif

	// Run a single test.
	if (!sOptionalTestName.IsEmpty())
	{
		Vector<String, MemoryBudgets::Developer> vsParts;
		SplitString(sOptionalTestName, '.', vsParts);
		if (vsParts.IsEmpty())
		{
			SEOUL_LOG_UNIT_TEST("Invalid name specifier \"%s\".", sOptionalTestName.CStr());
			return false;
		}

		Type const* pType = Registry::GetRegistry().GetType(HString(vsParts[0]));
		String sMethodName;
		if (vsParts.GetSize() > 1u)
		{
			sMethodName = vsParts[1];
		}

		if (!RunUnitTestsImpl(pType, uTests, sMethodName.CStr()))
		{
			SEOUL_LOG_UNIT_TEST("FAIL (TESTS: %u, stopped at 1 failed test)", uTests);
			return false;
		}
	}
	// Run all tests.
	else
	{
		UInt32 const nCount = Registry::GetRegistry().GetTypeCount();
		for (UInt32 iType = 0u; iType < nCount; ++iType)
		{
			Type const* pType = Registry::GetRegistry().GetType(iType);
			if (!RunUnitTestsImpl(pType, uTests))
			{
				SEOUL_LOG_UNIT_TEST("FAIL (TESTS: %u, stopped at 1 failed test)", uTests);
				return false;
			}
		}
	}

	SEOUL_LOG_UNIT_TEST("OK (PASS: %u, FAIL: 0, TOTAL: %u)", uTests, uTests);
	return true;
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul::UnitTesting
