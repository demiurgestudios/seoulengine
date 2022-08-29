/**
 * \file ParticleEmitterInstance.h
 * \brief ParticleEmitterInstance which holds the data
 * that is unique to a particular in-world instance of a particle emitter.
 * Shared data is stored in ParticleEmitter and ParticleMaterial.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PARTICLE_EMITTER_INSTANCE_H
#define PARTICLE_EMITTER_INSTANCE_H

#include "CheckedPtr.h"
#include "FakeRandom.h"
#include "ParticleEmitter.h"

namespace Seoul
{

/**
 * Instance of a ParticleEmitter. Used to store per-instance data.
 * Instances depend on and must be ticked by a corresponding ParticleEmitter,
 * which stores shared data across all Instances.
 */
class ParticleEmitterInstance SEOUL_SEALED
{
public:
	ParticleEmitterInstance(const SharedPtr<ParticleEmitter>& pEmitter);
	~ParticleEmitterInstance();

	/**
	 * @return The shared emitter data associated with this ParticleEmitterInstance.
	 */
	const SharedPtr<ParticleEmitter>& GetEmitter() const
	{
		return m_pEmitter;
	}

	/**
	 * Returns a uniform random value on [vMinMax.x, vMinMax.y].
	 */
	inline Float GetRandom(Vector2D vMinMax)
	{
		return (vMinMax.X >= vMinMax.Y)
			? vMinMax.X
			: Lerp(vMinMax.X, vMinMax.Y, m_Random.NextFloat32());
	}

	/**
	 * Resets the random number generator of this instance to
	 * the uSeed seed value.
	 */
	void ResetRandom(UInt32 uSeed = 0u)
	{
		m_Random.Reset(uSeed);
	}

	// Parent parameters. These are used to attach
	// the emitter to a parent, typically a Seoul engine object.
	Matrix4D m_mParentTransform;
	Matrix4D m_mParentIfWorldspaceTransform;
	Matrix4D m_mParentIfWorldspaceInverseTransform;
	Matrix4D m_mParentInverseTransform;
	Matrix4D m_mParentPreviousTransform;

	FakeRandom m_Random;

	SharedPtr<ParticleEmitter> m_pEmitter;
	CheckedPtr<Particle> m_pParticles;

	// Per emitter parameters. These are in addition to
	// and local to the parent parameters.
	Vector3D m_vEmitterPosition;
	Vector3D m_vEmitterVelocity;

	// Flags that affect per-instance drawing and behavior.
	UInt32 m_uFlags;

	enum
	{
		/** Mirrors the emitter around its parent origin along world X. */
		kMirrorX = (1u << 0u),

		/** Mirrors the emitter around its parent origin along world Y. */
		kMirrorY = (1u << 1u),

		/** Mirrors the emitter around its parent origin along world Z. */
		kMirrorZ = (1u << 2u),

		/** Overrides all emitter settings and forces clamping of particles to the transform Z value. */
		kForceSnapZ = (1u << 3u),
	};

	Bool ForceSnapZ() const { return (0u != (kForceSnapZ & m_uFlags)); }
	Bool MirrorX() const { return (0u != (kMirrorX & m_uFlags)); }
	Bool MirrorY() const { return (0u != (kMirrorY & m_uFlags)); }
	Bool MirrorZ() const { return (0u != (kMirrorZ & m_uFlags)); }

	// Emitting parameters. These control the per-instance
	// emitting state of the particle system.
	Float m_fParticleSpawnAccumulator;
	Float m_fInstanceEmitFactor;

	// The number of currently active particles.
	Int32 m_iActiveParticleCount;

	Float m_fGravityAcceleration;
	Vector3D m_vParticleRallyPointOverride;
	Bool m_bPendingApplyRallyPointOverride;

private:
	SEOUL_REFERENCE_COUNTED(ParticleEmitterInstance);

	SEOUL_DISABLE_COPY(ParticleEmitterInstance);
}; // class ParticleEmitterInstance

} // namespace Seoul

#endif // include guard
