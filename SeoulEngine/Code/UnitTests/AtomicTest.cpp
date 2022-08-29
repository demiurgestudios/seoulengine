/**
 * \file AtomicTest.cpp
 * \brief Unit tests for Seoul engine Atomic types. These include
 * Atomic32, AtomicPointer, and AtomicRingBuffer, which provide
 * thread-safe, lockless types and data structures.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AtomicTest.h"
#include "Atomic32.h"
#include "Atomic64.h"
#include "AtomicRingBuffer.h"
#include "AtomicPointer.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ScopedPtr.h"
#include "SeoulSignal.h"
#include "Thread.h"
#include "UnitTesting.h"
#include "Vector.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(AtomicTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAtomic32Basic)
	SEOUL_METHOD(TestAtomic32MultipleThread)
	SEOUL_METHOD(TestAtomic32ValueBasic)
	SEOUL_METHOD(TestAtomic32ValueMultipleThread)
	SEOUL_METHOD(TestAtomic32ValueFloat32Regression)
	SEOUL_METHOD(TestAtomic32ValueBoolRegression)
	SEOUL_METHOD(TestAtomic32ValueMax)
	SEOUL_METHOD(TestAtomic32ValueMin)
	SEOUL_METHOD(TestAtomic32ValueNeg0)
	SEOUL_METHOD(TestAtomic32ValueNaN)
	SEOUL_METHOD(TestAtomic64Basic)
	SEOUL_METHOD(TestAtomic64MultipleThread)
	SEOUL_METHOD(TestAtomic64ValueBasic)
	SEOUL_METHOD(TestAtomic64ValueMultipleThread)
	SEOUL_METHOD(TestAtomic64ValueFloat32Regression)
	SEOUL_METHOD(TestAtomic64ValueFloat64Regression)
	SEOUL_METHOD(TestAtomic64ValueBoolRegression)
	SEOUL_METHOD(TestAtomic64ValueMaxInt32)
	SEOUL_METHOD(TestAtomic64ValueMaxInt64)
	SEOUL_METHOD(TestAtomic64ValueMinInt32)
	SEOUL_METHOD(TestAtomic64ValueMinInt64)
	SEOUL_METHOD(TestAtomic64ValueFloat32Neg0)
	SEOUL_METHOD(TestAtomic64ValueFloat64Neg0)
	SEOUL_METHOD(TestAtomic64ValueFloat32NaN)
	SEOUL_METHOD(TestAtomic64ValueFloat64NaN)
	SEOUL_METHOD(TestAtomicPointer)
	SEOUL_METHOD(TestAtomicPointerMultipleThread)
	SEOUL_METHOD(TestAtomicRingBufferSingleThread)
	SEOUL_METHOD(TestAtomicRingBufferIdenticalValue)
	SEOUL_METHOD(TestAtomicRingBufferFull)
SEOUL_END_TYPE()

void AtomicTest::TestAtomic32Basic()
{
	{
		Atomic32 value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)0, (Atomic32Type)value);
	}

	// Construction
	{
		Atomic32 value((Atomic32Type)527);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)527, (Atomic32Type)value);
	}

	// Copy construction.
	{
		Atomic32 valueA((Atomic32Type)772);
		Atomic32 valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)772, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)772, (Atomic32Type)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)valueA, (Atomic32Type)valueB);
	}

	// Assignment.
	{
		Atomic32 valueA((Atomic32Type)772);
		Atomic32 valueB((Atomic32Type)2217);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)772, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)772, (Atomic32Type)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)valueA, (Atomic32Type)valueB);
	}

	// Or operator.
	{
		Atomic32 valueA((Atomic32Type)5);
		Atomic32Type const orValue = valueA |= 2;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)7, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)7, orValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)orValue, (Atomic32Type)valueA);
	}

	// And operator.
	{
		Atomic32 valueA((Atomic32Type)7);
		Atomic32Type const andValue = valueA &= 2;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)2, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)2, andValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)andValue, (Atomic32Type)valueA);
	}

	// Add operator.
	{
		Atomic32 valueA((Atomic32Type)7);
		Atomic32Type const addValue = valueA += 5;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)12, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)12, addValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)addValue, (Atomic32Type)valueA);
	}

	// Subtract operator.
	{
		Atomic32 valueA((Atomic32Type)9);
		Atomic32Type const subValue = valueA -= 3;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)6, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)6, subValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)subValue, (Atomic32Type)valueA);
	}

	// Pre increment operator.
	{
		Atomic32 valueA((Atomic32Type)17);
		Atomic32Type const resValue = ++valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)18, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)18, resValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)resValue, (Atomic32Type)valueA);
	}

	// Post increment operator.
	{
		Atomic32 valueA((Atomic32Type)13);
		Atomic32Type const resValue = valueA++;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)14, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)13, resValue);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL((Atomic32Type)resValue, (Atomic32Type)valueA);
	}

	// Pre decrement operator.
	{
		Atomic32 valueA((Atomic32Type)29);
		Atomic32Type const resValue = --valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)28, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)28, resValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)resValue, (Atomic32Type)valueA);
	}

	// Post decrement operator.
	{
		Atomic32 valueA((Atomic32Type)23);
		Atomic32Type const resValue = valueA--;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)22, (Atomic32Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic32Type)23, resValue);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL((Atomic32Type)resValue, (Atomic32Type)valueA);
	}

	// CAS method.
	{
		Atomic32 value((Atomic32Type)77);

		SEOUL_UNITTESTING_ASSERT_EQUAL(77, value.CompareAndSet(79, 79));
		SEOUL_UNITTESTING_ASSERT_EQUAL(77, (Atomic32Type)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(77, value.CompareAndSet(79, 77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(79, (Atomic32Type)value);
	}

	// Set method.
	{
		Atomic32 value((Atomic32Type)65);

		SEOUL_UNITTESTING_ASSERT_EQUAL(65, (Atomic32Type)value);
		value.Set(135);
		SEOUL_UNITTESTING_ASSERT_EQUAL(135, (Atomic32Type)value);
	}

	// Reset method.
	{
		Atomic32 value((Atomic32Type)53);

		SEOUL_UNITTESTING_ASSERT_EQUAL(53, (Atomic32Type)value);
		value.Reset();
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Atomic32Type)value);
	}
}

struct Atomic32Test
{
	SEOUL_DELEGATE_TARGET(Atomic32Test);

	Atomic32 m_Atomic;

	Int Decrement(const Thread& thread)
	{
		return (--m_Atomic);
	}

	Int Increment(const Thread& thread)
	{
		return (++m_Atomic);
	}

	Int AddTwo(const Thread& thread)
	{
		return (m_Atomic += 2);
	}

	Int SubtractTwo(const Thread& thread)
	{
		return (m_Atomic -= 2);
	}
}; // struct Atomic32Test

void AtomicTest::TestAtomic32MultipleThread()
{
	static const Int32 kTestThreadCount = 50;
	Atomic32Test test;

	ScopedPtr<Thread> apThreads[kTestThreadCount];

	// Increment
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&Atomic32Test::Increment, &test)));
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(kTestThreadCount == (long)test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			vValues[i] = apThreads[i]->GetReturnValue();
		}
		QuickSort(vValues.Begin(), vValues.End());

		for (Int32 i = 1; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(vValues[i] - 1 == vValues[i-1]);
		}
	}

	// Decrement
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&Atomic32Test::Decrement, &test)));
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(0 == test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			vValues[i] = apThreads[i]->GetReturnValue();
		}
		QuickSort(vValues.Begin(), vValues.End());

		for (Int32 i = 1; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(vValues[i] - 1 == vValues[i-1]);
		}
	}

	// Add 2
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&Atomic32Test::AddTwo, &test)));
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(kTestThreadCount * 2 == test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			vValues[i] = apThreads[i]->GetReturnValue();
		}
		QuickSort(vValues.Begin(), vValues.End());

		for (Int32 i = 1; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(vValues[i] - 2 == vValues[i-1]);
		}
	}

	// Subtract 2
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&Atomic32Test::SubtractTwo, &test)));
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(0 == test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			vValues[i] = apThreads[i]->GetReturnValue();
		}
		QuickSort(vValues.Begin(), vValues.End());

		for (Int32 i = 1; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(vValues[i] - 2 == vValues[i-1]);
		}
	}
}

void AtomicTest::TestAtomic32ValueBasic()
{
	{
		Atomic32Value<Bool> value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, (Bool)value);
	}

	// Construction
	{
		Atomic32Value<Bool> value(true);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)value);
	}

	// Copy construction.
	{
		Atomic32Value<Bool> valueA(true);
		Atomic32Value<Bool> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Bool)valueA, (Bool)valueB);
	}

	// Assignment.
	{
		Atomic32Value<Bool> valueA(true);
		Atomic32Value<Bool> valueB(false);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Bool)valueA, (Bool)valueB);
	}

	// Assignment value.
	{
		Atomic32Value<Bool> valueB(false);

		valueB = true;

		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueB);
	}

	// CAS method.
	{
		Atomic32Value<Bool> value(false);

		SEOUL_UNITTESTING_ASSERT_EQUAL(false, value.CompareAndSet(true, true));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, (Bool)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, value.CompareAndSet(true, false));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)value);
	}

	// Set method.
	{
		Atomic32Value<Bool> value(false);

		SEOUL_UNITTESTING_ASSERT_EQUAL(false, (Bool)value);
		value.Set(true);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)value);
	}
}

struct Atomic32ValueTest
{
	SEOUL_DELEGATE_TARGET(Atomic32ValueTest);

	Atomic32Value<Int8> m_Atomic;

	Int OneTwo7(const Thread&)
	{
		m_Atomic.Set(127);
		return (Int)m_Atomic;
	}

	Int NegOne(const Thread&)
	{
		m_Atomic = -1;
		return (Int)m_Atomic;
	}
}; // struct Atomic32ValueTest

void AtomicTest::TestAtomic32ValueMultipleThread()
{
	static const Int32 kTestThreadCount = 50;
	Atomic32ValueTest test;

	ScopedPtr<Thread> apThreads[kTestThreadCount];

	// Mixture - must be one or the other value we're setting.
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			if (i % 2 == 0)
			{
				apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
					SEOUL_BIND_DELEGATE(&Atomic32ValueTest::OneTwo7, &test)));
			}
			else
			{
				apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
					SEOUL_BIND_DELEGATE(&Atomic32ValueTest::NegOne, &test)));
			}
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(
			-1 == test.m_Atomic ||
			127 == test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(
				-1 == apThreads[i]->GetReturnValue() ||
				127 == apThreads[i]->GetReturnValue());
		}
	}
}

/** Regression for Float32 in an Atomic32Value<>, which prior to a fix would be corrupted. */
void AtomicTest::TestAtomic32ValueFloat32Regression()
{
	Atomic32Value<Float32> f;
	f.Set(-1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
	f.Set(1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, (Float32)f);
	f.Set(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)f);

	f.Set(-1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, f.CompareAndSet(0.0f, -1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)f);

	// Zero value test.
	f.Set(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, f.CompareAndSet(-1.0f, 0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
	f.Set(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, f.CompareAndSet(-1.0f, -0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
	f.Set(-0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, f.CompareAndSet(-1.0f, 0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
	f.Set(-0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, f.CompareAndSet(-1.0f, -0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
}

/** Regression of a bug introduced when Atomic32Value is specialized on Bool. */
void AtomicTest::TestAtomic32ValueBoolRegression()
{
	Atomic32Value<Bool> b;
	SEOUL_UNITTESTING_ASSERT(false == b.CompareAndSet(true, false));
	SEOUL_UNITTESTING_ASSERT(false != b.CompareAndSet(true, false));
	SEOUL_UNITTESTING_ASSERT(true == b.CompareAndSet(false, true));
	SEOUL_UNITTESTING_ASSERT(true != b.CompareAndSet(false, true));
}

void AtomicTest::TestAtomic32ValueMax()
{
	{
		Atomic32Value<Int32> value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
	}

	// Construction
	{
		Atomic32Value<Int32> value(IntMax);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)value);
	}

	// Copy construction.
	{
		Atomic32Value<Int32> valueA(IntMax);
		Atomic32Value<Int32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)valueA, (Int32)valueB);
	}

	// Assignment.
	{
		Atomic32Value<Int32> valueA(IntMax);
		Atomic32Value<Int32> valueB(0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)valueA, (Int32)valueB);
	}

	// Assignment value.
	{
		Atomic32Value<Int32> valueB(0);

		valueB = IntMax;

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueB);
	}

	// CAS method.
	{
		Atomic32Value<Int32> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(IntMax, IntMax));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(IntMax, 0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)value);
	}

	// Set method.
	{
		Atomic32Value<Int32> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
		value.Set(IntMax);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)value);
	}
}

