/**
 * \file FxStudioParticleEmitter.cpp
 * \brief Specialization of FxStudio::Component that implements a single particle emitter.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Camera.h"
#include "ContentLoadManager.h"
#include "Engine.h"
#include "Fx.h"
#include "FxStudioFactory.h"
#include "FxStudioManager.h"
#include "FxStudioParticleEmitter.h"
#include "FxStudioUtil.h"
#include "IndexBuffer.h"
#include "Logger.h"
#include "ParticleEmitterInstance.h"
#include "ParticleUtil.h"
#include "ReflectionDefine.h"
#include "ReflectionUtil.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "RenderPass.h"
#include "TextureManager.h"
#include "VertexBuffer.h"
#include "VertexFormat.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

/** Constant filename of the shader Effect used for particle systems. */
const char* kDefaultParticleEffectFilename = "Effects/Particles/Particle.fx";

template <typename T, typename U>
T ToSimpleCurve(const U& fxStudioCurve, Float fT);

template <typename T, typename U>
T ToSimpleCurveFriction(const U& fxStudioCurve, Float fT);

template <typename T, typename U>
T ToSimpleCurveDefaultValueOf1(const U& fxStudioCurve, Float fT);

/**
 * Helper method to set a value to an emitter curve.
 */
template <typename T, typename U>
inline void SetToCurve(
	SimpleCurve<T>& rDestinationValue,
	T (*populateDelegate)(const U&, Float fT),
	Byte const* sPropertyName,
	ParticleEmitter* pEmitter)
{
	SEOUL_ASSERT(pEmitter);
	U prop(sPropertyName, pEmitter);
	rDestinationValue.Set(populateDelegate, prop);
}

/**
 * Helper method to set a value to an emitter curve.
 */
template <typename T, typename U>
inline void SetToCurve(
	SimpleCurve<T>& rDestinationValue,
	T (*populateDelegate)(const U&, Float fT),
	Byte const* sPropertyNameA,
	Byte const* sPropertyNameB,
	ParticleEmitter* pEmitter)
{
	SEOUL_ASSERT(pEmitter);
	U prop(sPropertyNameA, sPropertyNameB, pEmitter);
	rDestinationValue.Set(populateDelegate, prop);
}

/**
* Helper method to set a value to an emitter curve.
*/
template <typename T, typename U>
inline void SetToCurve(
	SimpleCurve<T>& rDestinationValue,
	T(*populateDelegate)(const U&, Float fT),
	Byte const* sPropertyNameA,
	Byte const* sPropertyNameB,
	const T& defaultValue,
	ParticleEmitter* pEmitter)
{
	SEOUL_ASSERT(pEmitter);
	U prop(sPropertyNameA, sPropertyNameB, defaultValue, pEmitter);
	rDestinationValue.Set(populateDelegate, prop);
}

/**
 * Specialization, converts an FxStudio float key-frame curve
 * to a Seoul::SimpleCurve with 1 channel.
 *
 * Default value is 0.0.
 */
template <>
inline Float ToSimpleCurve<Float, ::FxStudio::FloatKeyFrameProperty>(
	const ::FxStudio::FloatKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	return fxStudioCurve.GetValueAtTime(0, fT);
}

/**
 * Specialization, converts an FxStudio float key-frame curve
 * to a Seoul::SimpleCurve with 1 channel, applying rescale and clamping
 * to the final value.
 *
 * Default value is 0.0.
 */
template <>
inline Float ToSimpleCurveFriction<Float, ::FxStudio::FloatKeyFrameProperty>(
	const ::FxStudio::FloatKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	// We rescale a friction value of [0, 1] to [0, 0.1]
	return Clamp(
		fxStudioCurve.GetValueAtTime(0, fT) * 0.1f,
		0.0f,
		1.0f);
}

/**
 * Specialization, converts an FxStudio float key-frame curve
 * to a Seoul::SimpleCurve with 1 channels.
 *
 * Default value is 0.0.
 */
template <>
inline Vector2D ToSimpleCurve<Vector2D, ::FxStudio::FloatKeyFrameProperty>(
	const ::FxStudio::FloatKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	return Vector2D(
		fxStudioCurve.GetValueAtTime(0, fT),
		fxStudioCurve.GetValueAtTime(1, fT));
}

/**
 * Specialization, converts an FxStudio float key-frame curve
 * to a Seoul::SimpleCurve with 2 channels.
 *
 * Default value is 1.0.
 */
template <>
inline Vector2D ToSimpleCurveDefaultValueOf1<Vector2D, ::FxStudio::FloatKeyFrameProperty>(
	const ::FxStudio::FloatKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	return Vector2D(
		fxStudioCurve.GetValueAtTime(0, fT, 1.0f),
		fxStudioCurve.GetValueAtTime(1, fT, 1.0f));
}

/**
 * Specialization, converts an FxStudio float key-frame curve
 * to a Seoul::SimpleCurve with 2 channels.
 *
 * Default value of channel 0 is 0, and default value of channel 1 is 1
 */
inline Vector2D ToSimpleCurveDefaultValueOf0And1(
	const ::FxStudio::FloatKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	return Vector2D(
		fxStudioCurve.GetValueAtTime(0, fT, 0.0f),
		fxStudioCurve.GetValueAtTime(1, fT, 1.0f));
}

