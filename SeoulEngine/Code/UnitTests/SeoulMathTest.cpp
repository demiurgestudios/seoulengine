/**
 * \file SeoulMathTest.cpp
 * \brief Unit tests for Seoul engine global math functions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "LinearCurve.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SeoulMathTest.h"
#include "SeoulTime.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(SeoulMathTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBitCount)
	SEOUL_METHOD(TestClampCircular)
	SEOUL_METHOD(TestLinearCurveDefault)
	SEOUL_METHOD(TestLinearCurveBasic)
	SEOUL_METHOD(TestMathFunctions)
	SEOUL_METHOD(TestInt32Clamped)
SEOUL_END_TYPE()

void SeoulMathTest::TestBitCount()
{
	UInt32 u = 0u;

	// No Bits.
	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, CountBits(u));

	// Single bits.
	for (UInt32 i = 0u; i < 32; ++i)
	{
		u = (1 << i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1u, CountBits(u));
	}

	// Progrsesive.
	u = 0u;
	for (UInt32 i = 0u; i < 32; ++i)
	{
		u |= (1 << i);
		SEOUL_UNITTESTING_ASSERT_EQUAL(i + 1u, CountBits(u));
	}

	// All but one.
	for (UInt32 i = 0u; i < 32; ++i)
	{
		u = 0xFFFFFFFF & ~(1 << 5);
		SEOUL_UNITTESTING_ASSERT_EQUAL(31u, CountBits(u));
	}
}

void SeoulMathTest::TestClampCircular()
{
	SEOUL_UNITTESTING_ASSERT_EQUAL(5.5f, ClampDegrees(725.5f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(5, ClampDegrees(725));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ClampDegrees(720));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, ClampDegrees(-720));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-5, ClampDegrees(-725));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-5.5f, ClampDegrees(-725.5f));

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.05, ClampRadians(2.0 * Pi + 0.05), 1e-12);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(0.05f, ClampRadians(2.0f * fPi + 0.05f), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, ClampRadians(2.0f * fPi));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, ClampRadians(-2.0f * fPi));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-0.05f, ClampRadians(-2.0f * fPi - 0.05f), 1e-5f);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-0.05, ClampRadians(-2.0f * Pi - 0.05), 1e-12);
}

void SeoulMathTest::TestLinearCurveDefault()
{
	LinearCurve<Float> curve;
	SEOUL_UNITTESTING_ASSERT(curve.m_vTimes.IsEmpty());
	SEOUL_UNITTESTING_ASSERT(curve.m_vValues.IsEmpty());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, curve.GetFirstT());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, curve.GetLastT());

	Float fResult = 1.0f;
	SEOUL_UNITTESTING_ASSERT_EQUAL(false, curve.Evaluate(GlobalRandom::UniformRandomFloat32(), fResult));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, fResult);
}

void SeoulMathTest::TestLinearCurveBasic()
{
	LinearCurve<Float> curve;

	curve.m_vTimes.PushBack(0.0f);
	curve.m_vTimes.PushBack(0.5f);
	curve.m_vTimes.PushBack(1.0f);
	curve.m_vValues.PushBack(1.0f);
	curve.m_vValues.PushBack(0.0f);
	curve.m_vValues.PushBack(-1.0f);

	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, curve.GetFirstT());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, curve.GetLastT());

	Float fResult = 10.0f;
	SEOUL_UNITTESTING_ASSERT(curve.Evaluate(0.0f, fResult));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL( 1.0f, fResult, fEpsilon);
	SEOUL_UNITTESTING_ASSERT(curve.Evaluate(0.5f, fResult));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL( 0.0f, fResult, fEpsilon);
	SEOUL_UNITTESTING_ASSERT(curve.Evaluate(1.0f, fResult));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-1.0f, fResult, fEpsilon);

	SEOUL_UNITTESTING_ASSERT(curve.Evaluate(0.25f, fResult));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL( 0.5f, fResult, 1e-6f);
	SEOUL_UNITTESTING_ASSERT(curve.Evaluate(0.75f, fResult));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-0.5f, fResult, 1e-6f);

	SEOUL_UNITTESTING_ASSERT(curve.Evaluate(-1.0f, fResult));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL( 1.0f, fResult, fEpsilon);
	SEOUL_UNITTESTING_ASSERT(curve.Evaluate(2.0f, fResult));
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(-1.0f, fResult, fEpsilon);
}

void SeoulMathTest::TestMathFunctions()
{
	const Float fNaN = std::numeric_limits<Float>::signaling_NaN();
	const Double NaN = std::numeric_limits<Double>::signaling_NaN();

	// Sign
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, Sign(-fEpsilon));
	SEOUL_UNITTESTING_ASSERT_EQUAL( 0, Sign(0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL( 1, Sign( fEpsilon));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, Sign(-Epsilon));
	SEOUL_UNITTESTING_ASSERT_EQUAL( 0, Sign(0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL( 1, Sign( Epsilon));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, fSign(-fEpsilon));
	SEOUL_UNITTESTING_ASSERT_EQUAL( 0.0f, fSign(0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL( 1.0f, fSign( fEpsilon));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, fSign(-Epsilon));
	SEOUL_UNITTESTING_ASSERT_EQUAL( 0.0f, fSign(0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL( 1.0f, fSign( Epsilon));

	// Angles
	SEOUL_UNITTESTING_ASSERT_EQUAL( 180.0f, RadiansToDegrees(fPi));
	SEOUL_UNITTESTING_ASSERT_EQUAL( 180.0,  RadiansToDegrees( Pi));
	SEOUL_UNITTESTING_ASSERT_EQUAL( fPi, DegreesToRadians(180.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL( Pi,  DegreesToRadians(180.0));

	// Angle clamp
	SEOUL_UNITTESTING_ASSERT( Equals(0.0f, RadianClampTo0ToTwoPi(-fTwoPi)) );
	SEOUL_UNITTESTING_ASSERT( Equals(0.0, RadianClampTo0ToTwoPi(-TwoPi)) );
	SEOUL_UNITTESTING_ASSERT( Equals(0.0f, RadianClampTo0ToTwoPi(2.0f * fTwoPi)) );
	SEOUL_UNITTESTING_ASSERT( Equals(0.0, RadianClampTo0ToTwoPi(2.0 * TwoPi)) );
	SEOUL_UNITTESTING_ASSERT( Equals(0.0f, DegreeClampTo0To360(-360.0f)) );
	SEOUL_UNITTESTING_ASSERT( Equals(0.0, DegreeClampTo0To360(-360.0)) );
	SEOUL_UNITTESTING_ASSERT( Equals(0.0f, DegreeClampTo0To360(2.0f * 360.0f)) );
	SEOUL_UNITTESTING_ASSERT( Equals(0.0, DegreeClampTo0To360(2.0 * 360.0)) );

	// Rand
	SEOUL_UNITTESTING_ASSERT(GlobalRandom::UniformRandomInt63() >= 0);
	SEOUL_UNITTESTING_ASSERT(GlobalRandom::UniformRandomFloat32() >= 0.0f);
	SEOUL_UNITTESTING_ASSERT(GlobalRandom::UniformRandomFloat32() < 1.0f);
	SEOUL_UNITTESTING_ASSERT(GlobalRandom::UniformRandomFloat64() >= 0.0);
	SEOUL_UNITTESTING_ASSERT(GlobalRandom::UniformRandomFloat64() < 1.0);

	// Min
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, Min(-1, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, Min(-1, 0, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, Min(-1, 0, 1, 2));

	// Max
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, Max(-1, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, Max(-1, 0, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2, Max(-1, 0, 1, 2));

	// Clamp
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, Clamp(0.0f, -1.0f, 1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, Clamp(-2.0f, -1.0f, 1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, Clamp(2.0f, -1.0f, 1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, Clamp(fNaN, -1.0f, 1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0, Clamp(NaN, -1.0, 1.0));

	// General math functions
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, Abs(-1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, Abs(-1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, Abs(-1.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, Acos(1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, Acos(1.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, Asin(0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, Asin(0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, Atan(0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, Atan(0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, Ceil(0.5f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, Ceil(0.5));
	SEOUL_UNITTESTING_ASSERT(Equals(1.0f, Cos(0.0f)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, Cos(0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, Exp(0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, Exp(0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, Floor(0.5f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, Floor(0.5));
	SEOUL_UNITTESTING_ASSERT(Equals(0.25f, Fmod(1.0f, 0.75f)));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.25, Fmod(1.0, 0.75));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, LogE(1.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, LogE(1.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, Log10(10.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, Log10(10.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4.0f, Pow(2.0f, 2.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4.0, Pow(2.0, 2.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, Sin(0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, Sin(0.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, Sqrt(4.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(2.0, Sqrt(4.0));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, Tan(0.0f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0.0, Tan(0.0));

	// IsZero
	SEOUL_UNITTESTING_ASSERT(IsZero(0.0));
	SEOUL_UNITTESTING_ASSERT(IsZero(0.0f));
	SEOUL_UNITTESTING_ASSERT(IsZero(fEpsilon));
	SEOUL_UNITTESTING_ASSERT(IsZero(Epsilon));
	SEOUL_UNITTESTING_ASSERT(IsZero(-fEpsilon));
	SEOUL_UNITTESTING_ASSERT(IsZero(-Epsilon));
	SEOUL_UNITTESTING_ASSERT(!IsZero(fEpsilon + fEpsilon));
	SEOUL_UNITTESTING_ASSERT(!IsZero(Epsilon + Epsilon));
	SEOUL_UNITTESTING_ASSERT(!IsZero(-fEpsilon - fEpsilon));
	SEOUL_UNITTESTING_ASSERT(!IsZero(-Epsilon - Epsilon));
	SEOUL_UNITTESTING_ASSERT(IsZero(1.0f - fEpsilon, 1.0f));
	SEOUL_UNITTESTING_ASSERT(IsZero(1.0 - Epsilon, 1.0));
	SEOUL_UNITTESTING_ASSERT(IsZero(-1.0f + fEpsilon, 1.0f));
	SEOUL_UNITTESTING_ASSERT(IsZero(-1.0 + Epsilon, 1.0));

	// Equals
	SEOUL_UNITTESTING_ASSERT(Equals(1.0, 1.0));
	SEOUL_UNITTESTING_ASSERT(Equals(1.0f, 1.0f));
	SEOUL_UNITTESTING_ASSERT(Equals(1.0f + fEpsilon, 1.0f));
	SEOUL_UNITTESTING_ASSERT(Equals(1.0 + Epsilon, 1.0));
	SEOUL_UNITTESTING_ASSERT(Equals(1.0f - fEpsilon, 1.0f));
	SEOUL_UNITTESTING_ASSERT(Equals(1.0 - Epsilon, 1.0));
	SEOUL_UNITTESTING_ASSERT(Equals(0.0f + fEpsilon, 1.0f, 1.0f));
	SEOUL_UNITTESTING_ASSERT(Equals(0.0 + Epsilon, 1.0, 1.0));
	SEOUL_UNITTESTING_ASSERT(Equals(2.0f - fEpsilon, 1.0f, 1.0f));
	SEOUL_UNITTESTING_ASSERT(Equals(2.0 - Epsilon, 1.0, 1.0));

	// Equal degrees and radians.
	for (Int i = -720; i <= 720; ++i)
	{
		// Double
		{
			Double const fDegrees = (Double)i;
			Double const fRadians = DegreesToRadians(fDegrees);

			SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fDegrees - 360.0));
			SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fDegrees + 360.0));
			SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fDegrees - 720.0));
			SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fDegrees + 720.0));
			SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fRadians - TwoPi, 1e-9));
			SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fRadians + TwoPi, 1e-9));
			SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fRadians - 2.0 * TwoPi, 1e-9));
			SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fRadians + 2.0 * TwoPi, 1e-9));
		}

		// Float
		{
			Float const fDegrees = (Float)i;
			Float const fRadians = DegreesToRadians(fDegrees);

			SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fDegrees - 360.0f));
			SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fDegrees + 360.0f));
			SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fDegrees - 720.0f));
			SEOUL_UNITTESTING_ASSERT(EqualDegrees(fDegrees, fDegrees + 720.0f));
			SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fRadians - fTwoPi, 1e-5f));
			SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fRadians + fTwoPi, 1e-5f));
			SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fRadians - 2.0f * fTwoPi, 1e-5f));
			SEOUL_UNITTESTING_ASSERT(EqualRadians(fRadians, fRadians + 2.0f * fTwoPi, 1e-5f));
		}
	}

	// IsNaN
	SEOUL_UNITTESTING_ASSERT(!IsNaN(GlobalRandom::UniformRandomFloat32()));
	SEOUL_UNITTESTING_ASSERT(!IsNaN(GlobalRandom::UniformRandomFloat64()));
	SEOUL_UNITTESTING_ASSERT(IsNaN(std::numeric_limits<Float>::signaling_NaN()));
	SEOUL_UNITTESTING_ASSERT(IsNaN(std::numeric_limits<Float>::quiet_NaN()));
	SEOUL_UNITTESTING_ASSERT(IsNaN(std::numeric_limits<Double>::signaling_NaN()));
	SEOUL_UNITTESTING_ASSERT(IsNaN(std::numeric_limits<Double>::quiet_NaN()));

	// IsInf
	SEOUL_UNITTESTING_ASSERT(!IsInf(GlobalRandom::UniformRandomFloat32()));
	SEOUL_UNITTESTING_ASSERT(!IsInf(GlobalRandom::UniformRandomFloat64()));
	SEOUL_UNITTESTING_ASSERT(IsInf(std::numeric_limits<Float>::infinity()));
	SEOUL_UNITTESTING_ASSERT(IsInf(std::numeric_limits<Double>::infinity()));
	SEOUL_UNITTESTING_ASSERT(IsInf(-std::numeric_limits<Float>::infinity()));
	SEOUL_UNITTESTING_ASSERT(IsInf(-std::numeric_limits<Double>::infinity()));

	// Lerp
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, Lerp(1, 1, GlobalRandom::UniformRandomFloat32()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, Lerp(1.0f, 1.0f, GlobalRandom::UniformRandomFloat32()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, Lerp(1.0, 1.0, GlobalRandom::UniformRandomFloat64()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, Lerp(-1, -1, GlobalRandom::UniformRandomFloat32()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, Lerp(-1.0f, -1.0f, GlobalRandom::UniformRandomFloat32()));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0, Lerp(-1.0, -1.0, GlobalRandom::UniformRandomFloat64()));

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, Lerp(0, 2, 0.5f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, Lerp(0.0f, 2.0f, 0.5f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(1.0, Lerp(0.0, 2.0, 0.5));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1, Lerp(0, -2, 0.5f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0f, Lerp(0.0f, -2.0f, 0.5f));
	SEOUL_UNITTESTING_ASSERT_EQUAL(-1.0, Lerp(0.0, -2.0, 0.5));

	// Conversion
	SEOUL_UNITTESTING_ASSERT_EQUAL((UInt64)1000u, ConvertMillisecondsToMicroseconds(1u));
}

void SeoulMathTest::TestInt32Clamped()
{
	// Add
	SEOUL_UNITTESTING_ASSERT_EQUAL(0 + IntMax, AddInt32Clamped(0, IntMax));
	SEOUL_UNITTESTING_ASSERT_EQUAL(0 + IntMin, AddInt32Clamped(0, IntMin));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax + IntMin, AddInt32Clamped(IntMax, IntMin));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, AddInt32Clamped(IntMax, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, AddInt32Clamped(IntMax, IntMax));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, AddInt32Clamped(IntMin, -1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, AddInt32Clamped(IntMin, IntMin));

	// Sub
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax - IntMax, SubInt32Clamped(IntMax, IntMax));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin - IntMin, SubInt32Clamped(IntMin, IntMin));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, SubInt32Clamped(IntMax, -1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMax, SubInt32Clamped(IntMax, -IntMin));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, SubInt32Clamped(IntMin, 1));
	SEOUL_UNITTESTING_ASSERT_EQUAL(IntMin, SubInt32Clamped(IntMin, IntMax));
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
