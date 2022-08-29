/**
 * \file ReflectionIntegrationTestRunner.cpp
 * \brief Defines a runner that executes integration tests using the Reflection system
 * to enumerate available integration tests.
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

#ifdef _MSC_VER
#include <ImageHlp.h>
#else
#include <setjmp.h>
#include <signal.h>
#endif // /#ifdef _MSC_VER

namespace Seoul::UnitTesting
{
#if SEOUL_UNIT_TESTS

using namespace Reflection;
using namespace Reflection::Attributes;

static Bool TestIntegrationMethods(
	IntegrationTest const* pIntegrationsTest,
	Type const* pRootType,
	Type const* pType,
	WeakAny& rpThis,
	UInt32& uTests,
	HString optionalMethodName)
{
	// Traverse parents first.
	for (UInt32 i = 0u; i < pType->GetParentCount(); ++i)
	{
		if (!TestIntegrationMethods(
			pIntegrationsTest,
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

		// Individual tests if pIntegrationsTest is null.
		if (nullptr == pIntegrationsTest)
		{
			continue;
		}

		// Skip methods not specified.
		if (!optionalMethodName.IsEmpty() && pMethod->GetName() != optionalMethodName)
		{
			continue;
		}

		++uTests;
		TestMethod(*pRootType, *pMethod, rpThis);
	}

	return true;
}

static Bool RunIntegrationTestsImpl(Type const* pType, UInt32& uTests, Byte const* sOptionalMethodName = "")
{
	if (nullptr == pType)
	{
		return true;
	}

	HString optionalMethodName(sOptionalMethodName);

	//Only full class is supported.
	if (pType->HasAttribute<IntegrationTest>())
	{
		IntegrationTest const* pIntegrationTest = pType->GetAttribute<IntegrationTest>();
		if (!pIntegrationTest)
		{
			return true;
		}

		// Create an instance of the object.
		WeakAny pThis = pType->New(MemoryBudgets::Developer);
		if (!pThis.IsValid())
		{
			return true;
		}

		auto const bReturn = TestIntegrationMethods(
			pIntegrationTest,
			pType,
			pType,
			pThis,
			uTests,
			optionalMethodName);

		pType->Delete(pThis);

		return bReturn;
	}
	return true;
}

Bool RunIntegrationTests(const String& sOptionalTestName)
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
	g_bRunningUnitTests = true; //Suppresses chatty logging during integration tests
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

		if (!RunIntegrationTestsImpl(pType, uTests, sMethodName.CStr()))
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
			if (!RunIntegrationTestsImpl(pType, uTests))
			{
				SEOUL_LOG_UNIT_TEST("FAIL (TESTS: %u, stopped at 1 failed test)", uTests);
				return false;
			}
		}
	}

	g_bRunningUnitTests = false;

	SEOUL_LOG_UNIT_TEST("OK (PASS: %u, FAIL: 0, TOTAL: %u)", uTests, uTests);
	return true;
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul::UnitTesting
