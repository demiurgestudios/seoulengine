/**
 * \file ScriptPseudoRandom.cpp
 * \brief 
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ScriptPseudoRandom.h"
#include "PseudoRandom.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"
#include "SecureRandom.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Script::PseudoRandom)
	SEOUL_METHOD(Construct)
	SEOUL_METHOD(UniformRandomFloat32)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "double", "")
	SEOUL_METHOD(UniformRandomInt32)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "double", "")
	SEOUL_METHOD(UniformRandomUInt32n)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "double", "double lower, double upper")
SEOUL_END_TYPE()

namespace Script
{

PseudoRandom::PseudoRandom()
	: m_PseudoRandom()
{
}

PseudoRandom::~PseudoRandom()
{
}

/**
 * If no arguments, seeds m_PseudoRandom from a secure random source.
 * Otherwise, expects a string to hash and generate a deterministic seed.
 */
void PseudoRandom::Construct(FunctionInterface* pInterface)
{
	if (pInterface->GetArgumentCount() == 0)
	{
		m_PseudoRandom = Seoul::PseudoRandom::SeededPseudoRandom();
		return;
	}

	String sSeed;
	if (!pInterface->GetString(1u, sSeed))
	{
		pInterface->RaiseError(1, "expected string seed.");
		return;
	}
	if (sSeed.IsEmpty())
	{
		pInterface->RaiseError(1, "expected non-empty string seed.");
		return;
	}

	m_PseudoRandom = Seoul::PseudoRandom::SeededFromString(sSeed);
}

Int32 PseudoRandom::UniformRandomInt32()
{
	return m_PseudoRandom.UniformRandomInt32();
}

Float32 PseudoRandom::UniformRandomFloat32()
{
	return m_PseudoRandom.UniformRandomFloat32();
}

UInt32 PseudoRandom::UniformRandomUInt32n(UInt32 lower, UInt32 upper)
{
	return m_PseudoRandom.UniformRandomUInt32n(lower, upper);
}

} // namespace Script

} // namespace Seoul