void AtomicTest::TestAtomic32ValueMin()
{
	{
		Atomic32Value<Int32> value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
	}

	// Construction
	{
		Atomic32Value<Int32> value(IntMin);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)value);
	}

	// Copy construction.
	{
		Atomic32Value<Int32> valueA(IntMin);
		Atomic32Value<Int32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)valueA, (Int32)valueB);
	}

	// Assignment.
	{
		Atomic32Value<Int32> valueA(IntMin);
		Atomic32Value<Int32> valueB(0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)valueA, (Int32)valueB);
	}

	// Assignment value.
	{
		Atomic32Value<Int32> valueB(0);

		valueB = IntMin;

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueB);
	}

	// CAS method.
	{
		Atomic32Value<Int32> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(IntMin, IntMin));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(IntMin, 0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)value);
	}

	// Set method.
	{
		Atomic32Value<Int32> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
		value.Set(IntMin);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)value);
	}
}

void AtomicTest::TestAtomic32ValueNeg0()
{
	{
		Atomic32Value<Float32> value(1.0f);

		// Default of 1.0f.
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, (Float32)value);
	}

	// Construction
	{
		Atomic32Value<Float32> value(-0.0f);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
	}

	// Copy construction.
	{
		Atomic32Value<Float32> valueA(-0.0f);
		Atomic32Value<Float32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Float32)valueA, (Float32)valueB);
	}

	// Assignment.
	{
		Atomic32Value<Float32> valueA(-0.0f);
		Atomic32Value<Float32> valueB(1.0f);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Float32)valueA, (Float32)valueB);
	}

	// Assignment value.
	{
		Atomic32Value<Float32> valueB(1.0f);

		valueB = -0.0f;

		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueB);
	}

	// CAS method.
	{
		Atomic32Value<Float32> value(1.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, value.CompareAndSet(-0.0f, -0.0f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, value.CompareAndSet(-0.0f, 1.0f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
	}

	// Set method.
	{
		Atomic32Value<Float32> value(1.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, (Float32)value);
		value.Set(-0.0f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
	}
}

void AtomicTest::TestAtomic32ValueNaN()
{
	auto const quiet = std::numeric_limits<Float32>::quiet_NaN();
	auto const signal = std::numeric_limits<Float32>::signaling_NaN();

	// Construction
	{
		Atomic32Value<Float32> value(quiet);

		// Value.
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}
	{
		Atomic32Value<Float32> value(signal);

		// Value.
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}

	// Copy construction.
	{
		Atomic32Value<Float32> valueA(quiet);
		Atomic32Value<Float32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}
	{
		Atomic32Value<Float32> valueA(signal);
		Atomic32Value<Float32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}

	// Assignment.
	{
		Atomic32Value<Float32> valueA(quiet);
		Atomic32Value<Float32> valueB(0.0f);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}
	{
		Atomic32Value<Float32> valueA(signal);
		Atomic32Value<Float32> valueB(0.0f);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}

	// Assignment value.
	{
		Atomic32Value<Float32> valueB(0.0f);

		valueB = quiet;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}
	{
		Atomic32Value<Float32> valueB(0.0f);

		valueB = signal;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}

	// CAS method.
	{
		Atomic32Value<Float32> value(0.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, value.CompareAndSet(quiet, quiet));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, value.CompareAndSet(quiet, 0.0f));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}
	{
		Atomic32Value<Float32> value(0.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, value.CompareAndSet(signal, signal));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, value.CompareAndSet(signal, 0.0f));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}

	// Set method.
	{
		Atomic32Value<Float32> value(0.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
		value.Set(quiet);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}
	{
		Atomic32Value<Float32> value(0.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
		value.Set(signal);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}
}

void AtomicTest::TestAtomic64Basic()
{
	{
		Atomic64 value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)0, (Atomic64Type)value);
	}

	// Construction
	{
		Atomic64 value((Atomic64Type)527);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)527, (Atomic64Type)value);
	}

	// Copy construction.
	{
		Atomic64 valueA((Atomic64Type)772);
		Atomic64 valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)772, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)772, (Atomic64Type)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)valueA, (Atomic64Type)valueB);
	}

	// Assignment.
	{
		Atomic64 valueA((Atomic64Type)772);
		Atomic64 valueB((Atomic64Type)2217);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)772, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)772, (Atomic64Type)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)valueA, (Atomic64Type)valueB);
	}

	// Or operator.
	{
		Atomic64 valueA((Atomic64Type)5);
		Atomic64Type const orValue = valueA |= 2;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)7, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)7, orValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)orValue, (Atomic64Type)valueA);
	}

	// And operator.
	{
		Atomic64 valueA((Atomic64Type)7);
		Atomic64Type const andValue = valueA &= 2;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)2, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)2, andValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)andValue, (Atomic64Type)valueA);
	}

	// Add operator.
	{
		Atomic64 valueA((Atomic64Type)7);
		Atomic64Type const addValue = valueA += 5;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)12, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)12, addValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)addValue, (Atomic64Type)valueA);
	}

	// Subtract operator.
	{
		Atomic64 valueA((Atomic64Type)9);
		Atomic64Type const subValue = valueA -= 3;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)6, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)6, subValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)subValue, (Atomic64Type)valueA);
	}

	// Pre increment operator.
	{
		Atomic64 valueA((Atomic64Type)17);
		Atomic64Type const resValue = ++valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)18, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)18, resValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)resValue, (Atomic64Type)valueA);
	}

	// Post increment operator.
	{
		Atomic64 valueA((Atomic64Type)13);
		Atomic64Type const resValue = valueA++;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)14, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)13, resValue);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL((Atomic64Type)resValue, (Atomic64Type)valueA);
	}

	// Pre decrement operator.
	{
		Atomic64 valueA((Atomic64Type)29);
		Atomic64Type const resValue = --valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)28, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)28, resValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)resValue, (Atomic64Type)valueA);
	}

	// Post decrement operator.
	{
		Atomic64 valueA((Atomic64Type)23);
		Atomic64Type const resValue = valueA--;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)22, (Atomic64Type)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Atomic64Type)23, resValue);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL((Atomic64Type)resValue, (Atomic64Type)valueA);
	}

	// CAS method.
	{
		Atomic64 value((Atomic64Type)77);

		SEOUL_UNITTESTING_ASSERT_EQUAL(77, value.CompareAndSet(79, 79));
		SEOUL_UNITTESTING_ASSERT_EQUAL(77, (Atomic64Type)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(77, value.CompareAndSet(79, 77));
		SEOUL_UNITTESTING_ASSERT_EQUAL(79, (Atomic64Type)value);
	}

	// Set method.
	{
		Atomic64 value((Atomic64Type)65);

		SEOUL_UNITTESTING_ASSERT_EQUAL(65, (Atomic64Type)value);
		value.Set(135);
		SEOUL_UNITTESTING_ASSERT_EQUAL(135, (Atomic64Type)value);
	}

	// Reset method.
	{
		Atomic64 value((Atomic64Type)53);

		SEOUL_UNITTESTING_ASSERT_EQUAL(53, (Atomic64Type)value);
		value.Reset();
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Atomic64Type)value);
	}
}

