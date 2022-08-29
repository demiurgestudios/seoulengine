/**
 * \file ParticleUtil.cpp
 * \brief Functions for ticking and generating render data for a ParticleEmitterInstance and its
 * associated ParticleEmitter data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Fx.h"
#include "ParticleEmitterInstance.h"
#include "ParticleUtil.h"
#include "Plane.h"
#include "Vector.h"

namespace Seoul
{

/**
 * Particles that have an alpha below this threshold
 * are considered invisible and will not be drawn.
 */
static const UInt8 kInvisibleParticleThreshold = 3u;

/**
 * @return True if a particle, based on its color, is renderable.
 */
inline Bool IsParticleRenderable(RGBA c)
{
	return (c.m_A > kInvisibleParticleThreshold);
}

/**
 * @return True if a particle, based on its render data, is renderable,
 * false otherwise.
 */
inline Bool IsParticleRenderable(const FxParticle& particle)
{
	return IsParticleRenderable(particle.m_Color);
}

/**
 * @return The full local-space to world space, for the particle
 * emitter instance, based on its coordinate space settings
 * defined by emitter.
 */
inline static Matrix4D GetParticleRenderTransform(
	const ParticleEmitter& emitter,
	const ParticleEmitterInstance& instance)
{
	switch (emitter.GetCoordinateSpace())
	{
	case ParticleEmitter::kCoordinateSpaceLocal:
		return instance.m_mParentTransform;
	case ParticleEmitter::kCoordinateSpaceLocalTranslationWorldRotation:
		{
			Matrix4D mTransform = instance.m_mParentTransform;
			Matrix3D upper3x3;
			instance.m_mParentIfWorldspaceTransform.GetRotation(upper3x3);
			mTransform.SetRotation(upper3x3);
			return mTransform;
		}
	case ParticleEmitter::kCoordinateSpaceWorldTranslationLocalRotation:
		{
			Matrix4D mTransform = instance.m_mParentTransform;
			mTransform.SetTranslation(instance.m_mParentIfWorldspaceTransform.GetTranslation());
			return mTransform;
		}
	case ParticleEmitter::kCoordinateSpaceWorld:
		return instance.m_mParentIfWorldspaceTransform;
	default:
		SEOUL_FAIL("Enum is out-of-sync.");
		return Matrix4D::Identity();
	};
}

/**
 * Compute the rotation about the -Y axis based on the velocity
 *            of a given particle
 */
inline Float ComputeRotationBasedOnVelocity(Vector3D vVelocity)
{
	Float fAngle = 0.f;

	vVelocity.Y = 0.f;

	if(vVelocity.Normalize() && !vVelocity.Equals(Vector3D::UnitX()))
	{
		fAngle = Acos(Clamp(Vector3D::Dot(vVelocity, Vector3D::UnitX()),-1.f,1.f));

		if(Vector3D::Cross(vVelocity, Vector3D::UnitX()).Y < 0.f)
		{
			fAngle = -fAngle;
		}
	}

	return fAngle;
}

inline void InternalStaticTickActiveParticles(
	Float fDeltaTimeInSeconds,
	const ParticleEmitter& emitter,
	ParticleEmitterInstance& instance)
{
	for (Int32 i = 0; i < instance.m_iActiveParticleCount; ++i)
	{
		Particle& particle = instance.m_pParticles[i];

		particle.m_fAge += fDeltaTimeInSeconds;

		// If we've reached the lifespan of the particle, deactivate it.
		if (!(particle.m_fAge <= particle.m_fLifespan))
		{
			// Swap the particle with the last particle to remove it from further consideration.
			Swap(instance.m_pParticles[i], instance.m_pParticles[instance.m_iActiveParticleCount - 1]);

			// Reduce the tickable count by one and swap the particle being
			// deactivated with the last active particle. We don't need
			// to swap the tick data, since it's referenced by the
			// m_iParticleDataIndex member of the render data and is never
			// rearranged.
			instance.m_iActiveParticleCount--;

			// Reconsider the particle at the current index.
			i--;

			continue;
		}
		else
		{
			// Particle age percent is a value on [0.0, 1.0] used to evaluate
			// curves based on the age of the particle.
			const Float fParticleAgePercent =
				(particle.m_fAge / particle.m_fLifespan);

			// Acceleration of the particle. XYZ contain acceleration in
			// 3 dimensions while W contains
			// "acceleration along emitter velocity", which is a magnitude that is applied
			// to the normalized direction of the emitter's pre-acceleration velocity and
			// added to the base XYZ acceleration.
			const Vector4D vAccelerationTerms =
				emitter.m_LinearAcceleration.Evaluate(fParticleAgePercent);

			// Get world gravity and then scale it so that the fx studio coordinate system
			// matches the flash coordinate system.
			Float fGravityAcceleration = -1.f * instance.m_fGravityAcceleration;
			const Float fGravity = emitter.m_GravityScalar.Evaluate(fParticleAgePercent) * fGravityAcceleration;

			// TODO: Don't hard code the gravity direction vector.
			// Calculate the total 3 term acceleration.
			const Vector3D vAcceleration =
				vAccelerationTerms.GetXYZ() +
				Vector3D::Normalize(particle.m_vLinearVelocity) * vAccelerationTerms.W +
				fGravity * Vector3D(0,1,0);

			// Update all the data of the particle. If the emitter is set to
			// snap along a particular axis, do not modify the particle's position
			// along that axis.
			particle.m_vLinearVelocity += (vAcceleration * fDeltaTimeInSeconds);
			particle.m_fAngularVelocity += (instance.GetRandom(
				emitter.m_AngularAcceleration.Evaluate(fParticleAgePercent)) * fDeltaTimeInSeconds);
			particle.m_vLinearVelocity *= (1.0f - emitter.m_LinearFriction.Evaluate(fParticleAgePercent));
			particle.m_fAngularVelocity *= (1.0f - emitter.m_AngularFriction.Evaluate(fParticleAgePercent));

			particle.m_vPosition.X = particle.m_vPosition.X + (particle.m_vLinearVelocity.X * fDeltaTimeInSeconds);
			particle.m_vPosition.Y = (emitter.SnapParticlesToEmitterY())
				? particle.m_vPosition.Y
				: particle.m_vPosition.Y + (particle.m_vLinearVelocity.Y * fDeltaTimeInSeconds);
			particle.m_vPosition.Z = (emitter.SnapParticlesToEmitterZ() || instance.ForceSnapZ())
				? particle.m_vPosition.Z
				: particle.m_vPosition.Z + (particle.m_vLinearVelocity.Z * fDeltaTimeInSeconds);
			particle.m_fRotation = (emitter.AlignParticlesToVelocity())
				? ComputeRotationBasedOnVelocity(particle.m_vLinearVelocity)
				: (particle.m_fRotation + particle.m_fAngularVelocity * fDeltaTimeInSeconds);
		}
	}
}

