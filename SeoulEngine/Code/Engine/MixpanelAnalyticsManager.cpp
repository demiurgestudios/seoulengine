/**
 * \file MixpanelAnalyticsManager.cpp
 * \brief Specialization of AnalyticsManager, wraps the
 * Mixpanel analytics service.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildChangelistPublic.h"
#include "BuildVersion.h"
#include "Engine.h"
#include "EventsManager.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "HTTPResponse.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "MemoryBarrier.h"
#include "MixpanelAnalyticsManager.h"
#include "PlatformData.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionDefine.h"
#include "ReflectionSerialize.h"
#include "RenderDevice.h"
#include "SaveLoadManager.h"
#include "SeoulUUID.h"
#include "Thread.h"
#include "ThreadId.h"
#include "ToString.h"

namespace Seoul
{

/** \sa https://mixpanel.com/help/reference/http */
static const UInt32 kuMaxMixpanelRequestsPerBatch = 50u;

/**
 * Maximum event age - events older than this will be pruned.
 *
 * Mixpanel will not accept these events anyway (we have to
 * send them using the "import" API) and this gives us a
 * common, low-impact way to prune persistent data for users
 * who are blocking Mixpanel traffic.
 */
static const TimeInterval kMaximumEventAge = TimeInterval::FromDaysInt64(5);

/**
 * Maximum queue size - additional constraint to the age constraint.
 */
static const UInt32 kuMaximumQueueSize = 1000u;

// Configuration.
static TimeInterval const kSessionExpirationTime = TimeInterval(15);
static Int32 const kiSaveVersion = 2;

static inline String GetMixpanelURL(
	MixpanelCommon::EntryType eType,
	const String& sBaseEventURL,
	const String& sBaseProfileURL,
	const String& sApiKey,
	const String& sAnalyticsUserId)
{
	using namespace MixpanelCommon;

	switch (eType)
	{
	case EntryType::kEvent: // fall-through
	case EntryType::kSessionEnd: // fall-through
	case EntryType::kSessionStart:
		return sBaseEventURL;

	case EntryType::kProfile:
		return sBaseProfileURL;

	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return String();
	};
}

static inline UInt32 ToAnalyticsTime(const TimeInterval& interval)
{
	return (UInt32)(interval.GetMicroseconds() / WorldTime::kSecondsToMicroseconds);
}

static inline UInt32 ToAnalyticsTime(const WorldTime& timestamp)
{
	return (UInt32)(timestamp.GetMicroseconds() / WorldTime::kSecondsToMicroseconds);
}

static inline Byte const* ToMixpanelProfileOpString(AnalyticsProfileUpdateOp e)
{
	switch (e)
	{
	case AnalyticsProfileUpdateOp::kAdd: return "$add";
	case AnalyticsProfileUpdateOp::kAppend: return "$append";
	case AnalyticsProfileUpdateOp::kRemove: return "$remove";
	case AnalyticsProfileUpdateOp::kSet: return "$set";
	case AnalyticsProfileUpdateOp::kSetOnce: return "$set_once";
	case AnalyticsProfileUpdateOp::kUnion: return "$union";
	case AnalyticsProfileUpdateOp::kUnset: return "$unset";
	case AnalyticsProfileUpdateOp::kUnknown:
	default:
		return "";
	};
}

class MixpanelStateData SEOUL_SEALED
{
public:
	typedef HashTable<String, Int32, MemoryBudgets::Analytics> ABTests;
	typedef Vector<String, MemoryBudgets::Analytics> Strings;

	MixpanelStateData()
		: m_iSessionSequenceNumber(0)
		, m_SessionUUID(UUID::GenerateV4())
		, m_PlatformData()
		, m_sUserID()
	{
	}

	void AddStandardProperties(
		const MixpanelCommon::Settings& settings,
		DataStore& rDataStore,
		const DataNode& propertiesTable) const
	{
		auto const& generic = settings.m_Generic;

		AddABTests(settings, rDataStore, propertiesTable);

		if (!m_PlatformData.m_sAdvertisingId.IsEmpty())
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kAdvertisingId, m_PlatformData.m_sAdvertisingId));
		}
		if (!m_PlatformData.m_sDeviceManufacturer.IsEmpty())
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kDeviceManufacturer, m_PlatformData.m_sDeviceManufacturer));
		}
		if (!m_PlatformData.m_sDeviceModel.IsEmpty())
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kDeviceModel, m_PlatformData.m_sDeviceModel));
		}
		if (!m_PlatformData.m_sDeviceId.IsEmpty())
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kDeviceId, m_PlatformData.m_sDeviceId));
		}

		if (generic.m_OsVersionDelegate.IsValid())
		{
			auto const sOsVersion = generic.m_OsVersionDelegate(m_PlatformData);
			if (!sOsVersion.IsEmpty())
			{
				SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kOs, sOsVersion));
			}
		}
		else if (!m_PlatformData.m_sOsVersion.IsEmpty())
		{
			auto const sOsVersion = m_PlatformData.m_sOsVersion;
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kOs, sOsVersion));
		}

		if (!m_PlatformData.m_sUACampaign.IsEmpty())
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kPropUACampaign, m_PlatformData.m_sUACampaign));
		}
		else
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kPropUACampaign, settings.kUnspecifiedPropertyValue));
		}
		if (!m_PlatformData.m_sUAMediaSource.IsEmpty())
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kPropUAMediaSource, m_PlatformData.m_sUAMediaSource));
		}
		else
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kPropUAMediaSource, settings.kUnspecifiedPropertyValue));
		}

		SEOUL_VERIFY(rDataStore.SetBooleanValueToTable(propertiesTable, settings.kRooted, m_PlatformData.m_bRooted));
		if (settings.m_Generic.m_bSetEventPropertyInSandbox)
		{
			SEOUL_VERIFY(rDataStore.SetBooleanValueToTable(propertiesTable, settings.kSandboxed, AnalyticsManager::Get()->GetAnalyticsSandboxed()));
		}

		if (Engine::Get()->IsSamsungPlatformFlavor())
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kPlatform, "samsung"));
		}
		else if (Engine::Get()->IsAmazonPlatformFlavor())
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kPlatform, "amazon"));
		}
		else
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kPlatform, String(GetCurrentPlatformName()).ToLowerASCII()));
		}

		SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kPlayerGUID, m_sUserID));

		SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kAppSubVersion, m_sAppSubVersion));
		if (RenderDevice::Get())
		{
			auto const viewport = RenderDevice::Get()->GetBackBufferViewport();
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kAspectRatio, String::Printf("%1.2f", viewport.GetTargetAspectRatio())));
		}

		if (settings.m_Generic.m_bReportBuildVersionMajorWithAppVersion)
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kAppVersion, String::Printf("%d.%d", BUILD_VERSION_MAJOR, g_iBuildChangelist)));
		}
		else
		{
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kAppVersion, ToString(g_iBuildChangelist)));
		}

		if (settings.m_Generic.m_bReportPushNotificationStatus)
		{
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
			SEOUL_VERIFY(rDataStore.SetBooleanValueToTable(propertiesTable, settings.kPushEnabledName, Engine::Get()->HasEnabledRemoteNotifications()));
#else
			SEOUL_VERIFY(rDataStore.SetBooleanValueToTable(propertiesTable, settings.kPushEnabledName, false));
#endif // /#if SEOUL_WITH_REMOTE_NOTIFICATIONS
		}

		// Connection status.
		{
			NetworkConnectionType eType = NetworkConnectionType::kUnknown;
			if (!Engine::Get()->QueryNetworkConnectionType(eType))
			{
				eType = NetworkConnectionType::kUnknown;
			}
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kConnectionType, EnumToString<NetworkConnectionType>(eType)));
		}

		// Battery level.
		{
			Float32 fLevel = 0.0f;
			if (Engine::Get()->QueryBatteryLevel(fLevel))
			{
				auto const iBatteryLevel = Clamp((Int32)Round(fLevel * 100.0f), 0, 100);
				SEOUL_VERIFY(rDataStore.SetInt32ValueToTable(propertiesTable, settings.kBatteryLevel, iBatteryLevel));
			}
		}

		if (generic.m_bTrackSessions)
		{
			// Session properties.
			SEOUL_VERIFY(rDataStore.SetInt64ValueToTable(propertiesTable, settings.kSessionCountName, m_iSessionSequenceNumber));
			SEOUL_VERIFY(rDataStore.SetStringToTable(propertiesTable, settings.kSessionUUIDName, m_SessionUUID.ToString()));
		}
	}

	const ABTests& GetABTests() const
	{
		return m_tABTests;
	}

	void SetABTests(const ABTests& tABTests)
	{
		Strings vABTests;
		Strings vABTestGroups;
		for (auto const& e : tABTests)
		{
			auto const sTest(e.First.ToLowerASCII());
			vABTests.PushBack(sTest);
			vABTestGroups.PushBack(ToGroup(sTest, e.Second));
		}

		// Sort for consistency.
		QuickSort(vABTests.Begin(), vABTests.End());
		QuickSort(vABTestGroups.Begin(), vABTestGroups.End());

		m_vABTests.Swap(vABTests);
		m_vABTestGroups.Swap(vABTestGroups);
		m_tABTests = tABTests;
	}

	const String& GetAppSubVersion() const
	{
		return m_sAppSubVersion;
	}

	void SetAppSubVersion(const String& s)
	{
		m_sAppSubVersion = s;
	}

	Int64 m_iSessionSequenceNumber;
	UUID m_SessionUUID;
	PlatformData m_PlatformData;
	String m_sUserID;

