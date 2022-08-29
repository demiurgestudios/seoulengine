/**
 * \file FMODSoundEvent.cpp
 * \brief A sound event can be thought of as a sound effect with more
 * flexibility and complexity. A single sound event can contain multiple
 * raw wave files, various audio processing, and sound events can also
 * react to runtime variables and modify their behavior based on changes
 * to these variables.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FMODSoundEvent.h"
#include "FMODSoundManager.h"
#include "FMODSoundUtil.h"
#include "Logger.h"
#include "Vector3D.h"

#if SEOUL_WITH_FMOD
#include "fmod_studio.h"
#include "fmod_studio.hpp"

namespace Seoul::FMODSound
{

/*
 * Helper method, convert FMOD_STUDIO_PARAMETER_ID to raw UInt32 values.
 */
inline void ParameterIdToValues(const FMOD_STUDIO_PARAMETER_ID& id, UInt32& ru1, UInt32& ru2)
{
	SEOUL_STATIC_ASSERT(sizeof(id) == sizeof(UInt32) * 2);
	ru1 = id.data1;
	ru2 = id.data2;
}

/*
 * Helper method, convert raw UInt32 values to FMOD_STUDIO_PARAMETER_ID.
 */
inline void ValuesToParameterId(UInt32 const u1, UInt32 const u2, FMOD_STUDIO_PARAMETER_ID& rId)
{
	SEOUL_STATIC_ASSERT(sizeof(rId) == sizeof(UInt32) * 2);
	rId.data1 = u1;
	rId.data2 = u2;
}

/**
 * Initialize this FMODSound::Event to a default, invalid state.
 */
Event::Event()
	: m_hAnchor()
	, m_pFMODEventInstance()
	, m_CachedParameterName()
	, m_uCachedParameterId1(0u)
	, m_uCachedParameterId2(0u)
{
}

/**
 * Destroys this FMODSoundEvent. If this FMODSound::Event
 * is currently playing and was started with the argument bStopOnDestruction
 * set to true (or if this is a looping sound), then this FMODSound::Event
 * will be stopped. Otherwise, it will finish playing its current duration.
 */
Event::~Event()
{
	SEOUL_ASSERT(IsMainThread());

	if ((m_uFlags & kStopOnDestruction) != 0u)
	{
		InternalSetAnchor(Content::Handle<EventAnchor>());
	}
	else
	{
		InternalResetHandle();
	}
}

/**
 * Initialize a clone of this and return it.
 *
 * While this FMODSound::Event will be an instance of the same
 * FMODSound::Event as b, it does not inherit any of the parameters set
 * to b and will not be in the same state (i.e. playing) as
 * b once this copy constructor has completed.
 */
Sound::Event* Event::Clone() const
{
	Event* pReturn = SEOUL_NEW(MemoryBudgets::Audio) Event;
	pReturn->m_hAnchor = m_hAnchor;
	pReturn->m_pFMODEventInstance.Reset(); // Deliberate, don't clone the active event.
	pReturn->m_uFlags = m_uFlags;
	pReturn->m_CachedParameterName = m_CachedParameterName;
	pReturn->m_uCachedParameterId1 = m_uCachedParameterId1;
	pReturn->m_uCachedParameterId2 = m_uCachedParameterId2;
	return pReturn;
}

/** @return The length of the event, in seconds. */
Bool Event::GetLengthInMilliseconds(Int32& riLengthMs) const
{
	if (IsLoading())
	{
		return false;
	}

	// Resolve the anchor and its description.
	auto pAnchor(m_hAnchor.GetPtr());
	auto pDescription(pAnchor.IsValid() ? pAnchor->ResolveDescription() : nullptr);
	if (!pDescription.IsValid())
	{
		return false;
	}

	// Query.
	return (FMOD_OK == pDescription->getLength(&riLengthMs));
}

/** @return The current timeline position of the event, in milliseconds. Must be playing to succeed. */
Int32 Event::GetTimelinePositionInMilliseconds() const
{
	if (!m_pFMODEventInstance.IsValid())
	{
		return 0;
	}

	// Query.
	Int32 iPosition = 0;
	if (FMOD_OK != m_pFMODEventInstance->getTimelinePosition(&iPosition))
	{
		return 0;
	}

	return iPosition;
}

/** @return Whether this event contains any streaming sound samples or not. */
Bool Event::HasStreamingSounds() const
{
	if (IsLoading())
	{
		return false;
	}

	// Resolve the anchor and its description.
	auto pAnchor(m_hAnchor.GetPtr());
	auto pDescription(pAnchor.IsValid() ? pAnchor->ResolveDescription() : nullptr);
	if (!pDescription.IsValid())
	{
		return false;
	}

	// Streaming query successful, return.
	Bool bStreaming = false;
	if (FMOD_OK == pDescription->isStream(&bStreaming))
	{
		return bStreaming;
	}

	return false;
}

/**
 * Returns true if this FMODSound::Event is currently playing,
 * false otherwise.
 */