struct Atomic64Test
{
	SEOUL_DELEGATE_TARGET(Atomic64Test);

	Atomic64 m_Atomic;

	Int Decrement(const Thread& thread)
	{
		return (Int)(--m_Atomic);
	}

	Int Increment(const Thread& thread)
	{
		return (Int)(++m_Atomic);
	}

	Int AddTwo(const Thread& thread)
	{
		return (Int)(m_Atomic += 2);
	}

	Int SubtractTwo(const Thread& thread)
	{
		return (Int)(m_Atomic -= 2);
	}
}; // struct Atomic64Test

void AtomicTest::TestAtomic64MultipleThread()
{
	static const Int32 kTestThreadCount = 50;
	Atomic64Test test;

	ScopedPtr<Thread> apThreads[kTestThreadCount];

	// Increment
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&Atomic64Test::Increment, &test)));
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(kTestThreadCount == (long)test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			vValues[i] = apThreads[i]->GetReturnValue();
		}
		QuickSort(vValues.Begin(), vValues.End());

		for (Int32 i = 1; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(vValues[i] - 1 == vValues[i - 1]);
		}
	}

	// Decrement
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&Atomic64Test::Decrement, &test)));
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(0 == test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			vValues[i] = apThreads[i]->GetReturnValue();
		}
		QuickSort(vValues.Begin(), vValues.End());

		for (Int32 i = 1; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(vValues[i] - 1 == vValues[i - 1]);
		}
	}

	// Add 2
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&Atomic64Test::AddTwo, &test)));
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(kTestThreadCount * 2 == test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			vValues[i] = apThreads[i]->GetReturnValue();
		}
		QuickSort(vValues.Begin(), vValues.End());

		for (Int32 i = 1; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(vValues[i] - 2 == vValues[i - 1]);
		}
	}

	// Subtract 2
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
				SEOUL_BIND_DELEGATE(&Atomic64Test::SubtractTwo, &test)));
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(0 == test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			vValues[i] = apThreads[i]->GetReturnValue();
		}
		QuickSort(vValues.Begin(), vValues.End());

		for (Int32 i = 1; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(vValues[i] - 2 == vValues[i - 1]);
		}
	}
}