private:
	Strings m_vABTests;
	Strings m_vABTestGroups;
	ABTests m_tABTests;
	String m_sAppSubVersion; // Not serialized

	void AddABTests(
		const MixpanelCommon::Settings& settings,
		DataStore& rDataStore,
		const DataNode& propertiesTable) const
	{
		MutableDataStoreTableUtil util(rDataStore, propertiesTable, HString());
		util.SetValue(settings.kAbTests, m_vABTests);
		util.SetValue(settings.kAbTestGroups, m_vABTestGroups);
	}

	static String ToGroup(const String& sTest, Int32 iGroup)
	{
		String const sGroup((UniChar)('a' + Max(iGroup, 0)), 1u);
		return sTest + "_" + sGroup;
	}
}; // class MixpanelStateData

class MixpanelState SEOUL_SEALED
{
public:
	typedef HashTable<String, Int32, MemoryBudgets::Analytics> ABTests;
	typedef Vector<CheckedPtr<MixpanelCommon::Entry>, MemoryBudgets::Analytics> Tasks;

	MixpanelState()
		: m_LastSessionStart()
		, m_vEvents()
		, m_vProfileUpdates()
		, m_sApiKey()
		, m_SaveTimestamp()
		, m_StateData()
		, m_tOnceTokens()
	{
	}

	~MixpanelState()
	{
		SafeDeleteVector(m_vProfileUpdates);
		SafeDeleteVector(m_vEvents);
	}

	void AddStandardProperties(
		const MixpanelCommon::Settings& settings,
		DataStore& rDataStore,
		const DataNode& propertiesTable) const
	{
		m_StateData.AddStandardProperties(settings, rDataStore, propertiesTable);
	}

	const ABTests& GetABTests() const { return m_StateData.GetABTests(); }
	void SetABTests(const ABTests& tABTests) { m_StateData.SetABTests(tABTests); }

	Int64 GetSessionSequenceNumber() const { return m_StateData.m_iSessionSequenceNumber; }
	void SetSessionSequenceNumber(Int64 iSessionSequenceNumber) { m_StateData.m_iSessionSequenceNumber = iSessionSequenceNumber; }
	void IncrementSessionSequenceNumber() { ++m_StateData.m_iSessionSequenceNumber; }

	UUID GetSessionUUID() const { return m_StateData.m_SessionUUID; }
	void SetSessionUUID(const UUID& sessionUUID) { m_StateData.m_SessionUUID = sessionUUID; }

	const PlatformData& GetPlatformData() const { return m_StateData.m_PlatformData; }
	void SetPlatformData(const PlatformData& platformData) { m_StateData.m_PlatformData = platformData; }

	const String& GetUserID() const { return m_StateData.m_sUserID;  }
	void SetUserID(const String& sUserID) { m_StateData.m_sUserID = sUserID; }

	const String& GetAppSubVersion() const { return m_StateData.GetAppSubVersion(); }
	void SetAppSubVersion(const String& s) { m_StateData.SetAppSubVersion(s); }

	WorldTime m_LastSessionStart;
	Tasks m_vEvents;
	Tasks m_vProfileUpdates;
	String m_sApiKey;
	WorldTime m_SaveTimestamp;
	MixpanelStateData m_StateData;

	typedef HashTable<String, String, MemoryBudgets::Analytics> OnceTokens;
	OnceTokens m_tOnceTokens;

private:
	SEOUL_DISABLE_COPY(MixpanelState);
}; // class MixpanelState

SEOUL_SPEC_TEMPLATE_TYPE(CheckedPtr<MixpanelCommon::Entry>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<CheckedPtr<MixpanelCommon::Entry>, 0>)
SEOUL_BEGIN_TYPE(MixpanelState, TypeFlags::kDisableCopy)
	SEOUL_PROPERTY_PAIR_N("ABTests", GetABTests, SetABTests)
	SEOUL_PROPERTY_PAIR_N("SessionSequenceNumber", GetSessionSequenceNumber, SetSessionSequenceNumber)
	SEOUL_PROPERTY_N("LastSessionStart", m_LastSessionStart)
	SEOUL_PROPERTY_PAIR_N("SessionUUID", GetSessionUUID, SetSessionUUID)
	SEOUL_PROPERTY_N("Events", m_vEvents)
	SEOUL_PROPERTY_N("ProfileUpdates", m_vProfileUpdates)
	SEOUL_PROPERTY_PAIR_N("PlatformData", GetPlatformData, SetPlatformData)
	SEOUL_PROPERTY_N("ApiKey", m_sApiKey)
	SEOUL_PROPERTY_PAIR_N("UserID", GetUserID, SetUserID)
	SEOUL_PROPERTY_N("SaveTimestamp", m_SaveTimestamp)
	SEOUL_PROPERTY_N("OnceTokens", m_tOnceTokens)
SEOUL_END_TYPE()

class MixpanelBuilder SEOUL_SEALED
{
public:
	MixpanelBuilder(const MixpanelCommon::Settings& settings, MixpanelState& r)
		: m_Settings(settings)
		, m_r(r)
		, m_vSessionEvents()
		, m_OrigSessionUUID(r.GetSessionUUID())
		, m_iOrigSessionSequenceNumber(r.GetSessionSequenceNumber())
		, m_OrigSessionStart(r.m_LastSessionStart)
	{
	}

	const MixpanelCommon::Settings& m_Settings;
	MixpanelState& m_r;

	typedef Vector<AnalyticsSessionChangeEvent, MemoryBudgets::Analytics> SessionEvents;
	SessionEvents m_vSessionEvents;

	void OnFailure()
	{
		// Clear SessionEvents for the next run.
		m_vSessionEvents.Clear();

		// Restore original values.
		m_r.SetSessionUUID(m_OrigSessionUUID);
		m_r.SetSessionSequenceNumber(m_iOrigSessionSequenceNumber);
		m_r.m_LastSessionStart = m_OrigSessionStart;
	}

