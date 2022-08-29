/**
 * \file GameClient.cpp
 * \brief Common game client class for HTTP RESTful communication with a server.
 * Higher level utility functions around Seoul::HTTP that define specific, game
 * agnostic requests and handling.
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
#include "DataStoreParser.h"
#include "Engine.h"
#include "GameClient.h"
#include "GameMain.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "HTTPRequestList.h"
#include "HTTPResponse.h"
#include "JobsManager.h"
#include "Lexer.h"
#include "LocManager.h"
#include "Logger.h"
#include "PatchablePackageFileSystem.h"
#include "ReflectionDefine.h"
#include "ReflectionDeserialize.h"
#include "ReflectionSerialize.h"
#include "SaveLoadManager.h"
#include "SeoulOS.h"
#include "ThreadId.h"

namespace Seoul::Game
{

/** Minimum confidence interval time we'll record, to prevent getting stuck at an unreasonable floor. */
static const Double kfMinimumConfidenceIntervalInSeconds = 0.1;

Client::CacheEntry::CacheEntry(HTTP::Response* pResponse)
	: m_pBody(nullptr)
	, m_uBodySize(pResponse->GetBodySize())
	, m_pHeaders(SEOUL_NEW(MemoryBudgets::Network) HTTP::HeaderTable)
{
	if (m_uBodySize > 0u)
	{
		m_pBody = MemoryManager::Allocate(m_uBodySize, MemoryBudgets::Network);
		memcpy(m_pBody, pResponse->GetBody(), m_uBodySize);
	}

	m_pHeaders->CloneFrom(pResponse->GetHeaders());
}

Client::CacheEntry::~CacheEntry()
{
	MemoryManager::Deallocate(m_pBody);
	m_pBody = nullptr;
	m_uBodySize = 0u;
}

void Client::CacheEntry::ReplaceBody(String const& newBody)
{
	m_uBodySize = newBody.GetSize();
	if (m_uBodySize > 0)
	{
		m_pBody = MemoryManager::Reallocate(m_pBody, m_uBodySize, MemoryBudgets::Network);
		memcpy(m_pBody, newBody.CStr(), m_uBodySize);
	}
	else
	{
		MemoryManager::Deallocate(m_pBody);
		m_pBody = nullptr;
	}
}

Client::Client()
	: m_plPendingRequests(SEOUL_NEW(MemoryBudgets::Network) HTTP::RequestList)
	, m_ServerTimeStamp()
	, m_ClientUptimeAtLastServerTimeStampUpdate()
	, m_fLastConfidenceIntervalInSeconds(DoubleMax)
	, m_sAuthToken()
	, m_CacheMutex()
	, m_tCache()
{
}

Client::~Client()
{
	// Lock the lifespan mutex for the body of this destructor.
	Lock lifespanLock(s_LifespanMutex);

	// Now wait until the count reaches 0.
	while (m_LifespanCount > 0)
	{
		Jobs::Manager::Get()->YieldThreadTime();
	}

	// Once the lifespan count has reached zero, we
	// can safely finish cleanup and release the
	// singleton - we must do this prior to returning,
	// or a threaded operation could see
	// the still valid singleton pointer after
	// we release the lifespan mutex.

	m_plPendingRequests->BlockingCancelAll();

	// Lock the cache and clean it up.
	Lock lock(m_CacheMutex);
	SafeDeleteTable(m_tCache);

	// Release the singleton before releasing the lifespan mutex.
	ReleaseSingleton();
}

/** Cancel any HTTP requests issued by Game::Client that are still pending. */
void Client::CancelPendingRequests()
{
	m_plPendingRequests->BlockingCancelAll();
}

HTTP::Request& Client::CreateRequest(
	const String& sURL,
	const HTTP::ResponseDelegate& callback,
	HString sMethod /* = HTTP::Method::ksGet */,
	Bool bResendOnFailure /* = true */,
	Bool bSuppressErrorMail /* = false */)
{
	auto& r = HTTP::Manager::Get()->CreateRequest(m_plPendingRequests.Get());
	r.SetMethod(sMethod);
	r.SetURL(sURL);
	r.SetCallback(callback);
	r.SetResendOnFailure(bResendOnFailure);

	PrepareRequest(r, bSuppressErrorMail);

	return r;
}