Bool Event::IsPlaying() const
{
	if (m_pFMODEventInstance.IsValid() && m_pFMODEventInstance->isValid())
	{
		FMOD_STUDIO_PLAYBACK_STATE eState = FMOD_STUDIO_PLAYBACK_STOPPED;
		if (FMOD_OK == m_pFMODEventInstance->getPlaybackState(&eState))
		{
			return (FMOD_STUDIO_PLAYBACK_STOPPED != eState);
		}
	}

	return false;
}

/**
 * If bPause is true, then pauses a playing sound. Otherwise,
 * unpauses a playing sound.
 *
 * This method has no effect if this FMODSound::Event has not been
 * Start()ed or has already finished playing.
 */
void Event::Pause(Bool bPause)
{
	SEOUL_ASSERT(IsMainThread());

	if (IsPlaying())
	{
		FMOD_VERIFY(m_pFMODEventInstance->setPaused(bPause));
	}
}

/**
 * Starts the sound event playing. If bStopOnDestruction is true,
 * Stop() will be called on this FMODSound::Event when the FMODSound::Event instance
 * is destroyed.
 *
 * @return True if the FMODSound::Event was successfully started, false otherwise.
 * This method can return false if FMOD designer will not allow another instance
 * of this particular sound event to be started.
 *
 * If this FMODSound::Event returns true from IsPlaying(), the existing
 * instance of the sound event will be left playing if:
 * - the sound event is not a looping sound event
 * - this FMODSound::Event was not started with bStopOnDestruction set to true.
 * Otherwise, the existing instance will be stopped.
 *
 * Looping sound events are always stopped on destruction.
 */
Bool Event::Start(
	const Vector3D& vPosition,
	const Vector3D& vVelocity,
	Bool bStopOnDestruction /* = false */,
	Int32 iStartOffsetInMilliseconds /*= 0 */)
{
	SEOUL_ASSERT(IsMainThread());

	// If the existing instance is marked as stop on destruction, we must stop it
	// before attempting to start a new instance of the sound event.
	if (0u != (kStopOnDestruction & m_uFlags))
	{
		Stop();
	}
	// Otherwise, let it play out, and start a new sound event.
	else
	{
		// Reset our internal handle to indicate we no longer own
		// the playing sound event.
		InternalResetHandle();
	}

	// Setup our stop on destruction flags, initially based on the bStopOnDestruction
	// variable.
	if (bStopOnDestruction)
	{
		m_uFlags |= kStopOnDestruction;
	}
	else
	{
		m_uFlags &= ~kStopOnDestruction;
	}

	// This FMODSound::Event should not be playing by this point.
	SEOUL_ASSERT(!IsPlaying());

	// Only attempt to play if this FMODSound::Event is ready to play.
	if (IsLoading())
	{
		return false;
	}

	// Resolve the anchor and its description.
	auto pAnchor(m_hAnchor.GetPtr());
	auto pDescription(pAnchor.IsValid() ? pAnchor->ResolveDescription() : nullptr);
	if (!pDescription.IsValid())
	{
		return false;
	}

	auto pSoundManager(Manager::Get());

	// Get the FMOD event system from the sound manager.
	auto pFMODSystem(pSoundManager->m_pFMODStudioSystem);

	// Get oneshot status.
	Bool bOneShot = false;
	FMOD_VERIFY(pDescription->isOneshot(&bOneShot));

	// Always stop on destruction for looping sounds or
	// any sound without a definite end.
	if (!bOneShot)
	{
		m_uFlags |= kStopOnDestruction;
	}

	// Instantiate the event instance.
	::FMOD::Studio::EventInstance* pInstance = nullptr;
	if (FMOD_OK != pDescription->createInstance(&pInstance))
	{
		return false;
	}

	// Sanity.
	SEOUL_ASSERT(nullptr != pInstance && pInstance->isValid());

	FMOD_3D_ATTRIBUTES attrs;
	memset(&attrs, 0, sizeof(attrs));
	attrs.forward = Vector3DToFMOD_VECTOR(-Vector3D::UnitZ());
	attrs.position = Vector3DToFMOD_VECTOR(vPosition);
	attrs.velocity = Vector3DToFMOD_VECTOR(vVelocity);
	attrs.up = Vector3DToFMOD_VECTOR(Vector3D::UnitY());
	FMOD_VERIFY(pInstance->set3DAttributes(&attrs));

	m_pFMODEventInstance = pInstance;

	// This can fail if we run out of channels or somesuch.
	if (FMOD_OK != m_pFMODEventInstance->start())
	{
		InternalResetHandle();
		return false;
	}

	// Now, finally, on success, adjust the timeline position
	// of one-shot sounds.
	if (iStartOffsetInMilliseconds > 0 && bOneShot)
	{
		(void)m_pFMODEventInstance->setTimelinePosition(iStartOffsetInMilliseconds);
	}

	return true;
}

/**
 * Stops playing this FMODSoundEvent.
 *
 * @param[in] bStopImmediately If true, the FMODSound::Event will be stopped
 * without allowing it to finish.
 */
