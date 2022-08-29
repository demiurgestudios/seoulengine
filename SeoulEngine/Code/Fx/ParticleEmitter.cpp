/**
 * \file ParticleEmitter.cpp
 * \brief Concrete implementation of the math necessary to generate
 * particles. Free of rendering details to be usable in contexts
 * without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HashFunctions.h"
#include "ParticleEmitter.h"
#include "ParticleEmitterInstance.h"

namespace Seoul
{

/** Thread safe counter to consistently but globally seed the fake rands of particle emitter instances. */
static Atomic32 s_ParticleEmitterInstanceSeedValue{};

ParticleEmitter::ParticleEmitter()
	: m_vEmitAxis(Vector3D::UnitX())
	, m_nMaxParticleCount(0u)
	, m_eEmitterShape(kShapePoint)
	, m_eCoordinateSpace(kCoordinateSpaceWorld)
	, m_eRotationAlignment(kRotationNoAlignment)
	, m_vInitialParticleCount(Vector2D::Zero())
	, m_BoxInnerDimensions()
	, m_BoxOuterDimensions()
	, m_LineWidth()
	, m_SphereRadius()
	, m_vEmitterVelocityAngleMinMax(Vector2D::Zero())
	, m_vInitialEmitterVelocityMagnitudeMinMax(Vector2D::Zero())
	, m_EmitterAcceleration()
	, m_GravityScalar()
	, m_AngularFriction()
	, m_LinearFriction()
	, m_AngularAcceleration()
	, m_Color()
	, m_EmitAngleRange()
	, m_EmitRate()
	, m_EmitOffset()
	, m_InitialRotation()
	, m_InitialRotationRange()
	, m_InitialAngularVelocity()
	, m_InitialScale()
	, m_InitialVelocity()
	, m_Lifetime()
	, m_LinearAcceleration()
	, m_LocalTranslation()
	, m_Scale()
	, m_TexcoordScaleAndShift()
	, m_vConfiguredRallyPoint(Vector3D::UnitX())
	, m_uFlags(0u)
{
}

/**
 * Effectively, construct instance in-place.
 *
 * It is safe to call this method in a thread other than the main thread.
 */
void ParticleEmitter::InitializeInstance(
	const Matrix4D& mInitialTransform,
	ParticleEmitterInstance& instance) const
{
	// Seed the instance.
	//
	// Hash the atomic increment "seed" value to avoid clumping in the (very predictable but still random)
	// FakeRandom distribution that we use for particles.
	auto const uSeedValue = GetHash((UInt32)((++s_ParticleEmitterInstanceSeedValue) - 1));

	instance.m_fParticleSpawnAccumulator = instance.GetRandom(m_vInitialParticleCount);
	instance.m_fInstanceEmitFactor = 1.0f;
	instance.m_iActiveParticleCount = 0;
	instance.ResetRandom(uSeedValue);
	instance.m_mParentTransform = mInitialTransform;
	instance.m_mParentIfWorldspaceTransform = Matrix4D::Identity();
	instance.m_mParentInverseTransform = mInitialTransform.Inverse();
	instance.m_mParentPreviousTransform = instance.m_mParentTransform;

	// Calculate the initial emitter velocity.
	const Vector3D vEmitterVelocityAxis = Vector3D::UnitZ();
	const Float fEmitterVelocityAngle = instance.GetRandom(m_vEmitterVelocityAngleMinMax);

	Vector3D vEmitterVelocityRight;
	Vector3D vEmitterVelocityUp;
	CalculateParticleUpAndRightAxes(
		vEmitterVelocityAxis,
		vEmitterVelocityUp,
		vEmitterVelocityRight);

	const Matrix4D mEmitterRotation =
		Matrix4D::CreateRotationFromAxisAngle(vEmitterVelocityAxis, instance.GetRandom(kParticleAngleZeroToTwoPi)) *
		Matrix4D::CreateRotationFromAxisAngle(vEmitterVelocityRight, fEmitterVelocityAngle);

	instance.m_vEmitterVelocity =
		Matrix4D::TransformDirection(mEmitterRotation, vEmitterVelocityAxis) *
		instance.GetRandom(m_vInitialEmitterVelocityMagnitudeMinMax);

	instance.m_vEmitterPosition = Vector3D::Zero();
}

} // namespace Seoul