void Client::PrepareRequest(HTTP::Request& rRequest, Bool bSuppressErrorMail) const
{
	if (bSuppressErrorMail)
	{
		static const String ksSuppressErrorMailHeader("x-demiurge-suppress-error-mail");
		rRequest.AddHeader(ksSuppressErrorMailHeader, "true");
	}

	rRequest.AddHeader("accept-language", Engine::Get()->GetSystemLanguageCode());
	rRequest.AddHeader("x-demiurge-current-language", LocManager::Get()->GetCurrentLanguage());

	if (!m_sAuthToken.IsEmpty())
	{
		static const String ksAuthTokenHeader("x-demiurge-auth-token");
		rRequest.AddHeader(ksAuthTokenHeader, m_sAuthToken);
	}

#if (SEOUL_PLATFORM_WINDOWS && !SEOUL_SHIP)
	{
		// For Windows Ship builds, include the Windows username so
		// we can get in touch with the people seeing errors faster
		String sUsername = GetUsername();
		if (!sUsername.IsEmpty())
		{
			static const String ksWindowsUsernameHeader("x-demiurge-windows-username");
			rRequest.AddHeader(ksWindowsUsernameHeader, sUsername);
		}
	}
#endif

	if (SaveLoadManager::Get())
	{
		static const String ksSessionGuidHeader = "x-demiurge-session-guid";
		rRequest.AddHeader(ksSessionGuidHeader, SaveLoadManager::Get()->GetSessionGuid());
	}

	// If either config or content changelist is missing, the server will have to fill in its own values
	auto pConfig = Main::Get()->GetConfigUpdatePackageFileSystem();
	if (pConfig.IsValid())
	{
		static const String ksConfigChangelistHeader = "x-demiurge-config-changelist";
		auto iChangelist = pConfig->GetBuildChangelist();
		rRequest.AddHeader(ksConfigChangelistHeader, ToString(iChangelist));
	}
}

void Client::SetAuthToken(String const& sToken)
{
	m_sAuthToken = sToken;
}

String const& Client::GetAuthToken() const
{
	return m_sAuthToken;
}

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
void Client::RemoteNotificationTokenChanged(RemoteNotificationType eType, Bool bIsDevelopmentEnvironment, const String& sDeviceToken)
{
	SEOUL_LOG(
		"Registered for remote notifications - type: %s; is_development: %s; token: %s",
		EnumToString<RemoteNotificationType>(eType), (bIsDevelopmentEnvironment) ? "t" : "f", sDeviceToken.CStr());

	// Register with Demiurge servers
	auto const sServerBaseURL(Main::Get()->GetServerBaseURL());
	if (!sServerBaseURL.IsEmpty())
	{
		auto const sURL(String::Printf("%s/v1/notifications/report_token", sServerBaseURL.CStr()));
		auto& rRequest = Client::Get()->CreateRequest(
			sURL,
			HTTP::ResponseDelegate(), // no response handling required
			HTTP::Method::ksPost,
			true);

		// TODO: Update our backend so this is not necessary.
		if (RemoteNotificationType::kFCM == eType)
		{
			rRequest.AddPostData("Type", "GCM");
		}
		else
		{
			rRequest.AddPostData("Type", EnumToString<RemoteNotificationType>(eType));
		}

		rRequest.AddPostData("Token", sDeviceToken);
		rRequest.AddPostData("IsDevelopment", (bIsDevelopmentEnvironment) ? "true" : "false");

		rRequest.Start();
	}

#if SEOUL_PLATFORM_IOS || (SEOUL_PLATFORM_ANDROID && !defined(AMAZON))
	// Register with analytics provider. Mixpanel only supports IOS and Google Play,
	// so this is more strict than #if SEOUL_WITH_REMOTE_NOTIFICATIONS
	{
		// Set "$ios_devices" = ["token"] or "$android_devices" = ["token"], depending on platform
#if SEOUL_PLATFORM_IOS
		// Mixpanel iOS APNS key
		static const HString kDevices("$ios_devices");
#elif (SEOUL_PLATFORM_ANDROID && !defined(AMAZON))
		// Mixpanel Android GCM key
		static const HString kDevices("$android_devices");
#endif

		SEOUL_LOG_ANALYTICS("Registering Device Token with Analytics Manager: %s.", sDeviceToken.CStr());

		AnalyticsProfileUpdate update(AnalyticsProfileUpdateOp::kUnion);
		auto& updateData = update.GetUpdates();

		// Create token array
		updateData.SetArrayToTable(updateData.GetRootNode(), kDevices, 1u);

		// Add token to array
		DataNode tokenArray;
		updateData.GetValueFromTable(updateData.GetRootNode(), kDevices, tokenArray);
		updateData.SetStringToArray(tokenArray, 0u, sDeviceToken);

		// Union with mixpanel profile
		AnalyticsManager::Get()->UpdateProfile(update);
	}
#endif
}

