/**
 * \file MixpanelAnalyticsManager.h
 * \brief Specialization of AnalyticsManager, wraps the
 * Mixpanel analytics service.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef MIXPANEL_ANALYTICS_MANAGER_H
#define MIXPANEL_ANALYTICS_MANAGER_H

#include "AnalyticsManager.h"
#include "AtomicRingBuffer.h"
#include "Delegate.h"
#include "GenericAnalyticsManager.h"
#include "HTTPCommon.h"
#include "Mutex.h"
#include "ReflectionDeclare.h"
#include "SeoulSignal.h"
#include "SeoulTime.h"
namespace Seoul { namespace HTTP { class Request; } }
namespace Seoul { class MixpanelBuilder; }
namespace Seoul { class MixpanelState; }
namespace Seoul { class MixpanelStateData; }
namespace Seoul { class Thread; }

namespace Seoul
{

namespace MixpanelCommon
{

enum class EntryType
{
	kEvent,
	kFlush,
	kProfile,
	kSessionEnd,
	kSessionStart,
};

class Entry SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(Entry);

	Entry(const WorldTime& timestamp)
		: m_SaveTimestamp()
		, m_Timestamp(timestamp)
	{
	}

	virtual ~Entry()
	{
	}

	virtual Bool AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const = 0;
	virtual EntryType GetType() const = 0;
	virtual void OnTrack(MixpanelState& r);

	const WorldTime& GetSaveTimestamp() const { return m_SaveTimestamp; }
	const WorldTime& GetTimestamp() const { return m_Timestamp; }
	void SetTimestamp(const WorldTime& timestamp) { m_Timestamp = timestamp; }

protected:
	// Reflection hook only.
	Entry()
		: m_SaveTimestamp()
		, m_Timestamp()
	{
	}

private:
	SEOUL_REFLECTION_FRIENDSHIP(Entry);

	WorldTime m_SaveTimestamp;
	WorldTime m_Timestamp;

	SEOUL_DISABLE_COPY(Entry);
}; // class Entry

class FlushEntry SEOUL_SEALED : public Entry
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(FlushEntry);

	FlushEntry(const WorldTime& timestamp)
		: Entry(timestamp)
	{
	}

	// Event overrides
	virtual Bool AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const SEOUL_OVERRIDE { return false; }
	virtual EntryType GetType() const SEOUL_OVERRIDE { return EntryType::kFlush; }

protected:
	// Reflection hook only.
	FlushEntry()
		: Entry()
	{
	}

private:
	SEOUL_REFLECTION_FRIENDSHIP(FlushEntry);

	SEOUL_DISABLE_COPY(FlushEntry);
}; // class FlushEntry

class SequencedEntry SEOUL_ABSTRACT : public Entry
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(SequencedEntry);

	SequencedEntry(const WorldTime& timestamp)
		: Entry(timestamp)
	{
	}

	// Event overrides
	virtual void OnTrack(MixpanelState& r) SEOUL_OVERRIDE;
	// /Event overrides

protected:
	// Reflection hook only.
	SequencedEntry()
		: Entry()
	{
	}

private:
	SEOUL_DISABLE_COPY(SequencedEntry);
}; // class SequencedEntry

class EventEntry SEOUL_SEALED : public SequencedEntry
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(EventEntry);

	EventEntry(const AnalyticsEvent& evt, const WorldTime& timestamp)
		: SequencedEntry(timestamp)
		, m_Event(evt)
	{
	}

	// Event overrides.
	virtual Bool AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const SEOUL_OVERRIDE;
	virtual EntryType GetType() const SEOUL_OVERRIDE { return EntryType::kEvent; }
	// /Event overrides.

	const AnalyticsEvent& GetEvent() const { return m_Event; }

private:
	SEOUL_REFLECTION_FRIENDSHIP(EventEntry);

	// Reflection hook only.
	EventEntry()
		: SequencedEntry()
		, m_Event()
	{
	}

	AnalyticsEvent m_Event;

	SEOUL_DISABLE_COPY(EventEntry);
}; // class EventEntry

class ProfileEntry SEOUL_SEALED : public Entry
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ProfileEntry);

	ProfileEntry(const AnalyticsProfileUpdate& update, const WorldTime& timestamp)
		: Entry(timestamp)
		, m_Update(update)
	{
	}

	// Event overrides.
	virtual Bool AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const SEOUL_OVERRIDE;
	virtual EntryType GetType() const SEOUL_OVERRIDE { return EntryType::kProfile; }
	/// Event overrides.

private:
	SEOUL_REFLECTION_FRIENDSHIP(ProfileEntry);

	// Reflection hook only.
	ProfileEntry()
		: Entry()
		, m_Update()
	{
	}

	AnalyticsProfileUpdate m_Update;

	SEOUL_DISABLE_COPY(ProfileEntry);
}; // class ProfileEntry

class SessionEndEntry SEOUL_SEALED : public SequencedEntry
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(SessionEndEntry);

	SessionEndEntry(const WorldTime& timestamp)
		: SequencedEntry(timestamp)
	{
	}

	// Event overrides.
	virtual Bool AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const SEOUL_OVERRIDE;
	virtual EntryType GetType() const SEOUL_OVERRIDE { return EntryType::kSessionEnd; }
	// /Event overrides.

private:
	SEOUL_REFLECTION_FRIENDSHIP(SessionEndEntry);

	// Reflection hook only.
	SessionEndEntry()
		: SequencedEntry()
	{
	}

	SEOUL_DISABLE_COPY(SessionEndEntry);
}; // class SessionEndEntry

class SessionStartEntry SEOUL_SEALED : public SequencedEntry
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(SessionStartEntry);

	SessionStartEntry(const WorldTime& timestamp)
		: SequencedEntry(timestamp)
	{
	}

	// Event overrides.
	virtual Bool AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const SEOUL_OVERRIDE;
	virtual EntryType GetType() const SEOUL_OVERRIDE { return EntryType::kSessionStart; }
	virtual void OnTrack(MixpanelState& r) SEOUL_OVERRIDE;
	// /Event overrides.

private:
	SEOUL_REFLECTION_FRIENDSHIP(SessionStartEntry);

	// Reflection hook only.
	SessionStartEntry()
		: SequencedEntry()
	{
	}

	SEOUL_DISABLE_COPY(SessionStartEntry);
}; // class SessionStartEntry

struct Settings SEOUL_SEALED
{
	Settings(const GenericAnalyticsManagerSettings& generic);

	GenericAnalyticsManagerSettings m_Generic;
	const HString kAbTests;
	const HString kAbTestGroups;
	const HString kAdvertisingId;
	const HString kAppSubVersion;
	const HString kAppVersion;
	const HString kAspectRatio;
	const HString kBatteryLevel;
	const HString kConnectionType;
	const HString kDeviceManufacturer;
	const HString kDeviceModel;
	const HString kDeviceId;
	const HString kDistinctIdEvent;
	const HString kDistinctIdUpdate;
	const HString kEventId;
	const HString kEventType;
	const HString kOs;
	const HString kPlatform;
	const HString kPlayerGUID;
	const HString kProperties;
	const HString kPushEnabledName;
	const HString kRooted;
	const HString kSandboxed;
	const HString kPropUACampaign;
	const HString kPropUAMediaSource;
	const HString kUnspecifiedPropertyValue;
	const HString kSessionEventName;
	const HString kSessionLengthName;
	const HString kSessionCountName;
	const HString kSessionUUIDName;
	const HString kSessionStartName;
	const HString kTime;
	const HString kTokenEvent;
	const HString kTokenUpdate;
};

} // namespace MixpanelCommon

class MixpanelAnalyticsManager SEOUL_SEALED : public AnalyticsManager
{
public:
	SEOUL_DELEGATE_TARGET(MixpanelAnalyticsManager);

	MixpanelAnalyticsManager(const GenericAnalyticsManagerSettings& settings);
	~MixpanelAnalyticsManager();

	virtual AnalyticsManagerType GetType() const SEOUL_OVERRIDE { return AnalyticsManagerType::kMixpanel; }

	// As necessary, tell the Analytics system to immediately attempt
	// to submit any pending analytics data.
	virtual void Flush() SEOUL_OVERRIDE;

	// Get the current analytics API key.
	virtual String GetApiKey() const SEOUL_OVERRIDE;

	virtual Int64 GetSessionCount() const SEOUL_OVERRIDE;

	/** Change the API key for future analytic events */
	virtual void SetApiKey(const String& sApiKey) SEOUL_OVERRIDE;

	/** Update the current analytics user ID. The analytics system will wait for this to be set before sending analytics events. */
	virtual void SetAnalyticsUserID(const String& sUserID) SEOUL_OVERRIDE;

	/**
	 * Odd utility hook - if the reported time of a time function changes (e.g. server time is received),
	 * this function is used to allow an AnalyticsManager subclass to react to the change.
	 */
	virtual void OnTimeFunctionTimeChange(const TimeFunction& function, TimeInterval deltaTime) SEOUL_OVERRIDE;

	virtual void UpdateSessionCountFromPersistence(Int64 iPersistenceSessionCount) SEOUL_OVERRIDE;

	/**
	* Add analytics state properties to a data store.  For events that are reported by an external service,
	* in particular purchase events reported by the server.
	*/
	virtual Bool AddStateProperties(DataStore& rDataStore, const DataNode& propertiesTable) const SEOUL_OVERRIDE;

	virtual void SetAttributionData(const String& sCampaign, const String& sMediaSource) SEOUL_OVERRIDE;

	virtual Bool ShouldSetInSandboxProfileProperty() SEOUL_OVERRIDE
	{
		return m_Settings.m_Generic.m_bShouldSetInSandboxProfileProperty;
	}

