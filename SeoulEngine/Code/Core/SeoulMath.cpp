/**
 * \file SeoulMath.cpp
 * \brief SeoulEngine rough equivalent to the standard <cmath> or <math.h>
 * header. Includes constants and basic utilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HashFunctions.h"
#include "Mutex.h"
#include "PseudoRandom.h"
#include "SeoulMath.h"
#include "SeoulTime.h"

// For good entropy initialization.
#define SECURE_RANDOM_H
#include "SecureRandomInternal.h"

namespace Seoul
{

#if !SEOUL_DEBUG
/**
 * Generate a pair of UInt64 for seeding our global
 * psuedo random from the system's secure
 * random number generator. We do
 * this with an explicit context
 * to avoid the SIOF
 * (https://isocpp.org/wiki/faq/ctors#static-init-order)
 */
static inline void GetSecureRandomUInt64(UInt64& ruA, UInt64& ruB)
{
	SecureRandomDetail::SecureRandomContext context;
	context.GetBytes((UInt8*)&ruA, sizeof(ruA));
	context.GetBytes((UInt8*)&ruB, sizeof(ruB));
}
#endif // /#if !SEOUL_DEBUG

static inline PseudoRandomSeed Seed()
{
	PseudoRandomSeed ret;

#	if SEOUL_DEBUG
	// In debug builds, leave ret alone and seed the
	// system's random with 0.
	srand(0);
#	else
	// If not a debug build, use a pair of UInt64 generated
	// by SecureRandom to initialize our seeds. We use
	// the private function above to avoid the SIOF:
	// https://isocpp.org/wiki/faq/ctors#static-init-order
	GetSecureRandomUInt64(ret.m_uX, ret.m_uY);

	// Also seed the system's PRNG.
	UInt32 const uSystemSeed = (UInt32)(ret.m_uX & ((UInt64)UIntMax));
	srand(uSystemSeed);
#endif // #endif

	return ret;
}


namespace GlobalRandom
{

// TODO: Eliminate thread contention.

// Global PseudoRandom is protected with a mutex.
static Mutex s_Mutex;
static PseudoRandom s_Random(Seed());

/** Get normal (Gaussian) random sample with mean 0 and standard deviation 1. */
Float64 NormalRandomFloat64()
{
	Lock lock(s_Mutex);
	return s_Random.NormalRandomFloat64();
}

/** Get normal (Gaussian) random sample with specified mean and standard deviation. */
Float64 NormalRandomFloat64(Float64 fMean, Float64 fStandardDeviation)
{
	Lock lock(s_Mutex);
	return s_Random.NormalRandomFloat64(fMean, fStandardDeviation);
}

/**
 * @return The current random seed of the global random.
 */
void GetSeed(UInt64& ruX, UInt64& ruY)
{
	Lock lock(s_Mutex);
	auto const seed = s_Random.GetSeed();
	ruX = seed.m_uX;
	ruY = seed.m_uY;
}

/**
 * Update the seed of the global random number generator.
 */
void SetSeed(UInt64 uX, UInt64 uY)
{
	Lock lock(s_Mutex);
	return s_Random.SetSeed(PseudoRandomSeed(uX, uY));
}

/**
 * Produce a uniform random sample on [0, 1)
 * (includes 0 but excludes 1, the most common convention).
 */
Float32 UniformRandomFloat32()
{
	Lock lock(s_Mutex);
	return s_Random.UniformRandomFloat32();
}

/**
 * Produce a uniform random sample on [0, 1)
 * (includes 0 but excludes 1, the most common convention).
 */
Float64 UniformRandomFloat64()
{
	Lock lock(s_Mutex);
	return s_Random.UniformRandomFloat64();
}

/**
 * Generate an Int32 psuedorandom number.
 */
Int32 UniformRandomInt32()
{
	Lock lock(s_Mutex);
	return s_Random.UniformRandomInt32();
}

/**
 * Generate a UInt32 psuedorandom number.
 */
UInt32 UniformRandomUInt32()
{
	Lock lock(s_Mutex);
	return s_Random.UniformRandomUInt32();
}

/**
 * Generates a non-negative Int64 psuedorandom number.
 */
Int64 UniformRandomInt63()
{
	Lock lock(s_Mutex);
	return s_Random.UniformRandomInt63();
}

/**
 * Generate an Int64 psuedorandom number.
 */
Int64 UniformRandomInt64()
{
	Lock lock(s_Mutex);
	return s_Random.UniformRandomInt64();
}

/**
 * Generate a UInt64 psuedo-random number using the xorshift128+ algorithm.
 *
 * \sa http://vigna.di.unimi.it/ftp/papers/xorshiftplus.pdf
 */
UInt64 UniformRandomUInt64()
{
	Lock lock(s_Mutex);
	return s_Random.UniformRandomUInt64();
}

/**
 * Generate a UInt32 psuedo-random number on [0, n).
 */
UInt32 UniformRandomUInt32n(UInt32 u)
{
	Lock lock(s_Mutex);
	return s_Random.UniformRandomUInt32n(u);
}

/**
 * Generate a UInt64 psuedo-random number on [0, n).
 */
UInt64 UniformRandomUInt64n(UInt64 u)
{
	Lock lock(s_Mutex);
	return s_Random.UniformRandomUInt64n(u);
}

} // namespace GlobalRandom

} // namespace Seoul
