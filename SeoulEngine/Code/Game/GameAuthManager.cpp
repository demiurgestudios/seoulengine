/**
 * \file GameAuthManager.cpp
 * \brief Global singleton, owned by Game::Main, that manages
 * login state with the server and downloading of updatable
 * .sar files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildDistro.h"
#include "Compress.h"
#include "CrashManager.h"
#include "FileManager.h"
#include "FileManagerRemap.h"
#include "GameAnalytics.h"
#include "GameAuthConflictResolveData.h"
#include "GameAuthManager.h"
#include "GameAutomation.h"
#include "GameClient.h"
#include "GameMain.h"
#include "GamePatcher.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "MemoryBarrier.h"
#include "PatchablePackageFileSystem.h"
#include "PlatformSignInManager.h"
#include "ReflectionDefine.h"
#include "ReflectionSerialize.h"
#include "SaveLoadUtil.h"
#include "SeoulString.h"
#include "StringUtil.h"
#include "ToString.h"
#include "UIManager.h"

namespace Seoul::Game
{

static HString const kHasAuthConflict("HasAuthConflict");
static HString const kRecommendedUpdateAvailable("RecommendedUpdateAvailable");
static String const ksTrue("true");

/**
 * The server sends this response if a platform auth method
 * would cause a conflict that can't be easily resolved - the
 * device auth resolves to a player and the player auth resolves
 * to a different player and both players have significant enough
 * progress that the server can't just assume to orphan the
 * device player.
 *
 * When this code is returned, the returned JSON will be
 * a ConflictResolve JSON body instead of an auth result.
 * This contains data that should be presented to the
 * user to perform conflict resolution.
 */
static const Int kAuthConflict = 250;

namespace
{

/** Internal utility, releases m_PendingRequest on exit from a callback, guarantees proper management of that member. */
struct Scoped SEOUL_SEALED
{
	Scoped(Atomic32& r)
		: m_r(r)
	{
	}

	~Scoped()
	{
		// Must always succeed - or something fishy is going on
		// (two requests were allowed to run simultaneously).
		SEOUL_VERIFY(1 == m_r.CompareAndSet(0, 1));
	}

	Atomic32& m_r;
}; // struct Scoped

} // namespace anonymous

static inline Bool NeedsRestart(const String& sUrl, CheckedPtr<PatchablePackageFileSystem> p)
{
	if (p.IsValid())
	{
		return (sUrl != p->GetURL());
	}

	return false;
}

static inline Bool NeedsRestart(const AuthDataRefresh& refresh)
{
	// Track whether a mismatch has occurred that requires bringing up the
	// patcher again.
	Bool b = false;

	// Mismatch between config update .sar and desired.
	auto pMain = Main::Get();
	b = b || (pMain->GetConfigUpdatePackageFileSystem() && NeedsRestart(refresh.m_sConfigUpdateUrl, pMain->GetConfigUpdatePackageFileSystem()));

	// Mismatch between content update .sar and desired.
	b = b || (pMain->GetContentUpdatePackageFileSystem() && NeedsRestart(refresh.m_sContentUpdateUrl, pMain->GetContentUpdatePackageFileSystem()));

	// Remap config change.
	b = b || (FileManagerRemap::ComputeHash(refresh.m_vRemapConfigs.Begin(), refresh.m_vRemapConfigs.End()) != FileManager::Get()->GetRemapHash());

	// Required version check failure.
	b = b || !refresh.m_VersionRequired.CheckCurrentBuild();

	return b;
}

AuthManager::AuthManager()
{
	m_NextAuthRefresh = MakeNextAuthRefreshTime();
}

AuthManager::~AuthManager()
{
}

