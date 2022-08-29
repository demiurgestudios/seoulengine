/**
 * \file FxStudioManager.h
 * \brief Equivalent to the Sound::Manager in the audio system, FxStudio::Manager is
 * a concrete sublcass of SeoulEngine FxManager for the FxStudio integration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_MANAGER_H
#define FX_STUDIO_MANAGER_H

#include "HashTable.h"
#include "FxManager.h"
#include "FxStudioBankFile.h"
#include "IPoseable.h"
#include "ParticleEmitter.h"
#include "ScopedPtr.h"
namespace Seoul { namespace Content { class Loader; } }

#if SEOUL_WITH_FX_STUDIO

// Forward declarations
namespace FxStudio
{
	class Manager;
}

namespace Seoul
{

// Forward declarations
class Camera;
class IFxRenderer;
namespace FxStudio { class Allocator; }
namespace FxStudio { class ErrorHandler; }
namespace FxStudio { class Preview; }

/**
 * GUID for the shared data used by FxParticleEmitters,
 * to connect FxStudio shared data to Seoul ParticleEmitter shared
 * data.
 */
struct ParticleEmitterSharedDataGUID
{
	FilePath m_FilePath;
	UInt32 m_uComponentIndex;

	ParticleEmitterSharedDataGUID()
		: m_FilePath()
		, m_uComponentIndex(0u)
	{
	}

	/**
	 * Construct a ParticleEmitterSharedDataGUID from data that describes it.
	 * filePath is an fx filePath to the Effect that contains the emitter, and
	 * uComponentIndex is the ordered index of the ParticleEmitter
	 * in the FxStudio effect.
	 */
	static ParticleEmitterSharedDataGUID Create(FilePath filePath, UInt32 uComponentIndex)
	{
		ParticleEmitterSharedDataGUID ret;
		ret.m_FilePath = filePath;
		ret.m_uComponentIndex = uComponentIndex;
		return ret;
	}

	Bool operator==(const ParticleEmitterSharedDataGUID& b) const
	{
		return (
			m_FilePath == b.m_FilePath &&
			m_uComponentIndex == b.m_uComponentIndex);
	}

	Bool operator!=(const ParticleEmitterSharedDataGUID& b) const
	{
		return !(*this == b);
	}

	UInt32 GetHash() const
	{
		UInt32 uReturn = 0u;
		IncrementalHash(uReturn, m_FilePath.GetHash());
		IncrementalHash(uReturn, Seoul::GetHash(m_uComponentIndex));
		return uReturn;
	}

	Bool IsValid() const
	{
		return (m_FilePath.IsValid());
	}
}; // struct ParticleEmitterSharedDataGUID

inline UInt32 GetHash(const ParticleEmitterSharedDataGUID& guid)
{
	return guid.GetHash();
}

template<>
struct DefaultHashTableKeyTraits<ParticleEmitterSharedDataGUID>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static ParticleEmitterSharedDataGUID GetNullKey()
	{
		return ParticleEmitterSharedDataGUID();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

namespace FxStudio
{

/**
 * FxStudio::Manager owns FxStudio bank file content and implements
 * polymorphic methods of FxManager, to allow for the creation of
 * Fx instances.
 */
class Manager SEOUL_SEALED : public FxManager
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<Manager> Get()
	{
		return static_cast<Manager*>(FxManager::Get().Get());
	}

	Manager();
	~Manager();

	virtual Bool GetFx(FilePath filePath, Seoul::Fx*& rpInstance) SEOUL_OVERRIDE;

	virtual Bool GetFxPreviewModeState(FxPreviewModeState& rState) const SEOUL_OVERRIDE;

	virtual void Prefetch(FilePath filePath) SEOUL_OVERRIDE;

	virtual void Tick(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

	/**
	 * @return The global FxStudio::Manager instance, for internal use only.
	 */
	::FxStudio::Manager& GetFxStudioManager()
	{
		return *m_pManager;
	}

	Bool GetParticleEmitter(
		const ParticleEmitterSharedDataGUID& guid,
		SharedPtr<ParticleEmitter>& rpParticleEmitter)
	{
		return m_tParticleEmitters.GetValue(guid, rpParticleEmitter);
	}

	/**
	 * Associates a ParticleEmitter handle with a sharedID. Allows
	 * FxStudio::Components to find the emitter data that they use and share
	 * amongst themselves.
	 */
	Bool RegisterParticleEmitter(
		const ParticleEmitterSharedDataGUID& guid,
		const SharedPtr<ParticleEmitter>& particleEmitter)
	{
		if (guid.IsValid())
		{
			return (m_tParticleEmitters.Insert(guid, particleEmitter).Second);
		}

		return false;
	}

	// True if an fx preview is currently active, false otherwise.
	virtual Bool IsPreviewFxValid() const SEOUL_OVERRIDE;

	// Call once per frame to render/update the preview Fx, if it
	// is active.
	virtual void RenderPreviewFx(IFxRenderer& rRenderer) SEOUL_OVERRIDE;

	// Update the camera to be used for rendering preview FX.
	virtual void SetPreviewFxCamera(const SharedPtr<Camera>& pCamera) SEOUL_OVERRIDE;

	// Update the flags used for preview FX.
	virtual void SetPreviewFxFlags(UInt32 uFlags) SEOUL_OVERRIDE;

	// Update the spawn position of the preview FX.
	virtual void SetPreviewFxPosition(const Vector3D& vPosition) SEOUL_OVERRIDE;

	// Parenting transform of the preview FX. In addition
	// to the position.
	virtual void SetPreviewFxTransform(const Matrix4D& mTransform) SEOUL_OVERRIDE;

	// Call once per frame to update the preview Fx, if it
	// is active.
	virtual void UpdatePreviewFx(Float fDeltaTimeInSeconds) SEOUL_OVERRIDE;

private:
	typedef HashTable<ParticleEmitterSharedDataGUID, SharedPtr<ParticleEmitter>, MemoryBudgets::Particles> ParticleEmitters;
	ParticleEmitters m_tParticleEmitters;

	friend struct Content::Traits<BankFile>;
	Bool PrepareDelete(FilePath bankFilePath);

	friend class ContentLoader;
	void OnBankReloaded(FilePath bankFilePath);

	Content::Store<BankFile> m_Banks;
	ScopedPtr<ErrorHandler> m_pErrorHandler;
	ScopedPtr<Allocator> m_pAllocator;
	CheckedPtr<::FxStudio::Manager> m_pManager;
	ScopedPtr<Preview> m_pFxStudioPreview;

	SEOUL_DISABLE_COPY(Manager);
}; // class FxStudio::Manager

} // namespace FxStudio

} // namespace Seoul

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
