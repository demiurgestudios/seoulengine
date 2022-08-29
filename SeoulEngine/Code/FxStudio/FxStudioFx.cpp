/**
 * \file FxStudioFx.cpp
 * \brief Equivalent to the Sound::Event in the audio system, FxStudio::Fx is
 * a concrete sublcass of SeoulEngine Fx for the FxStudio integration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FxStudioComponentBase.h"
#include "FxStudioFx.h"
#include "FxStudioManager.h"
#include "FxStudioSettings.h"
#include "ThreadId.h"

#if SEOUL_WITH_FX_STUDIO
#include "FxStudioRT.h"

namespace Seoul::FxStudio
{

static const HString kSettingsComponentTypeName = HString("Settings");

Fx::Fx(const Content::Handle<BankFile>& hBankFile)
	: m_hBankFile(hBankFile)
	, m_FxInstance()
{
}

Fx::~Fx()
{
	if (m_FxInstance.IsValid())
	{
		SEOUL_ASSERT(IsMainThread());

		// We need to stop the effect on destruction, since it is not
		// auto-play or update, and once we're gone, this instance is gone.
		m_FxInstance.SetPlayToEnd(true);
		m_FxInstance.Stop(true);
	}
}

/**
 * @return A new FxStudio::Fx instance that is an exact copy of this FxStudio::Fx instance.
 */
Fx* Fx::Clone() const
{
	return SEOUL_NEW(MemoryBudgets::Fx) Fx(m_hBankFile);
}

/** Return true if the Fx is currently playing, false otherwise. */
Bool Fx::IsPlaying() const
{
	if (m_FxInstance.IsValid())
	{
		return m_FxInstance.IsPlaying();
	}

	return false;
}

/** @return Assets that are used by this Fx. */
Bool Fx::AppendAssets(FxAssetsVector& rvAssets) const
{
	SEOUL_ASSERT(IsMainThread());

	SharedPtr<BankFile> pBankFile(m_hBankFile.GetPtr());
	if (!pBankFile.IsValid())
	{
		return false;
	}

	pBankFile->AppendAssetsOfFx(rvAssets);
	return true;
}

/**
 * Begin playing this Fx instance.
 */
Bool Fx::Start(const Matrix4D& mTransform, UInt32 uFlags)
{
	SEOUL_ASSERT(IsMainThread());

	// If the bank file is not loaded yet or failed to load, fail to start.
	SharedPtr<BankFile> pBankFile(m_hBankFile.GetPtr());
	if (!pBankFile.IsValid())
	{
		return false;
	}

	// We need to stop the existing effect (if defined) before
	// instantiating a new instance, since it is not auto-play
	// or auto-update, and once we're gone, this instance is gone.
	m_FxInstance.SetPlayToEnd(true);
	m_FxInstance.Stop(true);

	// Create a new fx instance - we don't need to do anything
	// more with an existing instance before overwriting it.
	m_FxInstance = pBankFile->CreateFx();
	m_FxInstance.SetAutoRender(false);
	m_FxInstance.SetAutoUpdate(false);

	// If the new instance is valid, set its initial position, and then start playing it.
	if (m_FxInstance.IsValid())
	{
		SetFlags(uFlags);
		SetTransform(mTransform);

		m_FxInstance.Play();
		if (ShouldPrewarm())
		{
			m_FxInstance.PrewarmIfLoopingFx();
		}

		return true;
	}

	return false;
}

/**
 * Stop this FxStudio Fx instance at its current position on the timeline.
 */
void Fx::Pause(Bool bPause /* = true */)
{
	SEOUL_ASSERT(IsMainThread());

	// If the instance is actively playing, pause it.
	if (m_FxInstance.IsPlaying())
	{
		m_FxInstance.Pause();
	}
	// If the instance is actively paused, play it to unpause it.
	else if (m_FxInstance.IsPaused())
	{
		m_FxInstance.Play();
	}
}

/**
 * Stop playback of this FxStudio::Fx instance - forces the stop
 * if bStopImmediately is true.
 */
void Fx::Stop(Bool bStopImmediately /* = false */)
{
	SEOUL_ASSERT(IsMainThread());

	// Tell the instance to stop.
	m_FxInstance.Stop(bStopImmediately);

	// Clear our handle if bStopImmediately is true.
	if (bStopImmediately)
	{
		m_FxInstance.Clear();
	}
}

/**
 * Update flags used to control component behavior - applied to
 * all components of this FxStudioFx.
 */