	void OnSuccess()
	{
		// Dispatch session change events - run on main thread.
		if (!m_vSessionEvents.IsEmpty())
		{
			Jobs::AsyncFunction(
				GetMainThreadId(),
				&DispatchSessionChangeEvents,
				m_vSessionEvents);

			// Copy has been made, done.
			m_vSessionEvents.Clear();
		}
	}

private:
	static void DispatchSessionChangeEvents(const SessionEvents& v)
	{
		// If analytics manager was already destroyed, done.
		if (!AnalyticsManager::Get())
		{
			return;
		}

		// Dispatch each.
		for (auto const& e : v)
		{
			Events::Manager::Get()->TriggerEvent<const AnalyticsSessionChangeEvent&>(
				AnalyticsSessionGameEventId,
				e);
		}
	}

	UUID const m_OrigSessionUUID;
	Int64 const m_iOrigSessionSequenceNumber;
	WorldTime const m_OrigSessionStart;

	SEOUL_DISABLE_COPY(MixpanelBuilder);
}; // class MixpanelBuilder

namespace MixpanelCommon
{

void Entry::OnTrack(MixpanelState& r)
{
	m_SaveTimestamp = r.m_SaveTimestamp;
}

Bool EventEntry::AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const
{
	auto const& settings = r.m_Settings;
	auto const& state = r.m_r;
	const AnalyticsEvent& evt = m_Event;
	const WorldTime& eventTimeStamp = GetTimestamp();
	UInt32 const uTimeStampInSeconds = ToAnalyticsTime(eventTimeStamp);

	DataStore ds;
	ds.MakeTable();

	auto const root = ds.GetRootNode();
	SEOUL_VERIFY(ds.SetStringToTable(root, settings.kEventId, evt.GetName()));
	SEOUL_VERIFY(ds.SetTableToTable(root, settings.kProperties));

	DataNode props;
	SEOUL_VERIFY(ds.GetValueFromTable(root, settings.kProperties, props));
	if (evt.GetProperties().GetRootNode().IsTable())
	{
		SEOUL_VERIFY(ds.DeepCopy(
			evt.GetProperties(),
			evt.GetProperties().GetRootNode(),
			props));
	}

	// Add token, time, and distinct_id
	SEOUL_VERIFY(ds.SetStringToTable(props, settings.kTokenEvent, state.m_sApiKey));
	SEOUL_VERIFY(ds.SetUInt32ValueToTable(props, settings.kTime, uTimeStampInSeconds));
	SEOUL_VERIFY(ds.SetStringToTable(props, settings.kDistinctIdEvent, state.GetUserID()));

	// Add common properties.
	state.AddStandardProperties(r.m_Settings, ds, props);

	// Append.
	String sDataValueAsJSON;
	ds.ToString(ds.GetRootNode(), sDataValueAsJSON, false, 0, true);
	rs.Append(sSep);
	rs.Append(sDataValueAsJSON);
	return true;
}

void SequencedEntry::OnTrack(MixpanelState& r)
{
	Entry::OnTrack(r);
}

Bool ProfileEntry::AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const
{
	auto const& settings = r.m_Settings;
	auto const& state = r.m_r;
	auto const& updates = m_Update.GetUpdates();
	HString const op(ToMixpanelProfileOpString(m_Update.GetOp()));

	// Setup the table (key is the operation token).
	DataStore ds;
	ds.MakeTable();

	// Prepare updates section - usually a table,
	// but can be an array in the case of $unset.
	if (updates.GetRootNode().IsArray())
	{
		SEOUL_VERIFY(ds.SetArrayToTable(ds.GetRootNode(), op));
	}
	else
	{
		SEOUL_VERIFY(ds.SetTableToTable(ds.GetRootNode(), op));
	}

	// Add token and distinct_id.
	SEOUL_VERIFY(ds.SetStringToTable(ds.GetRootNode(), settings.kTokenUpdate, state.m_sApiKey));
	SEOUL_VERIFY(ds.SetStringToTable(ds.GetRootNode(), settings.kDistinctIdUpdate, state.GetUserID()));

	// Add properties.
	DataNode props;
	SEOUL_VERIFY(ds.GetValueFromTable(ds.GetRootNode(), op, props));
	if (!updates.GetRootNode().IsNull())
	{
		SEOUL_VERIFY(ds.DeepCopy(
			updates,
			updates.GetRootNode(),
			props));
	}

	// Append.
	String sDataValueAsJSON;
	ds.ToString(ds.GetRootNode(), sDataValueAsJSON, false, 0, true);
	rs.Append(sSep);
	rs.Append(sDataValueAsJSON);
	return true;
}

Bool SessionEndEntry::AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const
{
	auto const& settings = r.m_Settings;
	auto const& state = r.m_r;
	const WorldTime& eventTimeStamp = GetTimestamp();
	UInt32 const uTimeStampInSeconds = ToAnalyticsTime(eventTimeStamp);
	TimeInterval const sessionLength = (state.m_LastSessionStart.IsZero()
		? TimeInterval(0)
		: (eventTimeStamp - state.m_LastSessionStart));

	// Record this entry for later (potential) dispatch to other systems.
	AnalyticsSessionChangeEvent evt;
	evt.m_bSessionStart = false;
	evt.m_Duration = sessionLength;
	evt.m_SessionUUID = state.GetSessionUUID();
	evt.m_TimeStamp = eventTimeStamp;
	r.m_vSessionEvents.PushBack(evt);

	DataStore ds;
	ds.MakeTable();

	auto const root = ds.GetRootNode();

	// Common properties.
	SEOUL_VERIFY(ds.SetStringToTable(root, settings.kEventId, settings.kSessionEventName));
	SEOUL_VERIFY(ds.SetTableToTable(root, settings.kProperties));

	DataNode props;
	SEOUL_VERIFY(ds.GetValueFromTable(root, settings.kProperties, props));

	// Session properties.
	SEOUL_VERIFY(ds.SetUInt32ValueToTable(props, settings.kSessionLengthName, ToAnalyticsTime(sessionLength)));

	// Add token, time, and distinct_id
	SEOUL_VERIFY(ds.SetStringToTable(props, settings.kTokenEvent, state.m_sApiKey));
	SEOUL_VERIFY(ds.SetUInt32ValueToTable(props, settings.kTime, uTimeStampInSeconds));
	SEOUL_VERIFY(ds.SetStringToTable(props, settings.kDistinctIdEvent, state.GetUserID()));

	// Add common properties.
	state.AddStandardProperties(r.m_Settings, ds, props);

	// Append.
	String sDataValueAsJSON;
	ds.ToString(ds.GetRootNode(), sDataValueAsJSON, false, 0, true);
	rs.Append(sSep);
	rs.Append(sDataValueAsJSON);
	return true;
}

Bool SessionStartEntry::AppendJSON(MixpanelBuilder& r, String& rs, Byte const* sSep) const
{
	auto const& settings = r.m_Settings;
	auto const& state = r.m_r;

	if (!settings.m_Generic.m_bTrackSessions)
	{
		return false; // Early out, no session tracking.
	}

	const WorldTime& eventTimeStamp = GetTimestamp();

	// Generate UUID.
	auto const uuid = UUID::GenerateV4();

	// Record this entry for later (potential) dispatch to other systems.
	AnalyticsSessionChangeEvent evt;
	evt.m_bSessionStart = true;
	evt.m_Duration = TimeInterval();
	evt.m_SessionUUID = uuid;
	evt.m_TimeStamp = eventTimeStamp;
	r.m_vSessionEvents.PushBack(evt);

	// Update last session start time and sequence number.
	r.m_r.SetSessionUUID(uuid);
	r.m_r.IncrementSessionSequenceNumber();
	r.m_r.m_LastSessionStart = eventTimeStamp;

	DataStore ds;
	ds.MakeTable();

	auto const root = ds.GetRootNode();

	// Common properties.
	SEOUL_VERIFY(ds.SetStringToTable(root, settings.kEventId, settings.kSessionStartName));
	SEOUL_VERIFY(ds.SetTableToTable(root, settings.kProperties));

	DataNode props;
	SEOUL_VERIFY(ds.GetValueFromTable(root, settings.kProperties, props));

	// Add token, time, and distinct_id
	SEOUL_VERIFY(ds.SetStringToTable(props, settings.kTokenEvent, state.m_sApiKey));
	SEOUL_VERIFY(ds.SetUInt32ValueToTable(props, settings.kTime, ToAnalyticsTime(eventTimeStamp)));
	SEOUL_VERIFY(ds.SetStringToTable(props, settings.kDistinctIdEvent, state.GetUserID()));

	// Add common properties.
	state.AddStandardProperties(r.m_Settings, ds, props);

	// Append.
	String sDataValueAsJSON;
	ds.ToString(ds.GetRootNode(), sDataValueAsJSON, false, 0, true);
	rs.Append(sSep);
	rs.Append(sDataValueAsJSON);

	return true;
}

void SessionStartEntry::OnTrack(MixpanelState& r)
{
	SequencedEntry::OnTrack(r);
}

Settings::Settings(const GenericAnalyticsManagerSettings& generic)
	: m_Generic(generic)
	, kAbTests(generic.m_sPropertyPrefix + "ab_tests_active")
	, kAbTestGroups(generic.m_sPropertyPrefix + "ab_test_groups_active")
	, kAdvertisingId(generic.m_sPropertyPrefix + "advertising_id")
	, kAppSubVersion(generic.m_sPropertyPrefix + "app_sub_version")
	, kAppVersion(generic.m_sPropertyPrefix + "app_version")
	, kAspectRatio(generic.m_sPropertyPrefix + "aspect_ratio")
	, kBatteryLevel(generic.m_sPropertyPrefix + "battery_level")
	, kConnectionType(generic.m_sPropertyPrefix + "connection_type")
	, kDeviceManufacturer(generic.m_sPropertyPrefix + "device_manufacturer")
	, kDeviceModel(generic.m_sPropertyPrefix + "device_model")
	, kDeviceId(generic.m_sPropertyPrefix + "device_id")
	, kDistinctIdEvent("distinct_id") // Mixpanel builtin, don't prefix.
	, kDistinctIdUpdate("$distinct_id") // Mixpanel builtin, don't prefix.
	, kEventId("event") // Mixpanel builtin, don't prefix.
	, kEventType("event_type") // Mixpanel builtin, don't prefix.
	, kOs(generic.m_sPropertyPrefix + "os")
	, kPlatform(generic.m_sPropertyPrefix + "platform")
	, kPlayerGUID(generic.m_sPropertyPrefix + "player_guid")
	, kProperties("properties") // Mixpanel builtin, don't prefix.
	, kPushEnabledName(generic.m_sPropertyPrefix + "push_enabled")
	, kRooted(generic.m_sPropertyPrefix + "is_rooted")
	, kSandboxed(generic.m_sPropertyPrefix + "in_sandbox")
	, kPropUACampaign(generic.m_sPropertyPrefix + "af_campaign")
	, kPropUAMediaSource(generic.m_sPropertyPrefix + "af_media_source")
	, kUnspecifiedPropertyValue("unspecified")
	, kSessionEventName("$ae_session")
	, kSessionLengthName("$ae_session_length")
	, kSessionCountName("s_player_sessions")
	, kSessionUUIDName("s_session_id")
	, kSessionStartName("SessionStart")
	, kTime("time")
	, kTokenEvent("token")
	, kTokenUpdate("$token")
{
}

} // namespace MixpanelCommon