/** Perform per-frame update operations on the main thread. */
void AuthManager::Update()
{
	// Trigger a login if allowed.
	if (m_bPendingLogin && InternalCanLogin())
	{
		InternalLogin();
	}

	// Synchronize auth conflict state to the UI system.
	UI::Manager::Get()->SetCondition(kHasAuthConflict, HasAuthConflict());

	// Synchronize recommended update status.
	{
		Bool bRecommendedUpdate = false;

		// Apply state.
		AuthData data;
		if (GetAuthData(data))
		{
			bRecommendedUpdate = !data.m_RefreshData.m_VersionRecommended.CheckCurrentBuild();
		}

		// Apply cheats.
#if SEOUL_ENABLE_CHEATS
		bRecommendedUpdate = bRecommendedUpdate || m_bFakeRecommendedUpdate;
#endif // /#if SEOUL_ENABLE_CHEATS

		// Commit.
		UI::Manager::Get()->SetCondition(kRecommendedUpdateAvailable, bRecommendedUpdate);
	}

	if (m_NextAuthRefresh.IsZero())
	{
		// Try grabbing a new refresh time - we need to wait for server
		// time to be populated.
		m_NextAuthRefresh = MakeNextAuthRefreshTime();
	}

	if (!m_NextAuthRefresh.IsZero() &&
		Client::StaticGetCurrentServerTime() > m_NextAuthRefresh)
	{
		Refresh();
	}

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	CheckedPtr<Engine> pEngine = Engine::Get();
	String sNotificationToken = pEngine->GetRemoteNotificationToken();
	if (HasAuthData() && m_LastRemoteNotificationToken != sNotificationToken)
	{
		m_LastRemoteNotificationToken = sNotificationToken;
		Client::Get()->RemoteNotificationTokenChanged(
			Engine::Get()->GetRemoteNotificationType(),
			Engine::Get()->IsRemoteNotificationDevelopmentEnvironment(),
			sNotificationToken);
	}
#endif // /#if SEOUL_WITH_REMOTE_NOTIFICATIONS

	// Handle trigger a force return to the patcher due
	// to an auth change or failure.
	CheckReturnToPatcher();
}

/**
 * Get the most recent auth data from the server.
 *
 * @return false until auth data has been received from the server,
 * true therafter (once HasAuthData() returns true or this
 * method succeeds once, it will always succeed after that).
 */
Bool AuthManager::GetAuthData(AuthData& rData) const
{
	if (!m_bHasAuthData)
	{
		return false;
	}

	{
		Lock lock(m_DataMutex);
		if (!m_bHasAuthData)
		{
			return false;
		}
		rData = m_AuthData;
#if SEOUL_ENABLE_CHEATS
		if (m_bFakeRequiredUpdate)
		{
			FakeRequiredUpdateForAuthData(rData);
		}
		if (!m_vRemapConfigsOverride.IsEmpty())
		{
			rData.m_RefreshData.m_vRemapConfigs = m_vRemapConfigsOverride;
		}
#endif // /#if SEOUL_ENABLE_CHEATS
	}
	return true;
}

/**
 * Retrieve a snapshot of current conflict data, if conflict data is defined.
 */
Bool AuthManager::GetAuthConflictData(AuthConflictResolveData& rData) const
{
	Lock lock(m_DataMutex);
	if (!m_pConflictData.IsValid())
	{
		return false;
	}

	rData = *m_pConflictData;
	return true;
}


static const Int64 iRefreshIntervalMinutes = 15;

WorldTime AuthManager::MakeNextAuthRefreshTime()
{
	// Return zero until we have a real server time.
	if (!Client::StaticHasCurrentServerTime())
	{
		return WorldTime();
	}

	auto result = Client::StaticGetCurrentServerTime();
	result.AddMinutes(iRefreshIntervalMinutes);
	return result;
}

/**
 * For cases where new refresh data has been received
 * outside the auth manager's normal handling. This method
 * returns true. If the new refresh data will trigger
 * a soft reboot/return to the patcher.
 */
Bool AuthManager::ManulUpdateRefreshData(const AuthDataRefresh& refreshData)
{
	// Always defer next auto update on a manual update.
	m_NextAuthRefresh = MakeNextAuthRefreshTime();

	// TODO: Decide on a better way to support games with no server.
	if (!m_bHasAuthData)
	{
		return false;
	}

	// Update the refresh data.
	{
		Lock lock(m_DataMutex);
		m_AuthData.m_RefreshData = refreshData;
	}

	// Commit data to the environment.
	Analytics::SetAnalyticsSandboxed(refreshData.m_bAnalyticsSandboxed);
	Analytics::SetAnalyticsABTests(refreshData.m_tABTests);

	// Check based on input refresh data.
	return NeedsRestart(refreshData);
}