/**
 * Specialization, converts an FxStudio float key-frame curve
 * to a Seoul::SimpleCurve with 3 channels.
 *
 * Default value is 0.0.
 */
template <>
inline Vector3D ToSimpleCurve<Vector3D, ::FxStudio::FloatKeyFrameProperty>(
	const ::FxStudio::FloatKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	return Vector3D(
		fxStudioCurve.GetValueAtTime(0, fT),
		fxStudioCurve.GetValueAtTime(1, fT),
		fxStudioCurve.GetValueAtTime(2, fT));
}

/**
 * Helper structure, allows 2 FxStudio curves, the first a 3 channel
 * curve and the second a 1 channel curve, to be converted into a single
 * Seoul::SimpleCurve with 4 channels.
 */
struct TwoCurves3And1
{
	::FxStudio::FloatKeyFrameProperty m_Curve3D;
	::FxStudio::FloatKeyFrameProperty m_Curve1D;

	TwoCurves3And1(
		const char* pCurve3D,
		const char* pCurve1D,
		::FxStudio::Component* pComponent)
		: m_Curve3D(pCurve3D, pComponent)
		, m_Curve1D(pCurve1D, pComponent)
	{
	}
}; // struct FxStudio::TwoCurves3And1

/**
 * Specialization, converts two FxStudio float key-frame curves
 * with 3 channels and 1 channel to a Seoul::SimpleCurve with 4 channels.
 *
 * Default value is 0.0.
 */
template <>
inline Vector4D ToSimpleCurve<Vector4D, TwoCurves3And1>(
	const TwoCurves3And1& fxStudioCurves,
	Float fT)
{
	return Vector4D(
		fxStudioCurves.m_Curve3D.GetValueAtTime(0, fT),
		fxStudioCurves.m_Curve3D.GetValueAtTime(1, fT),
		fxStudioCurves.m_Curve3D.GetValueAtTime(2, fT),
		fxStudioCurves.m_Curve1D.GetValueAtTime(0, fT));
}

/**
 * Helper structure, allows 2 FxStudio curves, the first a 2 channel
 * curve and the second a 2 channel curve, to be converted into a single
 * Seoul::SimpleCurve with 4 channels.
 */
struct TwoCurves2And2
{
	::FxStudio::FloatKeyFrameProperty const m_Curve2Da;
	::FxStudio::FloatKeyFrameProperty const m_Curve2Db;
	Vector4D const m_vDefault;

	TwoCurves2And2(
		const char* pCurve2Da,
		const char* pCurve2Db,
		const Vector4D& vDefault,
		::FxStudio::Component* pComponent)
		: m_Curve2Da(pCurve2Da, pComponent)
		, m_Curve2Db(pCurve2Db, pComponent)
		, m_vDefault(vDefault)
	{
	}
};

/**
 * Specialization, converts two FxStudio float key-frame curves
 * with 2 channels and 2 channels to a Seoul::SimpleCurve with 4 channels.
 *
 * Default value is 0.0.
 */
template <>
inline Vector4D ToSimpleCurve<Vector4D, TwoCurves2And2>(
	const TwoCurves2And2& fxStudioCurves,
	Float fT)
{
	return Vector4D(
		fxStudioCurves.m_Curve2Da.GetValueAtTime(0, fT, fxStudioCurves.m_vDefault.X),
		fxStudioCurves.m_Curve2Da.GetValueAtTime(1, fT, fxStudioCurves.m_vDefault.Y),
		fxStudioCurves.m_Curve2Db.GetValueAtTime(0, fT, fxStudioCurves.m_vDefault.Z),
		fxStudioCurves.m_Curve2Db.GetValueAtTime(1, fT, fxStudioCurves.m_vDefault.W));
}

/**
 * Specialization, converts an FxStudio ColorARGB key-frame curve
 * to a Seoul::SimpleCurve on a ColorARGBu8 value.
 *
 * Default value is 0.0.
 */
template <>
inline ColorARGBu8 ToSimpleCurve<ColorARGBu8, ::FxStudio::ColorARGBKeyFrameProperty>(
	const ::FxStudio::ColorARGBKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	return Int32ColorToColorARGBu8(fxStudioCurve.GetValueAtTime(0, fT));
}

/**
 * Converts an FxStudio Vector3 value to a Seoul::Vector3D value.
 */
inline Vector3D ConvertToVector3D(::FxStudio::Vector3 v)
{
	return Vector3D(v.m_fX, v.m_fY, v.m_fZ);
}

/*
 * Helper structure, allows an FxStudio curve with non uniform scale
 * in 3 channels and an FxStudio curve with uniform scale in 1 channel to
 * be merged into a single Seoul::SimpleCurve with 3 channels.
 */
struct CombineNonUniformAndUniformScaleCurves
{
	::FxStudio::FloatKeyFrameProperty m_Curve3D;
	::FxStudio::FloatKeyFrameProperty m_Curve1D;

	CombineNonUniformAndUniformScaleCurves(
		const char* pCurve3D,
		const char* pCurve1D,
		::FxStudio::Component* pComponent)
		: m_Curve3D(pCurve3D, pComponent)
		, m_Curve1D(pCurve1D, pComponent)
	{
	}
};

