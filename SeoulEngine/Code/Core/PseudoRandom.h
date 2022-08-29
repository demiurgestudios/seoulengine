/**
* \file PseudoRandom.h
*
* \brief 64-bit seedable PRNG with 128-bit keys. "xorshift128+" algorithm.
* NOT CRYPTOGRAPHICALLY SECURE.
* \sa http://vigna.di.unimi.it/ftp/papers/xorshiftplus.pdf
*
* There are three ways to use PseudoRandom:
*  - auto rand = PseudoRandom::SeededPseudoRandom(); // Seeded from the OS's secure random source
*  - auto rand = PseudoRandom::SeededFromString(sData); // Seeded deterministically from a hash of
*    the given string
*  - Use PseudoRandomSeedBuilder to generate a deterministic seed from more complex inputs.
*
* The last two options give you repeatable results as safely as possible. This approach is easy to
* break (it relies on the initial seeds being uniformly distributed across all its inputs), so avoid
* it whenever possible.
*
* To use PseudoRandomSeedBuilder, decide on the inputs, and feed them in a predictable order. For example:
*
*     PseudoRandomSeed seed;
*     {
*         PseudoRandomSeedBuilder builder(seed);

*         builder.AppendData(playerGuid);
*         builder.AppendData(gachaId);
*         builder.AppendData(dayOfYear);
*     }
*
*    PseudoRandom rand(seed);
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#pragma once
#ifndef PSEUDO_RANDOM_H
#define PSEUDO_RANDOM_H

#include "Prereqs.h"
#include "CheckedPtr.h"
#include "HashFunctions.h"
#include "SeoulMath.h"

namespace Seoul { class MD5; }

namespace Seoul
{

struct PseudoRandomSeed SEOUL_SEALED
{
	// Use PseudoRandom::SeededPseudoRandom() or PseudoRandom::SeededFromString() instead.
	PseudoRandomSeed()
		: m_uX(0xD3E3C425A47E911F)
		, m_uY(0xEDC11D7A3A01D1E8)
	{
	}

	PseudoRandomSeed(UInt64 uX, UInt64 uY)
		: m_uX(uX)
		, m_uY(uY)
	{
	}

	UInt64 m_uX;
	UInt64 m_uY;
}; // struct PseudoRandomSeed

class PseudoRandomSeedBuilder SEOUL_SEALED
{
public:
	PseudoRandomSeedBuilder(PseudoRandomSeed &rOutput);
	~PseudoRandomSeedBuilder();

	void AppendData(UInt8 const* pData, UInt32 zDataSizeInBytes);

	void AppendData(void const* pData, UInt32 zDataSizeInBytes)
	{
		AppendData((UInt8 const*)pData, zDataSizeInBytes);
	}

	void AppendData(Byte const* s)
	{
		AppendData(s, StrLen(s));
	}

	void AppendData(HString h)
	{
		AppendData(h.CStr(), h.GetSizeInBytes());
	}

	void AppendData(const String& s)
	{
		AppendData(s.CStr(), s.GetSize());
	}

	template <typename T>
	void AppendPOD(const T& v)
	{
		AppendData(&v, sizeof(v));
	}

private:
	PseudoRandomSeed & m_rOutput;
	FixedArray<UInt8, 16u> m_aMD5Out;
	CheckedPtr<MD5> m_pMD5;
};

class PseudoRandom SEOUL_SEALED
{
public:
	static PseudoRandom SeededPseudoRandom();
	static PseudoRandom SeededFromString(String const& toHash);

public:
	// Use PseudoRandom::SeededPseudoRandom() or PseudoRandom::SeededFromString() instead.
	PseudoRandom()
		: m_Seed()
	{
		Sanitize();
	}

	explicit PseudoRandom(const PseudoRandomSeed& seed)
		: m_Seed(seed)
	{
		Sanitize();
	}

	const PseudoRandomSeed& GetSeed() const
	{
		return m_Seed;
	}

	void SetSeed(const PseudoRandomSeed& seed)
	{
		m_Seed = seed;
		Sanitize();
	}

	/** Get normal (Gaussian) random sample with mean 0 and standard deviation 1. */
	Float64 NormalRandomFloat64()
	{
		// Use Box-Muller algorithm
		Float64 const f1 = UniformRandomFloat64();
		Float64 const f2 = UniformRandomFloat64();
		Float64 const fR = Sqrt(-2.0 * LogE(f1));
		Float64 const fTheta = 2.0 * Pi * f2;

		return fR * Sin(fTheta);
	}

	/** Get normal (Gaussian) random sample with specified mean and standard deviation. */
	Float64 NormalRandomFloat64(Float64 fMean, Float64 fStandardDeviation)
	{
		return fMean + (fStandardDeviation * NormalRandomFloat64());
	}

	/**
	 * Produce a uniform random sample on [0, 1)
	 * (includes 0 but excludes 1, the most common convention).
	 */
	Float32 UniformRandomFloat32()
	{
		// (1 << 24) is the largest PowerOf2 at which (Float32)(1 << (n - 1)) != 1.0f
		const UInt64 uMaxPowerOf2 = ((UInt64)1 << (UInt64)24);

		// 0 <= u < 2^24
		UInt64 const u = UniformRandomUInt64n(uMaxPowerOf2);

		return (Float32)u / (Float32)uMaxPowerOf2;
	}

	/**
	 * Produce a uniform random sample on [0, 1)
	 * (includes 0 but excludes 1, the most common convention).
	 */
	Float64 UniformRandomFloat64()
	{
		// (1 << 53) is the largest PowerOf2 at which (Float64)(1 << (n - 1)) != 1.0
		const UInt64 uMaxPowerOf2 = ((UInt64)1 << (UInt64)53);

		// 0 <= u < 2^53
		UInt64 const u = UniformRandomUInt64n(uMaxPowerOf2);

		return (Float64)u / (Float64)uMaxPowerOf2;
	}

	/**
	 * Generate an Int32 psuedorandom number.
	 */
	Int32 UniformRandomInt32()
	{
		return (Int32)(UniformRandomUInt64() >> (UInt64)32);
	}

	/**
	 * Generate a UInt32 psuedorandom number.
	 */
	UInt32 UniformRandomUInt32()
	{
		return (UInt32)(UniformRandomUInt64() >> (UInt64)32);
	}

	/**
	 * Generates a non-negative Int64 psuedorandom number.
	 */
	Int64 UniformRandomInt63()
	{
		return (Int64)(UniformRandomUInt64() >> (UInt64)1);
	}

	/**
	 * Generate an Int64 psuedorandom number.
	 */
	Int64 UniformRandomInt64()
	{
		return (Int64)UniformRandomUInt64();
	}

	/**
	 * Generate a UInt64 psuedo-random number using the xorshift128+ algorithm.
	 *
	 * \sa http://vigna.di.unimi.it/ftp/papers/xorshiftplus.pdf
	 */
	UInt64 UniformRandomUInt64()
	{
		UInt64 uX = m_Seed.m_uX;
		UInt64 const uY = m_Seed.m_uY;
		m_Seed.m_uX = uY;

		uX ^= uX << 23; // a
		uX ^= uX >> 17; // b
		uX ^= uY ^ (uY >> 26); // c

		m_Seed.m_uY = uX;
		return uX + uY;
	}

	/**
	* Generate a UInt32 psuedo-random number on [lower, upper].
	*/
	UInt32 UniformRandomUInt32n(UInt32 iLower, UInt32 iUpper)
	{
		SEOUL_ASSERT(iLower <= iUpper);
		// Compute n - since our generator function generates a range that excludes
		// n, we need to +1 the delta.
		auto const iDelta = (iUpper - iLower) + 1;

		// UniformRandomUInt32() is on [0, n), so we get the desired
		// output by adding the result to iLower.
		return (UniformRandomUInt32n(iDelta) + iLower);
	}

	/**
	 * Generate a UInt32 psuedo-random number on [0, n).
	 */
	UInt32 UniformRandomUInt32n(UInt32 u)
	{
		if (0 == (u & (u - 1)))
		{
			// PowerOf2 early out case.
			return UniformRandomUInt32() & (u - (UInt32)1);
		}

		// Otherwise, need to try and repeat until we hit
		// an appropriate multiple for the target.
		UInt32 const uMax = (UInt32)(
			(UInt64)UIntMax -
			((((UInt64)UIntMax % (UInt64)u) + (UInt64)1) & (UInt64)u));

		UInt32 uReturn = UniformRandomUInt32();
		while (uReturn > uMax)
		{
			uReturn = UniformRandomUInt32();
		}

		return (uReturn % u);
	}

	/**
	 * Generate a UInt64 psuedo-random number on [0, n).
	 */
	UInt64 UniformRandomUInt64n(UInt64 u)
	{
		if (0 == (u & (u - 1)))
		{
			// PowerOf2 early out case.
			return UniformRandomUInt64() & (u - (UInt64)1);
		}

		// Otherwise, need to try and repeat until we hit
		// an appropriate multiple for the target.
		UInt64 const uMax =
			UInt64Max -
			(((UInt64Max % u) + (UInt64)1) & u);

		UInt64 uReturn = UniformRandomUInt64();
		while (uReturn > uMax)
		{
			uReturn = UniformRandomUInt64();
		}

		return (uReturn % u);
	}

private:
	PseudoRandomSeed m_Seed;

	/**
	 * Must always be called after a new seed is set - should not be called after
	 * advancing the seed (although no correctness harm would be done from
	 * doing so, since a seed of (0, 0) is impossible as long as the seed does
	 * not start at (0, 0)). Calling this function needlessly is a performance penalty only.
	 */
	void Sanitize()
	{
		// See also: http://vigna.di.unimi.it/ftp/papers/xorshiftplus.pdf, section 3.2
		// xorshift128+ is degenerate if the seed value (combined - all 128-bits) is
		// exactly 0. So we set the "low bits" (m_uY) to 1 if that occurs.
		if (0u == m_Seed.m_uX && 0u == m_Seed.m_uY)
		{
			m_Seed.m_uY = 1u;
		}
	}
}; // class PseudoRandom

} // namespace Seoul

#endif // include guard