void AuthManager::Refresh()
{
	auto const sServerBaseURL(Main::Get()->GetServerBaseURL());
	m_NextAuthRefresh = MakeNextAuthRefreshTime();

	// TODO: Decide on a better way to support games with no server.
	if (!m_bHasAuthData || sServerBaseURL.IsEmpty())
	{
		return;
	}

	String const sURL(String::Printf("%s/v1/auth/refresh", sServerBaseURL.CStr()));
	InternalIssueRequest(sURL, SEOUL_BIND_DELEGATE(OnRefreshStatic));
}

void AuthManager::ResolveAuthConflict(Bool bAcceptPlatformDiscardDevice)
{
	// Lock data manipulations and state checking.
	{
		Lock lock(m_DataMutex);

		// Early out if in a state where conflict resolution may be
		// already pending.
		if (0 != m_PendingRequest ||
			PlatformSignInManager::Get()->IsSigningIn())
		{
			return;
		}

		// Early out if no conflict, but in this case,
		// reset conflict resolution.
		if (!m_pConflictData.IsValid())
		{
			m_bResolveConflict = false;
			return;
		}

		// If bAcceptPlatformDiscardDevice is true, just set
		// m_bResolveConflict.
		if (bAcceptPlatformDiscardDevice)
		{
			m_bResolveConflict = true;
		}
		// Otherwise, reset all conflict state and log out of
		// platform sign-in. We're re-trying a normal login
		// and leaving the device auth as-is.
		else
		{
			PlatformSignInManager::Get()->SignOut();
			m_bResolveConflict = false;
			m_pConflictData.Reset();
		}
	}

	// In either case, now issue a new login attempt.
	InternalLogin();
}

#if SEOUL_ENABLE_CHEATS
static const HString kXP("XP");
static const HString kLevel("Level");
static const HString kPlayer("Player");
static const HString kPlaytimeMicroseconds("TotalPlayTimeInMicroseconds");
static inline String GetFakeData(Int32 iLevel, Int32 iXP, Int32 iHours, Int32 iMinutes)
{
	DataStore dataStore;
	dataStore.MakeTable();

	auto node = dataStore.GetRootNode();
	dataStore.SetInt64ValueToTable(node, kPlaytimeMicroseconds, WorldTime::ConvertSecondsToInt64Microseconds((Double)((Int64)iHours * WorldTime::kHoursToSeconds + (Int64)iMinutes * WorldTime::kMinutesToSeconds)));
	dataStore.SetTableToTable(node, kPlayer);
	dataStore.GetValueFromTable(node, kPlayer, node);
	dataStore.SetInt32ValueToTable(node, kLevel, iLevel);
	dataStore.SetInt32ValueToTable(node, kXP, iXP);

	SaveLoadUtil::SaveFileMetadata metadata;
	metadata.m_iTransactionIdMin = 1;
	metadata.m_iTransactionIdMax = 1;
	metadata.m_iVersion = 1;
	metadata.m_sSessionGuid = UUID::GenerateV4().ToString();

	String sData;
	SaveLoadUtil::ToBase64(metadata, dataStore, sData);

	return sData;
}

void AuthManager::DevOnlyFakeAuthConflict()
{
	Lock lock(m_DataMutex);
	m_bResolveConflict = false;
	m_pConflictData.Reset(SEOUL_NEW(MemoryBudgets::Game) AuthConflictResolveData);
	m_pConflictData->m_DevicePlayer.m_CreatedAt = WorldTime::GetUTCTime();
	m_pConflictData->m_DevicePlayer.m_sData = GetFakeData(1, 32, 1, 1);
	m_pConflictData->m_DevicePlayer.m_sName = "Device Player";
	m_pConflictData->m_PlatformPlayer.m_CreatedAt = WorldTime::GetUTCTime();
	m_pConflictData->m_PlatformPlayer.m_sData = GetFakeData(69, 53500, 100, 59);
	m_pConflictData->m_PlatformPlayer.m_sName = "GPG Player";
}

void AuthManager::DevOnlyFakeRecommendedUpdate()
{
	m_bFakeRecommendedUpdate = true;
}

void AuthManager::DevOnlyToggleFakeRequiredUpdate()
{
	m_bFakeRequiredUpdate = !m_bFakeRequiredUpdate;
}