SEOUL_BEGIN_TYPE(MixpanelCommon::Entry)
	SEOUL_ATTRIBUTE(PolymorphicKey, "$type")
	SEOUL_PROPERTY_N("Timestamp", m_Timestamp)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(MixpanelCommon::SequencedEntry)
	SEOUL_PARENT(MixpanelCommon::Entry)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(MixpanelCommon::EventEntry, TypeFlags::kDisableCopy)
	SEOUL_PARENT(MixpanelCommon::SequencedEntry)
	SEOUL_PROPERTY_N("Event", m_Event)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(MixpanelCommon::FlushEntry, TypeFlags::kDisableCopy)
	SEOUL_PARENT(MixpanelCommon::Entry)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(MixpanelCommon::ProfileEntry, TypeFlags::kDisableCopy)
	SEOUL_PARENT(MixpanelCommon::Entry)
	SEOUL_PROPERTY_N("Update", m_Update)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(MixpanelCommon::SessionEndEntry, TypeFlags::kDisableCopy)
	SEOUL_PARENT(MixpanelCommon::SequencedEntry)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(MixpanelCommon::SessionStartEntry, TypeFlags::kDisableCopy)
	SEOUL_PARENT(MixpanelCommon::SequencedEntry)
SEOUL_END_TYPE()

struct MixpanelVerboseResponse SEOUL_SEALED
{
	Int m_iStatus = 0;
	String m_sError;
}; // struct MixpanelVerboseResponse
SEOUL_BEGIN_TYPE(MixpanelVerboseResponse)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("status", m_iStatus)
	SEOUL_PROPERTY_N("error", m_sError)
SEOUL_END_TYPE()

namespace
{

static Signal s_ActivitySignal;
static Signal s_TaskSignal;
class Callbacks SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(Callbacks);

	Callbacks()
		: m_bSuccess(false)
		, m_bDone(false)
		, m_bActive(false)
	{
	}

	void AcquireData(Reflection::WeakAny& rp)
	{
		rp = m_pData;
		m_pData.Reset();
	}

	HTTP::CallbackResult HTTPCallback(HTTP::Result eResult, HTTP::Response* pResponse)
	{
		// Only retry this request if there was a network failure; if we
		// connected over HTTP, don't resend (to avoid the risk of duplicate events).
		// Matches Mixpanel's Android SDK behavior:
		// https://github.com/mixpanel/mixpanel-android/blob/a89ba15568809add9f42e7253c7f9a6a349a522e/src/main/java/com/mixpanel/android/mpmetrics/AnalyticsMessages.java#L501
		m_bSuccess = (HTTP::Result::kSuccess == eResult);

#if SEOUL_LOGGING_ENABLED
		if (HTTP::Result::kSuccess == eResult)
		{
			auto const iStatus = pResponse->GetStatus();
			if (iStatus < 400)
			{
				// Check response body - will be a JSON blob if verbose is enabled, otherwise just the number value 1 or 0.
				String const s((Byte const*)pResponse->GetBody(), (UInt32)pResponse->GetBodySize());

				// Log a warning if the resulting body isn't a status of 1
				MixpanelVerboseResponse response;
				if (Reflection::DeserializeFromString(s, &response) && 1 != response.m_iStatus)
				{
					SEOUL_WARN("Mixpanel error message: %s",
						response.m_sError.CStr());
				}
			}

			if (iStatus >= 400 && iStatus <= 499)
			{
				if (408 != iStatus) // Timeout, suppress.
				{
					// Warn about this, malformed input likely.
					SEOUL_WARN("Mixpanel error (%d): %s",
						iStatus,
						String((Byte const*)pResponse->GetBody(), pResponse->GetBodySize()).CStr());
				}
			}
		}
#endif

		SeoulMemoryBarrier();
		m_bDone = true;
		SeoulMemoryBarrier();
		s_ActivitySignal.Activate();

		return HTTP::CallbackResult::kSuccess;
	}

	Bool IsDone() const
	{
		return m_bDone;
	}

	void Reset()
	{
		if (m_pData.IsValid())
		{
			m_pData.GetType().Delete(m_pData);
			m_pData.Reset();
		}
		m_bSuccess = false;
		m_bDone = false;
	}

	void OnLoad(
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult,
		const Reflection::WeakAny& pData)
	{
		// Cache load result.
		m_pData = pData;

		// Common handling.
		OnSaveOrLoad(eLocalResult, eCloudResult, eFinalResult);
	}