private:

	void SyncMixpanelStateData(MixpanelStateData& stateData);

	// AnalyticsManager overrides.
	virtual void DoEnterBackground() SEOUL_OVERRIDE;
	virtual void DoLeaveBackground() SEOUL_OVERRIDE;
	virtual void DoTrackEvent(const AnalyticsEvent& evt, const WorldTime& timestamp) SEOUL_OVERRIDE;
	virtual void DoTrackSessionEnd(const WorldTime& timestamp) SEOUL_OVERRIDE;
	virtual void DoTrackSessionStart(const WorldTime& timestamp) SEOUL_OVERRIDE;
	virtual void DoUpdateProfile(const AnalyticsProfileUpdate& update, const WorldTime& timestamp) SEOUL_OVERRIDE;
	// /AnalyticsManager overrides.

	typedef AtomicRingBuffer<MixpanelCommon::Entry*, MemoryBudgets::Analytics> Tasks;
	typedef AtomicRingBuffer<TimeInterval*, MemoryBudgets::Analytics> TimeChanges;

	MixpanelCommon::Settings const m_Settings;
	String const m_sBaseEventURL;
	String const m_sBaseProfileURL;
	String m_sApiKey;
	Tasks m_Tasks;
	TimeChanges m_TimeChanges;
	ScopedPtr<Thread> m_pWorkerThread;
	ScopedPtr<MixpanelStateData> m_pStateData;
	Atomic32Value<Bool> m_bInBackground;
	Atomic32Value<Bool> m_bShuttingDown;

	WorldTime Now() const;
	Int WorkerThread(const Thread&);
	void WorkerThreadApplyTimeChanges(MixpanelState& r);
	Bool WorkerThreadConsumeTasks(MixpanelState& r, Bool& rbFlush);
	Bool WorkerThreadIssueRequest(const MixpanelState& state, const String& sBody, MixpanelCommon::EntryType eType) const;
	MixpanelState* WorkerThreadLoadState() const;
	void WorkerThreadPrune(MixpanelState& rState) const;
	Bool WorkerThreadSaveState(const MixpanelState& state) const;
	void WorkerThreadSubmitTasks(MixpanelState& r);
	Bool WorkerThreadSubmitTasks(MixpanelState& r, MixpanelCommon::EntryType eType);

	SEOUL_DISABLE_COPY(MixpanelAnalyticsManager);
}; // class MixpanelAnalyticsManager

} // namespace Seoul

#endif // include guard