/**
 * Specialization, converts two FxStudio float key-frame curves
 * with 3 channels of non-uniform scale and 1 channel of uniform scale to
 * a Seoul::SimpleCurve with 2 channels.
 *
 * Default value is 1.0.
 */
template <>
inline Vector2D ToSimpleCurve<Vector2D, CombineNonUniformAndUniformScaleCurves>(
	const CombineNonUniformAndUniformScaleCurves& fxStudioCurves,
	Float fT)
{
	// TODO: Legacy - 3D scale of a particle never made sense for this system
	// (a particle is always a 2D plane), and X and Z were chosen as the significant channels.
	//
	// As a result, we maintain the use of 2 of the 3 channels (X and Z), and the FxStudio component
	// definition for non-uniform scale has been updated to label the Z component 'Y' and to hide
	// the actual Y component from the editor (even though it still exists under-the-hood). It is
	// labeled "unused Y component" in the editor, although an end user will never see this, only
	// when modifying the component definition.

	// Use a default value of 1.0 for scale.
	return Vector2D(
		fxStudioCurves.m_Curve3D.GetValueAtTime(0, fT, 1.0f),
		fxStudioCurves.m_Curve3D.GetValueAtTime(2, fT, 1.0f)) *
		fxStudioCurves.m_Curve1D.GetValueAtTime(0, fT, 1.0f);
}

/**
 * Converts a 2D angle range
 * defined in degrees into a Seoul::SimpleCurve with 2 channels in radians.
 *
 * Default value is 0.0.
 */
inline Vector2D Angle2DCurveToSimpleCurveRadians(
	const ::FxStudio::FloatKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	return Vector2D(
		DegreesToRadians(fxStudioCurve.GetValueAtTime(0, fT)),
		DegreesToRadians(fxStudioCurve.GetValueAtTime(1, fT)));
}

/**
 * Converts an angle defined in degrees into a Seoul::SimpleCurve with 1
 *          channel in radians
 *
 * Default value is 0.0.
 */
inline Float Angle1DCurveToSimpleCurveRadians(
	const ::FxStudio::FloatKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	return DegreesToRadians(fxStudioCurve.GetValueAtTime(0, fT));
}


/**
 * Specialization, converts an FxStudio curve with "Emit Angle Range"
 * defined in degrees into a Seoul::SimpleCurve with 2 channels in radians,
 * scaled as expected by ParticleEmitter.
 *
 * Default value is 0.0.
 */
inline Vector2D EmitAngleRangeDegreesToSimpleCurveRadians(
	const ::FxStudio::FloatKeyFrameProperty& fxStudioCurve,
	Float fT)
{
	return
		// Not a typo - max is in channel 0, min is in channel 1.
		0.5f * Vector2D(
		DegreesToRadians(fxStudioCurve.GetValueAtTime(1, fT)),
		DegreesToRadians(fxStudioCurve.GetValueAtTime(0, fT)));
}

/**
 * Converts an "emitter velocity angle" range
 * defined in degrees into a Seoul::SimpleCurve with 2 channels in radians,
 * scaled as expected by ParticleEmitter.
 *
 * Default value is 0.0.
 */
inline Vector2D EmitterVelocityAngleToVector2D(
	const ::FxStudio::FloatRange& fxFloatRange)
{
	return
		0.5f * Vector2D(
		DegreesToRadians(fxFloatRange.m_fMin),
		DegreesToRadians(fxFloatRange.m_fMax));
}

/**
 * Converts an FxStudio int value into the corresponding ParticleEmitter::CoordinateSpace
 * enum value.
 */
inline Seoul::ParticleEmitter::CoordinateSpace ConvertCoordinateSpace(Int iValue)
{
	switch (iValue)
	{
	case 0: return Seoul::ParticleEmitter::kCoordinateSpaceWorld;
	case 1: return Seoul::ParticleEmitter::kCoordinateSpaceLocal;
	case 2: return Seoul::ParticleEmitter::kCoordinateSpaceLocalTranslationWorldRotation;
	case 3: return Seoul::ParticleEmitter::kCoordinateSpaceWorldTranslationLocalRotation;
	default:
		return Seoul::ParticleEmitter::kCoordinateSpaceWorld;
	}
}

/**
 * Converts an FxStudio int value into the corresponding ParticleEmitter::EmitterShape
 * enum value.
 */
inline Seoul::ParticleEmitter::EmitterShape ConvertEmitterShape(Int iValue)
{
	switch (iValue)
	{
	case 0: return Seoul::ParticleEmitter::kShapePoint;
	case 1: return Seoul::ParticleEmitter::kShapeLine;
	case 2: return Seoul::ParticleEmitter::kShapeBox;
	case 3: return Seoul::ParticleEmitter::kShapeSphere;
	default:
		return Seoul::ParticleEmitter::kShapePoint;
	}
}

/**
 * Converts several booleans in an FxStudio definition into the equivalent
 * ParticleEmitter::RotationAlignmentMode enum value.
 */
inline Seoul::ParticleEmitter::RotationAlignmentMode ConvertRotationAlignment(
	ParticleEmitter* pEmitter)
{
	Bool bAlignToEmitAngle = ::FxStudio::BooleanProperty("Emitter.Behavior.Align to Emit Angle", pEmitter);

	if (bAlignToEmitAngle)
	{
		return Seoul::ParticleEmitter::kRotationAlignToEmitAngle;
	}
	else
	{
		return Seoul::ParticleEmitter::kRotationNoAlignment;
	}
}

