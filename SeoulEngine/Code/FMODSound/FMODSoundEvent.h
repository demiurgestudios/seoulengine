/**
 * \file FMODSoundEvent.h
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

#pragma once
#ifndef FMOD_SOUND_EVENT_H
#define	FMOD_SOUND_EVENT_H

#include "FMODSoundAnchors.h"
#include "SoundEvent.h"

#if SEOUL_WITH_FMOD

namespace FMOD { namespace Studio { class EventInstance; } }

namespace Seoul
{

// Forward declarations
namespace FMODSound { class Manager; }
struct Vector3D;

namespace FMODSound
{

/**
 * A FMODSound::Event is a sound effect specified in FMOD Designer. It
 * can contain multiple raw wave files, audio processing and various flexibility
 * to change and respond to changes to ingame state.
 */
class Event SEOUL_SEALED : public Sound::Event
{
public:
	Event();
	~Event();

	virtual Sound::Event* Clone() const SEOUL_OVERRIDE;

	/** @return The length of the event, in seconds. */
	virtual Bool GetLengthInMilliseconds(Int32& riLengthMs) const SEOUL_OVERRIDE;

	/** @return The current timeline position of the event, in milliseconds. Must be playing to succeed. */
	virtual Int32 GetTimelinePositionInMilliseconds() const SEOUL_OVERRIDE;

	/** @return Whether this event contains any streaming sound samples or not. */
	virtual Bool HasStreamingSounds() const SEOUL_OVERRIDE;

	/**
	 * Returns true if this FMODSound::Event is currently playing,
	 * false otherwise.
	 */
	virtual Bool IsPlaying() const SEOUL_OVERRIDE;

	/**
	 * Resets this FMODSound::Event to a default, invalid state.
	 */
	virtual void Reset() SEOUL_OVERRIDE
	{
		InternalSetAnchor(Content::Handle<EventAnchor>());
	}

	virtual void Pause(Bool bPause = true) SEOUL_OVERRIDE;
	virtual Bool Start(
		const Vector3D& vPosition,
		const Vector3D& vVelocity,
		Bool bStopOnDestruction = false,
		Int32 iStartOffsetInMilliseconds = 0) SEOUL_OVERRIDE;
	virtual void Stop(Bool bStopImmediately = false) SEOUL_OVERRIDE;

	virtual void Set3DAttributes(const Vector3D& vPosition, const Vector3D& vVelocity) SEOUL_OVERRIDE;

	virtual Bool SetParameter(HString const name, Float fValue) SEOUL_OVERRIDE;

	virtual Bool TriggerCue(HString name = HString()) SEOUL_OVERRIDE;

	/**
	 * @return True if the content dependencies of this FMODSound::Event are still
	 * being loaded, false otherwise.
	 */
	virtual Bool IsLoading() const SEOUL_OVERRIDE;

	/**
	 * @return The ContentKey associated with this FMODSoundEvent.
	 */
	virtual ContentKey GetKey() const SEOUL_OVERRIDE
	{
		return m_hAnchor.GetKey();
	}

private:
	friend class Manager;

	void InternalSetAnchor(const Content::Handle<EventAnchor>& hAnchor);
	void InternalResetHandle();
	Bool InternalCacheParameter(HString const name);

	Content::Handle<EventAnchor> m_hAnchor;
	CheckedPtr<::FMOD::Studio::EventInstance> m_pFMODEventInstance;

	HString m_CachedParameterName;
	UInt32 m_uCachedParameterId1;
	UInt32 m_uCachedParameterId2;
}; // class FMODSound::Event

} // namespace FMODSound

} // namespace Seoul

#endif // /#if SEOUL_WITH_FMOD

#endif // include guard
