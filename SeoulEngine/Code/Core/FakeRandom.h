/**
 * \file FakeRandom.h
 * \brief Random number generator using a global table to
 * provide consistent psuedo-random results.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FAKE_RANDOM_H
#define FAKE_RANDOM_H

#include "Prereqs.h"

namespace Seoul
{

/**
 * Uniform pseudo-random number generator on [0, 1). Not a great random
 * but "good enough" for (e.g.) randomness in particles and quite fast.
 */
class FakeRandom SEOUL_SEALED
{
public:
	FakeRandom()
		: m_uSeed(0u)
	{
	}

	/** @return The current seed value. */
	UInt32 GetSeed() const
	{
		return m_uSeed;
	}

	/** Compute the next number in the sequence. */
	UInt16 Next()
	{
		// old style fast rand() - linear congruential generator
		m_uSeed = (214013u * m_uSeed + 2531011u);
		// shift and truncate.
		return (UInt16)(m_uSeed >> 16u);
	}

	/** Get a float value on [0, 1). */
	Float32 NextFloat32()
	{
		// Acquire.
		auto const u16 = Next();
		// Convert.
		return (Float32)u16 / 65536.0f; // [0, 1).
	}

	/**
	 * Sets the FakeRandom index to the seed value uSeed.
	 */
	void Reset(UInt32 uSeed = 0u)
	{
		m_uSeed = uSeed;
	}

private:
	SEOUL_DISABLE_COPY(FakeRandom);
	UInt32 m_uSeed;
}; // class FakeRandom

} // namespace Seoul

#endif // include guard