void ParticleEmitter::GetAssets(::FxStudio::Component const* pComponent, AssetCallback assetCallback, void* pUserData)
{
	assetCallback(pUserData, ::FxStudio::StringProperty("Particle.Appearance.Texture", pComponent).GetValue());
}

ParticleEmitter::ParticleEmitter(
	Int iComponentIndex,
	const InternalDataType& internalData,
	FilePath filePath)
	: ComponentBase(internalData)
	, m_mTransform(Matrix4D::Identity())
	, m_mParentIfWorldspaceTransform(Matrix4D::Identity())
	, m_fTimeAccumulator(0.0f)
	, m_uFlags(0u)
	, m_eFxRendererMode(FxRendererMode::kNormal)
	, m_bNeedsScreenAlign(false)
	, m_fGravityAcceleration(1000.f)
	, m_pParticleEmitterInstance()
	, m_pParticleEmitter()
	, m_SharedDataGUID(ParticleEmitterSharedDataGUID::Create(filePath, (UInt32)iComponentIndex))
	, m_vParticleRallyPointOverride(Vector3D::Zero())
	, m_bPendingApplyRallyPointOverride(false)
	, m_bHasRallyPointOverride(false)
{
	InternalSetupParticleMaterial();
}

ParticleEmitter::~ParticleEmitter()
{
}

/**
 * Called when this FxStudio::ParticleEmitter needs to be ticked.
 */
void ParticleEmitter::Update(Float fDeltaTime)
{
	if (m_pParticleEmitterInstance.IsValid())
	{
		InternalUpdate(fDeltaTime);
	}
}

/**
 * Called when an update occurs on this FxStudio::ParticleEmitter but playback
 * is paused.
 */
void ParticleEmitter::UpdateWhilePaused()
{
	// Get pointers to the particle emitter and instance.
	SEOUL_ASSERT(Manager::Get());

	if (m_pParticleEmitterInstance.IsValid() && m_pParticleEmitter.IsValid())
	{
		m_pParticleEmitterInstance->m_uFlags = m_uFlags;
		m_pParticleEmitterInstance->m_mParentPreviousTransform = m_pParticleEmitterInstance->m_mParentTransform;
		m_pParticleEmitterInstance->m_mParentTransform = InternalGetTransform();
		m_pParticleEmitterInstance->m_mParentIfWorldspaceTransform = m_mParentIfWorldspaceTransform;
		m_pParticleEmitterInstance->m_mParentIfWorldspaceInverseTransform = m_mParentIfWorldspaceTransform.Inverse();
		m_pParticleEmitterInstance->m_mParentInverseTransform = m_pParticleEmitterInstance->m_mParentTransform.Inverse();
		m_pParticleEmitterInstance->m_vParticleRallyPointOverride = m_vParticleRallyPointOverride;
		m_pParticleEmitterInstance->m_bPendingApplyRallyPointOverride = m_bPendingApplyRallyPointOverride;
		m_pParticleEmitterInstance->m_fGravityAcceleration = m_fGravityAcceleration;
	}
}

/**
 * Implementation of updating logic.
 *
 * @return True if updating occurred successfully, false otherwise.
 */
void ParticleEmitter::InternalUpdate(Float fDeltaTime)
{
	// Get pointers to the particle emitter and instance.
	SEOUL_ASSERT(Manager::Get());

	if (m_pParticleEmitterInstance.IsValid() && m_pParticleEmitter.IsValid())
	{
		m_pParticleEmitterInstance->m_uFlags = m_uFlags;
		m_pParticleEmitterInstance->m_mParentPreviousTransform = m_pParticleEmitterInstance->m_mParentTransform;
		m_pParticleEmitterInstance->m_mParentTransform = InternalGetTransform();
		m_pParticleEmitterInstance->m_mParentInverseTransform = m_pParticleEmitterInstance->m_mParentTransform.Inverse();
		m_pParticleEmitterInstance->m_mParentIfWorldspaceTransform = m_mParentIfWorldspaceTransform;
		m_pParticleEmitterInstance->m_mParentIfWorldspaceInverseTransform = m_mParentIfWorldspaceTransform.Inverse();
		m_pParticleEmitterInstance->m_vParticleRallyPointOverride = m_vParticleRallyPointOverride;
		m_pParticleEmitterInstance->m_bPendingApplyRallyPointOverride = m_bPendingApplyRallyPointOverride;
		m_pParticleEmitterInstance->m_fGravityAcceleration = m_fGravityAcceleration;

		m_fTimeAccumulator += fDeltaTime;

		// Note that, we can't call GetUnitTime() here, because it will always point at the full timestep,
		// not the partial time step that we may receive if a StayAlive() call will finish off the second
		// half of the current frame's total time step. Also, see documentation on Clamp() - Clamp() is
		// "NaN safe" and thus we don't need to check for a 0 length denominator in the value argument.
		Float const fTimePercent = Clamp(m_fTimeAccumulator / (GetEndTime() - GetStartTime()), 0.0f, 1.0f);
		TickParticles(fDeltaTime, fTimePercent, *m_pParticleEmitterInstance);
	}
}