void AtomicTest::TestAtomic64ValueBasic()
{
	{
		Atomic64Value<Bool> value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, (Bool)value);
	}

	// Construction
	{
		Atomic64Value<Bool> value(true);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)value);
	}

	// Copy construction.
	{
		Atomic64Value<Bool> valueA(true);
		Atomic64Value<Bool> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Bool)valueA, (Bool)valueB);
	}

	// Assignment.
	{
		Atomic64Value<Bool> valueA(true);
		Atomic64Value<Bool> valueB(false);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Bool)valueA, (Bool)valueB);
	}

	// Assignment value.
	{
		Atomic64Value<Bool> valueB(false);

		valueB = true;

		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)valueB);
	}

	// CAS method.
	{
		Atomic64Value<Bool> value(false);

		SEOUL_UNITTESTING_ASSERT_EQUAL(false, value.CompareAndSet(true, true));
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, (Bool)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(false, value.CompareAndSet(true, false));
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)value);
	}

	// Set method.
	{
		Atomic64Value<Bool> value(false);

		SEOUL_UNITTESTING_ASSERT_EQUAL(false, (Bool)value);
		value.Set(true);
		SEOUL_UNITTESTING_ASSERT_EQUAL(true, (Bool)value);
	}
}

struct Atomic64ValueTest
{
	SEOUL_DELEGATE_TARGET(Atomic64ValueTest);

	Atomic64Value<Int8> m_Atomic;

	Int OneTwo7(const Thread&)
	{
		m_Atomic.Set(127);
		return (Int)m_Atomic;
	}

	Int NegOne(const Thread&)
	{
		m_Atomic = -1;
		return (Int)m_Atomic;
	}
}; // struct Atomic64ValueTest

