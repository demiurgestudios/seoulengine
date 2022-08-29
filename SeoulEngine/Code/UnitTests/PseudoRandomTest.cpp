/**
 * \file PseudoRandomTest.cpp
 * \brief Unit tests for the PseudoRandom class and corresponding
 * global math functions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HashFunctions.h"
#include "Logger.h"
#include "PseudoRandom.h"
#include "PseudoRandomTest.h"
#include "ReflectionDefine.h"
#include "SeoulTime.h"
#include "UnitTesting.h"

 // For good entropy initialization.
#define SECURE_RANDOM_H
#include "SecureRandomInternal.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

static const UInt32 kuIterations = 1024u;
static const UInt32 kuNormalIterations = 32768u;

SEOUL_BEGIN_TYPE(PseudoRandomTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestGlobalNormalRandomFloat)
	SEOUL_METHOD(TestGlobalNormalRandomFloatRange)
	SEOUL_METHOD(TestGlobalUniformFloat)
	SEOUL_METHOD(TestGlobalUniformIntRange)
	SEOUL_METHOD(TestInstanceNormalRandomFloat)
	SEOUL_METHOD(TestInstanceNormalRandomFloatRange)
	SEOUL_METHOD(TestInstanceUniformFloat)
	SEOUL_METHOD(TestInstanceUniformIntRange)
	SEOUL_METHOD(TestZeroZeroRegression)
	SEOUL_METHOD(TestBytesToSeed)
	SEOUL_METHOD(TestUniformRandomFloat64)
	SEOUL_METHOD(TestUniformRandomFloat32Regression)
SEOUL_END_TYPE()

template <typename FUNC>
static inline void NormalTest(Float64 fExpectedMean, Float64 fExpectedStdDev, FUNC& func)
{
	Vector<Float64, MemoryBudgets::Developer> v;
	v.Resize(kuNormalIterations);
	for (UInt32 i = 0u; i < kuNormalIterations; ++i)
	{
		Float64 const f = func();
		v[i] = f;
	}

	Float64 fMean = 0.0;
	for (auto const f : v)
	{
		fMean += f;
	}
	fMean /= (Float64)((Int64)kuNormalIterations);

	Float64 fStdDev = 0.0;
	for (auto const f : v)
	{
		auto const fDiff = (f - fMean);
		fStdDev += (fDiff * fDiff);
	}
	fStdDev = Sqrt(fStdDev / (Float64)((Int64)kuNormalIterations));

	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fExpectedMean, fMean, 1e-1);
	SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL(fExpectedStdDev, fStdDev, 1e-1);
}

namespace
{

struct GlobalNormalUtil { Float64 operator()() const { return GlobalRandom::NormalRandomFloat64(); } };
struct GlobalNormalUtilRange
{
	GlobalNormalUtilRange(Float64 fMean, Float64 fStdDev)
		: m_fMean(fMean)
		, m_fStdDev(fStdDev)
	{
	}

	Float64 operator()() const
	{
		return GlobalRandom::NormalRandomFloat64(m_fMean, m_fStdDev);
	}

	Float64 const m_fMean;
	Float64 const m_fStdDev;
}; // struct GlobalNormalUtilRange

struct InstanceNormalUtil
{
	InstanceNormalUtil()
		: m_Random(PseudoRandom::SeededPseudoRandom())
	{
	}

	Float64 operator()()
	{
		return m_Random.NormalRandomFloat64();
	}

	PseudoRandom m_Random;
};

struct InstanceNormalUtilRange
{
	InstanceNormalUtilRange(Float64 fMean, Float64 fStdDev)
		: m_fMean(fMean)
		, m_fStdDev(fStdDev)
		, m_Random(PseudoRandom::SeededPseudoRandom())
	{
	}

	Float64 operator()()
	{
		return m_Random.NormalRandomFloat64(m_fMean, m_fStdDev);
	}

	Float64 const m_fMean;
	Float64 const m_fStdDev;
	PseudoRandom m_Random;
}; // struct InstanceNormalUtilRange

} // namespace anonymous

void PseudoRandomTest::TestGlobalNormalRandomFloat()
{
	GlobalNormalUtil util;
	NormalTest(0.0, 1.0, util);
}

void PseudoRandomTest::TestGlobalNormalRandomFloatRange()
{
	{
		GlobalNormalUtilRange util(1.0, 3.0);
		NormalTest(1.0, 3.0, util);
	}
	{
		GlobalNormalUtilRange util(-1.0, 3.0);
		NormalTest(-1.0, 3.0, util);
	}
}

void PseudoRandomTest::TestGlobalUniformFloat()
{
	// Float32
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		Float32 const f = GlobalRandom::UniformRandomFloat32();
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0.0f, f);
		SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(1.0f, f);
	}

	// Float64
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		Float64 const f = GlobalRandom::UniformRandomFloat64();
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0.0, f);
		SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(1.0, f);
	}
}

void PseudoRandomTest::TestGlobalUniformIntRange()
{
	static const UInt32 kauTestUInt32[] =
	{
		25u, 32u, 302508u, 87u,
	};
	static const UInt64 kauTestUInt64[] =
	{
		12u, 33u, 120923409u, 209582039580u,
	};

	// Int63
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		Int64 const iValue = GlobalRandom::UniformRandomInt63();
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, iValue);
	}

	// UInt32
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		{
			UInt32 const u = GlobalRandom::UniformRandomUInt32n(1u);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, u);
		}
		for (size_t j = 0u; j < SEOUL_ARRAY_COUNT(kauTestUInt32); ++j)
		{
			UInt32 const u = GlobalRandom::UniformRandomUInt32n(kauTestUInt32[j]);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(kauTestUInt32[j], u);
		}
	}

	// UInt64
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		{
			UInt64 const u = GlobalRandom::UniformRandomUInt64n(1u);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, u);
		}
		for (size_t j = 0u; j < SEOUL_ARRAY_COUNT(kauTestUInt64); ++j)
		{
			UInt64 const u = GlobalRandom::UniformRandomUInt64n(kauTestUInt64[j]);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(kauTestUInt64[j], u);
		}
	}
}

void PseudoRandomTest::TestInstanceNormalRandomFloat()
{
	InstanceNormalUtil util;
	NormalTest(0.0, 1.0, util);
}

void PseudoRandomTest::TestInstanceNormalRandomFloatRange()
{
	{
		InstanceNormalUtilRange util(1.0, 3.0);
		NormalTest(1.0, 3.0, util);
	}
	{
		InstanceNormalUtilRange util(-1.0, 3.0);
		NormalTest(-1.0, 3.0, util);
	}
}

void PseudoRandomTest::TestInstanceUniformFloat()
{
	PseudoRandom random(PseudoRandom::SeededPseudoRandom());

	// Float32
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		Float32 const f = random.UniformRandomFloat32();
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0.0f, f);
		SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(1.0f, f);
	}

	// Float64
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		Float64 const f = random.UniformRandomFloat64();
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0.0, f);
		SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(1.0, f);
	}
}

void PseudoRandomTest::TestInstanceUniformIntRange()
{
	static const UInt32 kauTestUInt32[] =
	{
		25u, 32u, 302508u, 87u,
	};
	static const UInt64 kauTestUInt64[] =
	{
		12u, 33u, 120923409u, 209582039580u,
	};

	PseudoRandom random(PseudoRandom::SeededPseudoRandom());

	// Int63
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		Int64 const iValue = random.UniformRandomInt63();
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, iValue);
	}

	// UInt32
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		{
			UInt32 const u = random.UniformRandomUInt32n(1u);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, u);
		}
		for (size_t j = 0u; j < SEOUL_ARRAY_COUNT(kauTestUInt32); ++j)
		{
			UInt32 const u = random.UniformRandomUInt32n(kauTestUInt32[j]);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(kauTestUInt32[j], u);
		}
	}

	// UInt64
	for (UInt32 i = 0u; i < kuIterations; ++i)
	{
		{
			UInt64 const u = random.UniformRandomUInt64n(1u);
			SEOUL_UNITTESTING_ASSERT_EQUAL(0u, u);
		}
		for (size_t j = 0u; j < SEOUL_ARRAY_COUNT(kauTestUInt64); ++j)
		{
			UInt64 const u = random.UniformRandomUInt64n(kauTestUInt64[j]);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(kauTestUInt64[j], u);
		}
	}
}

// First implementation of pseudo random  was susceptible to decimation
// if the random number generator was constructed with a seed of (0, 0)
// (xorshift128+ is degenerate if seed values are 0 and the sanitizing to
// prevent this was not applied to the PseudoRandom constructor that
// accepted a seed value.
void PseudoRandomTest::TestZeroZeroRegression()
{
	static const UInt32 kuBuckets = 512u;
	static const UInt32 kuPerBucket = kuBuckets * kuBuckets;

	for (auto const& seed : { PseudoRandomSeed(1, 1), PseudoRandomSeed(0, 1), PseudoRandomSeed(1, 0), PseudoRandomSeed(0, 0) })
	{
		PseudoRandom random(seed);
		FixedArray<UInt32, kuBuckets> aBuckets;
		for (UInt32 i = 0u; i < kuBuckets * kuPerBucket; ++i)
		{
			auto const u = random.UniformRandomUInt32n(kuBuckets);
			aBuckets[u]++;
		}

		for (UInt32 i = 0u; i < aBuckets.GetSize(); ++i)
		{
			SEOUL_UNITTESTING_ASSERT_DOUBLES_EQUAL((Double)aBuckets[i], (Double)kuPerBucket, (Double)kuBuckets * 4);
		}
	}
}

void PseudoRandomTest::TestBytesToSeed()
{
	// Test a string with a known MD5
	PseudoRandomSeed seed;
	{
		PseudoRandomSeedBuilder builder(seed);
		// MD5: 9e107d9d372bb6826bd81d3542a419d6
		// MD5 as two UInt64s: 11389741571808933506, 7770993271616313814
		builder.AppendData("The quick brown fox jumps over the lazy dog");
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(11389741571808933506u, seed.m_uX);
	SEOUL_UNITTESTING_ASSERT_EQUAL(7770993271616313814u, seed.m_uY);
}

void PseudoRandomTest::TestUniformRandomFloat64()
{
	// Verify that this approach produces a value in the expected range.
	// Increase range to 1 << 54 to demonstrate failure of larger range.
	const UInt64 uMaxPowerOf2 = ((UInt64)1 << (UInt64)53);
	// Need a large step to complete in any sort of reasonable time.
	const UInt64 uStep = ((UInt64)1 << (UInt64)28);
	for (UInt64 u = 0u; u < uMaxPowerOf2; u += uStep)
	{
		auto const f = (Float64)u / (Float64)uMaxPowerOf2;
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, f);
		SEOUL_UNITTESTING_ASSERT_GREATER_THAN(1, f);
		auto const uTest = (UInt64)(f * (Float64)uMaxPowerOf2);
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL((UInt64)0, u);
		SEOUL_UNITTESTING_ASSERT_GREATER_THAN(uMaxPowerOf2, uTest);
	}
	// Manually check end points for sanity and due to stepping behavior mentioned above.
	{
		auto const u = 1u;
		auto const f = (Float64)u / (Float64)uMaxPowerOf2;
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, f);
		SEOUL_UNITTESTING_ASSERT_GREATER_THAN(1, f);
		auto const uTest = (UInt64)(f * (Float64)uMaxPowerOf2);
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL((UInt64)0, u);
		SEOUL_UNITTESTING_ASSERT_GREATER_THAN(uMaxPowerOf2, uTest);
	}
	{
		auto const u = (uMaxPowerOf2 - 1u);
		auto const f = (Float64)u / (Float64)uMaxPowerOf2;
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, f);
		SEOUL_UNITTESTING_ASSERT_GREATER_THAN(1, f);
		auto const uTest = (UInt64)(f * (Float64)uMaxPowerOf2);
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL((UInt64)0, u);
		SEOUL_UNITTESTING_ASSERT_GREATER_THAN(uMaxPowerOf2, uTest);
	}

	// Now test fixed pseudo random uTestCount times and verify value.
	const UInt64 uTestCount = ((UInt64)1 << (UInt64)26); // Large but not forever.
	{
		PseudoRandom rand(PseudoRandomSeed(0xD3E3C425A47E911F, 0xEDC11D7A3A01D1E8));
		for (UInt64 i = 0u; i < uTestCount; ++i)
		{
			auto const f = rand.UniformRandomFloat64();
			SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, f);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(1, f);
			auto const uTest = (UInt64)(f * (Float64)uMaxPowerOf2);
			SEOUL_UNITTESTING_ASSERT_LESS_EQUAL((UInt64)0, uTest);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(uMaxPowerOf2, uTest);
		}
	}

	// Same, test fixed pseudo random uMaxPowerOf2 times, this time on ranged backing function.
	{
		PseudoRandom rand(PseudoRandomSeed(0xD3E3C425A47E911F, 0xEDC11D7A3A01D1E8));
		for (UInt64 i = 0u; i < uTestCount; ++i)
		{
			// 0 <= u < 2^53
			UInt64 const u = rand.UniformRandomUInt64n(uMaxPowerOf2);

			auto const f = (Float64)u / (Float64)uMaxPowerOf2;
			SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, f);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(1, f);
			auto const uTest = (UInt64)(f * (Float64)uMaxPowerOf2);
			SEOUL_UNITTESTING_ASSERT_LESS_EQUAL((UInt64)0, u);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(uMaxPowerOf2, uTest);
		}
	}
}

// Regression for a bug in UniformRandomFloat32() that violated the
// [0, 1) guarantee. Bug was due to a typo in the body of that method
// that used too large a range for 32-bit floats and caused [big/big]
// divide, which would generate 1 due to decimation.
void PseudoRandomTest::TestUniformRandomFloat32Regression()
{
	// Verify that this approach produces a value in the expected range.
	// Increase range to 1 << 25 to demonstrate failure of larger range.
	const UInt64 uMaxPowerOf2 = ((UInt64)1 << (UInt64)24);
	for (UInt64 u = 0u; u < uMaxPowerOf2; ++u)
	{
		auto const f = (Float32)u / (Float32)uMaxPowerOf2;
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, f);
		SEOUL_UNITTESTING_ASSERT_GREATER_THAN(1, f);
		auto const uTest = (UInt64)(f * (Float32)uMaxPowerOf2);
		SEOUL_UNITTESTING_ASSERT_LESS_EQUAL((UInt64)0, u);
		SEOUL_UNITTESTING_ASSERT_GREATER_THAN(uMaxPowerOf2, uTest);
	}

	// Now test fixed pseudo random uTestCount times and verify value.
	const UInt64 uTestCount = ((UInt64)1 << (UInt64)26); // Bigger than uMaxPowerOf2, but not forever. Reproduces regression with unfixed code.
	{
		PseudoRandom rand(PseudoRandomSeed(0xD3E3C425A47E911F, 0xEDC11D7A3A01D1E8));
		for (UInt64 i = 0u; i < uTestCount; ++i)
		{
			auto const f = rand.UniformRandomFloat32();
			SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, f);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(1, f);
			auto const uTest = (UInt64)(f * (Float32)uMaxPowerOf2);
			SEOUL_UNITTESTING_ASSERT_LESS_EQUAL((UInt64)0, uTest);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(uMaxPowerOf2, uTest);
		}
	}

	// Same, test fixed pseudo random uMaxPowerOf2 times, this time on ranged backing function.
	{
		PseudoRandom rand(PseudoRandomSeed(0xD3E3C425A47E911F, 0xEDC11D7A3A01D1E8));
		for (UInt64 i = 0u; i < uTestCount; ++i)
		{
			// 0 <= u < 2^24
			UInt64 const u = rand.UniformRandomUInt64n(uMaxPowerOf2);

			auto const f = (Float32)u / (Float32)uMaxPowerOf2;
			SEOUL_UNITTESTING_ASSERT_LESS_EQUAL(0, f);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(1, f);
			auto const uTest = (UInt64)(f * (Float32)uMaxPowerOf2);
			SEOUL_UNITTESTING_ASSERT_LESS_EQUAL((UInt64)0, u);
			SEOUL_UNITTESTING_ASSERT_GREATER_THAN(uMaxPowerOf2, uTest);
		}
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
