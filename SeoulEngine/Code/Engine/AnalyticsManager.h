/**
 * \file AnalyticsManager.h
 * \brief Global singleton, exposes functionality to track
 * game events, revenue, and other analytics.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANALYTICS_MANAGER_H
#define ANALYTICS_MANAGER_H

#include "DataStore.h"
#include "Delegate.h"
#include "HashTable.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "SeoulTime.h"
#include "SeoulUUID.h"
#include "Singleton.h"
namespace Seoul { class AnalyticsEvent; }
namespace Seoul { class AnalyticsProfileUpdate; }
namespace Seoul { namespace Reflection { struct SerializeContext; } }

namespace Seoul
{

// Keys for analytics user properties used in multiple places
static HString const kAnalyticsUserPropertiesNameKey("$name");

struct AnalyticsSessionChangeEvent SEOUL_SEALED
{
	Bool m_bSessionStart{};
	UUID m_SessionUUID{};
	WorldTime m_TimeStamp{};
	TimeInterval m_Duration{};
}; // struct AnalyticsSessionChangeEvent

enum class AnalyticsManagerType
{
	kNull,
	kMixpanel,
};

/**
 * The game event for analytics session change events.
 *
 * This event will be fired whenever the analytics system has processed a session start or
 * session end event. It can be used by sister systems that want to exactly match the
 * analytics reporting W.R.T session start and end.
 */
static const HString AnalyticsSessionGameEventId("Analytics.OnSessionChange");

class AnalyticsManager SEOUL_ABSTRACT : public Singleton<AnalyticsManager>
{
public:
	/** Utility structure used to avoid the generation of duplicate keys from truncated keys. */
	typedef HashTable<String, String, MemoryBudgets::Analytics> StringTable;

	/** Signature of a custom function used to get the current world time. */
	typedef Delegate<WorldTime()> TimeFunction;

	virtual ~AnalyticsManager();

	virtual AnalyticsManagerType GetType() const = 0;

	// As necessary, tell the Analytics system to immediately attempt
	// to submit any pending analytics data.
	virtual void Flush()
	{
		// Nop by default.
	}

	/** @return The current analytics user ID, used to identify the current player. */
	String GetAnalyticsUserID() const
	{
		String sReturn;
		{
			Lock lock(m_Mutex);
			sReturn = m_sUserID;
		}

		return sReturn;
	}

	// Get the current analytics API key.
	virtual String GetApiKey() const = 0;

	// Get the current analytics session count.
	virtual Int64 GetSessionCount() const = 0;

	/**
	 * Odd utility hook - if the reported time of a time function changes (e.g. server time is received),
	 * this function is used to allow an AnalyticsManager subclass to react to the change.
	 */
	virtual void OnTimeFunctionTimeChange(const TimeFunction& function, TimeInterval deltaTime) = 0;

	/**
	* Update session count from persistence.  We use this to prevent the session count from being reset
	* when the player installs on a new device
	*/
	virtual void UpdateSessionCountFromPersistence(Int64 iPersistenceSessionCount) = 0;

	/** Update the current analytics user ID. The analytics system will wait for this to be set before sending analytics events. */
	virtual void SetAnalyticsUserID(const String& sUserID)
	{
		Lock lock(m_Mutex);
		m_sUserID = sUserID;
	}

	/** @return Whether the user is sandboxed or not. */
	Bool GetAnalyticsSandboxed() const
	{
		return m_bSandboxed;
	}

	/**
	 * Update the sandboxing state of the user. A "sandboxed" user is a cheater - in some cases this disables
	 * analytics completely while it others it just updates a property sent with analytics data.
	 */
	void SetAnalyticsSandboxed(Bool bSandboxed)
	{
		m_bSandboxed = bSandboxed;
	}

	// Change the API key for future analytic events
	virtual void SetApiKey(const String& s) = 0;

	/**
	* Add analytics state properties to a data store.  For events that are reported by an external service,
	* in particular purchase events reported by the server.
	*/
	virtual Bool AddStateProperties(DataStore& rDataStore, const DataNode& propertiesTable) const = 0;
	// NOTE:: This maybe should not be part of the AnalyticsManager API, but the way that State Properties
	// are currently handled internally with the MixpanelAnalyticsManager makes it difficult to avoid this. Properties
	// like attribution are cached in the analytics manager and updated infrequently, without an enforced update
	// of Mixpanel people properties. For attribution data, we want to enforce the update once an asynchronous operation
	// in the TrackingManager completes.
	virtual void SetAttributionData(const String& sCampaign, const String& sMediaSource) = 0;

	/**
	 * Track an event.
	 *
	 * @param[in] event Data that fully defines the event.
	 */
	void TrackEvent(const AnalyticsEvent& evt, const WorldTime& timestamp = WorldTime())
	{
		DoTrackEvent(evt, timestamp);
	}