struct EmitterShapeParameter
{
	Vector3D m_V0;
	Vector3D m_V1;
};

template <ParticleEmitter::EmitterShape EMITTER_SHAPE>
inline EmitterShapeParameter InternalGetEmitterShapeParameter(
	Float fTimePercent,
	const ParticleEmitter& emitter);

template <>
inline EmitterShapeParameter InternalGetEmitterShapeParameter<ParticleEmitter::kShapePoint>(
	Float,
	const ParticleEmitter&)
{
	return EmitterShapeParameter();
}

template <>
inline EmitterShapeParameter InternalGetEmitterShapeParameter<ParticleEmitter::kShapeLine>(
	Float fTimePercent,
	const ParticleEmitter& emitter)
{
	const Float fMinMax = emitter.m_LineWidth.Evaluate(fTimePercent) * 0.5f;
	EmitterShapeParameter param;
	param.m_V0 = Vector3D(-fMinMax, fMinMax, 0.0f);

	return param;
}

template <>
inline EmitterShapeParameter InternalGetEmitterShapeParameter<ParticleEmitter::kShapeBox>(
	Float fTimePercent,
	const ParticleEmitter& emitter)
{
	EmitterShapeParameter param;
	param.m_V0 = emitter.m_BoxInnerDimensions.Evaluate(fTimePercent) * 0.5f;
	param.m_V1 = emitter.m_BoxOuterDimensions.Evaluate(fTimePercent) * 0.5f;
	return param;
}

template <>
inline EmitterShapeParameter InternalGetEmitterShapeParameter<ParticleEmitter::kShapeSphere>(
	Float fTimePercent,
	const ParticleEmitter& emitter)
{
	EmitterShapeParameter param;
	param.m_V0 = Vector3D(emitter.m_SphereRadius.Evaluate(fTimePercent), 0.0f);
	return param;
}

inline EmitterShapeParameter InternalStaticGetEmitterShapeParameter(
	Float fTimePercent,
	const ParticleEmitter& emitter)
{
	switch (emitter.m_eEmitterShape)
	{
	case ParticleEmitter::kShapePoint:
		return InternalGetEmitterShapeParameter<ParticleEmitter::kShapePoint>(fTimePercent, emitter);
	case ParticleEmitter::kShapeLine:
		return InternalGetEmitterShapeParameter<ParticleEmitter::kShapeLine>(fTimePercent, emitter);
	case ParticleEmitter::kShapeBox:
		return InternalGetEmitterShapeParameter<ParticleEmitter::kShapeBox>(fTimePercent, emitter);
	case ParticleEmitter::kShapeSphere:
		return InternalGetEmitterShapeParameter<ParticleEmitter::kShapeSphere>(fTimePercent, emitter);
	default:
		SEOUL_FAIL("Out of sync enum.");
		return EmitterShapeParameter();
	};
}

template <ParticleEmitter::EmitterShape EMITTER_SHAPE>
inline Vector3D InternalAdjustInitialPositionForEmitterShape(
	const EmitterShapeParameter& parameter,
	ParticleEmitterInstance& instance,
	Vector3D v);

template <>
inline Vector3D InternalAdjustInitialPositionForEmitterShape<ParticleEmitter::kShapePoint>(
	const EmitterShapeParameter&,
	ParticleEmitterInstance&,
	Vector3D v)
{
	return v;
}

template <>
inline Vector3D InternalAdjustInitialPositionForEmitterShape<ParticleEmitter::kShapeLine>(
	const EmitterShapeParameter& parameter,
	ParticleEmitterInstance& instance,
	Vector3D v)
{
	return Vector3D(
		v.X + instance.GetRandom(parameter.m_V0.GetXY()),
		v.Y,
		v.Z);
}

static const Vector2D kZeroOne(0.0f, 1.0f);
static const Vector2D kNegativeOneOne(-1.0f, 1.0f);

