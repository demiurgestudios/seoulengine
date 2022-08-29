/**
 * \file MixpanelTest.cpp
 * \brief Unit test for our Mixpanel client.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "BuildChangelistPublic.h"
#include "BuildVersion.h"
#include "Compress.h"
#include "DataStoreParser.h"
#include "Engine.h"
#include "FromString.h"
#include "GenericAnalyticsManager.h"
#include "GenericInMemorySaveApi.h"
#include "HTTPServer.h"
#include "JobsManager.h"
#include "Logger.h"
#include "MixpanelTest.h"
#include "NullPlatformEngine.h"
#include "PlatformData.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ScriptVm.h"
#include "SeoulFile.h"
#include "SeoulUUID.h"
#include "TrackingManager.h"
#include "UnitTesting.h"
#include "UnitTestsEngineHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(MixpanelTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestProfile)
	SEOUL_METHOD(TestScript)
	SEOUL_METHOD(TestShutdown)
	SEOUL_METHOD(TestSessionFilter)
	SEOUL_METHOD(TestSessions)
	SEOUL_METHOD(TestTrackingManagerEcho)
	SEOUL_METHOD(TestPruningByAgeEvents)
	SEOUL_METHOD(TestPruningByAgeProfile)
	SEOUL_METHOD(TestPruningBySizeEvents)
	SEOUL_METHOD(TestPruningBySizeProfile)
	SEOUL_METHOD(TestAnalyticsDisable)
SEOUL_END_TYPE()

namespace
{

static const HString kContentLength("content-length");
String GetTestApiKey()
{
	return "asdf";
}

String GetEmptyApiKey()
{
	return String();
}

Bool DoNotSendAnalytics()
{
	return false;
}

String GetTestEventBaseURL()
{
	return "http://localhost:8057";
}

String GetTestProfileBaseURL()
{
	return "http://localhost:8058";
}

struct HeaderEntry
{
	HString m_sKey;
	String m_sValue;

	Bool operator<(const HeaderEntry& b) const
	{
		return (strcmp(m_sKey.CStr(), b.m_sKey.CStr()) < 0);
	}
};

String GetAnalyticsUserId()
{
	return "e7663156-5cb6-11e8-9c2d-fa7ae01bbebc";
}

static void TestHTTPHeaders(const HTTP::ServerRequestInfo& info, Bool bEvent)
{
	Vector<HeaderEntry> v;

	auto const& t = info.m_Headers.GetKeyValues();
	auto const iBegin = t.Begin();
	auto const iEnd = t.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		HeaderEntry entry;
		entry.m_sKey = i->First;
		entry.m_sValue.Assign(i->Second.m_sValue, i->Second.m_zValueSizeInBytes);
		v.PushBack(entry);
	}

	QuickSort(v.Begin(), v.End());

	UInt32 u = 0u;
	UInt32 uTestValue;
#if SEOUL_PLATFORM_IOS
	SEOUL_UNITTESTING_ASSERT_EQUAL(15, v.GetSize());
#else
	SEOUL_UNITTESTING_ASSERT_EQUAL(16, v.GetSize());
#endif
	SEOUL_UNITTESTING_ASSERT_EQUAL("accept", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("*/*", v[u].m_sValue); ++u;
#if SEOUL_PLATFORM_IOS
	SEOUL_UNITTESTING_ASSERT_EQUAL("accept-encoding", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("gzip, deflate", v[u].m_sValue); ++u;
#else
	SEOUL_UNITTESTING_ASSERT_EQUAL("accept-encoding", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("deflate, gzip", v[u].m_sValue); ++u;
#endif
#if SEOUL_PLATFORM_IOS
	SEOUL_UNITTESTING_ASSERT_EQUAL("accept-language", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("en-us", v[u].m_sValue); ++u;
#endif
#if SEOUL_PLATFORM_IOS
	SEOUL_UNITTESTING_ASSERT_EQUAL("connection", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("keep-alive", v[u].m_sValue); ++u;
#else
	SEOUL_UNITTESTING_ASSERT_EQUAL("connection", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("Upgrade, HTTP2-Settings", v[u].m_sValue); ++u;
#endif
	SEOUL_UNITTESTING_ASSERT_EQUAL("content-length", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT(FromString(v[u].m_sValue, uTestValue)); ++u;
	SEOUL_UNITTESTING_ASSERT_EQUAL("content-type", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("application/x-www-form-urlencoded", v[u].m_sValue); ++u;
	SEOUL_UNITTESTING_ASSERT_EQUAL("host", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL((bEvent ? "localhost:8057" : "localhost:8058"), v[u].m_sValue); ++u;
#if !SEOUL_PLATFORM_IOS
	SEOUL_UNITTESTING_ASSERT_EQUAL("http2-settings", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("AAMAAABkAARAAAAAAAIAAAAA", v[u].m_sValue); ++u;
	SEOUL_UNITTESTING_ASSERT_EQUAL("upgrade", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("h2c", v[u].m_sValue); ++u;
#endif
#if SEOUL_PLATFORM_IOS
	// Just skip this entry on iOS - user-agent has a lot of variability
	// depending on the device.
	++u;
#else
	SEOUL_UNITTESTING_ASSERT_EQUAL("user-agent", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL("libcurl/7.61.1 OpenSSL/1.0.1t zlib/1.2.8 nghttp2/1.33.0", v[u].m_sValue); ++u;
#endif
	SEOUL_UNITTESTING_ASSERT_EQUAL("x-amzn-trace-id", v[u].m_sKey); ++u;
	SEOUL_UNITTESTING_ASSERT_EQUAL("x-demiurge-build-changelist", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL(g_sBuildChangelistStr, v[u].m_sValue); ++u;
	SEOUL_UNITTESTING_ASSERT_EQUAL("x-demiurge-build-version", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL(BUILD_VERSION_STR, v[u].m_sValue); ++u;
	SEOUL_UNITTESTING_ASSERT_EQUAL("x-demiurge-client-platform", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL(GetCurrentPlatformName(), v[u].m_sValue); ++u;
	SEOUL_UNITTESTING_ASSERT_EQUAL("x-demiurge-device-token-hash", v[u].m_sKey); SEOUL_UNITTESTING_ASSERT_EQUAL(Engine::Get()->GetLoadShedPlatformUuidHash(), v[u].m_sValue); ++u;
	SEOUL_UNITTESTING_ASSERT_EQUAL("x-demiurge-request-id", v[u].m_sKey); ++u;
	SEOUL_UNITTESTING_ASSERT_EQUAL("x-demiurge-retry-token", v[u].m_sKey); ++u;
}

#define SEOUL_TEST_BOOL(key, expec) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString(key), value)); \
	SEOUL_UNITTESTING_ASSERT(dataStore.AsBoolean(value, bValue)); \
	SEOUL_UNITTESTING_ASSERT_EQUAL(expec, bValue)

#define SEOUL_TEST_INT(key, expec) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString(key), value)); \
	SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iValue)); \
	SEOUL_UNITTESTING_ASSERT_EQUAL(expec, iValue)

#define SEOUL_TEST_INT_GE(key, expec) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString(key), value)); \
	SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iValue)); \
	SEOUL_UNITTESTING_ASSERT_GREATER_EQUAL(expec, iValue)

#define SEOUL_TEST_INT_GT(key, expec) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString(key), value)); \
	SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iValue)); \
	SEOUL_UNITTESTING_ASSERT_GREATER_THAN(expec, iValue)

#define SEOUL_TEST_INT64(key, expec) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString(key), value)); \
	SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue64)); \
	SEOUL_UNITTESTING_ASSERT_EQUAL(expec, iValue64)

#define SEOUL_TEST_INT64_LT(key, expec) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString(key), value)); \
	SEOUL_UNITTESTING_ASSERT(dataStore.AsInt64(value, iValue64)); \
	SEOUL_UNITTESTING_ASSERT_LESS_THAN(expec, iValue)

#define SEOUL_TEST_STRING(key, expec) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString(key), value)); \
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sValue)); \
	SEOUL_UNITTESTING_ASSERT_EQUAL(expec, sValue)

#define SEOUL_TEST_TIME(key) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString(key), value)); \
	SEOUL_UNITTESTING_ASSERT(dataStore.AsInt32(value, iValue));

#define SEOUL_TEST_UUID(key) \
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString(key), value)); \
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sValue)); \
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(UUID::Zero(), UUID::FromString(sValue))