	void OnSave(
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult)
	{
		// Match parity with old code, make sure data is reset when
		// we receieve a save result.
		m_pData.Reset();

		// Common handling.
		OnSaveOrLoad(eLocalResult, eCloudResult, eFinalResult);
	}

	void SetActive(Bool bActive)
	{
		m_bActive = bActive;
	}

	Bool WasSuccessful() const
	{
		return m_bSuccess;
	}

private:
	Reflection::WeakAny m_pData;
	Atomic32Value<Bool> m_bSuccess;
	Atomic32Value<Bool> m_bDone;
	Atomic32Value<Bool> m_bActive;

	void OnSaveOrLoad(
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult)
	{
		// If not active, destroy immediately.
		if (!m_bActive)
		{
			Reset();
		}
		else
		{
			m_bSuccess = (SaveLoadResult::kSuccess == eFinalResult);
		}

		SeoulMemoryBarrier();
		m_bDone = true;
		SeoulMemoryBarrier();
		s_ActivitySignal.Activate();
	}
};  // struct Callbacks
static Callbacks s_Callbacks;

class CallbacksBind SEOUL_SEALED : public ISaveLoadOnComplete
{
public:
	CallbacksBind() = default;
	~CallbacksBind() = default;

	virtual Bool DispatchOnMainThread() const SEOUL_OVERRIDE
	{
		// Safe and desirable to find out about load or save
		// completion immediately without waiting for the main thread.
		return false;
	}

	virtual void OnLoadComplete(
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult,
		const Reflection::WeakAny& pData) SEOUL_OVERRIDE
	{
		s_Callbacks.OnLoad(eLocalResult, eCloudResult, eFinalResult, pData);
	}

	virtual void OnSaveComplete(
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult) SEOUL_OVERRIDE
	{
		s_Callbacks.OnSave(eLocalResult, eCloudResult, eFinalResult);
	}

private:
	SEOUL_DISABLE_COPY(CallbacksBind);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(CallbacksBind);
}; // class CallbacksBind

} // namespace anonymous

MixpanelAnalyticsManager::MixpanelAnalyticsManager(const GenericAnalyticsManagerSettings& settings)
	: m_Settings(MixpanelCommon::Settings(settings))
	, m_sBaseEventURL(settings.m_GetBaseEventURL.IsValid() ? settings.m_GetBaseEventURL() : "https://api.mixpanel.com/track")
	, m_sBaseProfileURL(settings.m_GetBaseProfileURL.IsValid() ? settings.m_GetBaseProfileURL() : "https://api.mixpanel.com/engage")
	, m_sApiKey()
	, m_Tasks()
	, m_bInBackground(false)
	, m_bShuttingDown(false)
{
	// Tell the callback handler that we're active - if
	// a late callback is returned (e.g. save system),
	// it will immediately destroy the received data.
	s_Callbacks.SetActive(true);

	if (m_Settings.m_Generic.m_GetApiKeyDelegate.IsValid())
	{
		m_sApiKey = m_Settings.m_Generic.m_GetApiKeyDelegate();
	}

	// Kick off the worker thread.
	m_pWorkerThread.Reset(SEOUL_NEW(MemoryBudgets::Analytics) Thread(SEOUL_BIND_DELEGATE(&MixpanelAnalyticsManager::WorkerThread, this), false));
	m_pWorkerThread->Start("Mixpanel Worker");
	m_pWorkerThread->SetPriority(Thread::kLow);
}

MixpanelAnalyticsManager::~MixpanelAnalyticsManager()
{
	// Tell the callback handler that we're done - if
	// a late callback is returned (e.g. save system),
	// it will immediately destroy the received data.
	s_Callbacks.SetActive(false);

	// Inform and then wait for completion.
	m_pWorkerThread->SetPriority(Thread::kCritical);
	m_bInBackground = false;
	m_bShuttingDown = true;
	SeoulMemoryBarrier();
	s_ActivitySignal.Activate();
	s_TaskSignal.Activate();
	m_pWorkerThread.Reset();

	// Cleanup any remaining time updates.
	{
		auto p = m_TimeChanges.Pop();
		while (nullptr != p)
		{
			SafeDelete(p);
			p = m_TimeChanges.Pop();
		}
	}

	// Cleanup any remaining entries.
	{
		auto p = m_Tasks.Pop();
		while (nullptr != p)
		{
			SafeDelete(p);
			p = m_Tasks.Pop();
		}
	}

	// Finally, cleanup callback.
	s_Callbacks.Reset();
}

/**
 * As necessary, tell the Analytics system to immediately attempt
 * to submit any pending analytics data.
 */
void MixpanelAnalyticsManager::Flush()
{
	using namespace MixpanelCommon;

	// Create a flush event entry.
	auto pEntry(SEOUL_NEW(MemoryBudgets::Analytics) FlushEntry(Now()));

	// Enqueue the event and poke the worker thread.
	m_Tasks.Push(pEntry);
	s_TaskSignal.Activate();
}

/** Get the current analytics API key. */
String MixpanelAnalyticsManager::GetApiKey() const
{
	String sReturn;
	{
		Lock lock(m_Mutex);
		sReturn = m_sApiKey;
	}

	return sReturn;
}

void MixpanelAnalyticsManager::UpdateSessionCountFromPersistence(Int64 iPersistenceSessionCount)
{
	Lock lock(m_Mutex);

	if (m_pStateData.IsValid())
	{
		m_pStateData->m_iSessionSequenceNumber = Max(m_pStateData->m_iSessionSequenceNumber, iPersistenceSessionCount);
		return;
	}

	SEOUL_WARN("Called MixpanelAnalyticsManager::UpdateSessionCountFromPersistence before the MixpanelStateData was initialized");
}

/** Get the current analytics session count. */
Int64 MixpanelAnalyticsManager::GetSessionCount() const
{
	Lock lock(m_Mutex);

	if (m_pStateData.IsValid())
	{
		return m_pStateData->m_iSessionSequenceNumber;
	}

	SEOUL_WARN("Called MixpanelAnalyticsManager::GetSessionCount before the MixpanelStateData was initialized, returning 0.");
	return 0;
}

/** Change the API key for future analytic events */
void MixpanelAnalyticsManager::SetApiKey(const String& sApiKey)
{
	{
		Lock lock(m_Mutex);
		m_sApiKey = sApiKey;
	}
	s_TaskSignal.Activate();
}

/** Update the current analytics user ID. The analytics system will wait for this to be set before sending analytics events. */
void MixpanelAnalyticsManager::SetAnalyticsUserID(const String& sUserID)
{
	AnalyticsManager::SetAnalyticsUserID(sUserID);
	s_TaskSignal.Activate();
}

/**
 * Odd utility hook - if the reported time of a time function changes (e.g. server time is received),
 * this function is used to allow an AnalyticsManager subclass to react to the change.
 */
void MixpanelAnalyticsManager::OnTimeFunctionTimeChange(const TimeFunction& function, TimeInterval deltaTime)
{
	// If the time changed against our custom delegate, refresh all pending event times.
	if (function == m_Settings.m_Generic.m_CustomCurrentTimeDelegate)
	{
		m_TimeChanges.Push(SEOUL_NEW(MemoryBudgets::Analytics) TimeInterval(deltaTime));
	}
}

void MixpanelAnalyticsManager::SyncMixpanelStateData(MixpanelStateData& stateData)
{
	Lock lock(m_Mutex);

	if (m_pStateData.IsValid())
	{
		// Don't allow the session sequence value to decrease
		// Store off the maximum value before we overwrite the manager's state data
		auto iSessionSequenceNumber = Max(m_pStateData->m_iSessionSequenceNumber, stateData.m_iSessionSequenceNumber);

		// update the shadow from the worker thread data
		*m_pStateData = stateData;

		// Force the worker thread and the analytics manager to agree WRT the session count
		// This is to handle the case where the user switches devices
		// TODO:find a better way to handle this - will require an extensive refactor,
		// eg to sync session count directly from the server with the first login
		m_pStateData->m_iSessionSequenceNumber = iSessionSequenceNumber;
		stateData.m_iSessionSequenceNumber = iSessionSequenceNumber;
	}
	else
	{
		auto pStateData = SEOUL_NEW(MemoryBudgets::Analytics) MixpanelStateData(stateData);
		m_pStateData.Reset(pStateData);
	}
}

