/**
 * \file FxStudioPlaySoundEffect.h
 * \brief Specialization of FxStudio::Component that implements an FxStudio component
 * that can play a SoundEvent.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FX_STUDIO_PLAY_SOUND_EFFECT_H
#define FX_STUDIO_PLAY_SOUND_EFFECT_H

#include "FxStudioComponentBase.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SoundEvent.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

class PlaySoundEffect SEOUL_SEALED : public ComponentBase
{
public:
	/**
	 * @return Fix class name used in the FxStudio ComponentDefinition file.
	 */
	static Byte const* StaticTypeName()
	{
		return "PlaySoundEffect";
	}

	typedef void (*AssetCallback)(void* pUserData, const char* sAssetID);
	static void GetAssets(::FxStudio::Component const* pComponent, AssetCallback assetCallback, void* pUserData)
	{
		// Nop;
	}

	PlaySoundEffect(Int iComponentIndex, const InternalDataType& internalData, FilePath filePath);
	virtual ~PlaySoundEffect();

	// ::FxStudio::Component overrides
	virtual void Update( Float /* fDeltaTime */ ) SEOUL_OVERRIDE;
	virtual void Activate() SEOUL_OVERRIDE;
	virtual void Deactivate() SEOUL_OVERRIDE;
	virtual void OnPause(bool bPause) SEOUL_OVERRIDE;
	virtual Bool OnPropertyChanged(const char* /* szPropertyName */) SEOUL_OVERRIDE;
	// /::FxStudio::Component overrides

	// FxBaseComponent overrides
	virtual void SetPosition(const Vector3D& vPosition) SEOUL_OVERRIDE;
	virtual void SetTransform(const Matrix4D& mTransform) SEOUL_OVERRIDE;
	virtual HString GetComponentTypeName() const SEOUL_OVERRIDE;
	// /FxBaseComponent overrides

protected:
	::FxStudio::StringProperty m_SoundEventProp;
	::FxStudio::BooleanProperty m_Enable3DSoundProp;
	::FxStudio::BooleanProperty m_StopProp;

	FilePath m_FilePath;
	ScopedPtr<Sound::Event> m_pSoundEvent;
	Vector3D m_vPosition;
	Bool m_bPendingStart;
	Int64 m_iStartTimeInTicks;

	Bool InternalUpdateModeProp();
	Bool InternalUpdateAudioProp();
	Bool InternalStopSound();
	Bool InternalPauseSound(Bool bPause = true);
	Bool InternalGetPositionAndVelocity(Vector3D& rvPosition, Vector3D& rvVelocity) const;
}; // class FxStudio::PlaySoundEffect

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO

#endif // include guard