namespace
{

	struct TestSessionEndState
	{
		String m_sExpectedSessionUUID{};
		String m_sLastSessionUUID{};
		Int64 m_iSessionCount = 1;
	};

} // namespace anonymous

static void TestSessionEnd(const DataStore& dataStore, DataNode node, TestSessionEndState& r, TimeInterval expectedDuration)
{
	PlatformData pd;
	Engine::Get()->GetPlatformData(pd);

	DataNode value;
	String sValue;
	Int32 iValue = 32;
	Int64 iValue64 = -5172;

	SEOUL_TEST_STRING("event", "$ae_session");

	// Properties
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString("properties"), node));

	if (!expectedDuration.IsZero())
	{
		SEOUL_TEST_INT64("$ae_session_length", expectedDuration.GetSeconds());
	}
	else
	{
		SEOUL_TEST_INT64_LT("$ae_session_length", 0);
	}

	SEOUL_TEST_STRING("distinct_id", GetAnalyticsUserId());
	SEOUL_TEST_TIME("time");
	SEOUL_TEST_STRING("token", GetTestApiKey());

	// Some special handling for the UUID and count - we want
	// UUID to be a valid UUID but also not equal to the last end UUID.
	SEOUL_TEST_UUID("s_session_id");
	SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(r.m_sLastSessionUUID , sValue);
	r.m_sLastSessionUUID = sValue;

	// Also reset the expected since we're starting a new one.
	SEOUL_UNITTESTING_ASSERT_EQUAL(r.m_sExpectedSessionUUID, sValue);
	r.m_sExpectedSessionUUID = String();

	SEOUL_TEST_INT64("s_player_sessions", r.m_iSessionCount);
	r.m_iSessionCount++;
}

static void TestSessionStart(const DataStore& dataStore, DataNode node, Int iSessionSequenceNumber)
{
	PlatformData pd;
	Engine::Get()->GetPlatformData(pd);

	DataNode value;
	String sValue;
	Int32 iValue = 32;

	SEOUL_TEST_STRING("event", "SessionStart");

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString("properties"), node));

	SEOUL_TEST_STRING("distinct_id", GetAnalyticsUserId());
	SEOUL_TEST_TIME("time");
	SEOUL_TEST_STRING("token", GetTestApiKey());

	SEOUL_TEST_UUID("s_session_id");
	SEOUL_TEST_INT("s_player_sessions", iSessionSequenceNumber);
}

static void TestBasicEvent(const DataStore& dataStore, DataNode node, const String& sName, TestSessionEndState& r)
{
	PlatformData pd;
	Engine::Get()->GetPlatformData(pd);

	DataNode value;
	String sValue;
	Int32 iValue = 32;
	Int64 iValue64 = 5127;

	SEOUL_TEST_STRING("event", sName);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString("properties"), node));

	// Test properties.
	SEOUL_TEST_STRING("distinct_id", GetAnalyticsUserId());
	SEOUL_TEST_TIME("time");
	SEOUL_TEST_STRING("token", GetTestApiKey());
	SEOUL_TEST_INT("TestAttributeInt", 21);
	SEOUL_TEST_STRING("TestAttributeString", "Hello World");

	// Some special handling for the UUID and count - we want
	// UUID to be a valid UUID but also not equal to the last end UUID.
	SEOUL_TEST_UUID("s_session_id");
	SEOUL_UNITTESTING_ASSERT(r.m_sExpectedSessionUUID.IsEmpty() || r.m_sExpectedSessionUUID == sValue);
	if (r.m_sExpectedSessionUUID.IsEmpty()) { r.m_sExpectedSessionUUID = sValue; }

	SEOUL_TEST_INT64("s_player_sessions", r.m_iSessionCount);
}