/**
 * Sets up particle render buffers for the current frame.
 */
void ParticleEmitter::Render(void* pRenderData)
{
	IFxRenderer& rRenderer = *((IFxRenderer*)pRenderData);
	if (m_pParticleEmitterInstance.IsValid())
	{
		// Nothing to do if no active particles.
		if (m_pParticleEmitterInstance->m_iActiveParticleCount > 0)
		{
			IFxRenderer::Buffer& r = rRenderer.LockFxBuffer();
			UInt32 const uSize = r.GetSize();
			RenderParticles(*m_pParticleEmitterInstance, r);
			rRenderer.UnlockFxBuffer(
				(r.GetSize() - uSize),
				m_FilePath,
				m_eFxRendererMode,
				m_bNeedsScreenAlign);
		}
	}
}

/**
 * Called when this FxStudio::ParticleEmitter should start
 * emitting particles.
 */
void ParticleEmitter::Activate()
{
	ComponentBase::Activate();

	// Populate the emitter handle - if this fails, we need to create the emitter data (this is
	// the first time this particle emitter definition has been used).
	if (!Manager::Get()->GetParticleEmitter(m_SharedDataGUID, m_pParticleEmitter))
	{
		Preload(true);
	}

	// Reset the time accumulator
	m_fTimeAccumulator = 0.0f;

	m_pParticleEmitterInstance.Reset(SEOUL_NEW(MemoryBudgets::Particles) ParticleEmitterInstance(m_pParticleEmitter));

	if (m_pParticleEmitterInstance.IsValid() && m_pParticleEmitter.IsValid())
	{
		m_pParticleEmitter->InitializeInstance(
			InternalGetTransform(),
			*m_pParticleEmitterInstance);

		if (m_bPendingApplyRallyPointOverride)
		{
			InternalApplyRallyPointOverride();
			m_bPendingApplyRallyPointOverride = false;
		}
	}
}

/**
 * Called when this FxStudio::ParticleEmitter needs to be
 * restarted.
 */
void ParticleEmitter::Reset()
{
	// Reset the time accumulator
	m_fTimeAccumulator = 0.0f;

	m_pParticleEmitterInstance.Reset();
}

/**
 * Called to let a component falloff passed its endpoint (deactivate call).
 * When this method returns false, it tells FxStudio that the component is
 * completely inactive.
 */
Bool ParticleEmitter::StayAlive(Float fDeltaTime, Bool bWithinLOD)
{
	// Keep ticking as long as there are particles being ticked.
	if (m_pParticleEmitterInstance.IsValid())
	{
		if (m_pParticleEmitterInstance->m_iActiveParticleCount > 0)
		{
			m_pParticleEmitterInstance->m_fInstanceEmitFactor = 0.0f;

			InternalUpdate(fDeltaTime);

			// Must always return true here, even if the update has resulted in 0
			// emitted particles, so we render 1 more frame.
			return true;
		}
	}

	return false;
}

/**
 * Called by FxStudio when a property of this
 * FxStudio::ParticleEmitter is changed. This should only occur when
 * a developer is authoring the emitter using FxStudio
 * preview mode.
 */
Bool ParticleEmitter::OnPropertyChanged(Byte const* szPropertyName)
{
	InternalPreload();

	return true;
}

/**
 * Gets the string representation of the type if this component
 */
HString ParticleEmitter::GetComponentTypeName() const
{
	return HString(ParticleEmitter::StaticTypeName());
}

/**
 * Called when the data of this FxStudio::ParticleEmitter needs
 * to be preloaded.
 */
void ParticleEmitter::Preload(Bool bForce)
{
	SEOUL_ASSERT(Manager::Get());

	// If the emitter data is not already valid.
	if (!Manager::Get()->GetParticleEmitter(m_SharedDataGUID, m_pParticleEmitter))
	{
		// Create new emitter data.
		m_pParticleEmitter.Reset(SEOUL_NEW(MemoryBudgets::Particles) Seoul::ParticleEmitter);

		// The GUID can be invalid if we're previewing. In this case,
		// don't register the shared data, and free it on destruction.
		// This will result in new shared data per instance, but that should
		// be fine for previewing.
		if (m_SharedDataGUID.IsValid())
		{
			// Add it to the FxManager and preload it.
			Manager::Get()->RegisterParticleEmitter(
				m_SharedDataGUID,
				m_pParticleEmitter);
		}

		InternalPreload();
	}
	// Always preload if bForce is true. This is used to support
	// particle emitter hot-loading.
	else if (bForce)
	{
		InternalPreload();
	}
}

void ParticleEmitter::SetPosition(const Vector3D& vPosition)
{
	m_mTransform.SetTranslation(vPosition);

	if (m_bHasRallyPointOverride && m_pParticleEmitter.IsValid())
	{
		InternalApplyRallyPointOverride();
	}
}

void ParticleEmitter::SetGravity(Float fGravityAcceleration)
{
	m_fGravityAcceleration = fGravityAcceleration;
}

