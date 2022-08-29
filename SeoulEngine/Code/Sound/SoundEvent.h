/**
 * \file SoundEvent.h
 * \brief A Sound::Event can be thought of as a sound effect with more
 * flexibility and complexity. A single Sound::Event can contain multiple
 * raw wave files, various audio processing, and Sound::Events can also
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
#ifndef SOUND_EVENT_H
#define	SOUND_EVENT_H

#include "ContentKey.h"
#include "Prereqs.h"
#include "SeoulHString.h"

namespace Seoul { namespace Sound { class Manager; } }
namespace Seoul { struct Vector3D; }

namespace Seoul::Sound
{

class Event SEOUL_ABSTRACT
{
public:
	virtual ~Event();

	enum Flags
	{
		/** No special behavior. */
		kNone = 0u,

		/**
		 * If set, destruction of this Sound::Event instance will stop
		 * its corresponding playback.
		 */
		kStopOnDestruction = (1 << 0u),
	}; // enum Flags

	/** Create a new instance of this SoundEvent. Must be freed with SEOUL_DELETE. */
	virtual Event* Clone() const = 0;

	/** @return The length of the event, in seconds. */
	virtual Bool GetLengthInMilliseconds(Int32& riLengthMs) const = 0;

	/** @return The current timeline position of the event, in milliseconds. Must be playing to succeed. */
	virtual Int32 GetTimelinePositionInMilliseconds() const = 0;

	/** @return Whether this event contains any streaming sound samples or not. */
	virtual Bool HasStreamingSounds() const = 0;

	/**
	 * Returns true if this Sound::Event is currently playing,
	 * false otherwise.
	 */
	virtual Bool IsPlaying() const = 0;

	/**
	 * Resets this Sound::Event to a default, invalid state.
	 */
	virtual void Reset() = 0;

	/**
	 * @return True if this Sound::Event will stop when the Sound::Event object
	 * is destroyed, false otherwise.
	 */
	Bool StopOnDestruction() const
	{
		return (0u != (kStopOnDestruction & m_uFlags));
	}

	virtual void Pause(Bool bPause = true) = 0;
	virtual Bool Start(
		const Vector3D& vPosition,
		const Vector3D& vVelocity,
		Bool bStopOnDestruction = false,
		Int32 iStartOffsetInMilliseconds = 0) = 0;
	virtual void Stop(Bool bStopImmediately = false) = 0;

	virtual void Set3DAttributes(const Vector3D& vPosition, const Vector3D& vVelocity) = 0;

	virtual Bool SetParameter(HString const name, Float fValue) = 0;

	virtual Bool TriggerCue(HString name = HString()) = 0;

	/**
	 * @return True if the content dependencies of this Sound::Event are still
	 * being loaded, false otherwise.
	 */
	virtual Bool IsLoading() const = 0;

	/**
	 * @return The ContentKey associated with this SoundEvent.
	 */
	virtual ContentKey GetKey() const = 0;

protected:
	Event();

	UInt32 m_uFlags;

private:
	SEOUL_DISABLE_COPY(Event);
}; // class Sound::Event

/**
 * Placeholder implementation of SoundEvent. Can be used for headless applications
 * or as a placeholder for platforms that do not support audio.
 */
class NullEvent SEOUL_SEALED : public Event
{
public:
	NullEvent();
	~NullEvent();

	virtual Event* Clone() const SEOUL_OVERRIDE;

	/** @return The length of the event, in milliseconds. */
	virtual Bool GetLengthInMilliseconds(Int32& riLengthMs) const SEOUL_OVERRIDE
	{
		return false;
	}

	/** @return The current timeline position of the event, in milliseconds. Must be playing to succeed. */
	virtual Int32 GetTimelinePositionInMilliseconds() const SEOUL_OVERRIDE
	{
		return 0;
	}

	/** @return Whether this event contains any streaming sound samples or not. */
	virtual Bool HasStreamingSounds() const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool IsPlaying() const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual void Reset() SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void Pause(Bool bPause = true) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual Bool Start(
		const Vector3D& vPosition,
		const Vector3D& vVelocity,
		Bool bStopOnDestruction = false,
		Int32 iStartOffsetInMilliseconds = 0) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual void Stop(Bool bStopImmediately = false) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void Set3DAttributes(const Vector3D& vPosition, const Vector3D& vVelocity) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual Bool SetParameter(HString const name, Float fValue) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool TriggerCue(HString name = HString()) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool IsLoading() const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual ContentKey GetKey() const SEOUL_OVERRIDE
	{
		return m_Key;
	}

private:
	friend class NullManager;
	ContentKey m_Key;

	SEOUL_DISABLE_COPY(NullEvent);
}; // class NullEvent

} // namespace Seoul::Sound

#endif // include guard