/**
* Add analytics state properties to a data store.  For events that are reported by an external service,
* in particular purchase events reported by the server.
*/
Bool MixpanelAnalyticsManager::AddStateProperties(DataStore& rDataStore, const DataNode& propertiesTable) const
{
	Lock lock(m_Mutex);

	if (m_pStateData.IsValid())
	{
		m_pStateData->AddStandardProperties(m_Settings, rDataStore, propertiesTable);
		return true;
	}

	return false;
}

void MixpanelAnalyticsManager::SetAttributionData(const String& sCampaign, const String& sMediaSource)
{
	{
		Lock lock(m_Mutex);

		if (m_pStateData.IsValid())
		{
			m_pStateData->m_PlatformData.m_sUACampaign = sCampaign;
			m_pStateData->m_PlatformData.m_sUAMediaSource = sMediaSource;
		}
	}

	// Send the update to Mixpanel immediately as well
	AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kSet);
	auto& ds = update.GetUpdates();
	ds.MakeTable();
	auto const root = ds.GetRootNode();
	ds.SetStringToTable(root, m_Settings.kPropUACampaign, sCampaign);
	ds.SetStringToTable(root, m_Settings.kPropUAMediaSource, sMediaSource);
	AnalyticsManager::Get()->UpdateProfile(update);
}

void MixpanelAnalyticsManager::DoEnterBackground()
{
	// Log for testing and debug tracking.
	SEOUL_LOG("MixpanelAnalyticsManager::DoEnterBackground()");

	// Now in the background.
	m_bInBackground = true;
}

void MixpanelAnalyticsManager::DoLeaveBackground()
{
	// Log for testing and debug tracking.
	SEOUL_LOG("MixpanelAnalyticsManager::DoLeaveBackground()");

	if (m_bInBackground)
	{
		// No longer in the background.
		m_bInBackground = false;

		// Wake up the worker thread.
		s_TaskSignal.Activate();
	}
}

/**
 * MixpanelAnalyticsManager implements DoTrackEvent() by using
 * the /event/ functionality of Mixpanel.
 */
void MixpanelAnalyticsManager::DoTrackEvent(
	const AnalyticsEvent& evt,
	const WorldTime& timestamp)
{
	using namespace MixpanelCommon;

	// Get the timestamp for the event.
	WorldTime const finalTimestamp = (WorldTime() == timestamp
		? Now()
		: timestamp);

	// Create an event entry.
	auto pEntry(SEOUL_NEW(MemoryBudgets::Analytics) EventEntry(
		evt,
		finalTimestamp));

	// Enqueue the event and poke the worker thread.
	m_Tasks.Push(pEntry);
	s_TaskSignal.Activate();
}

/**
 * MixpanelAnalyticsManager implements DoTrackSessionEnd() by using
 * the /event/ functionality of Mixpanel.
 */
void MixpanelAnalyticsManager::DoTrackSessionEnd(
	const WorldTime& timestamp)
{
	using namespace MixpanelCommon;

	// Don't report this event if it is disabled in settings
	if (!m_Settings.m_Generic.m_bReportAppSession)
	{
		return;
	}

	// Get the timestamp for the event.
	WorldTime const finalTimestamp = (WorldTime() == timestamp
		? Now()
		: timestamp);

	// Create an session end entry.
	auto pEntry(SEOUL_NEW(MemoryBudgets::Analytics) SessionEndEntry(
		finalTimestamp));

	// Enqueue the event and poke the worker thread.
	m_Tasks.Push(pEntry);
	s_TaskSignal.Activate();
}

/**
 * MixpanelAnalyticsManager implements DoTrackSessionEnd() by using
 * the /event/ functionality of Mixpanel.
 */
void MixpanelAnalyticsManager::DoTrackSessionStart(const WorldTime& timestamp)
{
	using namespace MixpanelCommon;

	// Get the timestamp for the event.
	WorldTime const finalTimestamp = (WorldTime() == timestamp
		? Now()
		: timestamp);

	// Create an session start entry.
	auto pEntry(SEOUL_NEW(MemoryBudgets::Analytics) SessionStartEntry(
		finalTimestamp));

	// Enqueue the event and poke the worker thread.
	m_Tasks.Push(pEntry);
	s_TaskSignal.Activate();
}

/**
 * MixpanelAnalyticsManager implements DoUpdateProfile() by using
 * the /engage/ functionality of Mixpanel.
 */
void MixpanelAnalyticsManager::DoUpdateProfile(
	const AnalyticsProfileUpdate& update,
	const WorldTime& timestamp)
{
	using namespace MixpanelCommon;

	// Get the timestamp for the event.
	WorldTime const finalTimestamp = (WorldTime() == timestamp
		? Now()
		: timestamp);

	// Create an event entry.
	auto pEntry(SEOUL_NEW(MemoryBudgets::Analytics) ProfileEntry(
		update,
		finalTimestamp));

	// Enqueue the event and poke the worker thread.
	m_Tasks.Push(pEntry);
	s_TaskSignal.Activate();
}

/** @return The current world time in UTC, possibly routed through our custom time delegate. */
WorldTime MixpanelAnalyticsManager::Now() const
{
	if (m_Settings.m_Generic.m_CustomCurrentTimeDelegate.IsValid())
	{
		return m_Settings.m_Generic.m_CustomCurrentTimeDelegate();
	}
	else
	{
		return WorldTime::GetUTCTime();
	}
}

/**
 * Body that handles saving and submission of analytics events.
 */
Int MixpanelAnalyticsManager::WorkerThread(const Thread&)
{
	// Initial heartbeat time.
	Int64 iHeartbeatTimerInTicks = SeoulTime::GetGameTimeInTicks();

	// Temporary and persistent state.
	ScopedPtr<MixpanelState> pState(WorkerThreadLoadState());

	// Load should only return null if we're shutting down.
	SEOUL_ASSERT(pState.IsValid() || m_bShuttingDown);

	// Early out if shutting down before loading state.
	if (m_bShuttingDown)
	{
		// Clear data prior to return.
		s_Callbacks.Reset();
		return 0;
	}

	// Propagate initial state to the MixpanelAnalyticsManager shadow.
	SyncMixpanelStateData(pState->m_StateData);

	// Loop forever.
	Bool bFlush = false;
	while (true)
	{
		// Get the API key.
		pState->m_sApiKey = GetApiKey();

		// Propagate the analytics user id.
		pState->SetUserID(GetAnalyticsUserID());

		// Propagate app sub version.
		pState->SetAppSubVersion(GetSubVersionString());

		// Get platform data state.
		Engine::Get()->GetPlatformData(pState->m_StateData.m_PlatformData);

		// Get A/B test state.
		pState->SetABTests(GetABTests());

		// Consume.
		{
			Bool bNeedFlush = false;
			(void)WorkerThreadConsumeTasks(*pState, bNeedFlush);
			bFlush = bFlush || bNeedFlush;
		}

		// TODO: This can potentially produce the wrong time stamp
		// if a time change occurs but we apply it to a new event
		// that has been buffered since the time change occured.

		// Apply any time changes.
		WorkerThreadApplyTimeChanges(*pState);

		// On shutdown, break immediately, unless
		// there are still entries in the queue.
		// In either case, don't pass this line,
		// so we don't wait on the signal again.
		if (m_bShuttingDown)
		{
			if (m_Tasks.IsEmpty())
			{
				break;
			}
			else
			{
				continue;
			}
		}

		// If the analytics user ID is still empty, can't process.
		if (pState->GetUserID().IsEmpty())
		{
			// Propagate state prior to indefinite wait.
			SyncMixpanelStateData(pState->m_StateData);

			s_TaskSignal.Wait();
			continue;
		}

		// Check if we should submit events.
		Int64 const iCurrentTimeInTicks = SeoulTime::GetGameTimeInTicks();
		Double const fDeltaInSeconds = SeoulTime::ConvertTicksToSeconds(iCurrentTimeInTicks - iHeartbeatTimerInTicks);
		if (bFlush || fDeltaInSeconds >= m_Settings.m_Generic.m_fHeartbeatTimeInSeconds)
		{
			// Process state.
			WorkerThreadSubmitTasks(*pState);

			// Reset the heartbeat timer.
			iHeartbeatTimerInTicks = SeoulTime::GetGameTimeInTicks();

			// Done with a Flush.
			bFlush = false;
		}

		// Propagate state after processing.
		SyncMixpanelStateData(pState->m_StateData);

		// Go to sleep if no pending tasks or in the background.
		if (m_Tasks.IsEmpty() || m_bInBackground)
		{
			// Wait for the heartbeat interval.
			UInt32 const uWaitTimeInMilliseconds = (UInt32)(Floor(
				Fmod(fDeltaInSeconds, m_Settings.m_Generic.m_fHeartbeatTimeInSeconds)) * 1000.0f);

			// Indefinite wait if in the background.
			if (m_bInBackground)
			{
				s_TaskSignal.Wait();
			}
			// Timed wait.
			else
			{
				s_TaskSignal.Wait(uWaitTimeInMilliseconds);
			}
		}
	}

	// Clear data prior to return.
	s_Callbacks.Reset();
	return 0;
}