template <>
inline Vector3D InternalAdjustInitialPositionForEmitterShape<ParticleEmitter::kShapeBox>(
	const EmitterShapeParameter& parameter,
	ParticleEmitterInstance& instance,
	Vector3D v)
{
	auto const fX = instance.GetRandom(kNegativeOneOne);
	auto const fY = instance.GetRandom(kNegativeOneOne);
	auto const fZ = instance.GetRandom(kNegativeOneOne);
	const Vector3D vDirection = Vector3D::Normalize(Vector3D(fX, fY, fZ));

	// Effective radius calculation - inlined here for speed.
	const Vector3D vAbsDirection = vDirection.Abs();
	const Float fOuterEffectiveRadius = Vector3D::Dot(vAbsDirection, 2.0f * parameter.m_V1);

	const Vector3D vProjectedPoint = 0.5f * fOuterEffectiveRadius * vDirection;
	const Vector3D vInnerPoint = Vector3D::Clamp(vProjectedPoint, -parameter.m_V0, parameter.m_V0);
	const Vector3D vOuterPoint = Vector3D::Clamp(vProjectedPoint, -parameter.m_V1, parameter.m_V1);

	return v + Vector3D::Lerp(vInnerPoint, vOuterPoint, instance.GetRandom(kZeroOne));
}

template <>
inline Vector3D InternalAdjustInitialPositionForEmitterShape<ParticleEmitter::kShapeSphere>(
	const EmitterShapeParameter& parameter,
	ParticleEmitterInstance& instance,
	Vector3D v)
{
	auto const fX = instance.GetRandom(kNegativeOneOne);
	auto const fY = instance.GetRandom(kNegativeOneOne);
	auto const fZ = instance.GetRandom(kNegativeOneOne);
	const Vector3D vDirection = Vector3D::Normalize(Vector3D(fX, fY, fZ));

	return v + vDirection * instance.GetRandom(parameter.m_V0.GetXY());
}

inline Vector3D InternalStaticAdjustInitialPositionForEmitterShape(
	const ParticleEmitter& emitter,
	const EmitterShapeParameter& parameter,
	ParticleEmitterInstance& instance,
	Vector3D v)
{
	switch (emitter.m_eEmitterShape)
	{
	case ParticleEmitter::kShapePoint:
		return InternalAdjustInitialPositionForEmitterShape<ParticleEmitter::kShapePoint>(parameter, instance, v);
	case ParticleEmitter::kShapeLine:
		return InternalAdjustInitialPositionForEmitterShape<ParticleEmitter::kShapeLine>(parameter, instance, v);
	case ParticleEmitter::kShapeBox:
		return InternalAdjustInitialPositionForEmitterShape<ParticleEmitter::kShapeBox>(parameter, instance, v);
	case ParticleEmitter::kShapeSphere:
		return InternalAdjustInitialPositionForEmitterShape<ParticleEmitter::kShapeSphere>(parameter, instance, v);
	default:
		SEOUL_FAIL("Enum out of sync.");
		return Vector3D::Zero();
	};
}

inline Vector3D GetInitialPosition(
	const ParticleEmitter& emitter,
	const ParticleEmitterInstance& instance)
{
	switch (emitter.m_eCoordinateSpace)
	{
	case ParticleEmitter::kCoordinateSpaceWorld: // fall-through
	case ParticleEmitter::kCoordinateSpaceWorldTranslationLocalRotation:
		return Matrix4D::TransformPosition(instance.m_mParentIfWorldspaceInverseTransform, instance.m_mParentTransform.GetTranslation());
	case ParticleEmitter::kCoordinateSpaceLocal: // fall-through
	case ParticleEmitter::kCoordinateSpaceLocalTranslationWorldRotation:
		return Vector3D::Zero();
	default:
		SEOUL_FAIL("Enum out of sync.");
		return Vector3D::Zero();
	};
}

inline Vector3D GetInitialPrevPosition(
	const ParticleEmitter& emitter,
	const ParticleEmitterInstance& instance)
{
	switch (emitter.m_eCoordinateSpace)
	{
	case ParticleEmitter::kCoordinateSpaceWorld: // fall-through
	case ParticleEmitter::kCoordinateSpaceWorldTranslationLocalRotation:
		return Matrix4D::TransformPosition(instance.m_mParentIfWorldspaceInverseTransform, instance.m_mParentPreviousTransform.GetTranslation());
	case ParticleEmitter::kCoordinateSpaceLocal: // fall-through
	case ParticleEmitter::kCoordinateSpaceLocalTranslationWorldRotation:
		return Vector3D::Zero();
	default:
		SEOUL_FAIL("Enum out of sync.");
		return Vector3D::Zero();
	};
}

// TODO: World space particles do not inherit
// the full transform.

inline Float GetReflection(
	const ParticleEmitter& emitter,
	const ParticleEmitterInstance& instance)
{
	switch (emitter.m_eCoordinateSpace)
	{
	case ParticleEmitter::kCoordinateSpaceLocalTranslationWorldRotation:
	case ParticleEmitter::kCoordinateSpaceWorld:
	{
		// If the rotation+scale portion of the transform has a negative
		// determinant, than it contains reflection.
		if (instance.m_mParentTransform.DeterminantUpper3x3() < 0.0f)
		{
			return -1.0f;
		}
	}
	case ParticleEmitter::kCoordinateSpaceLocal:
	case ParticleEmitter::kCoordinateSpaceWorldTranslationLocalRotation:
	default:
		break;
	};

	return 0.0f;
}