void AuthManager::FakeRequiredUpdateForAuthData(AuthData& rData) const
{
	// By setting the major version artificially high, we fake a required update
	rData.m_RefreshData.m_VersionRequired.m_iVersionMajor = 99999999;
}
#endif // /#if SEOUL_ENABLE_CHEATS

void AuthManager::CheckReturnToPatcher()
{
	// Get the current auth data. If this fails, we're done.
	AuthData data;
	if (!GetAuthData(data))
	{
		return;
	}

	// Check for auth differences from application state.
	// Any differences force a patch (game loaded becomes false).
	auto const bSoftReturnToPatcher = NeedsRestart(data.m_RefreshData);

	// These conditions not only return to patcher, but force a restart
	// of the patcher process if we're already running the patcher.

	// Also need to return to the patch if sign-in status has changed
	// with the platform sign-in manager.
	Bool bHardReturnToPatcher = (m_PlatformSignInManagerStateChangeCount != PlatformSignInManager::Get()->GetStateChangeCount());

	// Finally, need to return to the patcher if we have an auth conflict to resolve.
	bHardReturnToPatcher = bHardReturnToPatcher || HasAuthConflict();

	// If in a context where a restart is necessary, process it now.
	if (bHardReturnToPatcher || (bSoftReturnToPatcher && !Patcher::Get().IsValid()))
	{
		// If automation is active, yell about triggering a reboot. This should not
		// occur in normal automation scenarios.
#if !SEOUL_SHIP
		if (Automation::Get() &&
			Automation::Get()->GetSettings().m_bAutomatedTesting)
		{
			auto pMain = Main::Get();
			auto const& refresh = m_AuthData.m_RefreshData;
			SEOUL_WARN("GameAuthManager is triggering a soft reboot - this is unexpected. See log for relevant state. Possibly, a new patch was set on the server while the test was running.");
			SEOUL_LOG_AUTH("refresh.m_sVariationString: %s", refresh.m_sVariationString.CStr());
			SEOUL_LOG_AUTH("refresh.m_sConfigUpdateUrl: %s", refresh.m_sConfigUpdateUrl.CStr());
			SEOUL_LOG_AUTH("pMain->GetConfigUpdatePackageFileSystem()->GetURL(): %s", pMain->GetConfigUpdatePackageFileSystem().IsValid() ? pMain->GetConfigUpdatePackageFileSystem()->GetURL().CStr() : "<null");
			SEOUL_LOG_AUTH("refresh.m_sContentUpdateUrl: %s", refresh.m_sContentUpdateUrl.CStr());
			SEOUL_LOG_AUTH("pMain->GetContentUpdatePackageFileSystem()->GetURL(): %s", pMain->GetContentUpdatePackageFileSystem().IsValid() ? pMain->GetContentUpdatePackageFileSystem()->GetURL().CStr() : "<null");
			SEOUL_LOG_AUTH("m_VersionRequired.CheckCurrentBuild(): %s", refresh.m_VersionRequired.CheckCurrentBuild() ? "true" : "false");
			SEOUL_LOG_AUTH("m_VersionRequired.m_iChangelist: %d", refresh.m_VersionRequired.m_iChangelist);
			SEOUL_LOG_AUTH("m_VersionRequired.m_iVersionMajor: %d", refresh.m_VersionRequired.m_iVersionMajor);
			SEOUL_LOG_AUTH("m_PlatformSignInManagerStateChangeCount: %d", (Int)m_PlatformSignInManagerStateChangeCount);
			SEOUL_LOG_AUTH("PlatformSignInManager::Get()->GetStateChangeCount(): %d", (Int)PlatformSignInManager::Get()->GetStateChangeCount());
			SEOUL_LOG_AUTH("HasAuthConflict: %s", HasAuthConflict() ? "true" : "false");
			if (m_pConflictData.IsValid())
			{
				String s;
				(void)Reflection::SerializeToString(&m_pConflictData->m_DevicePlayer, s, true, 0, true);
				SEOUL_LOG_AUTH("m_pConflictData->m_DevicePlayer: %s", s.CStr());
				(void)Reflection::SerializeToString(&m_pConflictData->m_PlatformPlayer, s, true, 0, true);
				SEOUL_LOG_AUTH("m_pConflictData->m_PlatformPlayer: %s", s.CStr());
			}
		}
#endif // /#if !SEOUL_SHIP

		// Trigger a restart - will occur "eventually", but can be delayed
		// by UI state (passing true forces an immediate restart).
		UI::Manager::Get()->TriggerRestart(false);

		// If the patcher is already active, check if we need to force a restart of the patcher
		// when bHardReturnToPatcher is true.
		//
		// This is necessary if we have auth data and are not actively requesting auth data.
		if (bHardReturnToPatcher &&
			Patcher::Get().IsValid() &&
			HasAuthData() &&
			InternalCanLogin())
		{
			// If automation is active, yell about triggering a reboot. This should not
			// occur in normal automation scenarios.
#if !SEOUL_SHIP
			if (Automation::Get() &&
				Automation::Get()->GetSettings().m_bAutomatedTesting)
			{
				SEOUL_WARN("GameAuthManager is triggering a hard auth reset, this is not expected during automation testing.");
			}
#endif // /#if !SEOUL_SHIP

			Main_ResetAuth();
		}
	}
}