/**
 * Some time functions can shift time. This applies
 * those shifts to correct our already record analytics
 * times.
 */
void MixpanelAnalyticsManager::WorkerThreadApplyTimeChanges(MixpanelState& r)
{
	auto pDelta = m_TimeChanges.Pop();
	while (nullptr != pDelta)
	{
		auto const& delta = *pDelta;

		SEOUL_LOG_ANALYTICS("Mixpanel: Adjusting analytics times by %.2f seconds.",
			delta.GetSecondsAsDouble());

		// Events.
		for (auto const& p : r.m_vEvents)
		{
			p->SetTimestamp(p->GetTimestamp() + delta);
		}

		// Profile updates
		for (auto const& p : r.m_vProfileUpdates)
		{
			p->SetTimestamp(p->GetTimestamp() + delta);
		}

		SafeDelete(pDelta);
		pDelta = m_TimeChanges.Pop();
	}
}

/**
 * Pulls tasks from the atomic tasks buffer and appends to our persistent state.
 */
Bool MixpanelAnalyticsManager::WorkerThreadConsumeTasks(MixpanelState& r, Bool& rbFlush)
{
	using namespace MixpanelCommon;

	// Update the saved timestamp so it is picked up by newly added tasks.
	r.m_SaveTimestamp = Now();

	// Now consume and add tasks.
	Bool bChanged = false;
	auto p = m_Tasks.Pop();
	while (nullptr != p)
	{
		bChanged = true;
		p->OnTrack(r);

		switch (p->GetType())
		{
			// We don't keep flush entries, they just serve as markers for
			// immediate flushes.
		case EntryType::kFlush:
			rbFlush = true;
			SafeDelete(p);
			break;

			// Profile entries go in that list.
		case EntryType::kProfile:
			r.m_vProfileUpdates.PushBack(p);
			break;

			// All others go in events.
		case EntryType::kEvent:
			// Check for once token and if set, filter. Don't send once events
			// that have already been sent based on the token and analytics state.
			{
				auto const& evt = static_cast<MixpanelCommon::EventEntry*>(p)->GetEvent();
				String sExistingOnceToken;
				if (!evt.GetOnceToken().IsEmpty() &&
					r.m_tOnceTokens.GetValue(evt.GetName(), sExistingOnceToken) &&
					sExistingOnceToken == evt.GetOnceToken())
				{
					// Can skip this event, once token has not yet changed, event
					// already sent.
					SafeDelete(p);
					break;
				}
			}
			r.m_vEvents.PushBack(p);
			break;

		case EntryType::kSessionEnd:
		case EntryType::kSessionStart:
		default:
			r.m_vEvents.PushBack(p);
			break;
		};

		p = m_Tasks.Pop();
	}

	// On task list changes, save state.
	if (bChanged)
	{
		WorkerThreadPrune(r);
		WorkerThreadSaveState(r);
	}

	return bChanged;
}

/**
 * Actually issue a request to Mixpanel. Mostly the same, but slight variations
 * based on event type.
 */
Bool MixpanelAnalyticsManager::WorkerThreadIssueRequest(
	const MixpanelState& state,
	const String& sBody,
	MixpanelCommon::EntryType eType) const
{
	using namespace MixpanelCommon;

	// Acquire and cache the api key for the body of this function.
	String const sApiKey(GetApiKey());

	// Get the URL.
	String const sURL(GetMixpanelURL(eType, m_sBaseEventURL, m_sBaseProfileURL, sApiKey, state.GetUserID()));

	// Reset the callback handler prior to issuing the new request.
	s_Callbacks.Reset();

	// Create and submit the request.
	auto& r = HTTP::Manager::Get()->CreateRequest();

	// Setup state.
	r.SetDispatchCallbackOnMainThread(false);
	r.SetResendOnFailure(false);
	r.SetURL(sURL);
	r.SetCallback(SEOUL_BIND_DELEGATE(&Callbacks::HTTPCallback, &s_Callbacks));
	r.SetMethod(HTTP::Method::ksPost);
	r.AddPostData("data", sBody);
	if (EntryType::kEvent == eType) { r.AddPostData("ip", "1"); }
#if SEOUL_LOGGING_ENABLED
	r.AddPostData("verbose", "1");
#endif

	// Issue the request.
	r.Start();

	// While not shutting down, wait for request to complete.
	while (!m_bShuttingDown && !s_Callbacks.IsDone())
	{
		s_ActivitySignal.Wait();
	}

	return s_Callbacks.WasSuccessful();
}

/** Load a save state from disk. May return nullptr if m_bShuttingDown is set to true. */
MixpanelState* MixpanelAnalyticsManager::WorkerThreadLoadState() const
{
	// Prepare the callbacks binding.
	s_Callbacks.Reset();

	// Issue the load request.
	SaveLoadManager::Get()->QueueLoad(
		TypeOf<MixpanelState>(),
		FilePath::CreateSaveFilePath(m_Settings.m_Generic.m_sSaveFilename),
		String(),
		kiSaveVersion,
		SharedPtr<ISaveLoadOnComplete>(SEOUL_NEW(MemoryBudgets::Analytics) CallbacksBind),
		SaveLoadManager::Migrations(),
		// Analytics save data does not control the engine's session guid.
		false);

	// While not shutting down, wait for request to complete.
	while (!m_bShuttingDown && !s_Callbacks.IsDone())
	{
		s_ActivitySignal.Wait();
	}

	// Return early if shutting down.
	if (m_bShuttingDown)
	{
		return nullptr;
	}
	// Otherwise, if successful, return the instance.
	else if (s_Callbacks.WasSuccessful())
	{
		Reflection::WeakAny p;
		s_Callbacks.AcquireData(p);
		return p.Cast<MixpanelState*>();
	}
	// Final case, create a new instance.
	else
	{
		return SEOUL_NEW(MemoryBudgets::Analytics) MixpanelState;
	}
}

