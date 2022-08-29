/**
 * \file FxStudioManager.cpp
 * \brief Equivalent to the Sound::Manager in the audio system, FxStudio::Manager is
 * a concrete sublcass of SeoulEngine FxManager for the FxStudio integration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "CookManager.h"
#include "Engine.h"
#include "FxStudioAllocator.h"
#include "FxStudioErrorHandler.h"
#include "FxStudioFactory.h"
#include "FxStudioFx.h"
#include "FxStudioManager.h"
#include "FxStudioParticleEmitter.h"
#include "FxStudioPlaySoundEffect.h"
#include "FxStudioPreview.h"
#include "FxStudioScreenShakeEffect.h"
#include "FxStudioSettings.h"
#include "EventsManager.h"
#include "ThreadId.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

// TODO: Possibly a temporary location for this - need to avoid linker
// stripping like in the reflection system (which is accomplished
// with the SEOUL_LINK_ME() macro in that case).
Factory<ParticleEmitter>	g_FxStudioParticleEmitterFactory;
Factory<PlaySoundEffect>	g_FxStudioPlaySoundEffectFactory;
Factory<ScreenShakeEffect>  g_FxStudioScreenShakeEffect;
Factory<Settings>			g_FxStudioSettings;

Manager::Manager()
	: m_Banks(false)
	, m_pErrorHandler(SEOUL_NEW(MemoryBudgets::Fx) ErrorHandler())
	, m_pAllocator(SEOUL_NEW(MemoryBudgets::Fx) Allocator())
	, m_pManager(::FxStudio::CreateManager())
	, m_pFxStudioPreview(nullptr)
{
	SEOUL_ASSERT(IsMainThread());

	// If in a non-ship or profiling build, instantiate the FxStudio preview handler.
#if !SEOUL_SHIP || defined(SEOUL_PROFILING_BUILD)
	m_pFxStudioPreview.Reset(SEOUL_NEW(MemoryBudgets::Fx) Preview);
	m_pFxStudioPreview->Start(::FxStudio::Preview::DefaultPort);
#endif
}

Manager::~Manager()
{
	SEOUL_ASSERT(IsMainThread());

	// Stop any active effects
	m_pManager->ReleaseAllFx();

	// Empty the bank content before shutting down the manager.
	SEOUL_VERIFY(m_Banks.Clear());

	// Destroy the preview handler.
	m_pFxStudioPreview.Reset();

	::FxStudio::Manager* pManager = m_pManager.Get();
	m_pManager.Reset();

	::FxStudio::ReleaseManager(pManager);
}

/**
 * Instantiate a new Fx instance for the FxStudio bank at filePath.
 *
 * @return True on success, false otherwise. If this method
 * returns true, rpInstance will contain a non-null Fx instance.
 *
 * If rpInstance is non-null on input, the previous value will
 * be destroyed after assigning a new value to rpInstance.
 */
Bool Manager::GetFx(FilePath filePath, Seoul::Fx*& rpInstance)
{
	Seoul::Fx* pInstance = SEOUL_NEW(MemoryBudgets::Fx) Fx(m_Banks.GetContent(filePath));
	Seoul::Swap(pInstance, rpInstance);
	SafeDelete(pInstance);
	return true;
}

/** @return The current state of the Fx preview system. */
Bool Manager::GetFxPreviewModeState(FxPreviewModeState& rState) const
{
	if (m_pFxStudioPreview.IsValid() && m_pFxStudioPreview->GetPreviewFx().IsValid())
	{
		rState.m_bActive = true;
		rState.m_vPosition = m_pFxStudioPreview->GetSpawnPosition();
		return true;
	}
	else
	{
		return false;
	}
}

/** Equivalent to GetFx() but only prefetches the content. */
void Manager::Prefetch(FilePath filePath)
{
	(void)m_Banks.GetContent(filePath);
}

/** @return True if an fx preview is active, false otherwise. */
Bool Manager::IsPreviewFxValid() const
{
	return m_pFxStudioPreview.IsValid() && m_pFxStudioPreview->GetPreviewFx().IsValid();
}

/**
 * Call once per frame to render the preview Fx, if it
 * is active.
 */
void Manager::RenderPreviewFx(IFxRenderer& rRenderer)
{
	// Update the preview handler if it is defined.
	if (m_pFxStudioPreview.IsValid() && m_pFxStudioPreview->GetPreviewFx().IsValid())
	{
		m_pFxStudioPreview->Render(&rRenderer);
	}
}

/** Update the camera to be used for rendering preview FX. */
void Manager::SetPreviewFxCamera(const SharedPtr<Camera>& pCamera)
{
	if (m_pFxStudioPreview.IsValid())
	{
		m_pFxStudioPreview->SetPreviewFxCamera(pCamera);
	}
}