void AtomicTest::TestAtomic64ValueMultipleThread()
{
	static const Int32 kTestThreadCount = 50;
	Atomic64ValueTest test;

	ScopedPtr<Thread> apThreads[kTestThreadCount];

	// Mixture - must be one or the other value we're setting.
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			if (i % 2 == 0)
			{
				apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
					SEOUL_BIND_DELEGATE(&Atomic64ValueTest::OneTwo7, &test)));
			}
			else
			{
				apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
					SEOUL_BIND_DELEGATE(&Atomic64ValueTest::NegOne, &test)));
			}
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(
			-1 == test.m_Atomic ||
			127 == test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(
				-1 == apThreads[i]->GetReturnValue() ||
				127 == apThreads[i]->GetReturnValue());
		}
	}
}

/** Regression for Float32 in an Atomic64Value<>, which prior to a fix would be corrupted. */
void AtomicTest::TestAtomic64ValueFloat32Regression()
{
	Atomic64Value<Float32> f;
	f.Set(-1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
	f.Set(1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, (Float32)f);
	f.Set(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)f);

	f.Set(-1.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, f.CompareAndSet(0.0f, -1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)f);

	// Zero value test.
	f.Set(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, f.CompareAndSet(-1.0f, 0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
	f.Set(0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, f.CompareAndSet(-1.0f, -0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
	f.Set(-0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, f.CompareAndSet(-1.0f, 0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
	f.Set(-0.0f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, f.CompareAndSet(-1.0f, -0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, (Float32)f);
}

/** Regression for Float64 in an Atomic64Value<>, which prior to a fix would be corrupted. */
void AtomicTest::TestAtomic64ValueFloat64Regression()
{
	Atomic64Value<Float64> f;
	f.Set(-1.0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0, (Float64)f);
	f.Set(1.0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, (Float64)f);
	f.Set(0.0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)f);

	f.Set(-1.0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0, f.CompareAndSet(0.0, -1.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)f);

	// Zero value test.
	f.Set(0.0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, f.CompareAndSet(-1.0, 0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0, (Float64)f);
	f.Set(0.0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, f.CompareAndSet(-1.0, -0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0, (Float64)f);
	f.Set(-0.0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, f.CompareAndSet(-1.0, 0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0, (Float64)f);
	f.Set(-0.0);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, f.CompareAndSet(-1.0, -0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0, (Float64)f);
}

/** Regression of a bug introduced when Atomic64Value is specialized on Bool. */
void AtomicTest::TestAtomic64ValueBoolRegression()
{
	Atomic64Value<Bool> b;
	SEOUL_UNITTESTING_ASSERT(false == b.CompareAndSet(true, false));
	SEOUL_UNITTESTING_ASSERT(false != b.CompareAndSet(true, false));
	SEOUL_UNITTESTING_ASSERT(true == b.CompareAndSet(false, true));
	SEOUL_UNITTESTING_ASSERT(true != b.CompareAndSet(false, true));
}

void AtomicTest::TestAtomic64ValueMaxInt32()
{
	{
		Atomic64Value<Int32> value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
	}

	// Construction
	{
		Atomic64Value<Int32> value(IntMax);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)value);
	}

	// Copy construction.
	{
		Atomic64Value<Int32> valueA(IntMax);
		Atomic64Value<Int32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)valueA, (Int32)valueB);
	}

	// Assignment.
	{
		Atomic64Value<Int32> valueA(IntMax);
		Atomic64Value<Int32> valueB(0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)valueA, (Int32)valueB);
	}

	// Assignment value.
	{
		Atomic64Value<Int32> valueB(0);

		valueB = IntMax;

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)valueB);
	}

	// CAS method.
	{
		Atomic64Value<Int32> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(IntMax, IntMax));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(IntMax, 0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)value);
	}

	// Set method.
	{
		Atomic64Value<Int32> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
		value.Set(IntMax);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, (Int32)value);
	}
}

void AtomicTest::TestAtomic64ValueMaxInt64()
{
	{
		Atomic64Value<Int64> value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int64)value);
	}

	// Construction
	{
		Atomic64Value<Int64> value(Int64Max);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, (Int64)value);
	}

	// Copy construction.
	{
		Atomic64Value<Int64> valueA(Int64Max);
		Atomic64Value<Int64> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, (Int64)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, (Int64)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)valueA, (Int64)valueB);
	}

	// Assignment.
	{
		Atomic64Value<Int64> valueA(Int64Max);
		Atomic64Value<Int64> valueB(0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, (Int64)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, (Int64)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)valueA, (Int64)valueB);
	}

	// Assignment value.
	{
		Atomic64Value<Int64> valueB(0);

		valueB = Int64Max;

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, (Int64)valueB);
	}

	// CAS method.
	{
		Atomic64Value<Int64> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(Int64Max, Int64Max));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int64)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(Int64Max, 0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, (Int64)value);
	}

	// Set method.
	{
		Atomic64Value<Int64> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int64)value);
		value.Set(Int64Max);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Max, (Int64)value);
	}
}

void AtomicTest::TestAtomic64ValueMinInt32()
{
	{
		Atomic64Value<Int32> value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
	}

	// Construction
	{
		Atomic64Value<Int32> value(IntMin);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)value);
	}

	// Copy construction.
	{
		Atomic64Value<Int32> valueA(IntMin);
		Atomic64Value<Int32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)valueA, (Int32)valueB);
	}

	// Assignment.
	{
		Atomic64Value<Int32> valueA(IntMin);
		Atomic64Value<Int32> valueB(0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int32)valueA, (Int32)valueB);
	}

	// Assignment value.
	{
		Atomic64Value<Int32> valueB(0);

		valueB = IntMin;

		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)valueB);
	}

	// CAS method.
	{
		Atomic64Value<Int32> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(IntMin, IntMin));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(IntMin, 0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)value);
	}

	// Set method.
	{
		Atomic64Value<Int32> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int32)value);
		value.Set(IntMin);
		SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, (Int32)value);
	}
}

