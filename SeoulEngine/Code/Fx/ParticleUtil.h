/**
 * \file ParticleUtil.h
 * \brief Functions for ticking and generating render data for a ParticleEmitterInstance and its
 * associated ParticleEmitter data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PARTICLE_UTIL_H
#define PARTICLE_UTIL_H

#include "Prereqs.h"

namespace Seoul
{

// Forward declarations
struct FxParticle;
class ParticleEmitter;
class ParticleEmitterInstance;
template <typename T, int MEMORY_BUDGETS> class Vector;
typedef Vector<FxParticle, MemoryBudgets::Fx> FxParticleRenderBuffer;

// Call this function on instance in order to add its renderable particles
// to rBuffer.
void RenderParticles(
	ParticleEmitterInstance& instance,
	FxParticleRenderBuffer& rBuffer);

// Call this function on instance in order to update the state of its particle data
// from its current state to its next state, based on fDeltaTime and fTimePercent.
void TickParticles(
	Float fDeltaTimeInSeconds,
	Float fTimePercent,
	ParticleEmitterInstance& instance);

} // namespace Seoul

#endif // include guard