//
// InheritedVelocity - helper class, defines a static inline function used to calculate
// the velocity that should be inherited by a particle from its emitter, based on
// the emitter's coordinate space and the ParticlesInheritEmitterVelocity() flag.
//
template <
	ParticleEmitter::CoordinateSpace COORDINATE_SPACE,
	Bool B_PARTICLES_INHERIT_EMITTER_VELOCITY>
class InheritedVelocity
{
};

/**
 * This variation of InheritedVelocity handles all cases where the coordinate
 * space is not kCoordinateSpaceWorld and the ParticlesInheritEmitterVelocity() flag
 * is true.
 */
template <ParticleEmitter::CoordinateSpace COORDINATE_SPACE>
struct InheritedVelocity<COORDINATE_SPACE, true>
{
	static inline Vector3D Get(
		const ParticleEmitterInstance& instance,
		const Vector3D& vEmitterVelocity)
	{
		return Matrix4D::TransformDirection(instance.m_mParentInverseTransform, vEmitterVelocity);
	}
};

/**
 * This variation of InheritedVelocity handles all cases where the coordinate
 * space is not kCoordinateSpaceWorld and the ParticlesInheritEmitterVelocity() flag
 * is false.
 */
template <ParticleEmitter::CoordinateSpace COORDINATE_SPACE>
struct InheritedVelocity<COORDINATE_SPACE, false>
{
	static inline Vector3D Get(
		const ParticleEmitterInstance& instance,
		const Vector3D& vEmitterVelocity)
	{
		return Vector3D::Zero();
	}
};

/**
 * This variation of InheritedVelocity handles the case where the coordinate
 * space is kCoordinateSpaceWorld and the ParticlesInheritEmitterVelocity() flag
 * is true.
 */
template <>
struct InheritedVelocity<ParticleEmitter::kCoordinateSpaceWorld, true>
{
	static inline Vector3D Get(
		const ParticleEmitterInstance& instance,
		const Vector3D& vEmitterVelocity)
	{
		return vEmitterVelocity;
	}
};

/**
 * This variation of InheritedVelocity handles the case where the coordinate
 * space is kCoordinateSpaceWorld and the ParticlesInheritEmitterVelocity() flag
 * is false.
 */
template <>
struct InheritedVelocity<ParticleEmitter::kCoordinateSpaceWorld, false>
{
	static inline Vector3D Get(
		const ParticleEmitterInstance& instance,
		const Vector3D& vEmitterVelocity)
	{
		return Vector3D::Zero();
	}
};

/**
 * This variation of InheritedVelocity handles the case where the coordinate
 * space is kCoordinateSpaceWorldTranslationLocalRotation and the ParticlesInheritEmitterVelocity() flag
 * is true.
 */
template <>
struct InheritedVelocity<ParticleEmitter::kCoordinateSpaceWorldTranslationLocalRotation, true>
{
	static inline Vector3D Get(
		const ParticleEmitterInstance& instance,
		const Vector3D& vEmitterVelocity)
	{
		return vEmitterVelocity;
	}
};

/**
 * This variation of InheritedVelocity handles the case where the coordinate
 * space is kCoordinateSpaceWorldTranslationLocalRotation and the ParticlesInheritEmitterVelocity() flag
 * is false.
 */
template <>
struct InheritedVelocity<ParticleEmitter::kCoordinateSpaceWorldTranslationLocalRotation, false>
{
	static inline Vector3D Get(
		const ParticleEmitterInstance& instance,
		const Vector3D& vEmitterVelocity)
	{
		return Vector3D::Zero();
	}
};

inline Vector3D InternalGetInheritedVelocity(
	Bool bParticlesInheritEmitterVelocity,
	const ParticleEmitter& emitter,
	const ParticleEmitterInstance& instance,
	const Vector3D& vEmitterVelocity)
{
	if (bParticlesInheritEmitterVelocity)
	{
		switch (emitter.m_eCoordinateSpace)
		{
		case ParticleEmitter::kCoordinateSpaceWorld:
			return InheritedVelocity<ParticleEmitter::kCoordinateSpaceWorld, true>::Get(instance, vEmitterVelocity);
		case ParticleEmitter::kCoordinateSpaceLocal:
			return InheritedVelocity<ParticleEmitter::kCoordinateSpaceLocal, true>::Get(instance, vEmitterVelocity);
		case ParticleEmitter::kCoordinateSpaceLocalTranslationWorldRotation:
			return InheritedVelocity<ParticleEmitter::kCoordinateSpaceLocalTranslationWorldRotation, true>::Get(instance, vEmitterVelocity);
		case ParticleEmitter::kCoordinateSpaceWorldTranslationLocalRotation:
			return InheritedVelocity<ParticleEmitter::kCoordinateSpaceWorldTranslationLocalRotation, true>::Get(instance, vEmitterVelocity);
		default:
			SEOUL_FAIL("Out of sync enum.");
			return Vector3D::Zero();
		};
	}
	else
	{
		return Vector3D::Zero();
	}
}
//
// End InheritedVelocity
//