void AtomicTest::TestAtomic64ValueMinInt64()
{
	{
		Atomic64Value<Int64> value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int64)value);
	}

	// Construction
	{
		Atomic64Value<Int64> value(Int64Min);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, (Int64)value);
	}

	// Copy construction.
	{
		Atomic64Value<Int64> valueA(Int64Min);
		Atomic64Value<Int64> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, (Int64)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, (Int64)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)valueA, (Int64)valueB);
	}

	// Assignment.
	{
		Atomic64Value<Int64> valueA(Int64Min);
		Atomic64Value<Int64> valueB(0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, (Int64)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, (Int64)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int64)valueA, (Int64)valueB);
	}

	// Assignment value.
	{
		Atomic64Value<Int64> valueB(0);

		valueB = Int64Min;

		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, (Int64)valueB);
	}

	// CAS method.
	{
		Atomic64Value<Int64> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(Int64Min, Int64Min));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int64)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, value.CompareAndSet(Int64Min, 0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, (Int64)value);
	}

	// Set method.
	{
		Atomic64Value<Int64> value(0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, (Int64)value);
		value.Set(Int64Min);
		SEOUL_UNITTESTING_ASSERT_EQUAL(Int64Min, (Int64)value);
	}
}

void AtomicTest::TestAtomic64ValueFloat32Neg0()
{
	{
		Atomic64Value<Float32> value(1.0f);

		// Default of 1.0f.
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, (Float32)value);
	}

	// Construction
	{
		Atomic64Value<Float32> value(-0.0f);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
	}

	// Copy construction.
	{
		Atomic64Value<Float32> valueA(-0.0f);
		Atomic64Value<Float32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Float32)valueA, (Float32)valueB);
	}

	// Assignment.
	{
		Atomic64Value<Float32> valueA(-0.0f);
		Atomic64Value<Float32> valueB(1.0f);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Float32)valueA, (Float32)valueB);
	}

	// Assignment value.
	{
		Atomic64Value<Float32> valueB(1.0f);

		valueB = -0.0f;

		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)valueB);
	}

	// CAS method.
	{
		Atomic64Value<Float32> value(1.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, value.CompareAndSet(-0.0f, -0.0f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, value.CompareAndSet(-0.0f, 1.0f));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
	}

	// Set method.
	{
		Atomic64Value<Float32> value(1.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, (Float32)value);
		value.Set(-0.0f);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
	}
}

void AtomicTest::TestAtomic64ValueFloat64Neg0()
{
	{
		Atomic64Value<Float64> value(1.0);

		// Default of 1.0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, (Float64)value);
	}

	// Construction
	{
		Atomic64Value<Float64> value(-0.0);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0, (Float64)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)value);
	}

	// Copy construction.
	{
		Atomic64Value<Float64> valueA(-0.0);
		Atomic64Value<Float64> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0, (Float64)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Float64)valueA, (Float64)valueB);
	}

	// Assignment.
	{
		Atomic64Value<Float64> valueA(-0.0);
		Atomic64Value<Float64> valueB(1.0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0, (Float64)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Float64)valueA, (Float64)valueB);
	}

	// Assignment value.
	{
		Atomic64Value<Float64> valueB(1.0);

		valueB = -0.0;

		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)valueB);
	}

	// CAS method.
	{
		Atomic64Value<Float64> value(1.0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, value.CompareAndSet(-0.0, -0.0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, (Float64)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, value.CompareAndSet(-0.0, 1.0));
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0, (Float64)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)value);
	}

	// Set method.
	{
		Atomic64Value<Float64> value(1.0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, (Float64)value);
		value.Set(-0.0);
		SEOUL_UNITTESTING_ASSERT_EQUAL(-0.0, (Float64)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)value);
	}
}

void AtomicTest::TestAtomic64ValueFloat32NaN()
{
	auto const quiet = std::numeric_limits<Float32>::quiet_NaN();
	auto const signal = std::numeric_limits<Float32>::signaling_NaN();

	// Construction
	{
		Atomic64Value<Float32> value(quiet);

		// Value.
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}
	{
		Atomic64Value<Float32> value(signal);

		// Value.
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}

	// Copy construction.
	{
		Atomic64Value<Float32> valueA(quiet);
		Atomic64Value<Float32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}
	{
		Atomic64Value<Float32> valueA(signal);
		Atomic64Value<Float32> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}

	// Assignment.
	{
		Atomic64Value<Float32> valueA(quiet);
		Atomic64Value<Float32> valueB(0.0f);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}
	{
		Atomic64Value<Float32> valueA(signal);
		Atomic64Value<Float32> valueB(0.0f);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}

	// Assignment value.
	{
		Atomic64Value<Float32> valueB(0.0f);

		valueB = quiet;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}
	{
		Atomic64Value<Float32> valueB(0.0f);

		valueB = signal;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)valueB));
	}

	// CAS method.
	{
		Atomic64Value<Float32> value(0.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, value.CompareAndSet(quiet, quiet));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, value.CompareAndSet(quiet, 0.0f));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}
	{
		Atomic64Value<Float32> value(0.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, value.CompareAndSet(signal, signal));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, value.CompareAndSet(signal, 0.0f));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}

	// Set method.
	{
		Atomic64Value<Float32> value(0.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
		value.Set(quiet);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}
	{
		Atomic64Value<Float32> value(0.0f);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, (Float32)value);
		value.Set(signal);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float32)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float32)value));
	}
}

void AtomicTest::TestAtomic64ValueFloat64NaN()
{
	auto const quiet = std::numeric_limits<Float64>::quiet_NaN();
	auto const signal = std::numeric_limits<Float64>::signaling_NaN();

	// Construction
	{
		Atomic64Value<Float64> value(quiet);

		// Value.
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float64)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)value));
	}
	{
		Atomic64Value<Float64> value(signal);

		// Value.
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float64)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)value));
	}

	// Copy construction.
	{
		Atomic64Value<Float64> valueA(quiet);
		Atomic64Value<Float64> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float64)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueB));
	}
	{
		Atomic64Value<Float64> valueA(signal);
		Atomic64Value<Float64> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float64)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueB));
	}

	// Assignment.
	{
		Atomic64Value<Float64> valueA(quiet);
		Atomic64Value<Float64> valueB(0.0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float64)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueB));
	}
	{
		Atomic64Value<Float64> valueA(signal);
		Atomic64Value<Float64> valueB(0.0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float64)valueA);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueA));
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueB));
	}

	// Assignment value.
	{
		Atomic64Value<Float64> valueB(0.0);

		valueB = quiet;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueB));
	}
	{
		Atomic64Value<Float64> valueB(0.0);

		valueB = signal;

		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float64)valueB);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)valueB));
	}

	// CAS method.
	{
		Atomic64Value<Float64> value(0.0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, value.CompareAndSet(quiet, quiet));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, value.CompareAndSet(quiet, 0.0));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float64)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)value));
	}
	{
		Atomic64Value<Float64> value(0.0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, value.CompareAndSet(signal, signal));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, value.CompareAndSet(signal, 0.0));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float64)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)value));
	}

	// Set method.
	{
		Atomic64Value<Float64> value(0.0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)value);
		value.Set(quiet);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(quiet, (Float64)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)value));
	}
	{
		Atomic64Value<Float64> value(0.0);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, (Float64)value);
		value.Set(signal);
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(signal, (Float64)value);
		SEOUL_UNITTESTING_ASSERT(IsNaN((Float64)value));
	}
}