Bool AuthManager::InternalCanLogin() const
{
	// If a login is desired, wait for:
	// - pending requests.
	// - the platform sign-in manager.
	// - any pending conflict resolution.
	return (
		0 == m_PendingRequest &&
		!PlatformSignInManager::Get()->IsSigningIn() &&
		(!HasAuthConflict() || m_bResolveConflict));
}

void AuthManager::InternalLogin()
{
	if (!InternalCanLogin())
	{
		m_bPendingLogin = true;
		return;
	}

	// TODO: Decide on a better way to support games with no server.
	auto const sServerBaseURL(Main::Get()->GetServerBaseURL());
	if (sServerBaseURL.IsEmpty())
	{
		// Cache default auth data.
		Lock lock(m_DataMutex);
		m_AuthData = AuthData();

		// Now we have auth data, mark as such.
		SeoulMemoryBarrier();
		m_bHasAuthData = true;

		return;
	}

	auto const sURL(String::Printf("%s/v1/auth/login", sServerBaseURL.CStr()));

	// On successful request issue, track the state of platform
	// sign-in so we can trigger re-sign on changes.
	if (InternalIssueRequest(sURL, SEOUL_BIND_DELEGATE(OnLoginStatic)))
	{
		m_PlatformSignInManagerStateChangeCount = PlatformSignInManager::Get()->GetStateChangeCount();
	}
}

Bool AuthManager::InternalIssueRequest(
	const String& sURL,
	const HTTP::ResponseDelegate& callback)
{
	// Auth request already pending, or platform
	// manager is in the process of signing in, fail.
	if (PlatformSignInManager::Get()->IsSigningIn() ||
		0 != m_PendingRequest.CompareAndSet(1, 0))
	{
		return false;
	}

	// Create the request instance.
	auto& r = Client::Get()->CreateRequest(
		sURL,
		callback,
		HTTP::Method::ksPost,
		true);

	r.AddPostData("Platform", GetCurrentPlatformName());
	r.AddPostData("DeviceToken", Engine::Get()->GetPlatformUUID());
	if (m_bResolveConflict)
	{
		r.AddPostData("AllowOrphanDevicePlayer", ksTrue);
	}

	PlatformSignInManager::Get()->StartWithIdToken(r);
	return true;
}