	/**
	 * Called to indicate the start of a new session.
	 */
	void TrackSessionEnd(const WorldTime& timestamp = WorldTime())
	{
		DoTrackSessionEnd(timestamp);
	}

	/**
	 * Called to indicate the start of a new session.
	 */
	void TrackSessionStart(const WorldTime& timestamp = WorldTime())
	{
		DoTrackSessionStart(timestamp);
	}

	/** Foreground/background handling. */
	void OnEnterBackground()
	{
		DoEnterBackground();
	}
	void OnLeaveBackground()
	{
		DoLeaveBackground();
	}

	/**
	 * Issue a user profile update, if supported by the analytics system.
	 */
	void UpdateProfile(const AnalyticsProfileUpdate& update, const WorldTime& timestamp = WorldTime())
	{
		DoUpdateProfile(update, timestamp);
	}

	/** Tracking of A/B test membership for analytics reporting. */
	typedef HashTable<String, Int32, MemoryBudgets::Analytics> ABTests;
	ABTests GetABTests() const
	{
		Lock lock(m_Mutex);
		return m_tABTests;
	}

	void SetABTests(const ABTests& t)
	{
		Lock lock(m_Mutex);
		m_tABTests = t;
	}

	String GetSubVersionString() const
	{
		Lock lock(m_Mutex);
		return m_sSubVersionString;
	}

	void SetSubVersionString(const String& s)
	{
		Lock lock(m_Mutex);
		m_sSubVersionString = s;
	}

	virtual Bool ShouldSetInSandboxProfileProperty() = 0;

protected:
	AnalyticsManager();

	// Implement in base class
	virtual void DoEnterBackground() = 0;
	virtual void DoLeaveBackground() = 0;
	virtual void DoTrackEvent(const AnalyticsEvent& evt, const WorldTime& timestamp) = 0;
	virtual void DoTrackSessionEnd(const WorldTime& timestamp) = 0;
	virtual void DoTrackSessionStart(const WorldTime& timestamp) = 0;
	virtual void DoUpdateProfile(const AnalyticsProfileUpdate& update, const WorldTime& timestamp) = 0;

	Mutex m_Mutex;

private:
	String m_sUserID;
	ABTests m_tABTests;
	String m_sSubVersionString;

	Atomic32Value<Bool> m_bSandboxed;

	SEOUL_DISABLE_COPY(AnalyticsManager);
}; // class AnalyticsManager

/**
 * Specialization of AnalyticsManager for use in games that do not need
 * analytics or on platforms which do not support analytics.
 */
class NullAnalyticsManager SEOUL_SEALED : public AnalyticsManager
{
public:
	NullAnalyticsManager()
	{
	}

	~NullAnalyticsManager()
	{
	}

	virtual AnalyticsManagerType GetType() const SEOUL_OVERRIDE { return AnalyticsManagerType::kNull; }

	virtual String GetApiKey() const SEOUL_OVERRIDE
	{
		return String();
	}

	virtual Int64 GetSessionCount() const SEOUL_OVERRIDE
	{
		return 0;
	}

	/**
	 * Odd utility hook - if the reported time of a time function changes (e.g. server time is received),
	 * this function is used to allow an AnalyticsManager subclass to react to the change.
	 */
	virtual void OnTimeFunctionTimeChange(const TimeFunction& /*function*/, TimeInterval /*deltaTime*/) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void UpdateSessionCountFromPersistence(Int64 iPersistenceSessionCount) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void SetApiKey(const String& /*s*/) SEOUL_OVERRIDE
	{
		// Nop
	}

	/**
	* Add analytics state properties to a data store.  For events that are reported by an external service,
	* in particular purchase events reported by the server.
	*/
	virtual Bool AddStateProperties(DataStore& /*rDataStore*/, const DataNode& /*propertiesTable*/) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual void SetAttributionData(const String& sCampaign, const String& sMediaSource) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual Bool ShouldSetInSandboxProfileProperty() SEOUL_OVERRIDE;

private:
	virtual void DoEnterBackground() SEOUL_OVERRIDE;
	virtual void DoLeaveBackground() SEOUL_OVERRIDE;
	virtual void DoTrackEvent(const AnalyticsEvent& evt, const WorldTime& timestamp) SEOUL_OVERRIDE;
	virtual void DoTrackSessionEnd(const WorldTime& timestamp) SEOUL_OVERRIDE;
	virtual void DoTrackSessionStart(const WorldTime& timestamp) SEOUL_OVERRIDE;
	virtual void DoUpdateProfile(const AnalyticsProfileUpdate& update, const WorldTime& timestamp) SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(NullAnalyticsManager);
}; // class NullAnalyticsManager

/**
 * Defines a single event to track - pass to AnalyticsManager::TrackEvent().
 */
class AnalyticsEvent SEOUL_SEALED
{
public:
	/** Type used to store analytics event properties. */
	typedef DataStore Properties;