/**
 * Update the flags applied to the preview fx - typically
 * used to control rendering behavior such as mirroring, but the exact
 * meaning is FX instance specific.
 */
void Manager::SetPreviewFxFlags(UInt32 uFlags)
{
	if (m_pFxStudioPreview.IsValid())
	{
		m_pFxStudioPreview->SetPreviewFxFlags(uFlags);
	}
}

/**
 * Update the world position that is being used for the
 * preview FX.
 */
void Manager::SetPreviewFxPosition(const Vector3D& vPosition)
{
	// A nop if we don't have a preview handler.
	if (m_pFxStudioPreview.IsValid())
	{
		m_pFxStudioPreview->SetPreviewFxPosition(vPosition);
	}
}

/**
 * Parenting transform of the preview FX. In addition
 * to the position.
 */
void Manager::SetPreviewFxTransform(const Matrix4D& mTransform)
{
	// A nop if we don't have a preview handler.
	if (m_pFxStudioPreview.IsValid())
	{
		m_pFxStudioPreview->SetPreviewFxTransform(mTransform);
	}
}

/**
 * Call once per frame to update the preview Fx, if it
 * is active.
 */
void Manager::UpdatePreviewFx(Float fDeltaTimeInSeconds)
{
	if (m_pFxStudioPreview.IsValid())
	{
		m_pFxStudioPreview->Update(fDeltaTimeInSeconds);
	}
}

/**
 * Check that bankFilePath can be deleted.
 *
 * @return True if it is safe to destroy the data associated with bankFilePath,
 * false otherwise.
 */
Bool Manager::PrepareDelete(FilePath bankFilePath)
{
	SEOUL_ASSERT(IsMainThread());

	// For each bank being unloaded, walk the cache particle emitter
	// data and free any emitters that are associated with the bank.
	Vector<ParticleEmitterSharedDataGUID, MemoryBudgets::Fx> vToErase;
	for (auto i = m_tParticleEmitters.Begin();
		m_tParticleEmitters.End() != i;
		++i)
	{
		if (i->First.m_FilePath == bankFilePath)
		{
			vToErase.PushBack(i->First);
		}
	}

	UInt32 const nToErase = vToErase.GetSize();
	for (UInt32 i = 0u; i < nToErase; ++i)
	{
		SEOUL_VERIFY(m_tParticleEmitters.Erase(vToErase[i]));
	}

	return true;
}

/**
 * Called by FxStudio::ContentLoader when an FxStudio bank has been
 * reloaded - the data is expected to already be replaced when this method
 * is called.
 */
void Manager::OnBankReloaded(FilePath bankFilePath)
{
	SEOUL_ASSERT(IsMainThread());

	// For each bank being unloaded, walk the cache particle emitter
	// data and free any emitters that are associated with the bank.
	Vector<ParticleEmitterSharedDataGUID, MemoryBudgets::Fx> vToErase;
	for (ParticleEmitters::Iterator i = m_tParticleEmitters.Begin();
		m_tParticleEmitters.End() != i;
		++i)
	{
		// If an emitter uses this bank, check the emitter data.
		if (i->First.m_FilePath == bankFilePath)
		{
			// If the emitter data is not in use, remove it from the factory
			// so it will be recreated. Otherwise, it will be recreated
			// if any properties have been changed in active effects.
			if (i->Second.IsUnique())
			{
				vToErase.PushBack(i->First);
			}
		}
	}

	UInt32 const nToErase = vToErase.GetSize();
	for (UInt32 i = 0u; i < nToErase; ++i)
	{
		SEOUL_VERIFY(m_tParticleEmitters.Erase(vToErase[i]));
	}
}

/**
 * Performs per-frame updat operations for FxStudioManager.
 */
void Manager::Tick(Float fDeltaTimeInSeconds)
{
	// Update the FxStudio::Manager instance.
	m_pManager->Update(fDeltaTimeInSeconds);

	// Cleanup any emitters that are no longer referenced outside
	// of the manager.
	{
		FixedArray<ParticleEmitterSharedDataGUID, 4> aToRemove;
		Bool bDone = false;
		while (!bDone)
		{
			bDone = true;
			UInt32 nToRemove = 0u;

			for (auto i = m_tParticleEmitters.Begin();
				m_tParticleEmitters.End() != i;
				++i)
			{
				if (i->Second.IsUnique())
				{
					if (nToRemove == aToRemove.GetSize())
					{
						bDone = false;
						break;
					}

					aToRemove[nToRemove++] = i->First;
				}
			}

			for (UInt32 i = 0u; i < nToRemove; ++i)
			{
				m_tParticleEmitters.Erase(aToRemove[i]);
			}
		}
	}
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