void Fx::SetFlags(UInt32 uFlags)
{
	SEOUL_ASSERT(IsMainThread());

	// Commit the position
	for (::FxStudio::ComponentData const* p = m_FxInstance.BeginComponents();
		m_FxInstance.EndComponents() != p;
		++p)
	{
		ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
		pBase->SetFlags(uFlags);
	}
}

/**
 * Update the world position of this Fx instance.
 */
Bool Fx::SetPosition(const Vector3D& vPosition)
{
	SEOUL_ASSERT(IsMainThread());

	// Commit the position
	for (::FxStudio::ComponentData const* p = m_FxInstance.BeginComponents();
		m_FxInstance.EndComponents() != p;
		++p)
	{
		ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
		pBase->SetPosition(vPosition);
	}

	return true;
}

/**
 * Update gravity for this Fx instance
 */
Bool Fx::SetGravity(Float fGravityAcceleration)
{
	SEOUL_ASSERT(IsMainThread());

	// Commit the position
	for (::FxStudio::ComponentData const* p = m_FxInstance.BeginComponents();
		m_FxInstance.EndComponents() != p;
		++p)
	{
		ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
		pBase->SetGravity(fGravityAcceleration);
	}

	return true;
}

/**
 * Update the rally point of this Fx instance.
 */
Bool Fx::SetRallyPoint(const Vector3D& vRallyPoint)
{
	SEOUL_ASSERT(IsMainThread());

	// Commit the position
	for (::FxStudio::ComponentData const* p = m_FxInstance.BeginComponents();
		m_FxInstance.EndComponents() != p;
		++p)
	{
		ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
		pBase->SetParticleRallyPointOverride(vRallyPoint);
	}

	return true;
}

/**
 * Update the full 3D transform of this Fx.
 */
Bool Fx::SetTransform(const Matrix4D& mTransform)
{
	SEOUL_ASSERT(IsMainThread());

	// Commit the position
	for (::FxStudio::ComponentData const* p = m_FxInstance.BeginComponents();
		m_FxInstance.EndComponents() != p;
		++p)
	{
		ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
		pBase->SetTransform(mTransform);
	}

	return true;
}

/**
* Update the parent to use if in worldspace
*/
Bool Fx::SetParentIfWorldspace(const Matrix4D& mTransform)
{
	SEOUL_ASSERT(IsMainThread());

	// Commit the position
	for (::FxStudio::ComponentData const* p = m_FxInstance.BeginComponents();
		 m_FxInstance.EndComponents() != p;
		 ++p)
	{
		ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
		pBase->SetParentIfWorldspace(mTransform);
	}

	return true;
}

/**
 * Drawing support.
 */
void Fx::Draw(IFxRenderer& rRenderer)
{
	SEOUL_ASSERT(IsMainThread());

	// Draw
	m_FxInstance.Render(&rRenderer);
}

/**
 * Updating support.
 */
void Fx::Tick(Float fDeltaTimeInSeconds)
{
	SEOUL_ASSERT(IsMainThread());

	// Update
	m_FxInstance.Update(fDeltaTimeInSeconds);
}

/**
 * Returning properties about the FX
 */
Bool Fx::GetProperties(FxProperties& rProperties) const
{
	SEOUL_ASSERT(IsMainThread());

	auto pBankFile(m_hBankFile.GetPtr());
	if (!pBankFile.IsValid())
	{
		return false;
	}

	pBankFile->GetProperties(rProperties);
	return true;
}

/**
 * @return true if this Fx needs calls to Render or not.
 *
 * May be O(n), should be cached if evaluation time is important.
 */
Bool Fx::NeedsRender() const
{
	SEOUL_ASSERT(IsMainThread());

	// Check for render.
	for (::FxStudio::ComponentData const* p = m_FxInstance.BeginComponents();
		m_FxInstance.EndComponents() != p;
		++p)
	{
		ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
		if (pBase->NeedsRender())
		{
			return true;
		}
	}

	return false;
}

/**
 * Look for a Settings component and check for pre-warm being set
 */
Bool Fx::ShouldPrewarm() const
{
	for (::FxStudio::ComponentData const* p = m_FxInstance.BeginComponents();
		m_FxInstance.EndComponents() != p;
		++p)
	{
		ComponentBase* pBase = static_cast<ComponentBase*>(p->m_pComponent);
		if (kSettingsComponentTypeName == pBase->GetComponentTypeName())
		{
			Settings* pSettings = static_cast<Settings*>(pBase);
			return pSettings->ShouldPrewarm();
		}
	}

	return false;
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
