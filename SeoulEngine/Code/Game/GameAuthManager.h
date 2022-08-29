/**
 * \file GameAuthManager.h
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

#pragma once
#ifndef GAME_AUTH_MANAGER_H
#define GAME_AUTH_MANAGER_H

#include "Atomic32.h"
#include "Delegate.h"
#include "GameAuthData.h"
#include "HTTPCommon.h"
#include "Mutex.h"
#include "ScopedPtr.h"
#include "SeoulTime.h"
#include "Singleton.h"
namespace Seoul { namespace Game { struct AuthConflictResolveData; } }

namespace Seoul::Game
{

/**
 * Handles startup and midgame auth and download of
 * updateable *.sar files.
 */
class AuthManager SEOUL_SEALED : public Singleton<AuthManager>
{
public:
	AuthManager();
	~AuthManager();

	// Perform per-frame update operations on the main thread.
	void Update();

	// Get the most recent auth data from the server.
	Bool GetAuthData(AuthData& rData) const;

	/** @return true if auth data has been received from the server, false otherwise. */
	Bool HasAuthData() const
	{
		return m_bHasAuthData;
	}

	/**
	 * Retrieve a snapshot of current conflict data, if conflict data is defined.
	 */
	Bool GetAuthConflictData(AuthConflictResolveData& rData) const;

	/**
	 * When a platform ID and a device ID create an auth conflict that requires
	 * user action, conflict data will be populated. This conflict must be
	 * resolved before auth will be re-attempted and completed.
	 */
	Bool HasAuthConflict() const
	{
		Lock lock(m_DataMutex);
		return m_pConflictData.IsValid();
	}

	/**
	 * @return true if there is an outstanding server request (either initial login or auth refresh).
	 */
	Bool IsRequestPending() const
	{
		Atomic32Type const v = m_PendingRequest;
		return (0 != v);
	}

	// For cases where new refresh data has been received
	// outside the auth manager's normal handling. This method
	// returns true if the new refresh data will trigger
	// a soft reboot/return to the patcher.
	Bool ManulUpdateRefreshData(const AuthDataRefresh& refreshData);

	// Ask for the server to refresh any state that can change midsession (data URLs, A/B testing, or required version).
	void Refresh();

	// Tell the auth server to resolve an auth conflict - either
	// we accept the conflict, which tells the server to force
	// the platform auth method and orphan any auth that may be
	// associated with the current device, or reject it, in which
	// case we're just triggering a new login attempt with no
	// conflict resolution. In the latter case, the Game::AuthManager
	// assumes that the environment has changed such that conflict
	// resolution will now succeed (e.g. the user has signed out of
	// local platform sign-in).
	void ResolveAuthConflict(Bool bAcceptPlatformDiscardDevice);

#if SEOUL_ENABLE_CHEATS
	// Developer only, generates a fake auth conflict
	// for testing purposes.
	void DevOnlyFakeAuthConflict();
	void DevOnlyFakeRecommendedUpdate();
	void DevOnlyToggleFakeRequiredUpdate();
	void FakeRequiredUpdateForAuthData(AuthData& rData) const;

	// Sets an override remap config that is used
	// in place of any server defined config.
	AuthDataRefresh::RemapConfigs DevOnlyGetRemapConfigOverride() const
	{
		Lock lock(m_DataMutex);
		return m_vRemapConfigsOverride;
	}

	void DevOnlySetRemapConfigOverride(const AuthDataRefresh::RemapConfigs& vRemapConfigs)
	{
		Lock lock(m_DataMutex);
		m_vRemapConfigsOverride = vRemapConfigs;
	}
#endif // /#if SEOUL_ENABLE_CHEATS

private:
	Mutex m_DataMutex{};
	ScopedPtr<AuthConflictResolveData> m_pConflictData{};
	AuthData m_AuthData{};
	Atomic32 m_PendingRequest{};
	Atomic32Type m_PlatformSignInManagerStateChangeCount{};
	WorldTime m_NextAuthRefresh{};
	Atomic32Value<Bool> m_bHasAuthData{ false };
	Atomic32Value<Bool> m_bPendingLogin{ false };
	Atomic32Value<Bool> m_bResolveConflict{ false };
#if SEOUL_ENABLE_CHEATS
	AuthDataRefresh::RemapConfigs m_vRemapConfigsOverride;
	Atomic32Value<Bool> m_bFakeRecommendedUpdate{ false };
	Atomic32Value<Bool> m_bFakeRequiredUpdate{ false };
#endif // /#if SEOUL_ENABLE_CHEATS
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	String m_LastRemoteNotificationToken{};
#endif // /#if SEOUL_WITH_REMOTE_NOTIFICATIONS

	void CheckReturnToPatcher();
	Bool InternalCanLogin() const;
	void InternalLogin();
	Bool InternalIssueRequest(
		const String& sURL,
		const HTTP::ResponseDelegate& callback);

	static WorldTime MakeNextAuthRefreshTime();

	static HTTP::CallbackResult OnLoginStatic(HTTP::Result eResult, HTTP::Response* pResponse)
	{
		auto pGameAuthManager = Get();
		if (pGameAuthManager)
		{
			return pGameAuthManager->OnLogin(eResult, pResponse);
		}

		return HTTP::CallbackResult::kSuccess;
	}
	HTTP::CallbackResult OnLogin(HTTP::Result eResult, HTTP::Response* pResponse);

	static HTTP::CallbackResult OnRefreshStatic(HTTP::Result eResult, HTTP::Response* pResponse)
	{
		auto pGameAuthManager = Get();
		if (pGameAuthManager)
		{
			return pGameAuthManager->OnRefresh(eResult, pResponse);
		}

		return HTTP::CallbackResult::kSuccess;
	}
	HTTP::CallbackResult OnRefresh(HTTP::Result eResult, HTTP::Response* pResponse);

	// For Game::Main to trigger a reset during the patch.
	friend class Main;
	void Main_ResetAuth();

	SEOUL_DISABLE_COPY(AuthManager);
}; // class Game::AuthManager

} // namespace Seoul::Game

#endif // include guard