namespace
{

/** Called when send of crash data to the server has been completed. */
template <CrashManager::SendCrashType eType>
HTTP::CallbackResult OnCrashSendComplete(HTTP::Result eResult, HTTP::Response* pResponse)
{
	// Success if a successful transmission and a 200 response code.
	auto const bSuccess = (HTTP::Result::kSuccess == eResult && pResponse->GetStatus() == (Int)HTTP::Status::kOK);

	// Invoke - CrashManager is a prereq of Engine and HTTP and will always exist.
	CrashManager::Get()->OnCrashSendComplete(eType, bSuccess);

	return HTTP::CallbackResult::kSuccess;
}

void SendCrash(CrashManager::SendCrashType eType, void* p, UInt32 u)
{
	String s;
	s.TakeOwnership(p, u);

	if (!Client::Get())
	{
		CrashManager::Get()->OnCrashSendComplete(eType, false);
		return;
	}

	auto const& sServerBaseURL(Main::Get()->GetServerBaseURL());
	if (sServerBaseURL.IsEmpty())
	{
		CrashManager::Get()->OnCrashSendComplete(eType, false);
		return;
	}

	HTTP::ResponseDelegate callback = (CrashManager::SendCrashType::Custom == eType
		? SEOUL_BIND_DELEGATE(OnCrashSendComplete<CrashManager::SendCrashType::Custom>)
		: SEOUL_BIND_DELEGATE(OnCrashSendComplete<CrashManager::SendCrashType::Native>));

	{
		auto const sURL(String::Printf("%s/v1/crash/upload", sServerBaseURL.CStr()));
		auto& r = Client::Get()->CreateRequest(
			sURL,
			callback,
			HTTP::Method::ksPost,
			true,
			false);
		r.AddPostData("raw", s);
		r.Start();
	}
}

Bool EncodeCrash(const ScopedMemoryBuffer& buf, String& rs)
{
	// Gzip compress the data.
	void* p = nullptr;
	UInt32 u = 0u;
	if (!GzipCompress(buf.Get(), buf.GetSize(), p, u, ZlibCompressionLevel::kBest, MemoryBudgets::Network))
	{
		return false;
	}

	// Base64 encode the data.
	rs.Assign(Base64Encode(p, u));

	// Release the temp buffer.
	MemoryManager::Deallocate(p);
	p = nullptr;
	u = 0u;

	return true;
}

void PrepCrash(CrashManager::SendCrashType eType, void* p, UInt32 u)
{
	String s;

	// Attempt encode.
	ScopedMemoryBuffer r(p, u);
	auto const b = EncodeCrash(r, s);
	r.Reset();

	// Fail on failure.
	if (!b)
	{
		CrashManager::Get()->OnCrashSendComplete(eType, false);
		return;
	}

	void* pOut = nullptr;
	UInt32 uOut = 0u;
	s.RelinquishBuffer(pOut, uOut);

	// TODO: Need a way to synchronize around HTTP sends
	// from other threads, that would eliminate the need to pass this back
	// to the mian thread.
	Jobs::AsyncFunction(
		GetMainThreadId(),
		&SendCrash,
		eType,
		pOut,
		uOut);
}

void SendCrashDelegate(CrashManager::SendCrashType eType, ScopedMemoryBuffer& r)
{
	// Acquire data.
	void* p = nullptr;
	UInt32 u = 0u;
	r.Swap(p, u);

	// Move to a worker thread if on the main thread.
	if (IsMainThread())
	{
		Jobs::AsyncFunction(
			&PrepCrash,
			eType,
			p, u);
	}
	// Otherwise, perform the processing and continue on.
	else
	{
		PrepCrash(eType, p, u);
	}
}

} // namespace anonymous