#undef SEOUL_TEST_UUID
#undef SEOUL_TEST_TIME
#undef SEOUL_TEST_STRING
#undef SEOUL_TEST_INT64
#undef SEOUL_TEST_INT_GE
#undef SEOUL_TEST_INT
#undef SEOUL_TEST_BOOL

Bool TestBasicEvents(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
{
	TestSessionEndState endState;

	TestHTTPHeaders(info, true);

	String sBody(info.m_vBody.Data(), info.m_vBody.GetSize());
	sBody = URLDecode(sBody.CStr());
	sBody = TrimWhiteSpace(sBody);

	static const String ksPrefix("verbose=1&ip=1&data=");
	SEOUL_UNITTESTING_ASSERT(sBody.StartsWith(ksPrefix));
	String sData(sBody.Substring(ksPrefix.GetSize()));

	Vector<Byte> vData;
	SEOUL_VERIFY(Base64Decode(sData, vData));

	DataStore dataStore;
	SEOUL_VERIFY(DataStoreParser::FromString(vData.Data(), vData.GetSize(), dataStore));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsArray());
	DataNode arr = dataStore.GetRootNode();
	UInt32 uCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(arr, uCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(4u, uCount);

	DataNode node;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 0u, node));
	TestSessionStart(dataStore, node, 1);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 1u, node));
	TestBasicEvent(dataStore, node, "TestEvent", endState);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 2u, node));
	TestSessionEnd(dataStore, node, endState, TimeInterval());

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 3u, node));
	TestSessionStart(dataStore, node, 2);

	// Write expected response - verbose enabled, so a JSON blob.
	responseWriter.WriteStatusResponse(200, HTTP::HeaderTable(), R"({"status": 1})");

	return true;
}

void TestProfileUpdates(const DataStore& dataStore, const DataNode& dataNode, Byte const* sExpected)
{
	DataStore expected;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromString(sExpected, expected));

	SEOUL_UNITTESTING_ASSERT(DataStore::Equals(
		dataStore,
		dataNode,
		expected,
		expected.GetRootNode()));
}

Bool TestProfilesCommon(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info, Byte const* sExpected)
{
	TestHTTPHeaders(info, false);

	String sBody(info.m_vBody.Data(), info.m_vBody.GetSize());
	sBody = URLDecode(sBody.CStr());
	sBody = TrimWhiteSpace(sBody);

	static const String ksPrefix("verbose=1&data=");
	SEOUL_UNITTESTING_ASSERT(sBody.StartsWith(ksPrefix));
	String sData(sBody.Substring(ksPrefix.GetSize()));

	Vector<Byte> vData;
	SEOUL_VERIFY(Base64Decode(sData, vData));

	DataStore dataStore;
	SEOUL_VERIFY(DataStoreParser::FromString(vData.Data(), vData.GetSize(), dataStore));

	TestProfileUpdates(dataStore, dataStore.GetRootNode(), sExpected);

	// Write expected response - verbose enabled, so a JSON blob.
	responseWriter.WriteStatusResponse(200, HTTP::HeaderTable(), R"({"status": 1})");

	return true;
}

Bool TestBasicProfiles(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
{
	static Byte const* ksExpected = R"(
		[
			{
				"$token": "asdf",
				"$distinct_id": "e7663156-5cb6-11e8-9c2d-fa7ae01bbebc",
				"$set": {
					"TestInt32": 27,
					"TestString": "Hello World"
				}
			}
		])";

	return TestProfilesCommon(responseWriter, info, ksExpected);
}

Bool TestCompleteProfiles(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
{
	static Byte const* ksExpected = R"(
		[
			{
				"$token": "asdf",
				"$distinct_id": "e7663156-5cb6-11e8-9c2d-fa7ae01bbebc",
				"$union": {
					"TestSetAdd": ["Zero", "One"]
				}
			},
			{
				"$token": "asdf",
				"$distinct_id": "e7663156-5cb6-11e8-9c2d-fa7ae01bbebc",
				"$unset": ["TestAttrDelete"]
			},
			{
				"$token": "asdf",
				"$distinct_id": "e7663156-5cb6-11e8-9c2d-fa7ae01bbebc",
				"$remove": {
					"TestSetDelete": ["Zero", "One"]
				}
			},
			{
				"$token": "asdf",
				"$distinct_id": "e7663156-5cb6-11e8-9c2d-fa7ae01bbebc",
				"$add": {
					"TestIncrement": 5
				}
			},
			{
				"$token": "asdf",
				"$distinct_id": "e7663156-5cb6-11e8-9c2d-fa7ae01bbebc",
				"$set": {
					"TestSetMixed": ["Zero", 1],
					"TestInt32": 27,
					"TestSetInt32": [33, 38],
					"TestString": "Hello World",
					"TestSetString": ["Zero", "One"]
				}
			},
			{
				"$token": "asdf",
				"$distinct_id": "e7663156-5cb6-11e8-9c2d-fa7ae01bbebc",
				"$set": {
					"TestBool": true,
					"TestArray": [],
					"TestTable": {},
					"TestFilePath": "config://test.json",
					"TestFloat32": 3.5,
					"TestNull": null
				}
			}
		])";

	return TestProfilesCommon(responseWriter, info, ksExpected);
}

// Always fail - used to check that the analytics manager
// does *not* send events when we expect it shouldn't be.
Bool TestFail(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
{
	SEOUL_UNITTESTING_ASSERT(false);
	return true;
}

Bool TestEventsPruned(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
{
	TestSessionEndState endState;
	endState.m_iSessionCount = 0;

	TestHTTPHeaders(info, true);

	String sBody(info.m_vBody.Data(), info.m_vBody.GetSize());
	sBody = URLDecode(sBody.CStr());
	sBody = TrimWhiteSpace(sBody);

	static const String ksPrefix("verbose=1&ip=1&data=");
	SEOUL_UNITTESTING_ASSERT(sBody.StartsWith(ksPrefix));
	String sData(sBody.Substring(ksPrefix.GetSize()));

	Vector<Byte> vData;
	SEOUL_VERIFY(Base64Decode(sData, vData));

	DataStore dataStore;
	SEOUL_VERIFY(DataStoreParser::FromString(vData.Data(), vData.GetSize(), dataStore));
	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsArray());
	DataNode arr = dataStore.GetRootNode();
	UInt32 uCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(arr, uCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(50u, uCount);

	DataNode node;
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, i, node));
		TestBasicEvent(dataStore, node, "TestEvent", endState);
	}

	// Write expected response - verbose enabled, so a JSON blob.
	responseWriter.WriteStatusResponse(200, HTTP::HeaderTable(), R"({"status": 1})");

	return true;
}