struct AtomicPointerTest
{
	SEOUL_DELEGATE_TARGET(AtomicPointerTest);

	AtomicPointer<Int8> m_Atomic;

	Int OneTwo7(const Thread&)
	{
		m_Atomic.Set((Int8*)(size_t)127);
		return (Int)((size_t)(Int8*)m_Atomic);
	}

	Int NegOne(const Thread&)
	{
		m_Atomic = AtomicPointer<Int8>((Int8*)(size_t)-1);
		return (Int)((size_t)(Int8*)m_Atomic);
	}
}; // struct AtomicPointerTest

void AtomicTest::TestAtomicPointer()
{
	{
		AtomicPointer<Int8> value;

		// Default of 0.
		SEOUL_UNITTESTING_ASSERT_EQUAL(nullptr, (Int8*)value);
	}

	// Construction
	{
		AtomicPointer<Int8> value((Int8*)1);

		// Value.
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)1, (Int8*)value);
	}

	// Copy construction.
	{
		AtomicPointer<Int8> valueA((Int8*)1);
		AtomicPointer<Int8> valueB(valueA);

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)1, (Int8*)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)1, (Int8*)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)valueA, (Int8*)valueB);
	}

	// Assignment.
	{
		AtomicPointer<Int8> valueA((Int8*)1);
		AtomicPointer<Int8> valueB((Int8*)0);

		valueB = valueA;

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)1, (Int8*)valueA);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)1, (Int8*)valueB);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)valueA, (Int8*)valueB);
	}

	// Assignment value.
	{
		AtomicPointer<Int8> valueB((Int8*)0);

		valueB = AtomicPointer<Int8>((Int8*)1);

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)1, (Int8*)valueB);
	}

	// CAS method.
	{
		AtomicPointer<Int8> value((Int8*)0);

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)0, value.CompareAndSet((Int8*)1, (Int8*)1));
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)0, (Int8*)value);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)0, value.CompareAndSet((Int8*)1, (Int8*)0));
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)1, (Int8*)value);
	}

	// Reset method.
	{
		AtomicPointer<Int8> value((Int8*)1);

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)1, (Int8*)value);
		value.Reset();
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)0, (Int8*)value);
	}

	// Set method.
	{
		AtomicPointer<Int8> value((Int8*)0);

		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)0, (Int8*)value);
		value.Set((Int8*)1);
		SEOUL_UNITTESTING_ASSERT_EQUAL((Int8*)1, (Int8*)value);
	}
}

void AtomicTest::TestAtomicPointerMultipleThread()
{
	static const Int32 kTestThreadCount = 50;
	AtomicPointerTest test;

	ScopedPtr<Thread> apThreads[kTestThreadCount];

	// Mixture - must be one or the other value we're setting.
	{
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			if (i % 2 == 0)
			{
				apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
					SEOUL_BIND_DELEGATE(&AtomicPointerTest::OneTwo7, &test)));
			}
			else
			{
				apThreads[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
					SEOUL_BIND_DELEGATE(&AtomicPointerTest::NegOne, &test)));
			}
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_VERIFY(apThreads[i]->Start());
		}

		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			apThreads[i]->WaitUntilThreadIsNotRunning();
		}

		SEOUL_UNITTESTING_ASSERT(
			((Int8*)(size_t)-1) == test.m_Atomic ||
			((Int8*)(size_t)127) == test.m_Atomic);
		Vector<Int> vValues(kTestThreadCount);
		for (Int32 i = 0; i < kTestThreadCount; ++i)
		{
			SEOUL_UNITTESTING_ASSERT(
				-1 == apThreads[i]->GetReturnValue() ||
				127 == apThreads[i]->GetReturnValue());
		}
	}
}

void AtomicTest::TestAtomicRingBufferSingleThread()
{
	static const UInt32 kTestBufferSize = 1024u;

	typedef AtomicRingBuffer<void*> TestBuffer;
	TestBuffer buffer;

	SEOUL_UNITTESTING_ASSERT(buffer.IsEmpty());

	for (size_t i = 1; i <= kTestBufferSize; ++i)
	{
		buffer.Push((void*)i);
	}

	SEOUL_UNITTESTING_ASSERT(buffer.GetCount() == kTestBufferSize);

	for (size_t i = 1; i <= kTestBufferSize; ++i)
	{
		void* p = buffer.Pop();
		SEOUL_UNITTESTING_ASSERT(nullptr != p);
		SEOUL_UNITTESTING_ASSERT((size_t)p == i);
	}

	SEOUL_UNITTESTING_ASSERT(buffer.IsEmpty());
}

struct AtomicRingBufferIdenticalValueTest
{
	SEOUL_DELEGATE_TARGET(AtomicRingBufferIdenticalValueTest);

	static const UInt32 kPushLoopCount = 4096u;
	static const UInt32 kPopLoopCount = kPushLoopCount - 1;

	typedef AtomicRingBuffer<UInt32*> TestRingBuffer;
	static TestRingBuffer s_AtomicRingBuffer;

	Signal m_StartSignal;

