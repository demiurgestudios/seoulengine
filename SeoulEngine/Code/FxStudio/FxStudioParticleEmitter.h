/**
 * \file FxStudioParticleEmitter.h
 * \brief Specialization of FxStudio::Component that implements a single particle emitter.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_PARTICLE_EMITTER_H
#define FX_STUDIO_PARTICLE_EMITTER_H

#include "FilePath.h"
#include "FxStudioComponentBase.h"
#include "FxStudioUtil.h"
#include "SimpleCurve.h"
#include "Texture.h"
#include "Vector.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul
{

// Forward declarations
namespace FxStudio { class ParticleEmitter; }
class Material;
class ParticleEmitter;
class ParticleEmitterInstance;
class ParticleMaterial;
class RenderCommandStreamBuilder;
class VertexBuffer;

namespace FxStudio
{

// Constant filename of the default shader Effect used for particle systems.
extern const char* kDefaultParticleEffectFilename;

/**
 * FxStudio::ParticleEmitter intergrates SeoulEngine::ParticleEmitterInstance and SeoulEngine::ParticleEmitter
 * into the FxStudio editor.
 */
class ParticleEmitter SEOUL_SEALED : public ComponentBase
{
public:
	/**
	 * @return Fix class name used in the FxStudio ComponentDefinition file.
	 */
	static Byte const* StaticTypeName()
	{
		return "ParticleEmitter";
	}

	typedef void (*AssetCallback)(void* pUserData, const char* sAssetID);
	static void GetAssets(::FxStudio::Component const* pComponent, AssetCallback assetCallback, void* pUserData);

	ParticleEmitter(Int iComponentIndex, const InternalDataType& internalData, FilePath filePath);
	virtual ~ParticleEmitter();

	// FxStudio::Component overrides
	virtual void Update(Float /*fDeltaTime*/) SEOUL_OVERRIDE;
	virtual void UpdateWhilePaused() SEOUL_OVERRIDE;
	virtual void Render(void* /*pRenderData*/) SEOUL_OVERRIDE;
	virtual void Activate() SEOUL_OVERRIDE;
	virtual void Reset() SEOUL_OVERRIDE;
	virtual Bool StayAlive(Float fDeltaTime, Bool bWithinLOD) SEOUL_OVERRIDE;
	virtual Bool OnPropertyChanged(Byte const* /* szPropertyName */) SEOUL_OVERRIDE;
	// /FxStudio::Component overrides

	// FxBaseComponent overrides
	virtual HString GetComponentTypeName() const SEOUL_OVERRIDE;
	// /FxBaseComponent overrides

	/** @return True if this emitter is part of FxStudio preview playback, false otherwise. */
	Bool IsPartOfPreviewFx() const
	{
		return !m_SharedDataGUID.m_FilePath.IsValid();
	}

	void Preload(Bool bForce = false);

	// FxStudio::ParticleEmitter needs Render calls.
	virtual Bool NeedsRender() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool GetParticleRallyPointState(Vector3D& rvPoint, Bool& rbUseRallyPoint) const SEOUL_OVERRIDE;
	virtual void SetParticleRallyPointOverride(const Vector3D& vPoint) SEOUL_OVERRIDE;

	// Common method for all components, updates configuration
	// flags of the component.
	virtual void SetFlags(UInt32 uFlags) SEOUL_OVERRIDE
	{
		m_uFlags = uFlags;
	}

	virtual void SetPosition(const Vector3D& vPosition) SEOUL_OVERRIDE;
	virtual void SetGravity(Float fGravityAcceleration) SEOUL_OVERRIDE;
	virtual void SetTransform(const Matrix4D& mTransform) SEOUL_OVERRIDE;
	virtual void SetParentIfWorldspace(const Matrix4D& mTransform) SEOUL_OVERRIDE;

	// Particle emitters activate when prewarming.
	virtual bool ActivateDuringPrewarm() const SEOUL_OVERRIDE
	{
		return true;
	}

	const ParticleEmitterSharedDataGUID& GetSharedDataGUID() const
	{
		return m_SharedDataGUID;
	}

private:
	Matrix4D m_mTransform;
	Matrix4D m_mParentIfWorldspaceTransform;
	FilePath m_FilePath;
	Float m_fTimeAccumulator;
	UInt32 m_uFlags;
	FxRendererMode m_eFxRendererMode;
	Bool m_bNeedsScreenAlign;
	Float m_fGravityAcceleration;

	void InternalUpdate(Float fDeltaTime);

	SharedPtr<ParticleEmitterInstance> m_pParticleEmitterInstance;
	SharedPtr<Seoul::ParticleEmitter> m_pParticleEmitter;
	ParticleEmitterSharedDataGUID m_SharedDataGUID;

	Bool InternalCheckAlphaClamp() const;
	Bool InternalCheckTextureHasNoPreMultiply() const;
	void InternalSetupParticleMaterial();
	void InternalPreload();

	const Matrix4D& InternalGetTransform() const
	{
		return m_mTransform;
	}

	void InternalApplyRallyPointOverride();

	Vector3D m_vParticleRallyPointOverride;
	Bool m_bPendingApplyRallyPointOverride;
	Bool m_bHasRallyPointOverride;

	SEOUL_DISABLE_COPY(ParticleEmitter);
}; // class FxStudio::ParticleEmitter

} // namespace FxStudio

} // namespace Seoul

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
