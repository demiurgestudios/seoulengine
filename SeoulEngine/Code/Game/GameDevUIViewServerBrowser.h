/**
 * \file GameDevUIViewServerBrowser.h
 * \brief A developer UI view component for displaying Demiplane server status,
 * small-group development servers.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_DEV_UI_VIEW_SERVER_BROWSER_H
#define GAME_DEV_UI_VIEW_SERVER_BROWSER_H

#include "BuildFeatures.h"
#include "DelegateMemberBindHandle.h"
#include "DevUIView.h"
#include "HTTPManager.h"
#include "ReflectionWeakAny.h"
#include "SeoulTime.h"
#include "Vector.h"

#if (SEOUL_WITH_SERVER_BROWSER && SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul::Game
{

/**
 * Server representation of a Demiplane's config (mostly/entirely set at creation time)
 */
struct DemiplaneConfig
{
	DemiplaneConfig();

	String m_sName;
	String m_sOwner;
	WorldTime m_ClaimedAt;
};

/**
 * Server representation of a Demiplane
 */
struct Demiplane
{
	Demiplane();

	// Get a server base URL built from the demiplane's hostname
	String GetServerBaseURL() const;

	String m_sHost;
	WorldTime m_LastActiveAt;
	WorldTime m_GameDataPushedAt;
	DemiplaneConfig m_Config;
};

/**
 * API result struct for /v1/demiplane/claim
 */
struct DemiplaneClaimResult
{
	DemiplaneClaimResult()
		: m_Plane()
		, m_vAvailable()
		, m_vClaimed()
	{
	}

	Demiplane m_Plane;
	Vector<Demiplane> m_vAvailable;
	Vector<Demiplane> m_vClaimed;
};

/**
 * API result struct for /v1/demiplane/list
 */
struct DemiplaneListResult
{
	DemiplaneListResult()
		: m_vAvailable()
		, m_vClaimed()
	{
	}
	Vector<Demiplane> m_vAvailable;
	Vector<Demiplane> m_vClaimed;
};

/**
 * Dev UI for switching between demiplane/standard servers
 */
class DevUIViewServerBrowser SEOUL_SEALED : public DevUI::View
{
public:
	SEOUL_DELEGATE_TARGET(DevUIViewServerBrowser);

	/**
	 * State for the server browser UI. Helps avoid callbacks arriving at unexpected times.
	 */
	enum class ServerBrowserState
	{
		kInitial,			// Before initial load of server state
		kNotAvailable,		// No API key
		kRefreshing,		// Between QueryDemiplaneList and OnQueryDemiplaneList
		kClaiming,			// Between ClaimPlane and OnClaimPlane
		kCompressingConfig,	// Between PushConfigData and OnPushConfigData
		kPushingConfig,		// Between PushConfigData and OnPushConfigData
		kClearingConfig,	// Between ClearPushedConfigData and OnClearPushedConfigData
		kReady,				// No pending requests
	};

public:
	DevUIViewServerBrowser();
	~DevUIViewServerBrowser();

	// DevUIView overrides
	virtual HString GetId() const SEOUL_OVERRIDE
	{
		static const HString kId("Server Browser");
		return kId;
	}

protected:
	virtual void DoPrePose(DevUI::Controller& rController, RenderPass& rPass) SEOUL_OVERRIDE;
	virtual Vector2D GetInitialSize() const SEOUL_OVERRIDE
	{
		return Vector2D(640, 275);
	}
	// /DevUIView overrides

private:
	// Helper for deserializing HTTP responses
	static Bool DeserializeResponseJSON(HTTP::Result eResult, HTTP::Response const * const pResponse, const Reflection::WeakAny& outObject);

private:
	void SetState(DevUIViewServerBrowser::ServerBrowserState eState)
	{
		m_State = eState;
		m_StateChangedAt = WorldTime::GetUTCTime();
	}

	// Should the "claim" button be clickable?
	Bool CanClaimNewPlane() const;
	// Should the "change" button for existing planes be clickable?
	Bool CanChangePlane() const;
	// Should the "push config" button be clickable?
	Bool CanPushConfigData() const;

	// Add Demiplane auth header to a request
	Bool AuthorizeRequest(Seoul::HTTP::Request& r);

	void ReadDemiplaneList(HTTP::Result eResult, HTTP::Response* pResponse);

	// Refresh the list of planes (claimed and unclaimed)
	void QueryDemiplaneList();
	// Response handler for refreshing the list of planes
	HTTP::CallbackResult OnQueryDemiplaneList(HTTP::Result, HTTP::Response*);

	// Claim a specific demiplane instance
	void ClaimPlane(const Demiplane& plane, const String& sName, const String& sUsername);
	// Response handler for claiming a specific demiplane instance
	HTTP::CallbackResult OnClaimPlane(HTTP::Result, HTTP::Response*);

	// Clear custom config data from a demiplane instance
	void ClearPushedConfigData();
	// Response handler for clearing config data
	HTTP::CallbackResult OnClearPushedConfigData(HTTP::Result, HTTP::Response*);

	// Push config data to the server (passes work to another thread)
	void PushConfigData();
	// Push config data to the server (actual work, to be submitted to another thread)
	void InternalPushConfigData();
	// Response handler for pushing config data to a demiplane
	HTTP::CallbackResult OnPushConfigData(HTTP::Result, HTTP::Response*);

	// Open the browser admin UI for the given ServerBaseURL
	void OpenAdmin(const String& sBaseUrl, Bool bAttemptAutoLogin);
	// Disconnect from the current plane and return to the base server (QA/Staging). Resets the game to the patcher.
	void DisconnectFromPlane();
	// Connect to a demiplane. Resets the game to the patcher.
	void UsePlane(const Demiplane& plane);
	// Claims a random demiplane from the "available" list. On success, resets the game to the patcher.
	void ClaimRandomPlane(const String& sName);
	// Shared helper for resetting your client hostname and restarting to the patcher.
	void ChangeHostname(const String& sHostname);

private:
	/** UI state; determines what to show and whether you're allowed to make HTTP requests. */
	ServerBrowserState m_State;
	WorldTime m_StateChangedAt;
	/** List of demiplanes available to occupy. */
	Vector<Demiplane> m_vAvailablePlanes;
	/** List of demiplanes already set up by someone else (available to join). */
	Vector<Demiplane> m_vClaimedPlanes;

	/** Temporary buffer for "new plane" input text */
	String m_sNewPlaneName;
	/** Information about the currently active demiplane (if any). */
	Demiplane m_CurrentPlane;

	SEOUL_DISABLE_COPY(DevUIViewServerBrowser);
}; // class Game::DevUIViewServerBrowser

} // namespace Seoul::Game

#endif // include guard

#endif // /(SEOUL_WITH_SERVER_BROWSER && SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)