	Int Pop(const Thread&)
	{
		m_StartSignal.Wait();

		for (UInt32 i = 0u; i < kPopLoopCount; ++i)
		{
			UInt32* p = nullptr;
			while (nullptr == p)
			{
				Thread::YieldToAnotherThread();
				p = s_AtomicRingBuffer.Pop();
			}

			SEOUL_ASSERT((size_t)p == 1u);
		}

		return 0;
	}

	Int Push(const Thread&)
	{
		m_StartSignal.Wait();

		union
		{
			UInt32* p;
			size_t z;
		};

		z = 1u;

		for (UInt32 i = 0u; i < kPushLoopCount; ++i)
		{
			s_AtomicRingBuffer.Push(p);
		}

		return 0;
	}
}; // struct AtomicRingBufferIdenticalValueTest

AtomicRingBufferIdenticalValueTest::TestRingBuffer AtomicRingBufferIdenticalValueTest::s_AtomicRingBuffer;

void AtomicTest::TestAtomicRingBufferIdenticalValue()
{
	static const Int32 kTestThreadCount = 16;

	AtomicRingBufferIdenticalValueTest tests[2*kTestThreadCount];

	ScopedPtr<Thread> apThreadsPop[kTestThreadCount];
	ScopedPtr<Thread> apThreadsPush[kTestThreadCount];

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		apThreadsPop[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
			SEOUL_BIND_DELEGATE(&AtomicRingBufferIdenticalValueTest::Pop, &tests[2*i])));
		apThreadsPush[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
			SEOUL_BIND_DELEGATE(&AtomicRingBufferIdenticalValueTest::Push, &tests[2*i+1])));
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreadsPush[i]->Start());
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreadsPop[i]->Start());
	}

	for (Int32 i = 0; i < 2*kTestThreadCount; ++i)
	{
		// TODO: Replace this loop with a ActivateAll() when
		// SeoulSignal::ActivateAll() is implemented
		tests[i].m_StartSignal.Activate();
	}

	for (Int32 i = (kTestThreadCount - 1); i >= 0; --i)
	{
		apThreadsPop[i]->WaitUntilThreadIsNotRunning();
		apThreadsPush[i]->WaitUntilThreadIsNotRunning();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(kTestThreadCount, (Int32)AtomicRingBufferIdenticalValueTest::s_AtomicRingBuffer.GetCount());

	Int32 nCount = 0;
	UInt32* p = AtomicRingBufferIdenticalValueTest::s_AtomicRingBuffer.Pop();
	SEOUL_UNITTESTING_ASSERT(nullptr != p);
	while (nullptr != p)
	{
		++nCount;
		SEOUL_ASSERT((size_t)p == 1u);
		p = AtomicRingBufferIdenticalValueTest::s_AtomicRingBuffer.Pop();
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(kTestThreadCount, nCount);
	SEOUL_UNITTESTING_ASSERT(AtomicRingBufferIdenticalValueTest::s_AtomicRingBuffer.IsEmpty());
}

struct AtomicRingBufferFullTest
{
	SEOUL_DELEGATE_TARGET(AtomicRingBufferFullTest);

	static const UInt32 kPushLoopCount = 4096u;
	static const UInt32 kPopLoopCount = kPushLoopCount - 1;

	typedef AtomicRingBuffer<UInt32*> TestRingBuffer;
	static TestRingBuffer s_AtomicRingBuffer;

	Signal m_StartSignal;

	Int Pop(const Thread&)
	{
		m_StartSignal.Wait();

		for (UInt32 i = 0u; i < kPopLoopCount; ++i)
		{
			UInt32* p = nullptr;
			while (nullptr == p)
			{
				Thread::YieldToAnotherThread();
				p = s_AtomicRingBuffer.Pop();
			}

			SafeDelete(p);
		}

		return 0;
	}

	Int Push(const Thread&)
	{
		m_StartSignal.Wait();

		for (UInt32 i = 0u; i < kPushLoopCount; ++i)
		{
			UInt32* p = SEOUL_NEW(MemoryBudgets::TBD) UInt32;
			s_AtomicRingBuffer.Push(p);
		}

		return 0;
	}
}; // struct AtomicRingBufferFullTest

AtomicRingBufferFullTest::TestRingBuffer AtomicRingBufferFullTest::s_AtomicRingBuffer;

void AtomicTest::TestAtomicRingBufferFull()
{
	static const Int32 kTestThreadCount = 16;

	AtomicRingBufferFullTest tests[2*kTestThreadCount];

	ScopedPtr<Thread> apThreadsPop[kTestThreadCount];
	ScopedPtr<Thread> apThreadsPush[kTestThreadCount];

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		apThreadsPop[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
			SEOUL_BIND_DELEGATE(&AtomicRingBufferFullTest::Pop, &tests[2*i])));
		apThreadsPush[i].Reset(SEOUL_NEW(MemoryBudgets::TBD) Thread(
			SEOUL_BIND_DELEGATE(&AtomicRingBufferFullTest::Push, &tests[2*i+1])));
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreadsPush[i]->Start());
	}

	for (Int32 i = 0; i < kTestThreadCount; ++i)
	{
		SEOUL_VERIFY(apThreadsPop[i]->Start());
	}

	for (Int32 i = 0; i < 2*kTestThreadCount; ++i)
	{
		tests[i].m_StartSignal.Activate();
	}

	for (Int32 i = (kTestThreadCount - 1); i >= 0; --i)
	{
		apThreadsPop[i]->WaitUntilThreadIsNotRunning();
		apThreadsPush[i]->WaitUntilThreadIsNotRunning();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(kTestThreadCount, (Int32)AtomicRingBufferFullTest::s_AtomicRingBuffer.GetCount());

	Int32 nCount = 0;
	UInt32* p = AtomicRingBufferFullTest::s_AtomicRingBuffer.Pop();
	SEOUL_UNITTESTING_ASSERT(nullptr != p);
	while (nullptr != p)
	{
		++nCount;
		SafeDelete(p);
		p = AtomicRingBufferFullTest::s_AtomicRingBuffer.Pop();
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(kTestThreadCount, nCount);
	SEOUL_UNITTESTING_ASSERT(AtomicRingBufferFullTest::s_AtomicRingBuffer.IsEmpty());
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