Bool TestProfilesPruned(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
{
	static Byte const* ksExpected = R"(
		{
			"$token": "asdf",
			"$distinct_id": "e7663156-5cb6-11e8-9c2d-fa7ae01bbebc",
			"$set": {
				"TestInt32": 27,
				"TestString": "Hello World"
			}
		})";

	TestHTTPHeaders(info, false);

	String sBody(info.m_vBody.Data(), info.m_vBody.GetSize());
	sBody = URLDecode(sBody.CStr());
	sBody = TrimWhiteSpace(sBody);

	static const String ksPrefix("verbose=1&data=");
	SEOUL_UNITTESTING_ASSERT(sBody.StartsWith(ksPrefix));
	String sData(sBody.Substring(ksPrefix.GetSize()));

	Vector<Byte> vData;
	SEOUL_VERIFY(Base64Decode(sData, vData));

	DataStore dataStore;
	SEOUL_VERIFY(DataStoreParser::FromString(vData.Data(), vData.GetSize(), dataStore));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsArray());
	DataNode arr = dataStore.GetRootNode();
	UInt32 uCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(arr, uCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(50u, uCount);

	DataNode node;
	for (UInt32 i = 0u; i < uCount; ++i)
	{
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, i, node));
		TestProfileUpdates(dataStore, node, ksExpected);
	}

	// Write expected response - verbose enabled, so a JSON blob.
	responseWriter.WriteStatusResponse(200, HTTP::HeaderTable(), R"({"status": 1})");

	return true;
}

} // namespace anonymous

void MixpanelTest::TestBasic()
{
	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings settings;
	settings.m_AnalyticsSettings = analyticsSettings;
	settings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, settings);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicEvents);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicProfiles);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());

	AnalyticsEvent evt("TestEvent");
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetInt32ValueToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeInt"), 21));
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetStringToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeString"), "Hello World"));
	AnalyticsManager::Get()->TrackSessionStart();
	AnalyticsManager::Get()->TrackEvent(evt);
	AnalyticsManager::Get()->TrackSessionEnd();

	// End is not submitted until a following start to check for spurious end/start.
	AnalyticsManager::Get()->TrackSessionStart(WorldTime::GetUTCTime() + TimeInterval(15));

	AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kSet);
	update.GetUpdates().MakeTable();
	SEOUL_VERIFY(update.GetUpdates().SetInt32ValueToTable(update.GetUpdates().GetRootNode(), HString("TestInt32"), 27));
	SEOUL_VERIFY(update.GetUpdates().SetStringToTable(update.GetUpdates().GetRootNode(), HString("TestString"), "Hello World"));
	AnalyticsManager::Get()->UpdateProfile(update);

	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (
		(eventServer.GetReceivedRequestCount() < 1 || profileServer.GetReceivedRequestCount() < 1) &&
		SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 5.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, profileServer.GetReceivedRequestCount());
}