void Client::RequestRemoteNotificationsIfSilent()
{
	if (!Engine::Get()->CanRequestRemoteNotificationsWithoutPrompt())
	{
		return;
	}

	Engine::Get()->RegisterForRemoteNotifications();
}
#endif

/**
 * Context for de/serializing save game. Ignores some warnings and errors that aren't appropriate for
 * save games.
 */
class ServerJsonSerializeContext SEOUL_SEALED : public Reflection::DefaultSerializeContext
{
public:
	ServerJsonSerializeContext(
		const ContentKey& contentKey,
		const DataStore& dataStore,
		const DataNode& table,
		const Reflection::TypeInfo& typeInfo,
		Bool bRequireProperties)
		: DefaultSerializeContext(contentKey, dataStore, table, typeInfo)
		, m_bRequireProperties(bRequireProperties)
	{
	}

	virtual Bool HandleError(Reflection::SerializeError eError, HString additionalData = HString()) SEOUL_OVERRIDE
	{
		using namespace Reflection;

		if ((m_bRequireProperties || SerializeError::kRequiredPropertyHasNoCorrespondingValue != eError) &&
			SerializeError::kDataStoreContainsUndefinedProperty != eError)
		{
			return DefaultSerializeContext::HandleError(eError, additionalData);
		}
		else
		{
			return true;
		}
	}

	Bool m_bRequireProperties;
}; // class ServerJsonSerializeContext

/**
 * Helper for deserializing HTTP responses
 */
Bool Client::DeserializeResponseJSON(HTTP::Response const * const pResponse, const Reflection::WeakAny& outObject, Bool bRequireProperties /* = false */)
{
	SharedPtr<DataStore> pDataStore;
	return DeserializeResponseJSON(pResponse, outObject, pDataStore, bRequireProperties);
}

Bool Client::DeserializeResponseJSON(HTTP::Response const * const pResponse, const Reflection::WeakAny& outObject, SharedPtr<DataStore>& pOutDataStore, Bool bRequireProperties /*= false*/)
{
	if (pResponse == nullptr)
	{
		return false;
	}

	DataStore dataStore;
	if (!DataStoreParser::FromString((Byte const*)pResponse->GetBody(), pResponse->GetBodySize(), dataStore))
	{
		return false;
	}

	ServerJsonSerializeContext context(ContentKey(), dataStore, dataStore.GetRootNode(), outObject.GetTypeInfo(), bRequireProperties);
	Bool const bReturn = DeserializeObject(context, dataStore, dataStore.GetRootNode(), outObject);
	if (bReturn)
	{
		pOutDataStore.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStore);
		pOutDataStore->Swap(dataStore);
		return true;
	}
	else
	{
		return false;
	}
}

Bool Client::SerializeToJson(const Reflection::WeakAny& object, String& sResult)
{
	DataStore dataStore;
	dataStore.MakeTable();

	HString key("key");
	if(!Reflection::SerializeObjectToTable(ContentKey(), dataStore, dataStore.GetRootNode(), key, object))
	{
		SEOUL_WARN("Error serializing player save data");
		return false;
	}

	// Write the DataStore to a string
	DataNode saveRoot;
	if(!dataStore.GetValueFromTable(dataStore.GetRootNode(), key, saveRoot))
	{
		SEOUL_WARN("Error reading %s section", key.CStr());
		return false;
	}

	dataStore.ToString(saveRoot, sResult, false, 0, true);
	return true;
}