inline Vector3D GetParticleLinearVelocity(
	Bool bVelocityToWorldSpace,
	const ParticleEmitterInstance& instance,
	const Matrix4D& mEmitRotation,
	const Vector3D& vEmitterForwardVector)
{
	return (bVelocityToWorldSpace)
		? Matrix4D::TransformDirection(instance.m_mParentTransform * mEmitRotation, vEmitterForwardVector)
		: Matrix4D::TransformDirection(mEmitRotation, vEmitterForwardVector);
}

inline Vector3D ApplyEmitVelocity(
	const ParticleEmitter& emitter,
	const Matrix4D& mEmitRotation,
	const Vector3D& vEmitOffset)
{
	return (emitter.AlignOffsetToEmitVelocity())
		? Matrix4D::TransformDirection(mEmitRotation, vEmitOffset)
		: vEmitOffset;
}

inline Vector3D ApplyWorldSpaceToEmitOffset(
	Bool bOffsetToWorldSpace,
	const ParticleEmitter& emitter,
	const ParticleEmitterInstance& instance,
	const Vector3D& vEmitOffset)
{
	return (bOffsetToWorldSpace)
		? Matrix4D::TransformDirection(instance.m_mParentTransform, vEmitOffset)
		: vEmitOffset;
}

inline Vector3D GetEmitOffset(
	Bool bOffsetToWorldSpace,
	const ParticleEmitter& emitter,
	ParticleEmitterInstance& instance,
	const EmitterShapeParameter& emitterShapeParameter,
	const Matrix4D& mEmitRotation,
	const Vector3D& vEmitterOffset,
	const Vector3D& vLinearVelocity)
{
	Vector3D vEmitOffset =
		InternalStaticAdjustInitialPositionForEmitterShape(
			emitter,
			emitterShapeParameter,
			instance,
			vEmitterOffset);

	vEmitOffset =
		ApplyEmitVelocity(
			emitter,
			mEmitRotation,
			vEmitOffset);

	vEmitOffset =
		ApplyWorldSpaceToEmitOffset(
			bOffsetToWorldSpace,
			emitter,
			instance,
			vEmitOffset);

	return vEmitOffset;
}

template <ParticleEmitter::RotationAlignmentMode>
inline Float InternalAlignRotation(
	const Vector3D& vEmitOffset,
	const Vector3D& vEmitVelocity,
	const Vector3D& vEmitDirection,
	Float fRotation);

template <>
inline Float InternalAlignRotation<ParticleEmitter::kRotationNoAlignment>(
	const Vector3D& vEmitOffset,
	const Vector3D& vEmitVelocity,
	const Vector3D& vEmitDirection,
	Float fRotation)
{
	return fRotation;
}

template <>
inline Float InternalAlignRotation<ParticleEmitter::kRotationAlignToEmitAngle>(
	const Vector3D& vEmitOffset,
	const Vector3D& vEmitVelocity,
	const Vector3D& vEmitDirection,
	Float fRotation)
{
	const Vector3D vDirection = Vector3D::Normalize(Vector3D(vEmitDirection.X, 0.0f, vEmitDirection.Z));
	const Float fDeltaAngle =
		(vDirection.Z < -fEpsilon ||
		(vDirection.Z < fEpsilon && vDirection.X < -fEpsilon))
		? -Acos(Vector3D::Dot(vDirection, Vector3D::UnitX()))
		: Acos(Vector3D::Dot(vDirection, Vector3D::UnitX()));

	return fDeltaAngle - fPiOverTwo + fRotation;
}

inline static Float InternalStaticAlignRotation(
	ParticleEmitter::RotationAlignmentMode eMode,
	const Vector3D& vEmitOffset,
	const Vector3D& vEmitVelocity,
	const Vector3D& vEmitDirection,
	Float fRotation)
{
	switch (eMode)
	{
	case ParticleEmitter::kRotationNoAlignment:
		return InternalAlignRotation<ParticleEmitter::kRotationNoAlignment>(vEmitOffset, vEmitVelocity, vEmitDirection, fRotation);
	case ParticleEmitter::kRotationAlignToEmitAngle:
		return InternalAlignRotation<ParticleEmitter::kRotationAlignToEmitAngle>(vEmitOffset, vEmitVelocity, vEmitDirection, fRotation);
	default:
		SEOUL_FAIL("Enum out of sync.");
		return 0.0f;
	};
}

// TODO: Tick and render bodies below can be optimized by using
// templating to generate n-ary variations for all the configuration options.
//
// We used to do this but removed it because the n-power was too high (we
// were generating 2^16 function variations and this just failed on some
// platforms).
//
// The complexity is now much lower and compilers are better, so it would
// likely worth attempting this again.

/**
 * Handles emitting new particles into the set of active particles in instance.
 */