void MixpanelTest::TestProfile()
{
	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings settings;
	settings.m_AnalyticsSettings = analyticsSettings;
	settings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, settings);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestFail);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestCompleteProfiles);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());

	// Add to set.
	{
		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kUnion);
		auto& ds = update.GetUpdates();
		ds.MakeTable();
		auto const root = ds.GetRootNode();
		SEOUL_UNITTESTING_ASSERT(ds.SetArrayToTable(root, HString("TestSetAdd")));
		DataNode arr;
		SEOUL_UNITTESTING_ASSERT(ds.GetValueFromTable(root, HString("TestSetAdd"), arr));
		SEOUL_UNITTESTING_ASSERT(ds.SetStringToArray(arr, 0, "Zero"));
		SEOUL_UNITTESTING_ASSERT(ds.SetStringToArray(arr, 1, "One"));

		AnalyticsManager::Get()->UpdateProfile(update);
	}

	// Delete
	{
		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kUnset);
		auto& ds = update.GetUpdates();
		ds.MakeArray();
		auto const root = ds.GetRootNode();
		SEOUL_UNITTESTING_ASSERT(ds.SetStringToArray(root, 0u, "TestAttrDelete"));

		AnalyticsManager::Get()->UpdateProfile(update);
	}

	// DeleteFromSet
	{
		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kRemove);
		auto& ds = update.GetUpdates();
		ds.MakeTable();
		auto const root = ds.GetRootNode();
		SEOUL_UNITTESTING_ASSERT(ds.SetArrayToTable(root, HString("TestSetDelete")));
		DataNode arr;
		SEOUL_UNITTESTING_ASSERT(ds.GetValueFromTable(root, HString("TestSetDelete"), arr));
		SEOUL_UNITTESTING_ASSERT(ds.SetStringToArray(arr, 0, "Zero"));
		SEOUL_UNITTESTING_ASSERT(ds.SetStringToArray(arr, 1, "One"));

		AnalyticsManager::Get()->UpdateProfile(update);
	}

	// Increment
	{
		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kAdd);
		auto& ds = update.GetUpdates();
		ds.MakeTable();
		auto const root = ds.GetRootNode();
		SEOUL_UNITTESTING_ASSERT(ds.SetInt32ValueToTable(root, HString("TestIncrement"), 5));

		AnalyticsManager::Get()->UpdateProfile(update);
	}

	// Set update.
	{
		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kSet);
		auto& ds = update.GetUpdates();
		ds.MakeTable();
		auto const root = ds.GetRootNode();
		SEOUL_UNITTESTING_ASSERT(ds.SetInt32ValueToTable(root, HString("TestInt32"), 27));
		SEOUL_UNITTESTING_ASSERT(ds.SetStringToTable(root, HString("TestString"), "Hello World"));
		SEOUL_UNITTESTING_ASSERT(ds.SetArrayToTable(root, HString("TestSetInt32")));
		DataNode arr;
		SEOUL_UNITTESTING_ASSERT(ds.GetValueFromTable(root, HString("TestSetInt32"), arr));
		SEOUL_UNITTESTING_ASSERT(ds.SetInt32ValueToArray(arr, 0, 33));
		SEOUL_UNITTESTING_ASSERT(ds.SetInt32ValueToArray(arr, 1, 38));

		SEOUL_UNITTESTING_ASSERT(ds.SetArrayToTable(root, HString("TestSetString")));
		SEOUL_UNITTESTING_ASSERT(ds.GetValueFromTable(root, HString("TestSetString"), arr));
		SEOUL_UNITTESTING_ASSERT(ds.SetStringToArray(arr, 0, "Zero"));
		SEOUL_UNITTESTING_ASSERT(ds.SetStringToArray(arr, 1, "One"));

		SEOUL_UNITTESTING_ASSERT(ds.SetArrayToTable(root, HString("TestSetMixed")));
		SEOUL_UNITTESTING_ASSERT(ds.GetValueFromTable(root, HString("TestSetMixed"), arr));
		SEOUL_UNITTESTING_ASSERT(ds.SetStringToArray(arr, 0, "Zero"));
		SEOUL_UNITTESTING_ASSERT(ds.SetInt32ValueToArray(arr, 1, 1));

		AnalyticsManager::Get()->UpdateProfile(update);
	}

	// Miscellaneous - types as strings.
	{
		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kSet);
		auto& ds = update.GetUpdates();
		ds.MakeTable();
		auto const root = ds.GetRootNode();
		SEOUL_UNITTESTING_ASSERT(ds.SetArrayToTable(root, HString("TestArray")));
		SEOUL_UNITTESTING_ASSERT(ds.SetBooleanValueToTable(root, HString("TestBool"), true));
		SEOUL_UNITTESTING_ASSERT(ds.SetFilePathToTable(root, HString("TestFilePath"), FilePath::CreateConfigFilePath("test.json")));
		SEOUL_UNITTESTING_ASSERT(ds.SetFloat32ValueToTable(root, HString("TestFloat32"), 3.5f));
		SEOUL_UNITTESTING_ASSERT(ds.SetNullValueToTable(root, HString("TestNull")));
		SEOUL_UNITTESTING_ASSERT(ds.SetTableToTable(root, HString("TestTable")));

		AnalyticsManager::Get()->UpdateProfile(update);
	}

	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (
		(profileServer.GetReceivedRequestCount() < 1) &&
		SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 5.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, profileServer.GetReceivedRequestCount());
}

void MixpanelTest::TestScript()
{
	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings engineSettings;
	engineSettings.m_AnalyticsSettings = analyticsSettings;
	engineSettings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, engineSettings);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicEvents);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicProfiles);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());

	Script::VmSettings scriptSettings;
	SharedPtr<Script::Vm> pVm(SEOUL_NEW(MemoryBudgets::Developer) Script::Vm(scriptSettings));

	AnalyticsManager::Get()->TrackSessionStart();
	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptEngineAnalyticsManager')\n"
		"native:TrackEvent('TestEvent', {TestAttributeInt = 21, TestAttributeString = 'Hello World'})\n"));
	AnalyticsManager::Get()->TrackSessionEnd();

	// End is not submitted until a following start to check for spurious end/start.
	AnalyticsManager::Get()->TrackSessionStart(WorldTime::GetUTCTime() + TimeInterval(15));

	SEOUL_UNITTESTING_ASSERT(pVm->RunCode(
		"local native = SeoulNativeNewNativeUserData('ScriptEngineAnalyticsManager')\n"
		"native:UpdateProfile('Set', {TestInt32 = 27, TestString = 'Hello World'})\n"));

	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (
		(eventServer.GetReceivedRequestCount() < 1 || profileServer.GetReceivedRequestCount() < 1) &&
		SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 5.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, profileServer.GetReceivedRequestCount());
}

void MixpanelTest::TestShutdown()
{
	SharedPtr<GenericInMemorySaveApiSharedMemory> pShared(SEOUL_NEW(MemoryBudgets::Io) GenericInMemorySaveApiSharedMemory);
	for (Int32 i = 0; i < 2; ++i)
	{
		GenericAnalyticsManagerSettings analyticsSettings;
		analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
		analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
		analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
		analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
		analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
		NullPlatformEngineSettings settings;
		settings.m_AnalyticsSettings = analyticsSettings;
		settings.m_pSharedMemory = pShared;
		settings.m_bEnableSaveApi = true;
		UnitTestsEngineHelper helper(nullptr, settings);

		HTTP::ServerSettings httpSettings;
		httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicEvents);
		httpSettings.m_iPort = 8057;
		httpSettings.m_iThreadCount = 1;
		HTTP::Server eventServer(httpSettings);

		httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicProfiles);
		httpSettings.m_iPort = 8058;
		HTTP::Server profileServer(httpSettings);

		AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());

		if (0 == i)
		{
			AnalyticsEvent evt("TestEvent");
			SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetInt32ValueToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeInt"), 21));
			SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetStringToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeString"), "Hello World"));
			AnalyticsManager::Get()->TrackSessionStart();
			AnalyticsManager::Get()->TrackEvent(evt);
			AnalyticsManager::Get()->TrackSessionEnd();

			// End is not submitted until a following start to check for spurious end/start.
			AnalyticsManager::Get()->TrackSessionStart(WorldTime::GetUTCTime() + TimeInterval(15));

			AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kSet);
			update.GetUpdates().MakeTable();
			SEOUL_VERIFY(update.GetUpdates().SetInt32ValueToTable(update.GetUpdates().GetRootNode(), HString("TestInt32"), 27));
			SEOUL_VERIFY(update.GetUpdates().SetStringToTable(update.GetUpdates().GetRootNode(), HString("TestString"), "Hello World"));
			AnalyticsManager::Get()->UpdateProfile(update);
		}
		else if (1 == i)
		{
			AnalyticsManager::Get()->Flush();

			Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
			while (
				(eventServer.GetReceivedRequestCount() < 1 || profileServer.GetReceivedRequestCount() < 1) &&
				SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 5.0)
			{
				helper.Tick();
			}
		}

		SEOUL_UNITTESTING_ASSERT_MESSAGE(i == eventServer.GetReceivedRequestCount(), "Expected %d, got %d", i, eventServer.GetReceivedRequestCount());
		SEOUL_UNITTESTING_ASSERT_MESSAGE(i == profileServer.GetReceivedRequestCount(), "Expected %d, got %d", i, profileServer.GetReceivedRequestCount());
	}
}