HTTP::CallbackResult AuthManager::OnLogin(HTTP::Result eResult, HTTP::Response* pResponse)
{
	// On success, update time server time sync.
	if (HTTP::Result::kSuccess == eResult &&
		(200 == pResponse->GetStatus() || kAuthConflict == pResponse->GetStatus()))
	{
		if (Client::Get())
		{
			Client::Get()->UpdateCurrentServerTimeFromResponse(pResponse);
		}
	}

	if (pResponse->GetStatus() != 200)
	{
		// Conflict resolution, start that now - we resolve the request immediately if deserialization
		// of the conflict data succeeds.
		if (pResponse->GetStatus() == kAuthConflict)
		{
			// Deserialize.
			ScopedPtr<AuthConflictResolveData> pData(SEOUL_NEW(MemoryBudgets::Game) AuthConflictResolveData);
			if (!Client::DeserializeResponseJSON(pResponse, pData.Get(), false))
			{
				SEOUL_WARN("[GameAuthManager]: Deserialize of auth response data for auth conflict in login failed: %s",
					(Byte const*)pResponse->GetBody());

				// Need to resend on deserialization failure.
				return HTTP::CallbackResult::kNeedsResend;
			}

			// No resends after this point; make sure we release pending request.
			Scoped scoped(m_PendingRequest);
			m_bPendingLogin = false;

			// Lock, swap in the conflict data.
			{
				Lock lock(m_DataMutex);
				m_bResolveConflict = false;
				m_pConflictData.Swap(pData);
			}

			// Done with the current request - a new request for
			// login must be issued with conflict resolution.
			return HTTP::CallbackResult::kSuccess;
		}
		else
		{
			return HTTP::CallbackResult::kNeedsResend;
		}
	}

	// Deserialize and verify the data.
	AuthData data;
	if (!Client::DeserializeResponseJSON(pResponse, &data, true))
	{
		SEOUL_WARN("[GameAuthManager]: Deserialize of auth response data for in login failed: %s",
			(Byte const*)pResponse->GetBody());

		return HTTP::CallbackResult::kNeedsResend;
	}

	// Auth token is required for everything, check if it's empty and if so,
	// force a retry.
	if (data.m_sAuthToken.IsEmpty())
	{
		SEOUL_WARN("[GameAuthManager]: AuthToken from the server is empty, retrying: %s", data.m_sAnalyticsGuid.CStr());
		return HTTP::CallbackResult::kNeedsResend;
	}

	// Fill in stats.
	data.m_RequestStats = pResponse->GetStats();

	// No resends after this point; make sure we release pending request.
	Scoped scoped(m_PendingRequest);
	m_bPendingLogin = false;

	// Return immediately if the result is cancelled - means we're shutting down.
	if (HTTP::Result::kCanceled == eResult)
	{
		return HTTP::CallbackResult::kSuccess;
	}

	// Cache the auth data.
	{
		Lock lock(m_DataMutex);
		m_AuthData = data;
		SEOUL_LOG("Analytics Guid: %s", data.m_sAnalyticsGuid.CStr());

		// If we get here, we're also no longer in conflict.
		m_pConflictData.Reset();
		m_bResolveConflict = false;
	}

	// Now we have auth data, mark as such.
	SeoulMemoryBarrier();
	m_bHasAuthData = true;

	// Commit data to the environment.
	Client::Get()->SetAuthToken(data.m_sAuthToken);
	CrashManager::Get()->SetSendCrashDelegate(SEOUL_BIND_DELEGATE(SendCrashDelegate));
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	Client::Get()->RequestRemoteNotificationsIfSilent();
#endif
	Analytics::SetAnalyticsSandboxed(data.m_RefreshData.m_bAnalyticsSandboxed);
	Analytics::SetAnalyticsABTests(data.m_RefreshData.m_tABTests);
	Analytics::SetAnalyticsUserID(data.m_sAnalyticsGuid);

	return HTTP::CallbackResult::kSuccess;
}


HTTP::CallbackResult AuthManager::OnRefresh(HTTP::Result eResult, HTTP::Response* pResponse)
{
	// On success, update time server time sync.
	if (HTTP::Result::kSuccess == eResult &&
		(200 == pResponse->GetStatus() || kAuthConflict == pResponse->GetStatus()))
	{
		if (Client::Get())
		{
			Client::Get()->UpdateCurrentServerTimeFromResponse(pResponse);
		}
	}

	if (pResponse->GetStatus() != 200)
	{
		return HTTP::CallbackResult::kNeedsResend;
	}

	// Deserialize and verify the data.
	AuthDataRefresh data;
	if (!Client::DeserializeResponseJSON(pResponse, &data, true))
	{
		return HTTP::CallbackResult::kNeedsResend;
	}

	// Fill in stats.
	data.m_RequestStats = pResponse->GetStats();

	// No resends after this point; make sure we release pending request.
	Scoped scoped(m_PendingRequest);
	if (m_bPendingLogin)
	{
		// We have a pending login, so the refresh data is about to be obsolete
		return HTTP::CallbackResult::kSuccess;
	}

	// Return immediately if the result is cancelled - means we're shutting down.
	if (HTTP::Result::kCanceled == eResult)
	{
		return HTTP::CallbackResult::kSuccess;
	}

	// Update the refresh data.
	{
		Lock lock(m_DataMutex);
		m_AuthData.m_RefreshData = data;
	}

	// Commit data to the environment.
	Analytics::SetAnalyticsSandboxed(data.m_bAnalyticsSandboxed);
	Analytics::SetAnalyticsABTests(data.m_tABTests);

	return HTTP::CallbackResult::kSuccess;
}

void AuthManager::Main_ResetAuth()
{
	{
		Lock lock(m_DataMutex);

		m_bHasAuthData = false;
		m_AuthData = AuthData();
	}

	InternalLogin();
}

} // namespace Seoul::Game