inline static void InternalStaticEmitParticles(
	Float fDeltaTimeInSeconds,
	Float fTimePercent,
	const ParticleEmitter& emitter,
	ParticleEmitterInstance& instance)
{
	// Helper constant - used to simplify template specialization.
	// As a result, must be conditional only on template arguments, so it
	// can be evaluated at compile time.
	const Bool bVelocityToWorldSpace =
		(emitter.ParentSpaceEmitDirection() &&
		ParticleEmitter::kCoordinateSpaceLocal != emitter.m_eCoordinateSpace);

	// Helper constant - used to simplify template specialization.
	// As a result, must be conditional only on template arguments, so it
	// can be evaluated at compile time.
	const Bool bOffsetToWorldSpace =
		(emitter.ParentSpaceEmitOffset() || emitter.ParentSpaceEmitDirection()) &&
		ParticleEmitter::kCoordinateSpaceLocal != emitter.m_eCoordinateSpace;

	// Emit rate in particles per second.
	const Float fEmitRate = emitter.m_EmitRate.Evaluate(fTimePercent);

	// Calculate the number of particles to be emitted.
	Int const iFreeSlots = (emitter.m_nMaxParticleCount - instance.m_iActiveParticleCount);
	instance.m_fParticleSpawnAccumulator += (instance.m_fInstanceEmitFactor * fEmitRate * fDeltaTimeInSeconds);
	Int iParticlesToSpawn = (Int)instance.m_fParticleSpawnAccumulator;
	instance.m_fParticleSpawnAccumulator -= iParticlesToSpawn;
	iParticlesToSpawn = Min(iParticlesToSpawn, iFreeSlots);

	// Early out if no particles to spawn.
	if (0 == iParticlesToSpawn)
	{
		return;
	}

	// This factor is used to linearly interpolate the emit position
	// of particles from the current emit position to the previous. This is
	// to prevent clumps of particles emitting at each discrete time step.
	const Float fEmitPositionLerpFactor = (iParticlesToSpawn > 1)
		? (1.0f / (iParticlesToSpawn - 1))
		: 0.0f;

	// Calculate the emitter shape parameter. This is dependent on the emitter
	// shape mode of the emitter. It will be used in the calculation of the
	// emit offset later in the function.
	const EmitterShapeParameter shapeParameter =
			InternalStaticGetEmitterShapeParameter(fTimePercent, emitter);

	// Acceleration of the emitter itself, separate from any motion of the emitter
	// owner. XYZ contain acceleration in 3 dimensions while W contains
	// "acceleration along emitter velocity", which is a magnitude that is applied
	// to the normalized direction of the emitter's pre-acceleration velocity and
	// added to the base XYZ acceleration.
	const Vector4D vEmitterAccelerationTerms =
		emitter.m_EmitterAcceleration.Evaluate(fTimePercent);

	// Total 3D emitter acceleration for this tick.
	const Vector3D vEmitterAcceleration =
		vEmitterAccelerationTerms.GetXYZ() +
			Vector3D::Normalize(instance.m_vEmitterVelocity) * vEmitterAccelerationTerms.W;

	// Accumulate emitter velocity and store for later use in the function.
	instance.m_vEmitterVelocity += (vEmitterAcceleration * fDeltaTimeInSeconds);
	const Vector3D vEmitterVelocity = instance.m_vEmitterVelocity;

	// Emitter displacement is used to lerp the position of particles between
	// the current emitter center and the previous emitter center. This is to
	// prevent bunches of particles at each discrete tick timestep from forming.
	const Vector3D vEmitterDisplacement = (vEmitterVelocity * fDeltaTimeInSeconds);

	// Prev emitter position used for "declumping" mentioned above and derived
	// velocity computation.
	const Vector3D vPreviousEmitterPosition =
		GetInitialPrevPosition(emitter, instance) + instance.m_vEmitterPosition;

	// Accumulate emitter translation and store for later use in the function.
	instance.m_vEmitterPosition += vEmitterDisplacement;
	const Vector3D vEmitterPosition = instance.m_vEmitterPosition;

	// Owner velocity is the velocity of the object that the emitter is parented
	// to, separate from the emitter's velocity.
	const Vector3D vOwnerVelocity =
		(instance.m_mParentTransform.GetTranslation() -
		instance.m_mParentPreviousTransform.GetTranslation())
			/ fDeltaTimeInSeconds;

	// Forward axis of the emitter, used to define a set of basis vectors
	// for orienting particles to the emitter.
	const Vector3D vEmitterForward = emitter.EmitAlongOwnerVelocity()
		? Vector3D::Normalize(vOwnerVelocity)
		: emitter.m_vEmitAxis;

	// Right and up axes based on the forward axis - basis vectors of the emitter coordinate space.
	Vector3D vEmitterRight;
	Vector3D vEmitterUp;
	CalculateParticleUpAndRightAxes(vEmitterForward, vEmitterUp, vEmitterRight);

	// Offset term - used to translate the position from which particles are emitter
	// away from the center of the emitter.
	const Vector3D vEmitterOffset = emitter.m_EmitOffset.Evaluate(fTimePercent);

	// Base position of any emitter particles, the offset is not factored in yet.
	const Vector3D vInitialPosition =
		GetInitialPosition(emitter, instance) + vEmitterPosition;

	// Reflection based on mode.
	auto const fReflection = GetReflection(emitter, instance);

	// Base rotation of any emitted particles, will be further modified by
	// per-particle parameters. Specified as a range.
	const Float fInitialRotation = emitter.m_InitialRotation.Evaluate(fTimePercent);
	Vector2D const vInitialRotationRange = emitter.m_InitialRotationRange.Evaluate(fTimePercent);

	// Inherited velocity - depending on the mode of the emitter, this is the velocity
	// of the emitter that will be transfered to each emitted particle.
	const Vector3D vInheritedVelocity =
		InternalGetInheritedVelocity(emitter.ParticlesInheritEmitterVelocity(), emitter, instance, vEmitterVelocity);

	// Parameters dependent on the age of the emitter - these are evaluated
	// at the progression time percentage of the emitter.
	Vector2D const vInitialAngularVelocity = emitter.m_InitialAngularVelocity.Evaluate(fTimePercent);
	Vector2D const vInitialScaleMinMax = emitter.m_InitialScale.Evaluate(fTimePercent);
	Vector2D const vInitialVelocity = emitter.m_InitialVelocity.Evaluate(fTimePercent);
	Vector2D const vParticleAngleMinMax = emitter.m_EmitAngleRange.Evaluate(fTimePercent);
	Vector2D const vParticleLifetimeMinMax = emitter.m_Lifetime.Evaluate(fTimePercent);

	// Particle emit loop.
	for (Int i = 0; i < iParticlesToSpawn; ++i)
	{
		// Emit position is the base particle position. It will be further
		// combined with the emitter offset. The position is lerped based on the
		// emitter displacement in order to avoid bunches of particles.
		const Vector3D vEmitPosition = Vector3D::Lerp(
			vInitialPosition,
			vPreviousEmitterPosition,
			i * fEmitPositionLerpFactor);

		// Rotation of the particle, randomized to provide variation per particle.
		const Matrix4D mEmitRotation =
			Matrix4D::CreateRotationFromAxisAngle(vEmitterForward, instance.GetRandom(kParticleAngleZeroToTwoPi)) *
			Matrix4D::CreateRotationFromAxisAngle(vEmitterRight, instance.GetRandom(vParticleAngleMinMax));

		// Base linear velocity of the particle. The exact value is dependent on several
		// configuration parameters of the emitter - see the definition of bVelocityToWorldSpace.
		const Vector3D vLinearVelocity = GetParticleLinearVelocity(
			bVelocityToWorldSpace,
			instance,
			mEmitRotation,
			vEmitterForward);

		// The total emit offset of the particle, used to translate the position of the
		// partile from its base position. Dependent on several config variables of the
		// emitter, including EMITTER_SHAPE and B_ALIGN_OFFSET_TO_EMIT_VELOCITY. Also
		// see the definition of bOffsetToWorldSpace.
		const Vector3D vEmitOffset = GetEmitOffset(
			bOffsetToWorldSpace,
			emitter,
			instance,
			shapeParameter,
			mEmitRotation,
			vEmitterOffset,
			vLinearVelocity);

		// Now that all the data is calculated, populate the particle.
		Particle& particle = instance.m_pParticles[instance.m_iActiveParticleCount++];

		// Initialize particle parameters used for ticking.
		particle.m_fAge = 0.0f;
		particle.m_fAngularVelocity = instance.GetRandom(vInitialAngularVelocity);
		particle.m_fInitialScale = instance.GetRandom(vInitialScaleMinMax);
		particle.m_fLifespan = Max(instance.GetRandom(vParticleLifetimeMinMax), 0.01f);
		particle.m_vLinearVelocity =
			vInheritedVelocity +
			instance.GetRandom(vInitialVelocity) *
			vLinearVelocity;

		particle.m_vPosition.X = (vEmitPosition.X + vEmitOffset.X);
		particle.m_vPosition.Y = (emitter.SnapParticlesToEmitterY())
			? (vEmitPosition.Y + vEmitterOffset.Y)
			: (vEmitPosition.Y + vEmitOffset.Y);
		particle.m_vPosition.Z = (emitter.SnapParticlesToEmitterZ() || instance.ForceSnapZ())
			? (vEmitPosition.Z + vEmitterOffset.Z)
			: (vEmitPosition.Z + vEmitOffset.Z);
		particle.m_fRotation = (emitter.AlignParticlesToVelocity())
			? ComputeRotationBasedOnVelocity(particle.m_vLinearVelocity)
			: InternalStaticAlignRotation(
				emitter.m_eRotationAlignment,
				vEmitOffset,
				particle.m_vLinearVelocity,
				vLinearVelocity,
				emitter.RandomInitialParticleRotation()
					? instance.GetRandom(kParticleAngleZeroToTwoPi)
					: fInitialRotation + instance.GetRandom(vInitialRotationRange));

		particle.m_vReflection = Vector4D(vEmitPosition, fReflection);
	}
}

