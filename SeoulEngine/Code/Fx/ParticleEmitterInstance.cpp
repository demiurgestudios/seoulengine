/**
 * \file ParticleEmitterInstance.cpp
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

#include "ParticleEmitterInstance.h"

namespace Seoul
{

ParticleEmitterInstance::ParticleEmitterInstance(const SharedPtr<ParticleEmitter>& pEmitter)
	: m_mParentTransform(Matrix4D::Identity())
	, m_mParentIfWorldspaceTransform(Matrix4D::Identity())
	, m_mParentInverseTransform(Matrix4D::Identity())
	, m_mParentPreviousTransform(Matrix4D::Identity())
	, m_pEmitter(pEmitter)
	, m_pParticles((Particle*)MemoryManager::AllocateAligned(sizeof(Particle) * m_pEmitter->GetMaxParticleCount(), MemoryBudgets::Particles, SEOUL_ALIGN_OF(Particle)))
	, m_vEmitterPosition(Vector3D::Zero())
	, m_vEmitterVelocity(Vector3D::Zero())
	, m_uFlags(0u)
	, m_fParticleSpawnAccumulator(0.0f)
	, m_fInstanceEmitFactor(0.0f)
	, m_iActiveParticleCount(0)
	, m_fGravityAcceleration(0.f)
	, m_vParticleRallyPointOverride(Vector3D::Zero())
	, m_bPendingApplyRallyPointOverride(false)
{
}

ParticleEmitterInstance::~ParticleEmitterInstance()
{
	MemoryManager::Deallocate(m_pParticles);
}

} // namespace Seoul
