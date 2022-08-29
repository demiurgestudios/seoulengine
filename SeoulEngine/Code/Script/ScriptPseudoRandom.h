/**
 * \file ScriptPseudoRandom.h
 * \brief 
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_PSEUDO_RANDOM_H
#define SCRIPT_PSEUDO_RANDOM_H

#include "PseudoRandom.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SharedPtr.h"

namespace Seoul { namespace Script { class FunctionInterface; } }

namespace Seoul::Script
{

/** Binder, wraps a PseudoRandom instance and exposes functionality to a script VM. */
class PseudoRandom SEOUL_SEALED
{
public:
	PseudoRandom();
	~PseudoRandom();
	// Seeds the PseudoRandom from a secure random source, or the hash of a passed-in string.
	void Construct(FunctionInterface* pInterface);

	Float32 UniformRandomFloat32();
	Int32 UniformRandomInt32();
	UInt32 UniformRandomUInt32n(UInt32 lower, UInt32 upper);

private:
	SEOUL_REFLECTION_FRIENDSHIP(PseudoRandom);

	Seoul::PseudoRandom m_PseudoRandom;
}; // class Script::PseudoRandom

} // namespace Seoul::Script

#endif // include guard