// Test for a regression - MixpanelAnalyticsManager filters
// rapid session end/start pairs generated by quick background/foreground
// actions. This test verifies that the analytics manager
// behaves as expected in this case.
void MixpanelTest::TestSessionFilter()
{
	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings settings;
	settings.m_AnalyticsSettings = analyticsSettings;
	settings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, settings);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestFail);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestFail);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());

	// Send quick end/start events in the middle of events with sufficient separation.
	auto const baseTime = WorldTime::GetUTCTime();
	AnalyticsManager::Get()->TrackSessionEnd(baseTime);
	AnalyticsManager::Get()->TrackSessionStart(baseTime + TimeInterval(1));
	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 2.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, profileServer.GetReceivedRequestCount());
}

namespace
{

class TestingTrackingManager SEOUL_SEALED : public TrackingManager
{
public:
	static CheckedPtr<TestingTrackingManager> Get()
	{
		if (TrackingManager::Get() &&
			TrackingManager::Get()->GetType() == TrackingManagerType::kTesting)
		{
			return (TestingTrackingManager*)TrackingManager::Get().Get();
		}
		else
		{
			return CheckedPtr<TestingTrackingManager>();
		}
	}

	TestingTrackingManager(const WorldTime& baseTime)
		: m_BaseTime(baseTime)
	{
	}

	~TestingTrackingManager()
	{
	}

	virtual TrackingManagerType GetType() const SEOUL_OVERRIDE { return TrackingManagerType::kTesting; }

	virtual String GetExternalTrackingUserID() const SEOUL_OVERRIDE
	{
		return String();
	}

	virtual Bool OpenThirdPartyURL(const String& sURL) const SEOUL_OVERRIDE
	{
		return false;
	}

	virtual Bool ShowHelp() const SEOUL_OVERRIDE
	{
		// Nop
		return false;
	}

	virtual void SetTrackingUserID(const String& sUserName, const String& sUserID) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void TrackEvent(const String& sEventName) SEOUL_OVERRIDE
	{
		// Nop
	}

	Vector<UUID, MemoryBudgets::Developer> m_vExpectedUUIDs;
	Vector<UUID, MemoryBudgets::Developer> m_vUUIDs;

private:
	virtual void OnSessionChange(const AnalyticsSessionChangeEvent& evt) SEOUL_OVERRIDE
	{
		auto const i = m_iEvent;
		SEOUL_UNITTESTING_ASSERT_EQUAL((i % 2 == 0), evt.m_bSessionStart);
		if (evt.m_bSessionStart)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(m_BaseTime + TimeInterval((i / 2) * 17), evt.m_TimeStamp);
		}
		else
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(m_BaseTime + TimeInterval((i / 2) * 17 + 2), evt.m_TimeStamp);
			SEOUL_UNITTESTING_ASSERT_EQUAL(TimeInterval(2), evt.m_Duration);
		}
		m_vUUIDs.PushBack(evt.m_SessionUUID);
		++m_iEvent;
	}

	WorldTime m_BaseTime{};
	Int m_iEvent{};

	SEOUL_DISABLE_COPY(TestingTrackingManager);
}; // class TestingTrackingManager

Bool TestSessionsEvents(HTTP::ServerResponseWriter& responseWriter, const HTTP::ServerRequestInfo& info)
{
	TestSessionEndState endState;

	TestHTTPHeaders(info, true);

	String sBody(info.m_vBody.Data(), info.m_vBody.GetSize());
	sBody = URLDecode(sBody.CStr());
	sBody = TrimWhiteSpace(sBody);

	static const String ksPrefix("verbose=1&ip=1&data=");
	SEOUL_UNITTESTING_ASSERT(sBody.StartsWith(ksPrefix));
	String sData(sBody.Substring(ksPrefix.GetSize()));

	Vector<Byte> vData;
	SEOUL_VERIFY(Base64Decode(sData, vData));

	DataStore dataStore;
	SEOUL_VERIFY(DataStoreParser::FromString(vData.Data(), vData.GetSize(), dataStore));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetRootNode().IsArray());
	DataNode arr = dataStore.GetRootNode();
	UInt32 uCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(arr, uCount));
	SEOUL_UNITTESTING_ASSERT_EQUAL(13u, uCount);

	DataNode node;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 0u, node));
	TestSessionStart(dataStore, node, 1);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 1u, node));
	TestBasicEvent(dataStore, node, "TestEvent", endState);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 2u, node));
	TestSessionEnd(dataStore, node, endState, TimeInterval(2));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 3u, node));
	TestSessionStart(dataStore, node, 2);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 4u, node));
	TestBasicEvent(dataStore, node, "TestEvent", endState);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 5u, node));
	TestSessionEnd(dataStore, node, endState, TimeInterval(2));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 6u, node));
	TestSessionStart(dataStore, node, 3);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 7u, node));
	TestBasicEvent(dataStore, node, "TestEvent", endState);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 8u, node));
	TestSessionEnd(dataStore, node, endState, TimeInterval(2));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 9u, node));
	TestSessionStart(dataStore, node, 4);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 10u, node));
	TestBasicEvent(dataStore, node, "TestEvent", endState);

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 11u, node));
	TestSessionEnd(dataStore, node, endState, TimeInterval(2));

	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, 12u, node));
	TestSessionStart(dataStore, node, 5);

	// If the TestingTrackingManager exists, also push UUIDS
	// for validation. We accumulate UUIDs in start session
	// and end session pairs, plus 1 final "terminator" start
	// session event, which is necessary (since an end session
	// is not sent until the following start session, to filter
	// out short sessions).
	auto pTracking(TestingTrackingManager::Get());
	if (pTracking)
	{
		auto& v = pTracking->m_vExpectedUUIDs;
		static const UInt32 kauSteps[] = { 2u, 1u };
		auto uStep = 0u;
		for (UInt32 i = 0u; i < 13u; )
		{
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(arr, i, node));
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString("properties"), node));
			String sUUID;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(node, HString("s_session_id"), node));
			SEOUL_UNITTESTING_ASSERT(dataStore.AsString(node, sUUID));
			v.PushBack(UUID::FromString(sUUID));
			i += kauSteps[uStep];
			uStep = (uStep + 1u) % 2u;
		}
	}

	// Write expected response - verbose enabled, so a JSON blob.
	responseWriter.WriteStatusResponse(200, HTTP::HeaderTable(), R"({"status": 1})");

	return true;
}

} // namespace anonymous