/**
 * Add renderable particles in instance to its global shared render data.
 */
void RenderParticles(
	ParticleEmitterInstance& instance,
	FxParticleRenderBuffer& rBuffer)
{
	const ParticleEmitter& emitter = *instance.GetEmitter();

	// The parent transform of the particle system
	Matrix4D mParentTransform = GetParticleRenderTransform(
		emitter,
		instance);

	// Apply mirroring if enabled.
	Vector3D const vParentOrigin = instance.m_mParentTransform.GetTranslation();
	if (instance.MirrorX())
	{
		mParentTransform =
			Matrix4D::CreateReflection(Plane::CreateFromPositionAndNormal(vParentOrigin, Vector3D::UnitX())) *
			mParentTransform;
	}
	if (instance.MirrorY())
	{
		mParentTransform =
			Matrix4D::CreateReflection(Plane::CreateFromPositionAndNormal(vParentOrigin, Vector3D::UnitY())) *
			mParentTransform;
	}
	if (instance.MirrorZ())
	{
		mParentTransform =
			Matrix4D::CreateReflection(Plane::CreateFromPositionAndNormal(vParentOrigin, Vector3D::UnitZ())) *
			mParentTransform;
	}

	// For each currently active particle. Process in reverse order,
	// to draw back-to-front.
	for (Int32 i = (instance.m_iActiveParticleCount - 1); i >= 0; --i)
	{
		const Particle& particle = instance.m_pParticles[i];

		// Particle age percent is a value on [0.0, 1.0] used to evaluate
		// curves based on the age of the particle.
		const Float fParticleAgePercent = Clamp(particle.m_fAge / particle.m_fLifespan, 0.0f, 1.0f);

		// Compute the new color of the particle.
		RGBA const cNewColor = RGBA::Create(emitter.m_Color.Evaluate(fParticleAgePercent));

		// If the particle is not renderable, don't add it to the draw buffer.
		if (!IsParticleRenderable(cNewColor))
		{
			continue;
		}

		// Get the next renderable particle slot and set its values based on the particle
		// being rendered.
		FxParticle renderableParticle;

		// Pass the color through.
		renderableParticle.m_Color = cNewColor;

		// Compute alpha clamp, if enabled.
		if (emitter.AlphaClamp())
		{
			auto const vAlphaClamp = emitter.m_AlphaClamp.Evaluate(fParticleAgePercent);

			renderableParticle.m_uAlphaClampMin = (UInt8)Clamp(vAlphaClamp.X * 255.0f + 0.5f, 0.0f, 254.0f);
			renderableParticle.m_uAlphaClampMax = (UInt8)Clamp(vAlphaClamp.Y * 255.0f + 0.5f, (Float)renderableParticle.m_uAlphaClampMin + 1.0f, 255.0f);
		}

		// Evaluate the current texcoords.
		renderableParticle.m_vTexcoordScaleAndShift = emitter.m_TexcoordScaleAndShift.Evaluate(fParticleAgePercent);

		// Set the particle transform.
		Vector2D const vScale = (emitter.m_Scale.Evaluate(fParticleAgePercent) * particle.m_fInitialScale);
		Vector2D const vLocalTranslation = emitter.m_LocalTranslation.Evaluate(fParticleAgePercent);

		// Setting used in conjunction with "rally point" functionality.
		if (emitter.ParticleScaleAndRotationTransformIndependant())
		{
			renderableParticle.m_mTransform = Matrix3x4(
				Matrix4D::CreateTranslation(Matrix4D::TransformPosition(mParentTransform, particle.m_vPosition)) *
				Matrix4D::CreateRotationZ(particle.m_fRotation) *
				Matrix4D::CreateScale(Vector3D(vScale, 1.0f)) *
				Matrix4D::CreateTranslation(Vector3D(vLocalTranslation, 0.0f)));
		}
		// For particles in world space or world rotation space, we need
		// to manually apply any reflection present in the parent transform.
		else if (emitter.GetCoordinateSpace() != ParticleEmitter::kCoordinateSpaceLocal &&
			particle.m_vReflection.W != 0.0f)
		{
			auto const vXYZ = particle.m_vReflection.GetXYZ();

			Matrix4D mParticleTransform = Matrix4D::CreateRotationZ(particle.m_fRotation)
				* Matrix4D::CreateScale(Vector3D(vScale, 1.0f));
			mParticleTransform.SetTranslation(particle.m_vPosition);
			renderableParticle.m_mTransform = Matrix3x4(
				mParentTransform *
				Matrix4D::CreateTranslation(vXYZ) *
				Matrix4D::CreateScale(particle.m_vReflection.W, 1.0f, 1.0f) *
				Matrix4D::CreateTranslation(-vXYZ) *
				mParticleTransform *
				Matrix4D::CreateTranslation(Vector3D(vLocalTranslation, 0.0f)));
		}
		// For local space particles, or particles with no reflection in the parent
		// transform, we can use a simpler calculation.
		else
		{
			Matrix4D mParticleTransform = Matrix4D::CreateRotationZ(particle.m_fRotation)
				* Matrix4D::CreateScale(Vector3D(vScale, 1.0f));
			mParticleTransform.SetTranslation(particle.m_vPosition);
			renderableParticle.m_mTransform = Matrix3x4(
				mParentTransform *
				mParticleTransform *
				Matrix4D::CreateTranslation(Vector3D(vLocalTranslation, 0.0f)));
		}

		rBuffer.PushBack(renderableParticle);
	}
}

/**
 * Update the state of particles in instance from their current state
 * to their next state, based on fDeltaTimeInSeconds and TimePercent.
 *
 * @param[in] fDeltaTimeInSeconds Time change from the previous update to the current in seconds.
 * @param[in] fTimePercent Value on [0,1], where 0 is the initial state of particle curves defined
 * in emitter and 1 is the final state.
 */
void TickParticles(
	Float fDeltaTimeInSeconds,
	Float fTimePercent,
	ParticleEmitterInstance& instance)
{
	const ParticleEmitter& emitter = *instance.GetEmitter();

	// This function handles ticking and updating already active particles. It
	// may also deactivate active particles when their lifespan is exceeded.
	InternalStaticTickActiveParticles(
		fDeltaTimeInSeconds,
		emitter,
		instance);

	// This function handles emitting new particles, based on
	// the emitter's max particles variable and the emitter's emit rate.
	InternalStaticEmitParticles(
		fDeltaTimeInSeconds,
		fTimePercent,
		emitter,
		instance);
}

} // namespace Seoul