Bool ParticleEmitter::GetParticleRallyPointState(Vector3D& rvPoint, Bool& rbUseRallyPoint) const
{
	if (m_pParticleEmitter.IsValid() && m_pParticleEmitter->UseRallyPoint())
	{
		rvPoint = (m_bHasRallyPointOverride
			? m_vParticleRallyPointOverride
			: (m_mTransform.GetTranslation() + m_pParticleEmitter->m_vConfiguredRallyPoint));
		rbUseRallyPoint = true;
	}
	else
	{
		rbUseRallyPoint = false;
	}

	return true;
}

void ParticleEmitter::SetParticleRallyPointOverride(const Vector3D& vPoint)
{
	m_vParticleRallyPointOverride = vPoint;

	if (m_pParticleEmitter.IsValid())
	{
		InternalApplyRallyPointOverride();
	}
	else
	{
		m_bPendingApplyRallyPointOverride = true;
	}
}

void ParticleEmitter::SetTransform(const Matrix4D& mTransform)
{
	m_mTransform = mTransform;

	if (m_bHasRallyPointOverride && m_pParticleEmitter.IsValid())
	{
		InternalApplyRallyPointOverride();
	}
}

void ParticleEmitter::SetParentIfWorldspace(const Matrix4D& mTransform)
{
	m_mParentIfWorldspaceTransform = mTransform;
}

// NOTE: Ignores the Z-axis of the rally points
void ParticleEmitter::InternalApplyRallyPointOverride()
{
	if (m_pParticleEmitter->UseRallyPoint())
	{
		SEOUL_ASSERT(m_pParticleEmitter.IsValid());

		Vector3D const vRallyPointConfiguredVector = m_pParticleEmitter->m_vConfiguredRallyPoint;
		Vector3D const vCenter = m_mTransform.GetTranslation();
		Vector3D const vDesiredVector = (m_vParticleRallyPointOverride - vCenter);

		Matrix4D const mRotation = Matrix4D::CreateRotationFromDirection(
			Vector3D::Normalize(vDesiredVector),
			Vector3D::Normalize(vRallyPointConfiguredVector));
		Float const fConfiguredLength = vRallyPointConfiguredVector.Length();
		Float const fDesiredLength = vDesiredVector.Length();
		Matrix4D const mScale = Matrix4D::CreateScale(fConfiguredLength > 1e-5f
			? (fDesiredLength / fConfiguredLength)
			: 1.0f);

		m_mTransform =
			Matrix4D::CreateTranslation(vCenter) *
			mRotation *
			mScale;

		// Mark that an override has been applied.
		m_bHasRallyPointOverride = true;
	}
}

/**
 * Check the FxStudio alpha clamp curve to see if alpha clamping
 * is needed.
 */
Bool ParticleEmitter::InternalCheckAlphaClamp() const
{
	SimpleCurve<Vector2D> curve;
	SetToCurve(
		curve,
		ToSimpleCurveDefaultValueOf0And1,
		"Particle.Appearance.Alpha Clamp",
		const_cast<ParticleEmitter*>(this));

	// Iterate - if any sample has a X value (min) != 0 or a Y value (max) != 1,
	// then alpha clamping is enabled.
	for (auto const& sample : curve)
	{
		if (sample.X != 0.0f || sample.Y != 1.0f)
		{
			return true;
		}
	}

	return false;
}

Bool ParticleEmitter::InternalCheckTextureHasNoPreMultiply() const
{
	auto const sRelative(m_FilePath.GetRelativeFilenameWithoutExtension().ToString());

	return sRelative.EndsWith("_nopre");
}

// Map FX settings to various FX render modes.
static FxRendererMode GetFxRendererMode(Int iMode, Bool bAlphaClamp, Bool bNoPreMultiply)
{
	// Guaranteed override.
	if (bAlphaClamp)
	{
		if (bNoPreMultiply)
		{
			return FxRendererMode::kColorAlphaClamp;
		}
		else
		{
			return FxRendererMode::kAlphaClamp;
		}
	}
	else
	{
		return (FxRendererMode)iMode;
	}
}

/**
 * Configure the material that uniquely identifies this particle emitter
 * during render.
 */
void ParticleEmitter::InternalSetupParticleMaterial()
{
	m_FilePath = FilePath::CreateContentFilePath(
		::FxStudio::StringProperty("Particle.Appearance.Texture", this).GetValue());
	m_eFxRendererMode = GetFxRendererMode(
		::FxStudio::IntegerProperty("Particle.Appearance.Blend Mode", this).GetValue((Int)FxRendererMode::kNormal),
		InternalCheckAlphaClamp(),
		InternalCheckTextureHasNoPreMultiply());
	m_bNeedsScreenAlign = ::FxStudio::BooleanProperty("Particle.Appearance.Screen Align", this).GetValue(false);
}

/**
 * Implementation of preloading. Primarily, converts
 * FxStudio values to more compact or faster-to-evaluate representations.
 */
