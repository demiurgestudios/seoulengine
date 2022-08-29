/**
 * \file ParticleEmitter.h
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

#pragma once
#ifndef PARTICLE_EMITTER_H
#define PARTICLE_EMITTER_H

#include "Color.h"
#include "Matrix4D.h"
#include "SharedPtr.h"
#include "Prereqs.h"
#include "SimpleCurve.h"
#include "Vector2D.h"
#include "Vector3D.h"

namespace Seoul
{

// Forward declarations
class ParticleEmitterInstance;

/**
 * Constant used for calculating rotation axes.
 */
static const Float kParticleDotOneThreshold = 1e-2f;

/**
 * Constant used for generating a random angle between 0 and 360 degrees.
 */
static const Vector2D kParticleAngleZeroToTwoPi = Vector2D(0.0f, fTwoPi);

/**
 * Helper method, calculates right and up axes for rotation
 * calculations from a forward axis.
 */
inline void CalculateParticleUpAndRightAxes(
	const Vector3D& vForwardAxis,
	Vector3D& rvUpAxis,
	Vector3D& rvRightAxis)
{
	const Vector3D vRight = Vector3D::UnitCross(
		((Equals(Abs(Vector3D::Dot(vForwardAxis, Vector3D::UnitZ())), 1.0f, kParticleDotOneThreshold))
			? Vector3D::UnitY()
			: Vector3D::UnitZ()),
		vForwardAxis);

	const Vector3D vUp = Vector3D::UnitCross(
		vRight,
		vForwardAxis);

	rvUpAxis = vUp;
	rvRightAxis = vRight;
}

/**
 * Structure of data that describes CPU only parameters of a particle.
 * This data is used when ticking and updating the state of particle,
 * but not used for rendering.
 */
struct Particle SEOUL_SEALED
{
	Particle()
		: m_vPosition()
		, m_fRotation(0.0f)
		, m_vLinearVelocity()
		, m_fAge(0.0f)
		, m_fAngularVelocity(0.0f)
		, m_fInitialScale(0.0f)
		, m_fLifespan(0.0f)
	{
	}

	/**
	 * Local or world space position of the particle (depending on mode).
	 */
	Vector3D m_vPosition;

	/**
	 * Rotation of the particle, in world or local space (depending on mode).
	 */
	Float m_fRotation;

	/**
	 * Linear velocity of the particle in centimeters per second.
	 */
	Vector3D m_vLinearVelocity;

	/** Age of the particle in seconds. */
	Float m_fAge;

	/** Angular velocity around view space -Z, in centimeters per second. */
	Float m_fAngularVelocity;

	/** Initial scaling factor, used to give variation to equally scaled particles. */
	Float m_fInitialScale;

	/**
	 * Lifespan of the particle in seconds. When the age reaches this
	 * value, the particle will die.
	 */
	Float m_fLifespan;

	/**
	 * For world space particles, world space reflection. Defined
	 * as the reference point to reflect around (the original emit position)
	 * in XYZ and then the reflection factor in W, which is currently
	 * always 0 (no reflection) or -1 (reflection along the X axis).
	 */
	Vector4D m_vReflection;
};
template <> struct CanMemCpy<Particle> { static const Bool Value = true; };
template <> struct CanZeroInit<Particle> { static const Bool Value = true; };

/**
 * Heavy class of particles. Contains all the shared data
 * that defines a particular particle emitter.
 */
class ParticleEmitter SEOUL_SEALED
{
public:
	enum CoordinateSpace
	{
		kCoordinateSpaceWorld,
		kCoordinateSpaceLocal,
		kCoordinateSpaceLocalTranslationWorldRotation,
		kCoordinateSpaceWorldTranslationLocalRotation,
	};

	enum EmitterShape
	{
		kShapePoint,
		kShapeLine,
		kShapeBox,
		kShapeSphere,
	};

	enum Flags
	{
		kFlagNone,
		kFlagAlignOffsetToEmitVelocity = (1 << 0u),
		kFlagEmitAlongOwnerVelocity = (1 << 1u),
		kFlagParticlesInheritEmitterVelocity = (1 << 3u),
		kFlagRandomInitialParticleRotation = (1 << 4u),
		kFlagParentSpaceEmitDirection = (1 << 5u),
		kFlagParentSpaceEmitOffset = (1 << 6u),
		kFlagSnapParticlesToEmitterY = (1 << 8u),
		kFlagSnapParticlesToEmitterZ = (1 << 9u),
		kFlagAlignParticlesToVelocity = (1 << 10u),
		kFlagUseRallyPoint = (1 << 11u),
		kFlagParticleScaleAndRotationTransformIndependant = (1 << 12u),
		kFlagAlphaClamp = (1 << 13u),
	};