	AnalyticsEvent()
		: m_sName()
		, m_Properties()
		, m_sOnceToken()
	{
		m_Properties.MakeTable();
	}

	AnalyticsEvent(const AnalyticsEvent& b)
		: m_sName(b.m_sName)
		, m_Properties()
		, m_sOnceToken(b.m_sOnceToken)
	{
		m_Properties.CopyFrom(b.m_Properties);
	}

	AnalyticsEvent& operator=(const AnalyticsEvent& b)
	{
		m_sName = b.m_sName;
		m_Properties.CopyFrom(b.m_Properties);
		m_sOnceToken = b.m_sOnceToken;
		return *this;
	}

	explicit AnalyticsEvent(const String& sName)
		: m_sName(sName)
		, m_Properties()
		, m_sOnceToken()
	{
		m_Properties.MakeTable();
	}

	/** Name of this event. */
	const String& GetName() const { return m_sName; }

	/** @return The once token previously assigned to this event. Empty by default, which disables "once" functionality. */
	const String& GetOnceToken() const { return m_sOnceToken; }

	/** Attributes associated with the event. */
	Properties& GetProperties() { return m_Properties; }

	/** Attributes associated with the event. */
	const Properties& GetProperties() const { return m_Properties; }

	/** Update the name of this event. */
	void SetName(const String& sName) { m_sName = sName; }

	/**
	 * If not empty, a "once token" is used to track whether an event has ever been sent,
	 * by storing a key-to-value mapping (from event name to token) in the analytics
	 * persistent state store.
	 *
	 * Events of this type are silently dropped until their "once token" changes.
	 */
	void SetOnceToken(const String& sOnceToken) { m_sOnceToken = sOnceToken; }

private:
	SEOUL_REFLECTION_FRIENDSHIP(AnalyticsEvent);

	Bool DeserializeProperties(Reflection::SerializeContext* pContext, DataStore const* pDataStore, const DataNode& value);
	Bool SerializeProperties(Reflection::SerializeContext* pContext, HString propertyName, DataStore* pDataStore, const DataNode& table) const;

	String m_sName;
	Properties m_Properties;
	String m_sOnceToken;
}; // class AnalyticsEvent

enum class AnalyticsProfileUpdateOp
{
	/** Unknown/invalid operation. */
	kUnknown,

	/** Matches Mixpanel profile update operations - see alos https://mixpanel.com/help/reference/http */

	/** Adds a numeric value to an existing numeric value. */
	kAdd,

	/** Adds values to a list. */
	kAppend,

	/** Remove a value from an existing list. */
	kRemove,

	/** Sets a value to a named property, always. */
	kSet,

	/** Sets a value to a named property only if it is not already set. */
	kSetOnce,

	/** Merge a list of values with an existing list of values, deduped. */
	kUnion,

	/** Permanently delete the named property from the profile. */
	kUnset,
};

class AnalyticsProfileUpdate SEOUL_SEALED
{
public:
	typedef DataStore Updates;

	AnalyticsProfileUpdate()
		: m_eOp(AnalyticsProfileUpdateOp::kUnknown)
		, m_Updates()
	{
		m_Updates.MakeTable();
	}

	AnalyticsProfileUpdate(const AnalyticsProfileUpdate& b)
		: m_eOp(b.m_eOp)
		, m_Updates()
	{
		m_Updates.CopyFrom(b.m_Updates);
	}

	AnalyticsProfileUpdate& operator=(const AnalyticsProfileUpdate& b)
	{
		m_eOp = b.m_eOp;
		m_Updates.CopyFrom(b.m_Updates);
		return *this;
	}

	explicit AnalyticsProfileUpdate(AnalyticsProfileUpdateOp eOp)
		: m_eOp(eOp)
		, m_Updates()
	{
		m_Updates.MakeTable();
	}

	/** Op to perform on all key-value pairs in PropertyUpdates. */
	AnalyticsProfileUpdateOp GetOp() const
	{
		return m_eOp;
	}

	/** Table of key-value pairs to perform updates against. */
	Updates& GetUpdates() { return m_Updates; }

	/** Table of key-value pairs to perform updates against. */
	const Updates& GetUpdates() const { return m_Updates; }

	/** Update the op of this update. */
	void SetOp(AnalyticsProfileUpdateOp eOp)
	{
		m_eOp = eOp;
	}

private:
	SEOUL_REFLECTION_FRIENDSHIP(AnalyticsProfileUpdate);

	Bool DeserializeUpdates(Reflection::SerializeContext* pContext, DataStore const* pDataStore, const DataNode& value);
	Bool SerializeUpdates(Reflection::SerializeContext* pContext, HString propertyName, DataStore* pDataStore, const DataNode& table) const;

	AnalyticsProfileUpdateOp m_eOp;
	DataStore m_Updates;
}; // class AnalyticsProfileUpdate

} // namespace Seoul

#endif // include guard