void Event::Stop(Bool bStopImmediately)
{
	SEOUL_ASSERT(IsMainThread());

	if (IsPlaying())
	{
		// This should always succeed.
		FMOD_STUDIO_STOP_MODE eMode = (bStopImmediately
			? FMOD_STUDIO_STOP_IMMEDIATE
			: FMOD_STUDIO_STOP_ALLOWFADEOUT);
		FMOD_VERIFY(m_pFMODEventInstance->stop(eMode));

		// Reset our internal handle to indicate we no longer own
		// a playing sound event.
		InternalResetHandle();
	}
}

/**
 * Updates the 3D position and linear velocity of this FMODSoundEvent.
 *
 * This method only succeeds if this FMODSound::Event is playing and if
 * this FMODSound::Event is not in "force 2D" mode.
 */
void Event::Set3DAttributes(
	const Vector3D& vPosition,
	const Vector3D& vVelocity)
{
	SEOUL_ASSERT(IsMainThread());

	if (IsPlaying())
	{
		FMOD_3D_ATTRIBUTES attrs;
		memset(&attrs, 0, sizeof(attrs));
		attrs.forward = Vector3DToFMOD_VECTOR(-Vector3D::UnitZ());
		attrs.position = Vector3DToFMOD_VECTOR(vPosition);
		attrs.velocity = Vector3DToFMOD_VECTOR(vVelocity);
		attrs.up = Vector3DToFMOD_VECTOR(Vector3D::UnitY());
		FMOD_VERIFY(m_pFMODEventInstance->set3DAttributes(&attrs));
	}
}

/**
 * Sets a parameter fValue to this FMODSoundEvent.
 *
 * @return True if the parameter was set successfully, false
 * otherwise. This method can return false if this FMODSound::Event
 * is not playing or if the parameter name name is invalid.
 */
Bool Event::SetParameter(HString const name, Float fValue)
{
	SEOUL_ASSERT(IsMainThread());

	if (!InternalCacheParameter(name))
	{
		return false;
	}

	// Resolve id.
	FMOD_STUDIO_PARAMETER_ID id;
	memset(&id, 0, sizeof(id));
	ValuesToParameterId(m_uCachedParameterId1, m_uCachedParameterId2, id);

	// Return.
	return (FMOD_OK == m_pFMODEventInstance->setParameterByID(id, fValue));
}

/**
 * Turns off the next sustain point for the event.
 *
 * @return True if the sustain point was successfully turned off, false
 * otherwise.
 */
Bool Event::TriggerCue(HString name /*= HString()*/)
{
	SEOUL_ASSERT(IsMainThread());

	if (IsPlaying())
	{
		return (FMOD_OK == m_pFMODEventInstance->triggerCue());
	}

	return false;
}

/**
 * @return True if the content dependencies of this FMODSound::Event are still
 * being loaded, false otherwise.
 */
Bool Event::IsLoading() const
{
	if (m_hAnchor.IsLoading())
	{
		return true;
	}

	auto pAnchor(m_hAnchor.GetPtr());
	if (pAnchor.IsValid())
	{
		return pAnchor->IsProjectLoading();
	}

	return false;
}

/**
 * Attempt to resolve the id of the given parameter.
 *
 * @return true on success, false otherwise.
 */
Bool Event::InternalCacheParameter(HString const name)
{
	SEOUL_ASSERT(IsMainThread());

	// Already cached.
	if (name == m_CachedParameterName)
	{
		return true;
	}

	// Resolve event description.
	::FMOD::Studio::EventDescription* pEventDescription = nullptr;
	FMOD_VERIFY(m_pFMODEventInstance->getDescription(&pEventDescription));
	if (nullptr == pEventDescription)
	{
		return false;
	}

	// Resolve parameter description.
	FMOD_STUDIO_PARAMETER_DESCRIPTION desc;
	memset(&desc, 0, sizeof(desc));
	if (FMOD_OK != pEventDescription->getParameterDescriptionByName(name.CStr(), &desc))
	{
		return false;
	}

	// Cache.
	m_CachedParameterName = name;
	ParameterIdToValues(desc.id, m_uCachedParameterId1, m_uCachedParameterId2);
	return true;
}

/**
 * Resets this FMODSound::Event to its default state, stopping any
 * playback, and then resets it as owning the FMOD sound event defined
 * by system id uEventSystemId.
 */
void Event::InternalSetAnchor(
	const Content::Handle<EventAnchor>& hAnchor)
{
	Stop();
	m_hAnchor = hAnchor;
}

/**
 * Resets the active handle of this FMODSoundEvent.
 */
void Event::InternalResetHandle()
{
	SEOUL_ASSERT(IsMainThread());

	if (m_pFMODEventInstance.IsValid() && m_pFMODEventInstance->isValid())
	{
		FMOD_VERIFY(m_pFMODEventInstance->release());
		m_pFMODEventInstance.Reset();
	}
}

} // namespace Seoul::FMODSound

#endif // /#if SEOUL_WITH_FMOD