// Utility.
static inline void PruneByAge(const WorldTime& now, MixpanelState::Tasks& r)
{
	for (auto i = r.Begin(); r.End() != i; )
	{
		auto const age = (now - (*i)->GetTimestamp());
		if (age > kMaximumEventAge)
		{
			auto p = *i;
			i = r.Erase(i);
			SafeDelete(p);
		}
		// Events are expected to be ordered by time stamp, so we
		// exploit this and early out. Pruning is inexact so if/when
		// this is violated, it's ok.
		else
		{
			break;
		}
	}
}

static inline void PruneBySize(MixpanelState::Tasks& r)
{
	auto const uSize = r.GetSize();
	if (uSize <= kuMaximumQueueSize)
	{
		// Nothing to do, at or under max size.
		return;
	}

	// Erase necessary elements from front.
	auto const iEraseBegin = r.Begin();
	auto const iEraseEnd = iEraseBegin + (uSize - kuMaximumQueueSize);

	// Release memory first.
	for (auto i = iEraseBegin; i < iEraseEnd; ++i)
	{
		SafeDelete(*i);
	}

	// Now delete the range.
	r.Erase(iEraseBegin, iEraseEnd);

	// Sanity check result.
	SEOUL_ASSERT(r.GetSize() == kuMaximumQueueSize);
}

/*
 * Apply queue limits.
 */
void MixpanelAnalyticsManager::WorkerThreadPrune(MixpanelState& rState) const
{
	// Prune based on age.
	auto const now = Now();

	// Prune events and profile updates.
	PruneByAge(now, rState.m_vEvents);
	PruneByAge(now, rState.m_vProfileUpdates);

	// Now limit by size.
	PruneBySize(rState.m_vEvents);
	PruneBySize(rState.m_vProfileUpdates);
}

/** Commit a state to disk. */
Bool MixpanelAnalyticsManager::WorkerThreadSaveState(const MixpanelState& state) const
{
	// Prepare the callbacks binding.
	s_Callbacks.Reset();

	// Issue the save request.
	SaveLoadManager::Get()->QueueSave(
		FilePath::CreateSaveFilePath(m_Settings.m_Generic.m_sSaveFilename),
		String(),
		&state,
		kiSaveVersion,
		SharedPtr<ISaveLoadOnComplete>(SEOUL_NEW(MemoryBudgets::Analytics) CallbacksBind),
		false);

	// While not shutting down, wait for request to complete.
	while (!m_bShuttingDown && !s_Callbacks.IsDone())
	{
		s_ActivitySignal.Wait();
	}

	return (s_Callbacks.IsDone() && s_Callbacks.WasSuccessful());
}

/** Walk the task arrays and apply all pending events. */
void MixpanelAnalyticsManager::WorkerThreadSubmitTasks(MixpanelState& r)
{
	using namespace MixpanelCommon;

	// Submit tasks of both types - on success or either, save.
	Bool bSave = false;
	bSave = WorkerThreadSubmitTasks(r, EntryType::kEvent) || bSave;
	bSave = WorkerThreadSubmitTasks(r, EntryType::kProfile) || bSave;

	if (bSave)
	{
		// Commit current state to disk.
		r.m_SaveTimestamp = Now();
		WorkerThreadSaveState(r);
	}
}

/** Walk either the event or profile arrays and apply all pending tasks. */
Bool MixpanelAnalyticsManager::WorkerThreadSubmitTasks(MixpanelState& r, MixpanelCommon::EntryType eType)
{
	using namespace MixpanelCommon;

	// Get the desired task vector.
	auto& rv = (MixpanelCommon::EntryType::kEvent == eType ? r.m_vEvents : r.m_vProfileUpdates);

	// Return immediatley if the vector is empty.
	if (rv.IsEmpty())
	{
		return false;
	}

	// Process all tasks and append to the appropriate string.
	Bool bSkipNextStart = false;
	MixpanelBuilder builder(m_Settings, r);
	String sBody;

	// Start batch.
	sBody.Append('[');
	UInt32 uSend = 0u;
	UInt32 uConsumed = 0u;
	Bool bHasOnce = false;

	// Add entries to batch.
	for (auto i = rv.Begin(); rv.End() != i; ++i)
	{
		// In all cases, we've now consumed this entry.
		++uConsumed;

		// Cache for access.
		auto p = *i;

		// Special handling for end/start - when we hit an end, we stop processing
		// until its corresponding start exists. Further, if the time separation is too
		// small, we just skip the end/start events.
		if (p->GetType() == EntryType::kSessionEnd)
		{
			auto iStart = (i + 1);
			for (; rv.End() != iStart; ++iStart)
			{
				if ((*iStart)->GetType() == EntryType::kSessionStart)
				{
					break;
				}
			}

			// If we've hit the end, return false immediately - wait for
			// start to exist. This is fine, as in the vast majority of
			// proper use cases, there should be no events between end and
			// start.
			if (rv.End() == iStart)
			{
				return false;
			}

			// Now check separation - if too small, set bSkipNextStart
			// to true and skip this end event.
			auto pStart = (*iStart);
			if ((pStart->GetTimestamp() - p->GetTimestamp()) < kSessionExpirationTime)
			{
				bSkipNextStart = true;
				continue;
			}
		}

		switch (p->GetType())
		{
		case EntryType::kEvent: // fall-through
		case EntryType::kSessionEnd: // fall-through
		case EntryType::kSessionStart:
			{
				// Track once tokens.
				if (p->GetType() == EntryType::kEvent &&
					!static_cast<EventEntry const*>(p.Get())->GetEvent().GetOnceToken().IsEmpty())
				{
					bHasOnce = true;
				}

				// Possibly skip a session start.
				if (EntryType::kSessionStart == p->GetType() && bSkipNextStart)
				{
					bSkipNextStart = false;
				}
				else
				{
					// Append.
					Byte const* sSep = (0 != uSend ? "," : "");
					if (p->AppendJSON(builder, sBody, sSep)) { ++uSend; }
				}
			}
			break;
		case EntryType::kProfile:
			{
				Byte const* sSep = (0 != uSend ? "," : "");
				if (p->AppendJSON(builder, sBody, sSep)) { ++uSend; }
			}
			break;
		default:
			SEOUL_FAIL("Enum mismatch");
			break;
		};

		// Done if we've hit the max per batch limit.
		if (uSend >= kuMaxMixpanelRequestsPerBatch)
		{
			break;
		}
	}

	// Terminate.
	sBody.Append(']');

	// Base64 encode data.
	sBody = Base64Encode(sBody);

	// Uploads results.
	Bool bSent = true;

	// No need to if uSend is 0.
	if (uSend > 0u)
	{
		bSent = WorkerThreadIssueRequest(r, sBody, eType);
	}

	// On successful send (or no send), erase the number we consumed.
	if (bSent)
	{
		// Prior to completion, apply once token results if present.
		if (bHasOnce)
		{
			for (UInt32 i = 0u; i < uConsumed; ++i)
			{
				if (rv[i]->GetType() != MixpanelCommon::EntryType::kEvent)
				{
					continue;
				}

				auto const& evt = static_cast<MixpanelCommon::EventEntry const*>(rv[i].Get())->GetEvent();
				if (!evt.GetOnceToken().IsEmpty())
				{
					SEOUL_VERIFY(r.m_tOnceTokens.Overwrite(evt.GetName(), evt.GetOnceToken()).Second);
				}
			}
		}

		// Delete entire vector if we've processed all.
		if (uConsumed >= rv.GetSize())
		{
			SafeDeleteVector(rv);
		}
		// Otherwise, remove up to uCount.
		else
		{
			for (UInt32 i = 0u; i < uConsumed; ++i)
			{
				SafeDelete(rv[i]);
			}
			rv.Erase(rv.Begin(), rv.Begin() + uConsumed);
		}

		// Commit any builder changes to permanent state.
		builder.OnSuccess();
		return true;
	}
	else
	{
		// Give a chance for the builder to cleanup.
		builder.OnFailure();
		return false;
	}
}

} // namespace Seoul