static String CheckRequestForDownTimeString(HTTP::Response *pResponse)
{
	if (!pResponse)
	{
		return String();
	}

	if (pResponse->GetStatus() != 503)
	{
		return String();
	}

	// If the response is a 503, look for a message from the server to display
	static const HString ksServerDownHeader("x-demiurge-server-down-message");
	String sServerDownMessage;
	if (!pResponse->GetHeaders().GetValue(ksServerDownHeader, sServerDownMessage))
	{
		// Header not present
		return String();
	}

	return sServerDownMessage;
}

void Client::UpdateCurrentServerTimeFromResponse(HTTP::Response *pResponse)
{
	if (Main::Get())
	{
		Main::Get()->SetServerDownMessage(CheckRequestForDownTimeString(pResponse));
	}

	const HTTP::HeaderTable& tHeaders = pResponse->GetHeaders();

	static HString const ksHeaderTimestamp("x-demiurge-timestamp");
	static HString const ksHeaderRequestDurationNs("x-demiurge-request-duration-ns");

	String sServerTime;
	if (!tHeaders.GetValue(ksHeaderTimestamp, sServerTime))
	{
		return;
	}
	String sRequestDurationNs;
	if (!tHeaders.GetValue(ksHeaderRequestDurationNs, sRequestDurationNs))
	{
		return;
	}

	WorldTime ServerTime = WorldTime::ParseISO8601DateTime(sServerTime);
	if (ServerTime.IsZero())
	{
		return;
	}

	Int64 iRequestDurationNs;
	if (!FromString(sRequestDurationNs, iRequestDurationNs))
	{
		return;
	}

	UpdateCurrentServerTime(
			ServerTime,
			WorldTime::ConvertInt64NanosecondsToSeconds(iRequestDurationNs),
			pResponse->GetRoundTripTimeInSeconds(),
			pResponse->GetUptimeValueAtReceive());
}

WorldTime Client::StaticGetCurrentServerTime()
{
	if (Client::Get().IsValid())
	{
		return Client::Get()->GetCurrentServerTime();
	}

	return WorldTime::GetUTCTime();
}

Bool Client::StaticHasCurrentServerTime()
{
	return (Client::Get().IsValid() && Client::Get()->HasCurrentServerTime());
}

/** Call with a new server time stamp value. */
void Client::UpdateCurrentServerTime(
	const WorldTime& newServerTimeStamp,
	Double fServerRequestFunctionTimeInSeconds,
	Double fRequestRoundTripTimeInSeconds,
	TimeInterval uptimeInMillisecondsAtRequestReceive)
{
	// Update the uptime so it is reasonable.
	if (uptimeInMillisecondsAtRequestReceive == TimeInterval())
	{
		uptimeInMillisecondsAtRequestReceive = Engine::Get()->GetUptime();
	}

	// Always accept if we don't have a time yet.
	// A bad estimate in this case is better than no estimate.
	Bool bAccept = (WorldTime() == m_ServerTimeStamp);
	Bool const bRefreshQueuedTimes = bAccept;

	// Compute time adjustment, try to account for as much server
	// communication time as we can. Based on the NTP protocol
	// adjustment (we subtract the processing time, which is time spent
	// on the server, from the round trip time, and divide the remaining
	// time by 2, as an approximation for the confidence interval of the
	// time estimate).
	Double const fConfidenceIntervalInSeconds = (fRequestRoundTripTimeInSeconds -
		Min(fRequestRoundTripTimeInSeconds, fServerRequestFunctionTimeInSeconds)) / 2.0;

	// Test if we should accept.
	if (!bAccept)
	{
		// Accept if the confidence interval is better than our best sample so far.
		bAccept = (fConfidenceIntervalInSeconds < m_fLastConfidenceIntervalInSeconds);
	}

	// If using the time, update members.
	if (bAccept)
	{
		// Compute the adjustment to apply.
		Int64 const iConfidenceIntervalInMicroseconds = WorldTime::ConvertSecondsToInt64Microseconds(fConfidenceIntervalInSeconds);

		// Advance the server time by the adjustment factor.
		WorldTime const adjustedServerTimeStamp = (newServerTimeStamp + TimeInterval::FromMicroseconds(
			iConfidenceIntervalInMicroseconds));

		// Set values.
		{
			// Refresh the client time marker.
			m_ClientUptimeAtLastServerTimeStampUpdate = uptimeInMillisecondsAtRequestReceive;

			// Update the server time stamp.
			m_ServerTimeStamp = adjustedServerTimeStamp;

			// Update confidence interval - clamp so we don't end up in an unreasonable minimum.
			m_fLastConfidenceIntervalInSeconds = Max(fConfidenceIntervalInSeconds, kfMinimumConfidenceIntervalInSeconds);
		}
	}

	// If we need to refresh times (we just converted from local time to server time),
	// run that now.
	if (bRefreshQueuedTimes)
	{
		// Adjust the timestamp of any queued events before we allow them to be sent,
		// since they will have been queued with local time.
		TimeInterval const timeInterval = (GetCurrentServerTime() - WorldTime::GetUTCTime());
		AnalyticsManager::Get()->OnTimeFunctionTimeChange(
			SEOUL_BIND_DELEGATE(&Client::StaticGetCurrentServerTime),
			timeInterval);
	}
}