void ParticleEmitter::InternalPreload()
{
	if (m_pParticleEmitter.IsValid())
	{
		// Clear initial flags settings.
		m_pParticleEmitter->m_uFlags = Seoul::ParticleEmitter::kFlagNone;

		// Particle
		// - Appearance
		SetToCurve(
			m_pParticleEmitter->m_Color,
			ToSimpleCurve<ColorARGBu8, ::FxStudio::ColorARGBKeyFrameProperty>,
			"Particle.Appearance.Color",
			this);

		SetToCurve(
			m_pParticleEmitter->m_TexcoordScaleAndShift,
			ToSimpleCurve<Vector4D, TwoCurves2And2>,
			"Particle.Appearance.UV Scale",
			"Particle.Appearance.UV Offset",
			Vector4D(1, 1, 0, 0),
			this);

		SetToCurve(
			m_pParticleEmitter->m_LocalTranslation,
			ToSimpleCurve<Vector2D, ::FxStudio::FloatKeyFrameProperty>,
			"Particle.Appearance.Local Origin",
			this);

		SetToCurve(
			m_pParticleEmitter->m_Scale,
			ToSimpleCurve<Vector2D, CombineNonUniformAndUniformScaleCurves>,
			"Particle.Appearance.Non-Uniform Scale",
			"Particle.Appearance.Uniform Scale",
			this);

		SetToCurve(
			m_pParticleEmitter->m_LinearAcceleration,
			ToSimpleCurve<Vector4D, TwoCurves3And1>,
			"Particle.Appearance.Acceleration",
			"Particle.Appearance.Acceleration Along Velocity",
			this);

		SetToCurve(
			m_pParticleEmitter->m_GravityScalar,
			ToSimpleCurve<Float, ::FxStudio::FloatKeyFrameProperty>,
			"Particle.Appearance.Gravity Scalar",
			this);

		SetToCurve(
			m_pParticleEmitter->m_LinearFriction,
			ToSimpleCurveFriction<Float, ::FxStudio::FloatKeyFrameProperty>,
			"Particle.Appearance.Linear Friction",
			this);

		SetToCurve(
			m_pParticleEmitter->m_AngularAcceleration,
			Angle2DCurveToSimpleCurveRadians,
			"Particle.Appearance.Angular Acceleration",
			this);

		SetToCurve(
			m_pParticleEmitter->m_AngularFriction,
			ToSimpleCurveFriction<Float, ::FxStudio::FloatKeyFrameProperty>,
			"Particle.Appearance.Angular Friction",
			this);

		SetToCurve(
			m_pParticleEmitter->m_AlphaClamp,
			ToSimpleCurveDefaultValueOf0And1,
			"Particle.Appearance.Alpha Clamp",
			this);
		if (InternalCheckAlphaClamp())
		{
			m_pParticleEmitter->m_uFlags |= Seoul::ParticleEmitter::kFlagAlphaClamp;
		}

		// Emitter
		// - Behavior
		m_pParticleEmitter->m_eCoordinateSpace = ConvertCoordinateSpace(
			::FxStudio::IntegerProperty("Emitter.Behavior.Coordinate Space", this).GetValue());

		m_pParticleEmitter->m_nMaxParticleCount =
			(UInt32)::FxStudio::IntegerProperty("Emitter.Behavior.Maximum Particle Count", this).GetValue();

		::FxStudio::IntegerRangeProperty initialParticleCount("Emitter.Behavior.Initial Particle Count", this);
		m_pParticleEmitter->m_vInitialParticleCount = Vector2D(
			(Float)initialParticleCount.GetValue().m_nMin,
			(Float)initialParticleCount.GetValue().m_nMax);

		SetToCurve(
			m_pParticleEmitter->m_EmitRate,
			ToSimpleCurve<Float, ::FxStudio::FloatKeyFrameProperty>,
			"Emitter.Behavior.Emit Rate",
			this);

		SetToCurve(
			m_pParticleEmitter->m_Lifetime,
			ToSimpleCurve<Vector2D, ::FxStudio::FloatKeyFrameProperty>,
			"Emitter.Behavior.Lifetime",
			this);

		SetToCurve(
			m_pParticleEmitter->m_InitialScale,
			ToSimpleCurveDefaultValueOf1<Vector2D, ::FxStudio::FloatKeyFrameProperty>,
			"Emitter.Behavior.Initial Scale",
			this);

		SetToCurve(
			m_pParticleEmitter->m_InitialRotationRange,
			Angle2DCurveToSimpleCurveRadians,
			"Emitter.Behavior.Initial Rotation Range",
			this);

		SetToCurve(
			m_pParticleEmitter->m_InitialAngularVelocity,
			Angle2DCurveToSimpleCurveRadians,
			"Emitter.Behavior.Initial Angular Velocity",
			this);

		SetToCurve(
			m_pParticleEmitter->m_InitialVelocity,
			ToSimpleCurve<Vector2D, ::FxStudio::FloatKeyFrameProperty>,
			"Emitter.Behavior.Initial Velocity",
			this);

		SetToCurve(
			m_pParticleEmitter->m_EmitAngleRange,
			EmitAngleRangeDegreesToSimpleCurveRadians,
			"Emitter.Behavior.Emit Angle Range",
			this);

		m_pParticleEmitter->m_vEmitAxis = Vector3D::Normalize(ConvertToVector3D(
			::FxStudio::Vector3Property("Emitter.Behavior.Emit Direction", this).GetValue()));

		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Parent Space Emitter Offset", this).GetValue())
			? Seoul::ParticleEmitter::kFlagParentSpaceEmitOffset
			: Seoul::ParticleEmitter::kFlagNone;

		SetToCurve(
			m_pParticleEmitter->m_EmitOffset,
			ToSimpleCurve<Vector3D, ::FxStudio::FloatKeyFrameProperty>,
			"Emitter.Behavior.Emitter Offset",
			this);

		SetToCurve(
			m_pParticleEmitter->m_EmitterAcceleration,
			ToSimpleCurve<Vector4D, TwoCurves3And1>,
			"Emitter.Emitter Movement.Emitter Acceleration",
			"Emitter.Emitter Movement.Emitter Acceleration Along Velocity",
			this);

		m_pParticleEmitter->m_vEmitterVelocityAngleMinMax = EmitterVelocityAngleToVector2D(
			::FxStudio::FloatRangeProperty("Emitter.Emitter Movement.Emitter Velocity Angle Range", this).GetValue());

		m_pParticleEmitter->m_vInitialEmitterVelocityMagnitudeMinMax = FloatRangeToVector2D(
			::FxStudio::FloatRangeProperty("Emitter.Emitter Movement.Initial Emitter Velocity", this).GetValue());

		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Inherit Velocity", this).GetValue())
			? Seoul::ParticleEmitter::kFlagParticlesInheritEmitterVelocity
			: Seoul::ParticleEmitter::kFlagNone;

		// - Emitter Shape
		m_pParticleEmitter->m_eEmitterShape = ConvertEmitterShape(
			::FxStudio::IntegerProperty("Emitter.Behavior.Emitter Shape.Shape", this).GetValue());

		// - Rally Point
		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Rally Point.Use Rally Point", this).GetValue(false))
			? Seoul::ParticleEmitter::kFlagUseRallyPoint
			: Seoul::ParticleEmitter::kFlagNone;
		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Rally Point.Particle's Scale And Rotation Are Transform Independent", this).GetValue(false))
			? Seoul::ParticleEmitter::kFlagParticleScaleAndRotationTransformIndependant
			: Seoul::ParticleEmitter::kFlagNone;
		m_pParticleEmitter->m_vConfiguredRallyPoint =
			ConvertToVector3D(::FxStudio::Vector3Property("Emitter.Behavior.Rally Point.Configured Rally Point", this).GetValue());

		// -- Line Properties
		SetToCurve(
			m_pParticleEmitter->m_LineWidth,
			ToSimpleCurve<Float, ::FxStudio::FloatKeyFrameProperty>,
			"Emitter.Behavior.Emitter Shape.Line Properties.Width",
			this);

		// -- Box Properties
		SetToCurve(
			m_pParticleEmitter->m_BoxInnerDimensions,
			ToSimpleCurve<Vector3D, ::FxStudio::FloatKeyFrameProperty>,
			"Emitter.Behavior.Emitter Shape.Box Properties.Inner Dimensions",
			this);

		SetToCurve(
			m_pParticleEmitter->m_BoxOuterDimensions,
			ToSimpleCurve<Vector3D, ::FxStudio::FloatKeyFrameProperty>,
			"Emitter.Behavior.Emitter Shape.Box Properties.Outer Dimensions",
			this);

		// -- Sphere Properties
		SetToCurve(
			m_pParticleEmitter->m_SphereRadius,
			ToSimpleCurve<Vector2D, ::FxStudio::FloatKeyFrameProperty>,
			"Emitter.Behavior.Emitter Shape.Sphere Properties.Radius",
			this);

		m_pParticleEmitter->m_eEmitterShape = ConvertEmitterShape(
			::FxStudio::IntegerProperty("Emitter.Behavior.Emitter Shape.Shape", this).GetValue());

		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Align Offset to Emit Velocity", this).GetValue())
			? Seoul::ParticleEmitter::kFlagAlignOffsetToEmitVelocity
			: Seoul::ParticleEmitter::kFlagNone;

		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Emit Along Parent Velocity", this).GetValue())
			? Seoul::ParticleEmitter::kFlagEmitAlongOwnerVelocity
			: Seoul::ParticleEmitter::kFlagNone;

		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Random Initial Rotation", this).GetValue())
			? Seoul::ParticleEmitter::kFlagRandomInitialParticleRotation
			: Seoul::ParticleEmitter::kFlagNone;

		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Parent Space Emit Direction", this).GetValue())
			? Seoul::ParticleEmitter::kFlagParentSpaceEmitDirection
			: Seoul::ParticleEmitter::kFlagNone;

		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Snap Particles To Emitter Y", this).GetValue())
			? Seoul::ParticleEmitter::kFlagSnapParticlesToEmitterY
			: Seoul::ParticleEmitter::kFlagNone;

		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Emitter.Behavior.Snap Particles To Emitter Z", this).GetValue())
			? Seoul::ParticleEmitter::kFlagSnapParticlesToEmitterZ
			: Seoul::ParticleEmitter::kFlagNone;

		m_pParticleEmitter->m_uFlags |=
			(::FxStudio::BooleanProperty("Particle.Appearance.Align To Velocity", this).GetValue(false))
			? Seoul::ParticleEmitter::kFlagAlignParticlesToVelocity
			: Seoul::ParticleEmitter::kFlagNone;

		SetToCurve(
			m_pParticleEmitter->m_InitialRotation,
			Angle1DCurveToSimpleCurveRadians,
			"Emitter.Behavior.Initial Rotation",
			this);

		m_pParticleEmitter->m_eRotationAlignment =
			ConvertRotationAlignment(this);
	}
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
