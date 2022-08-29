/**
 * \file FxStudioPlaySoundEffect.cpp
 * \brief Specialization of FxStudio::Component that implements an FxStudio component
 * that can play a SoundEvent.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStore.h"
#include "DataStoreParser.h"
#include "FxStudioPlaySoundEffect.h"
#include "FxStudioUtil.h"
#include "Logger.h"
#include "Matrix4D.h"
#include "SeoulTime.h"
#include "SoundManager.h"

#if SEOUL_WITH_FX_STUDIO

namespace Seoul::FxStudio
{

PlaySoundEffect::PlaySoundEffect(
	Int iComponentIndex,
	const InternalDataType& internalData,
	FilePath filePath)
	: ComponentBase(internalData)
	, m_SoundEventProp("Sound Event", this)
	, m_Enable3DSoundProp("Enable 3D Sound", this)
	, m_StopProp("Stop At Deactivate", this)
	, m_FilePath(filePath)
	, m_pSoundEvent(Sound::Manager::Get()->NewSoundEvent())
	, m_vPosition(Vector3D::Zero())
	, m_bPendingStart(false)
	, m_iStartTimeInTicks(-1)
{
	// "Trigger a change" to preload the sound event.
	OnPropertyChanged(m_SoundEventProp.GetPropertyName());
}

PlaySoundEffect::~PlaySoundEffect()
{
}

void PlaySoundEffect::Update(float fDeltaTime)
{
	if (m_bPendingStart && !m_pSoundEvent->IsPlaying())
	{
		// Compute an offset if m_iStartTimeInTicks >= 0. If it is not,
		// then this is the first attempt and we want no offset.
		Int32 iOffset = 0;
		if (m_iStartTimeInTicks >= 0)
		{
			iOffset = (Int32)SeoulTime::ConvertTicksToMilliseconds(
				SeoulTime::GetGameTimeInTicks() - m_iStartTimeInTicks);
		}

		m_bPendingStart = !m_pSoundEvent->Start(m_vPosition, Vector3D::Zero(), false, iOffset);
	}

	// If the sound event is playing, update its 3D attributes.
	if (m_pSoundEvent->IsPlaying() && m_Enable3DSoundProp.GetValue())
	{
		m_pSoundEvent->Set3DAttributes(m_vPosition, Vector3D::Zero());
	}
}

void PlaySoundEffect::Activate()
{
	m_bPendingStart = !m_pSoundEvent->Start(m_vPosition, Vector3D::Zero());
	m_iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
}

void PlaySoundEffect::Deactivate()
{
	// Stop the sound on deactivate if the sound
	// is either looping or is explicitly marked
	// to be stopped.
	if (m_StopProp.GetValue() || m_pSoundEvent->StopOnDestruction())
	{
		m_pSoundEvent->Stop();
		m_bPendingStart = false;
	}
}

void PlaySoundEffect::OnPause(bool bPause)
{
	m_pSoundEvent->Pause(bPause);
}

Bool PlaySoundEffect::OnPropertyChanged(const char* szPropertyName)
{
	if (!IsProperty(m_SoundEventProp, szPropertyName))
	{
		return false;
	}

	Byte const* sValue = m_SoundEventProp.GetValue();

	DataStore dataStore;
	if (!DataStoreParser::FromString(sValue, dataStore))
	{
		SEOUL_WARN("%s: failed parsing sound event property '%s'", m_FilePath.CStr(), sValue);
		return false;
	}

	ContentKey key;
	if (!key.SetFromDataStore(dataStore, dataStore.GetRootNode()))
	{
		SEOUL_WARN("%s: sound event property '%s' is valid syntax but invalid format for sound event key.", m_FilePath.CStr(), sValue);
		return false;
	}

	Sound::Manager::Get()->AssociateSoundEvent(key, *m_pSoundEvent);
	return false;
}

void PlaySoundEffect::SetPosition(const Vector3D& vPosition)
{
	m_vPosition = vPosition;
}

void PlaySoundEffect::SetTransform(const Matrix4D& mTransform)
{
	m_vPosition = mTransform.GetTranslation();
}

HString PlaySoundEffect::GetComponentTypeName() const
{
	return HString(PlaySoundEffect::StaticTypeName());
}

} // namespace Seoul::FxStudio

#endif // /#if SEOUL_WITH_FX_STUDIO