namespace
{

struct CacheRequestBinder SEOUL_SEALED
{
	CacheRequestBinder(const HTTP::ResponseDelegate& callback, const String& sURL)
		: m_Callback(callback)
		, m_sURL(sURL)
	{
	}

	HTTP::ResponseDelegate const m_Callback;
	String const m_sURL;
}; // struct CacheRequestBinder

} // namespace anonymous

/**
 * Wraps a request callback so that successful results (status code 200)
 * are cached. Cached data can be accessed via Game::ClientCacheLock
 * using the given sURL as a key.
 *
 * IMPORTANT: The returned callback *must* be invoked once and only once,
 * or a memory leak will occur. Assigning it to an HTTP request with SetCallback()
 * is the expected use case.
 */
HTTP::ResponseDelegate Client::WrapCallbackForCache(const HTTP::ResponseDelegate& callback, const String& sURL)
{
	auto pBinder = SEOUL_NEW(MemoryBudgets::Network) CacheRequestBinder(callback, sURL);
	return SEOUL_BIND_DELEGATE(&OnCachedRequest, (void*)pBinder);
}

/** Wrapper middleware for WrapCallbackForCache(). */
HTTP::CallbackResult Client::OnCachedRequest(void* pUserData, HTTP::Result eResult, HTTP::Response* pResponse)
{
	// Resolve the binder.
	auto pBinder = (CacheRequestBinder*)pUserData;

	// Success, perform processing.
	if (HTTP::Result::kSuccess == eResult && pResponse->GetStatus() == (Int)HTTP::Status::kOK)
	{
		// Check for the Game::Client singleton.
		auto pClient(Client::Get());
		if (pClient)
		{
			Lock lock(pClient->m_CacheMutex);

			// Cleanup existing.
			{
				CacheEntry* p = nullptr;
				if (pClient->m_tCache.GetValue(pBinder->m_sURL, p))
				{
					SEOUL_VERIFY(pClient->m_tCache.Erase(pBinder->m_sURL));
					SafeDelete(p);
				}
			}

			// Insert a new entry.
			CacheEntry* pEntry = SEOUL_NEW(MemoryBudgets::Network) CacheEntry(pResponse);
			SEOUL_VERIFY(pClient->m_tCache.Insert(pBinder->m_sURL, pEntry).Second);
		}
	}

	HTTP::CallbackResult eCallbackResult = HTTP::CallbackResult::kSuccess;
	// Now invoke the original callback, if defined.
	if (pBinder->m_Callback)
	{
		eCallbackResult = pBinder->m_Callback(eResult, pResponse);
	}

	// Cleanup the binder unless a resend is about to be attempted.
	if (HTTP::CallbackResult::kNeedsResend != eCallbackResult)
	{
		// Done, cleanup the binder.
		SafeDelete(pBinder);
	}

	return eCallbackResult;
}

void Client::OverrideCachedDataBody(String const& sURL, DataStore const& dataBodyTable)
{
	String sBodyJson;
	dataBodyTable.ToString(dataBodyTable.GetRootNode(), sBodyJson);

	Lock lock(m_CacheMutex);

	CacheEntry* p = nullptr;
	if (!m_tCache.GetValue(sURL, p))
	{
		return;
	}

	p->ReplaceBody(sBodyJson);
}

void Client::ClearCache(String const& sURL)
{
	Lock lock(m_CacheMutex);

	m_tCache.Erase(sURL);
}

Mutex Client::s_LifespanMutex{};

} // namespace Seoul::Game
