/**
 * \file CoroutineTest.cpp
 * \brief Unit tests for SeoulEngine coroutines.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "CoroutineTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
#include "Thread.h"
#include "UnitTesting.h"
#include "UnsafeHandle.h"
#include "Vector.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(CoroutineTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasicCoroutines)
SEOUL_END_TYPE()

static UnsafeHandle s_hThreadCoroutine;
static const UInt32 kStackSize = 16384;
static const Int kThreadCoroutineData = 1053;
static const Int kTestCoroutineData = 5017;
static Atomic32 s_nCounter(0);

void InternalStaticHandleConvertThreadToCoroutine()
{
	SEOUL_UNITTESTING_ASSERT(!s_hThreadCoroutine.IsValid());
	s_hThreadCoroutine = ConvertThreadToCoroutine((void*)(size_t)kThreadCoroutineData);
	SEOUL_UNITTESTING_ASSERT(s_hThreadCoroutine.IsValid());
	SEOUL_UNITTESTING_ASSERT(s_hThreadCoroutine == GetCurrentCoroutine());
	SEOUL_UNITTESTING_ASSERT(((void*)(size_t)kThreadCoroutineData) == GetCoroutineUserData());
}

void InternalStaticHandleConvertCoroutineToThread()
{
	SEOUL_UNITTESTING_ASSERT(s_hThreadCoroutine.IsValid());
	s_hThreadCoroutine.Reset();
	ConvertCoroutineToThread();
}

void TestCoroutineState(void* pData)
{
	SEOUL_UNITTESTING_ASSERT(s_hThreadCoroutine != GetCurrentCoroutine());
	SEOUL_UNITTESTING_ASSERT(GetCurrentCoroutine().IsValid());
	SEOUL_UNITTESTING_ASSERT(GetCoroutineUserData() == pData);
	SEOUL_UNITTESTING_ASSERT(((void*)(size_t)kTestCoroutineData) == pData);
}

void SEOUL_STD_CALL BasicCoroutineTest(void* pData)
{
	TestCoroutineState(pData);

	++s_nCounter;
	SwitchToCoroutine(s_hThreadCoroutine);
}

void CoroutineTest::TestBasicCoroutines()
{
	s_nCounter.Reset();

	auto const scope(MakeScopedAction(
		&InternalStaticHandleConvertThreadToCoroutine,
		&InternalStaticHandleConvertCoroutineToThread));

	UnsafeHandle hCoroutine = CreateCoroutine(
		kStackSize,
		kStackSize,
		BasicCoroutineTest,
		(void*)(size_t)kTestCoroutineData);
	SEOUL_UNITTESTING_ASSERT(s_hThreadCoroutine == GetCurrentCoroutine());
	SEOUL_UNITTESTING_ASSERT(((void*)(size_t)kThreadCoroutineData) == GetCoroutineUserData());

	SwitchToCoroutine(hCoroutine);

	SEOUL_UNITTESTING_ASSERT(1 == s_nCounter);
	SEOUL_UNITTESTING_ASSERT(s_hThreadCoroutine == GetCurrentCoroutine());
	SEOUL_UNITTESTING_ASSERT(((void*)(size_t)kThreadCoroutineData) == GetCoroutineUserData());

	DeleteCoroutine(hCoroutine);

	SEOUL_UNITTESTING_ASSERT(!hCoroutine.IsValid());
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
