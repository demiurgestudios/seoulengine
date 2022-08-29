/**
 * \file PseudoRandom.cpp
 *
 * \brief 64-bit seedable PRNG with 128-bit keys. "xorshift128+" algorithm.
* NOT CRYPTOGRAPHICALLY SECURE.
* \sa
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "PseudoRandom.h"
#include "MemoryManager.h"
#include "Platform.h"
#include "SeoulMD5.h"

// Uses internal API of SecureRandomInternal to avoid static initialization bugs
#define SECURE_RANDOM_H
#include "SecureRandomInternal.h"

namespace Seoul
{

/**
* Generate a pair of UInt64 from the system's secure
* random number generator. We do this with an explicit
* context to avoid the SIOF
* (https://isocpp.org/wiki/faq/ctors#static-init-order)
*/
PseudoRandom PseudoRandom::SeededPseudoRandom()
{
	PseudoRandomSeed seed;

	SecureRandomDetail::SecureRandomContext context;
	context.GetBytes((UInt8*)&seed.m_uX, sizeof(seed.m_uX));
	context.GetBytes((UInt8*)&seed.m_uY, sizeof(seed.m_uY));

	return PseudoRandom(seed);
}

PseudoRandom PseudoRandom::SeededFromString(String const& toHash)
{
	PseudoRandomSeed seed;
	{
		PseudoRandomSeedBuilder builder(seed);
		builder.AppendData(toHash);
	}

	return PseudoRandom(seed);
}


PseudoRandomSeedBuilder::PseudoRandomSeedBuilder(PseudoRandomSeed &rOutput)
	: m_rOutput(rOutput)
	, m_aMD5Out()
	, m_pMD5(SEOUL_NEW(MemoryBudgets::Game) MD5(m_aMD5Out))
{
}

PseudoRandomSeedBuilder::~PseudoRandomSeedBuilder()
{
	SafeDelete(m_pMD5); // Deleting the MD5 finalizes the digest to m_aMD5Out

	m_rOutput.m_uX = (
		  static_cast<UInt64>(m_aMD5Out[0]) << 56
		| static_cast<UInt64>(m_aMD5Out[1]) << 48
		| static_cast<UInt64>(m_aMD5Out[2]) << 40
		| static_cast<UInt64>(m_aMD5Out[3]) << 32
		| static_cast<UInt64>(m_aMD5Out[4]) << 24
		| static_cast<UInt64>(m_aMD5Out[5]) << 16
		| static_cast<UInt64>(m_aMD5Out[6]) << 8
		| static_cast<UInt64>(m_aMD5Out[7]));
	m_rOutput.m_uY = (
		  static_cast<UInt64>(m_aMD5Out[8])  << 56
		| static_cast<UInt64>(m_aMD5Out[9])  << 48
		| static_cast<UInt64>(m_aMD5Out[10]) << 40
		| static_cast<UInt64>(m_aMD5Out[11]) << 32
		| static_cast<UInt64>(m_aMD5Out[12]) << 24
		| static_cast<UInt64>(m_aMD5Out[13]) << 16
		| static_cast<UInt64>(m_aMD5Out[14]) << 8
		| static_cast<UInt64>(m_aMD5Out[15]));
}

void PseudoRandomSeedBuilder::AppendData(UInt8 const* pData, UInt32 zDataSizeInBytes)
{
	m_pMD5->AppendData(pData, zDataSizeInBytes);
}

} // namespace Seoul