// Test for a regression - previously session_id and player_sessions were not
// set reliably and could not be the expected values (different UUID per
// start/end pair and the same session count for all events between).
void MixpanelTest::TestSessions()
{
	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings settings;
	settings.m_AnalyticsSettings = analyticsSettings;
	settings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, settings);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestSessionsEvents);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestFail);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	// Set a series of events with multiple start/end times.
	auto time = WorldTime::GetUTCTime();
	AnalyticsEvent evt("TestEvent");
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetInt32ValueToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeInt"), 21));
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetStringToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeString"), "Hello World"));
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackEvent(evt, time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackSessionEnd(time); time += TimeInterval(15);
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackEvent(evt, time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackSessionEnd(time); time += TimeInterval(15);
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackEvent(evt, time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackSessionEnd(time); time += TimeInterval(15);
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackEvent(evt, time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackSessionEnd(time); time += TimeInterval(15);
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1); // Need a "dangling" start to send end.

	// Set the analytics id now so events are sent together.
	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());

	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 2.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, profileServer.GetReceivedRequestCount());
}

// Verify that analytics manager session events are
// reliably echoed to the tracking manager.
void MixpanelTest::TestTrackingManagerEcho()
{
	auto time = WorldTime::GetUTCTime();

	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings settings;
	settings.m_AnalyticsSettings = analyticsSettings;
	settings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, settings);

	// Cheat and replace the tracking manager with a testing version.
	SEOUL_DELETE TrackingManager::Get();
	SEOUL_NEW(MemoryBudgets::Developer) TestingTrackingManager(time);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestSessionsEvents);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestFail);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	// Set a series of events with multiple start/end times.
	AnalyticsEvent evt("TestEvent");
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetInt32ValueToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeInt"), 21));
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetStringToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeString"), "Hello World"));
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackEvent(evt, time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackSessionEnd(time); time += TimeInterval(15);
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackEvent(evt, time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackSessionEnd(time); time += TimeInterval(15);
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackEvent(evt, time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackSessionEnd(time); time += TimeInterval(15);
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackEvent(evt, time); time += TimeInterval(1);
	AnalyticsManager::Get()->TrackSessionEnd(time); time += TimeInterval(15);
	AnalyticsManager::Get()->TrackSessionStart(time); time += TimeInterval(1); // Need a "dangling" start to send end.

																			   // Set the analytics id now so events are sent together.
	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());

	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 2.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, profileServer.GetReceivedRequestCount());

	// Give the Jobs::Manager some time to make sure tracking manager is engaged.
	while (Jobs::Manager::Get()->YieldThreadTime()) {}

	// Now verifed expected UUIDs against actual.
	SEOUL_UNITTESTING_ASSERT_EQUAL(9u, TestingTrackingManager::Get()->m_vExpectedUUIDs.GetSize());
	SEOUL_UNITTESTING_ASSERT_EQUAL(9u, TestingTrackingManager::Get()->m_vUUIDs.GetSize());
	for (UInt32 i = 0u; i < 9u; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			TestingTrackingManager::Get()->m_vExpectedUUIDs[i],
			TestingTrackingManager::Get()->m_vUUIDs[i]);
	}
}

void MixpanelTest::TestPruningByAgeEvents()
{
	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings settings;
	settings.m_AnalyticsSettings = analyticsSettings;
	settings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, settings);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicEvents);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicProfiles);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());

	AnalyticsEvent evt("TestEvent");
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetInt32ValueToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeInt"), 21));
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetStringToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeString"), "Hello World"));

	// Insert some old events that will be pruned.
	auto const now = WorldTime::GetUTCTime();
	AnalyticsManager::Get()->TrackSessionStart(now - TimeInterval::FromDaysInt64(6));
	AnalyticsManager::Get()->TrackEvent(evt, now - TimeInterval::FromDaysInt64(6) + TimeInterval::FromHoursInt64(1));
	AnalyticsManager::Get()->TrackSessionEnd(now - TimeInterval::FromDaysInt64(6) + TimeInterval::FromHoursInt64(2));

	// These events should show.
	AnalyticsManager::Get()->TrackSessionStart();
	AnalyticsManager::Get()->TrackEvent(evt);
	AnalyticsManager::Get()->TrackSessionEnd();

	// End is not submitted until a following start to check for spurious end/start.
	AnalyticsManager::Get()->TrackSessionStart(WorldTime::GetUTCTime() + TimeInterval(15));

	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (
		(eventServer.GetReceivedRequestCount() < 1) &&
		SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 5.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(1, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, profileServer.GetReceivedRequestCount());
}