	enum RotationAlignmentMode
	{
		kRotationNoAlignment,
		kRotationAlignToEmitAngle,
	};

	ParticleEmitter();

	void InitializeInstance(
		const Matrix4D& mInitialTransform,
		ParticleEmitterInstance& instance) const;

	Bool AlignOffsetToEmitVelocity() const { return (m_uFlags & kFlagAlignOffsetToEmitVelocity) != 0u; }
	Bool EmitAlongOwnerVelocity() const { return (m_uFlags & kFlagEmitAlongOwnerVelocity) != 0u; }
	Bool ParentSpaceEmitDirection() const { return (m_uFlags & kFlagParentSpaceEmitDirection) != 0u; }
	Bool ParentSpaceEmitOffset() const { return (m_uFlags & kFlagParentSpaceEmitOffset) != 0u; }
	Bool ParticlesInheritEmitterVelocity() const { return (m_uFlags & kFlagParticlesInheritEmitterVelocity) != 0u; }
	Bool RandomInitialParticleRotation() const { return (m_uFlags & kFlagRandomInitialParticleRotation) != 0u; }
	Bool SnapParticlesToEmitterY() const { return (m_uFlags & kFlagSnapParticlesToEmitterY) != 0u; }
	Bool SnapParticlesToEmitterZ() const { return (m_uFlags & kFlagSnapParticlesToEmitterZ) != 0u; }
	Bool AlignParticlesToVelocity() const { return (m_uFlags & kFlagAlignParticlesToVelocity) != 0u; }
	Bool UseRallyPoint() const { return (m_uFlags & kFlagUseRallyPoint) != 0u; }
	Bool ParticleScaleAndRotationTransformIndependant() const { return (m_uFlags & kFlagParticleScaleAndRotationTransformIndependant) != 0; }
	Bool AlphaClamp() const { return (m_uFlags & kFlagAlphaClamp) != 0u; }

	CoordinateSpace GetCoordinateSpace() const { return m_eCoordinateSpace; }

	Int32 GetMaxParticleCount() const { return m_nMaxParticleCount; }

	Vector3D m_vEmitAxis;

	Int32 m_nMaxParticleCount;

	EmitterShape m_eEmitterShape;
	CoordinateSpace m_eCoordinateSpace;
	RotationAlignmentMode m_eRotationAlignment;
	Vector2D m_vInitialParticleCount;

	SimpleCurve<Vector3D> m_BoxInnerDimensions;
	SimpleCurve<Vector3D> m_BoxOuterDimensions;
	SimpleCurve<Float> m_LineWidth;
	SimpleCurve<Vector2D> m_SphereRadius;

	Vector2D m_vEmitterVelocityAngleMinMax;
	Vector2D m_vInitialEmitterVelocityMagnitudeMinMax;

	SimpleCurve<Vector4D> m_EmitterAcceleration;
	SimpleCurve<Float> m_GravityScalar;

	SimpleCurve<Float> m_AngularFriction;
	SimpleCurve<Float> m_LinearFriction;

	SimpleCurve<Vector2D> m_AngularAcceleration;
	SimpleCurve<ColorARGBu8> m_Color;
	SimpleCurve<Vector2D> m_EmitAngleRange;
	SimpleCurve<Float> m_EmitRate;
	SimpleCurve<Vector3D> m_EmitOffset;
	SimpleCurve<Float> m_InitialRotation;
	SimpleCurve<Vector2D> m_InitialRotationRange;
	SimpleCurve<Vector2D> m_InitialAngularVelocity;
	SimpleCurve<Vector2D> m_InitialScale;
	SimpleCurve<Vector2D> m_InitialVelocity;
	SimpleCurve<Vector2D> m_Lifetime;
	SimpleCurve<Vector4D> m_LinearAcceleration;
	SimpleCurve<Vector2D> m_LocalTranslation;
	SimpleCurve<Vector2D> m_Scale;
	SimpleCurve<Vector4D> m_TexcoordScaleAndShift;
	SimpleCurve<Vector2D> m_AlphaClamp;

	Vector3D m_vConfiguredRallyPoint;

	UInt32 m_uFlags;

private:
	SEOUL_REFERENCE_COUNTED(ParticleEmitter);
	SEOUL_DISABLE_COPY(ParticleEmitter);
}; // class ParticleEmitter

} // namespace Seoul

#endif // include guard