void MixpanelTest::TestPruningByAgeProfile()
{
	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings settings;
	settings.m_AnalyticsSettings = analyticsSettings;
	settings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, settings);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicEvents);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicProfiles);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());

	AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kSet);
	update.GetUpdates().MakeTable();
	SEOUL_VERIFY(update.GetUpdates().SetInt32ValueToTable(update.GetUpdates().GetRootNode(), HString("TestInt32"), 27));
	SEOUL_VERIFY(update.GetUpdates().SetStringToTable(update.GetUpdates().GetRootNode(), HString("TestString"), "Hello World"));

	// Update should get pruned.
	AnalyticsManager::Get()->UpdateProfile(update, WorldTime::GetUTCTime() - TimeInterval::FromDaysInt64(6));

	AnalyticsManager::Get()->UpdateProfile(update);

	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (
		(profileServer.GetReceivedRequestCount() < 1) &&
		SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 5.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(1, profileServer.GetReceivedRequestCount());
}

void MixpanelTest::TestPruningBySizeEvents()
{
	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1 / 1000.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings settings;
	settings.m_AnalyticsSettings = analyticsSettings;
	settings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, settings);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestEventsPruned);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicProfiles);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	AnalyticsEvent evt("TestEvent");
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetInt32ValueToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeInt"), 21));
	SEOUL_UNITTESTING_ASSERT(evt.GetProperties().SetStringToTable(evt.GetProperties().GetRootNode(), HString("TestAttributeString"), "Hello World"));

	// Add 2000 updates, should only get 1000.
	// These events should show.
	for (Int i = 0; i < 2000; ++i)
	{
		AnalyticsManager::Get()->TrackEvent(evt);
	}

	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());
	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (
		(eventServer.GetReceivedRequestCount() < 20) &&
		SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 15.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(20, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, profileServer.GetReceivedRequestCount());
}

void MixpanelTest::TestPruningBySizeProfile()
{
	GenericAnalyticsManagerSettings analyticsSettings;
	analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0 / 1000.0;
	analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
	analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
	analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
	analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
	NullPlatformEngineSettings settings;
	settings.m_AnalyticsSettings = analyticsSettings;
	settings.m_bEnableSaveApi = true;
	UnitTestsEngineHelper helper(nullptr, settings);

	HTTP::ServerSettings httpSettings;
	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestBasicEvents);
	httpSettings.m_iPort = 8057;
	httpSettings.m_iThreadCount = 1;
	HTTP::Server eventServer(httpSettings);

	httpSettings.m_Handler = SEOUL_BIND_DELEGATE(&TestProfilesPruned);
	httpSettings.m_iPort = 8058;
	HTTP::Server profileServer(httpSettings);

	AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kSet);
	update.GetUpdates().MakeTable();
	SEOUL_VERIFY(update.GetUpdates().SetInt32ValueToTable(update.GetUpdates().GetRootNode(), HString("TestInt32"), 27));
	SEOUL_VERIFY(update.GetUpdates().SetStringToTable(update.GetUpdates().GetRootNode(), HString("TestString"), "Hello World"));

	// Add 2000 updates, should only get 1000.
	for (Int i = 0; i < 2000; ++i)
	{
		AnalyticsManager::Get()->UpdateProfile(update);
	}

	AnalyticsManager::Get()->SetAnalyticsUserID(GetAnalyticsUserId());
	AnalyticsManager::Get()->Flush();

	Int64 const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
	while (
		(profileServer.GetReceivedRequestCount() < 20) &&
		SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks) < 15.0)
	{
		helper.Tick();
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0, eventServer.GetReceivedRequestCount());
	SEOUL_UNITTESTING_ASSERT_EQUAL(20, profileServer.GetReceivedRequestCount());
}

void MixpanelTest::TestAnalyticsDisable()
{
	// Test analytics disable by returning an empty API key.
	{
		GenericAnalyticsManagerSettings analyticsSettings;
		analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
		analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
		analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetEmptyApiKey);
		analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
		analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
		NullPlatformEngineSettings settings;
		settings.m_AnalyticsSettings = analyticsSettings;
		settings.m_bEnableSaveApi = true;
		UnitTestsEngineHelper helper(nullptr, settings);

		SEOUL_UNITTESTING_ASSERT_EQUAL(AnalyticsManagerType::kNull, AnalyticsManager::Get()->GetType());
	}

	// Real initialization to confirm.
	{
		GenericAnalyticsManagerSettings analyticsSettings;
		analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
		analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
		analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
		analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
		analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
		NullPlatformEngineSettings settings;
		settings.m_AnalyticsSettings = analyticsSettings;
		settings.m_bEnableSaveApi = true;
		UnitTestsEngineHelper helper(nullptr, settings);

		SEOUL_UNITTESTING_ASSERT_EQUAL(AnalyticsManagerType::kMixpanel, AnalyticsManager::Get()->GetType());
	}

	// Test analytics disable with the should-send-analytics delegate query.
	{
		GenericAnalyticsManagerSettings analyticsSettings;
		analyticsSettings.m_fHeartbeatTimeInSeconds = 1.0;
		analyticsSettings.m_eType = GenericAnalyticsManagerType::kMixpanel;
		analyticsSettings.m_GetApiKeyDelegate = SEOUL_BIND_DELEGATE(GetTestApiKey);
		analyticsSettings.m_ShouldSendAnalyticsDelegate = SEOUL_BIND_DELEGATE(DoNotSendAnalytics);
		analyticsSettings.m_GetBaseEventURL = SEOUL_BIND_DELEGATE(GetTestEventBaseURL);
		analyticsSettings.m_GetBaseProfileURL = SEOUL_BIND_DELEGATE(GetTestProfileBaseURL);
		NullPlatformEngineSettings settings;
		settings.m_AnalyticsSettings = analyticsSettings;
		settings.m_bEnableSaveApi = true;
		UnitTestsEngineHelper helper(nullptr, settings);

		SEOUL_UNITTESTING_ASSERT_EQUAL(AnalyticsManagerType::kNull, AnalyticsManager::Get()->GetType());
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
