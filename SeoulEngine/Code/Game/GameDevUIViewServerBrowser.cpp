/**
 * \file GameDevUIViewServerBrowser.cpp
 * \brief A developer UI view component for displaying Demiplane server status,
 * small-group development servers.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "ConfigZipBuilder.h"
#include "JobsFunction.h"
#include "Delegate.h"
#include "DevUIRoot.h"
#include "DevUIUtil.h"
#include "Engine.h"
#include "GameClientSettings.h"
#include "GameDevUIRoot.h"
#include "GameDevUIViewServerBrowser.h"
#include "GameMain.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "ReflectionDataStoreTableUtil.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionPrereqs.h"
#include "ReflectionDefine.h"
#include "ScopedAction.h"
#include "SeoulMath.h"
#include "SeoulOS.h"
#include "UIManager.h"

#include <imgui.h>

#if (SEOUL_WITH_SERVER_BROWSER && SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)

namespace Seoul
{

namespace Game
{

DemiplaneConfig::DemiplaneConfig()
	: m_sName()
	, m_sOwner()
	, m_ClaimedAt()
{
}

Demiplane::Demiplane()
	: m_sHost()
	, m_LastActiveAt()
	, m_GameDataPushedAt()
	, m_Config()
{
}

/** Get a server base URL built from the demiplane's hostname */
String Demiplane::GetServerBaseURL() const
{
	// Demiplanes are serving HTTP directly to an instance -- no ELB to add SSL.
	// The planes are locked to the office IP.
	return String::Printf("http://%s", m_sHost.CStr());
}

} // namespace Game

SEOUL_BEGIN_TYPE(Game::DevUIViewServerBrowser, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(DisplayName, "Server Browser")
	SEOUL_PARENT(DevUI::View)
SEOUL_END_TYPE()

SEOUL_SPEC_TEMPLATE_TYPE(Vector<Demiplane, 48>)
SEOUL_BEGIN_TYPE(Game::Demiplane)
	SEOUL_PROPERTY_N("Host", m_sHost)
	SEOUL_PROPERTY_N("LastActiveAt", m_LastActiveAt)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("GameDataPushedAt", m_GameDataPushedAt)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("Config", m_Config)
		SEOUL_ATTRIBUTE(NotRequired)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Game::DemiplaneConfig)
	SEOUL_PROPERTY_N("Name", m_sName)
	SEOUL_PROPERTY_N("Owner", m_sOwner)
	SEOUL_PROPERTY_N("ClaimedAt", m_ClaimedAt)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Game::DemiplaneClaimResult)
	SEOUL_PROPERTY_N("Plane", m_Plane)
	SEOUL_PROPERTY_N("Available", m_vAvailable)
	SEOUL_PROPERTY_N("Claimed", m_vClaimed)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Game::DemiplaneListResult)
	SEOUL_PROPERTY_N("Available", m_vAvailable)
	SEOUL_PROPERTY_N("Claimed", m_vClaimed)
SEOUL_END_TYPE()

namespace Game
{

DevUIViewServerBrowser::DevUIViewServerBrowser()
	: m_State(ServerBrowserState::kInitial)
	, m_StateChangedAt(WorldTime::GetUTCTime())
	, m_vAvailablePlanes()
	, m_vClaimedPlanes()
{
}

DevUIViewServerBrowser::~DevUIViewServerBrowser()
{
}

String ReadableDateString(const WorldTime& time)
{
	auto deltaTime = WorldTime::GetUTCTime().SubtractWorldTime(time);
	auto deltaSeconds = deltaTime.GetSeconds();
	if (deltaSeconds < 2 * 60)
	{
		return "a minute ago";
	}
	else if (deltaSeconds < 60 * 60)
	{
		return String::Printf("%d minutes ago", static_cast<Int>(deltaSeconds / 60));
	}
	else if (deltaSeconds < 24 * 60 * 60)
	{
		return String::Printf("%d hours ago", static_cast<Int>(deltaSeconds / (60*60)));
	}
	else if (deltaSeconds < 48 * 60 * 60)
	{
		return "1 day ago";
	}
	else
	{
		auto days = deltaSeconds / (24 * 60 * 60);
		if (days > 999)
		{
			days = 999;
		}
		return String::Printf("%d days ago", static_cast<Int>(days));
	}
}

void DevUIViewServerBrowser::DoPrePose(DevUI::Controller& rController, RenderPass& rPass)
{
	using namespace DevUI::Util;
	using namespace ImGui;

	auto pGameMain = Main::Get();
	if (!pGameMain)
	{
		Text("Initializing...");
		return;
	}

	if (m_State == ServerBrowserState::kInitial)
	{
		// Starts initial refresh, and updates state to kRefreshing
		QueryDemiplaneList();
	}
	if (m_State == ServerBrowserState::kNotAvailable)
	{
		Text("Not available");
		return;
	}

	HString sServerType; // QA, Staging, ...
	SEOUL_VERIFY(
		EnumOf<Game::ServerType>().TryGetName(ClientSettings::GetServerType(), sServerType));

	// Block 1: Current connection status
	Bool bOnPlane = !m_CurrentPlane.m_sHost.IsEmpty();
	if (bOnPlane)
	{
		Text("Current server:");
		SameLine();
		Text("%s", m_CurrentPlane.m_Config.m_sName.CStr());

		Text("Owned by:");
		SameLine();
		{
			ImVec4 ownerColor(1, 1, 0, 1);
			if (m_CurrentPlane.m_Config.m_sOwner == GetUsername())
			{
				ownerColor = ImVec4(1, 1, 1, 1);
			}
			TextColored(ownerColor, "%s", m_CurrentPlane.m_Config.m_sOwner.CStr());
		}

		// Paranoia: check for desync between Game::Main and this UI
		if (m_CurrentPlane.GetServerBaseURL() != pGameMain->GetServerBaseURL())
		{
			TextColored(
				ImVec4(1, 0, 0, 1),
				"DNS does not match: %s",
				pGameMain->GetServerBaseURL().CStr());
		}
	}
	else if (m_State == ServerBrowserState::kClaiming)
	{
		TextColored(ImVec4(0, 1, .25, 1), "Creating plane...");
	}
	else
	{
		Text("Current server: %s", sServerType.CStr());
	}

	Separator();
	if (bOnPlane)
	{
		if (!m_CurrentPlane.m_GameDataPushedAt.IsZero())
		{
			{
				auto dt = WorldTime::GetUTCTime() - m_CurrentPlane.m_GameDataPushedAt;
				Int64 const oneDayInSeconds = 60 * 60 * 24;
				Int64 const threeDaysInSeconds = oneDayInSeconds * 3;
				ImVec4 color(1, 1, 0.5f, 1);
				if (dt.GetSeconds() > threeDaysInSeconds)
				{
					color = ImVec4(1, 0.15f, 0.15f, 1);
				}
				else if (dt.GetSeconds() > oneDayInSeconds)
				{
					color = ImVec4(1, 1, 0.25f, 1);
				}
				TextColored(color, "Using config data override from %s", ReadableDateString(m_CurrentPlane.m_GameDataPushedAt).CStr());
			}

			if (Button("Clear Pushed Config", ImVec2(), CanPushConfigData()))
			{
				ClearPushedConfigData();
			}
		}
	}
#if SEOUL_PLATFORM_WINDOWS
	if (bOnPlane && !m_CurrentPlane.m_GameDataPushedAt.IsZero())
	{
		SameLine();
	}
	if (Button("Push Config Data", ImVec2(), CanPushConfigData()))
	{
		PushConfigData();
	}
	if (m_State == ServerBrowserState::kCompressingConfig || m_State == ServerBrowserState::kPushingConfig)
	{
		SameLine();
		auto msg = (m_State == ServerBrowserState::kCompressingConfig) ? "Compressing config (%.1fs)..." : "Pushing config (%.1fs)...";
		TextColored(ImVec4(0, 1, 1, 1), msg, (WorldTime::GetUTCTime() - m_StateChangedAt).GetSecondsAsDouble());
	}
#endif

	if (Button("Open Admin"))
	{
		OpenAdmin(pGameMain->GetServerBaseURL(), bOnPlane);
	}

	// Block 2: Create a new plane/return to QA
	if (m_State == ServerBrowserState::kRefreshing)
	{
		Text("(refreshing...)");
		return;
	}

	if (bOnPlane)
	{
		SameLine();
		if (Button(String::Printf("Return to %s", sServerType.CStr()).CStr(), ImVec2(), CanChangePlane()))
		{
			DisconnectFromPlane();
		}
	}
	else if (m_State != ServerBrowserState::kClaiming)
	{
		Separator();
#if SEOUL_PLATFORM_WINDOWS
		if (m_vAvailablePlanes.GetSize() == 0)
		{
			TextColored(ImVec4(1, 0, 0, 1), "No open slots. Please wait for a new slot (about 10 minutes)");
			TextColored(ImVec4(1, 0, 0, 1), "or ask Engineering if they can make one available.");
		}
		else
		{
			Text("New plane name: (%d open slots)", m_vAvailablePlanes.GetSize());
		}

		Vector<Byte, MemoryBudgets::DevUI> vNameBuffer(Max(151u, m_sNewPlaneName.GetSize()), '\0');
		memcpy(vNameBuffer.Data(), m_sNewPlaneName.CStr(), m_sNewPlaneName.GetSize());
		vNameBuffer[m_sNewPlaneName.GetSize()] = '\0';
		if (ImGui::InputText("##PlaneName", vNameBuffer.Data(), vNameBuffer.GetSize()))
		{
			m_sNewPlaneName.Assign(vNameBuffer.Data());
		}
		SameLine();
		if (Button("New Plane", ImVec2(), CanClaimNewPlane()))
		{
			ClaimRandomPlane(m_sNewPlaneName);
			m_sNewPlaneName.Clear();
		}
#else
		Text("New planes can only be created from a Windows build.");
		Text("You can join an existing plane below:");
#endif
	}
	Separator();

	// Block 3: Already-created planes from other people
	if (TreeNode("AvailablePlanes", "Active Planes (%d)", m_vClaimedPlanes.GetSize()))
	{
		Columns(4);
		Text("Plane"); NextColumn();
		Text("Owner"); NextColumn();
		Text("Actions"); NextColumn();
		Text("Last Active"); NextColumn();
		Separator();

		for (auto const &plane : m_vClaimedPlanes)
		{
			// Add a unique id scope so repeated Button("Go")s all work from here to PopID()
			PushID(plane.m_sHost.CStr());

			Bool bIsCurrent = (plane.m_sHost == m_CurrentPlane.m_sHost);

			if (bIsCurrent)
			{
				TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", plane.m_Config.m_sName.CStr());

			}
			else
			{
				Text("%s", plane.m_Config.m_sName.CStr());
			}
			NextColumn();

			ImVec4 ownerColor(1, 1, 1, 1);
			if (bIsCurrent)
			{
				ownerColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
			}
			else if (plane.m_Config.m_sOwner == GetUsername())
			{
				ownerColor = ImVec4(0, 1, 0, 1);
			}
			TextColored(ownerColor, "%s", plane.m_Config.m_sOwner.CStr());
			NextColumn();

			if (!bIsCurrent)
			{
				if (Button("Go", ImVec2(), CanChangePlane()))
				{
					UsePlane(plane);
				}
				SameLine();
			}
			if (Button("Admin"))
			{
				OpenAdmin(plane.GetServerBaseURL(), true);
			}
			NextColumn();

			Text("%s", ReadableDateString(plane.m_LastActiveAt).CStr());
			NextColumn();

			PopID();
		}

		ImGui::Columns();

		Separator();
		TreePop();
	}

	if (Button("Refresh List", ImVec2(), m_State == ServerBrowserState::kReady))
	{
		QueryDemiplaneList();
	}
}

void DevUIViewServerBrowser::ClearPushedConfigData()
{
	if (m_State != ServerBrowserState::kReady)
	{
		SEOUL_LOG("Not clearing plane config data: wrong state");
		return;
	}

	auto pGameMain = Main::Get();
	if (!pGameMain)
	{
		SEOUL_LOG("Not clearing plane config data: No GameMain");
		return;
	}

	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetMethod(HTTP::Method::ksPost);
	r.SetURL(String::Printf("%s/v1/demiplane/clear_config", pGameMain->GetServerBaseURL().CStr()));
	// TODO: If the game shuts down with a pending request, it could crash
	r.SetCallback(SEOUL_BIND_DELEGATE(&DevUIViewServerBrowser::OnClearPushedConfigData, this));
	r.SetResendOnFailure(false);
	if (!AuthorizeRequest(r))
	{
		return;
	}

	SetState(ServerBrowserState::kClearingConfig);
	r.Start();
}

HTTP::CallbackResult DevUIViewServerBrowser::OnClearPushedConfigData(HTTP::Result eResult, HTTP::Response* pResponse)
{
	if (m_State == ServerBrowserState::kClearingConfig)
	{
		SetState(ServerBrowserState::kReady);
	}

	ReadDemiplaneList(eResult, pResponse);

	SEOUL_LOG("OnPushConfigData: eResult=%d; status=%d", eResult, pResponse->GetStatus());
	return HTTP::CallbackResult::kSuccess;
}


void DevUIViewServerBrowser::PushConfigData()
{
	if (m_State != ServerBrowserState::kReady)
	{
		SEOUL_LOG("Not pushing config data: wrong state");
		return;
	}

	SetState(ServerBrowserState::kCompressingConfig);
	Jobs::AsyncFunction(SEOUL_BIND_DELEGATE(&DevUIViewServerBrowser::InternalPushConfigData, this));
}

void DevUIViewServerBrowser::InternalPushConfigData()
{
	auto pGameMain = Main::Get();
	if (!pGameMain)
	{
		SEOUL_LOG("Not pushing config data: No GameMain");
		SetState(ServerBrowserState::kReady);
		return;
	}

	// Can only push config to demiplanes or local servers
	if (m_CurrentPlane.m_sHost.IsEmpty() && ClientSettings::GetServerType() != ServerType::kLocal)
	{
		SEOUL_LOG("Not pushing config data: only supported on Demiplanes and local VMs");
		SetState(ServerBrowserState::kReady);
		return;
	}

	// Create the request object before building the .zip, in case we exit early
	SEOUL_LOG("Pushing config zip...");
	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetMethod(HTTP::Method::ksPost);
	r.SetURL(String::Printf("%s/v1/demiplane/config", pGameMain->GetServerBaseURL().CStr()));
	// TODO: If the game shuts down with a pending request, it could crash
	r.SetCallback(SEOUL_BIND_DELEGATE(&DevUIViewServerBrowser::OnPushConfigData, this));
	r.SetResendOnFailure(false);
	if (!AuthorizeRequest(r))
	{
		SetState(ServerBrowserState::kReady);
		return;
	}
	r.AddHeader("Content-Type", "application/zlib");

	// Build the zlib-compressed bytes, and write that as the POST request body
	void* pCompressed(nullptr);
	UInt32 uCompressedSizeInBytes(0);
	auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(pCompressed); }));

	{
		MemorySyncFile configZip;
		if (!ConfigZipBuilder::WriteAllJson(configZip))
		{
			SEOUL_WARN("Can't collect config data to push");
			SetState(ServerBrowserState::kReady);
			return;
		}
		auto& buffer = configZip.GetBuffer();
		if (!ZlibCompress(buffer.GetBuffer(), buffer.GetTotalDataSizeInBytes(), pCompressed,
						  uCompressedSizeInBytes, ZlibCompressionLevel::kFast))
		{
			SEOUL_WARN("Can't compress config data to push");
			SetState(ServerBrowserState::kReady);
			return;
		}
	}

	SEOUL_LOG("Uploading %" PRIu32 " bytes of config zip", uCompressedSizeInBytes);
	r.AddHeader("Content-Length", String::Printf("%" PRIu32, uCompressedSizeInBytes));
	auto& postBody = r.AcquirePostBody();
	postBody.Write(pCompressed, uCompressedSizeInBytes);
	SetState(ServerBrowserState::kPushingConfig);
	r.Start();
}

HTTP::CallbackResult DevUIViewServerBrowser::OnPushConfigData(HTTP::Result eResult, HTTP::Response* pResponse)
{
	if (m_State == ServerBrowserState::kPushingConfig)
	{
		SetState(ServerBrowserState::kReady);
	}

	if (eResult != HTTP::Result::kSuccess || pResponse->GetStatus() != 200)
	{
		SEOUL_WARN("OnPushConfigData failed: eResult=%d; status=%d", eResult, (nullptr != pResponse) ? pResponse->GetStatus() : 0);
	}
	else
	{
		SEOUL_LOG("OnPushConfigData: eResult=%d; status=%d", eResult, pResponse->GetStatus());
	}

	ReadDemiplaneList(eResult, pResponse);

	return HTTP::CallbackResult::kSuccess;
}


String GetDemiplaneApiKey()
{
	String sDemiplaneApiKey;
	auto p(ClientSettings::Load());
	if (p.IsValid())
	{
		DataStoreTableUtil defaultSection(*p, p->GetRootNode(), HString());
		static HString sDemiplaneApiKeyId("DemiplaneApiKey");
		(void)defaultSection.GetValue(sDemiplaneApiKeyId, sDemiplaneApiKey);
	}

	return sDemiplaneApiKey;
}


// HTTP API
Bool DevUIViewServerBrowser::DeserializeResponseJSON(HTTP::Result eResult, HTTP::Response const * const pResponse, const Reflection::WeakAny& outObject)
{
	if (eResult != HTTP::Result::kSuccess
		|| pResponse == nullptr
		|| pResponse->GetStatus() != (Int)HTTP::Status::kOK
		|| pResponse->GetBodySize() == 0)
	{
		return false;
	}

	DataStore dataStore;
	if (!DataStoreParser::FromString((const Byte*)pResponse->GetBody(), pResponse->GetBodySize(), dataStore))
	{
		return false;
	}

	Reflection::DefaultSerializeContext context(ContentKey(), dataStore, dataStore.GetRootNode(), outObject.GetTypeInfo());
	return DeserializeObject(context, dataStore, dataStore.GetRootNode(), outObject);
}

/**
 * Add Demiplane auth header to a request
 *
 * @return Whether the Demiplane API key could be added to the request
 */
Bool DevUIViewServerBrowser::AuthorizeRequest(Seoul::HTTP::Request& r)
{
	String sApiKey(GetDemiplaneApiKey());
	if (sApiKey.IsEmpty())
	{
		SEOUL_LOG("No Demiplane API key");
		SetState(ServerBrowserState::kNotAvailable);
		return false;
	}

	static const String sAuthorizationHeader("Authorization");
	r.AddHeader(sAuthorizationHeader, String::Printf("Bearer %s", sApiKey.CStr()));
	return true;
}

/**
 * Refresh the list of planes (claimed and unclaimed)
 */
void DevUIViewServerBrowser::QueryDemiplaneList()
{
	if (m_State == ServerBrowserState::kRefreshing)
	{
		SEOUL_LOG("Not querying demiplane list: wrong state");
		return;
	}

	auto pGameMain = Main::Get();
	if (!pGameMain)
	{
		SEOUL_LOG("Not querying demiplane list: No GameMain");
		return;
	}

	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetMethod(HTTP::Method::ksGet);
	r.SetURL(String::Printf("%s/v1/demiplane/list", pGameMain->GetServerBaseURL().CStr()));
	// TODO: If the game shuts down with a pending request, it could crash
	r.SetCallback(SEOUL_BIND_DELEGATE(&DevUIViewServerBrowser::OnQueryDemiplaneList, this));
	r.SetResendOnFailure(false);
	if (!AuthorizeRequest(r))
	{
		return;
	}
	r.Start();

	SetState(ServerBrowserState::kRefreshing);
}

/**
 * Response handler for refreshing the list of planes
 */
HTTP::CallbackResult DevUIViewServerBrowser::OnQueryDemiplaneList(HTTP::Result eResult, HTTP::Response* pResponse)
{
	if (m_State == ServerBrowserState::kRefreshing)
	{
		SetState(ServerBrowserState::kReady);
	}

	ReadDemiplaneList(eResult, pResponse);

	return HTTP::CallbackResult::kSuccess;
}

void DevUIViewServerBrowser::ReadDemiplaneList(HTTP::Result eResult, HTTP::Response* pResponse)
{
	DemiplaneListResult result;
	if (!DeserializeResponseJSON(eResult, pResponse, &result))
	{
		SEOUL_WARN("Couldn't parse demiplane response");
	}

	m_vAvailablePlanes.Swap(result.m_vAvailable);
	m_vClaimedPlanes.Swap(result.m_vClaimed);

	// Update the current plane's info if possible
	if (!m_CurrentPlane.m_sHost.IsEmpty())
	{
		for (auto const& plane : m_vClaimedPlanes)
		{
			if (plane.m_sHost == m_CurrentPlane.m_sHost)
			{
				m_CurrentPlane = plane;
				break;
			}
		}
	}
}


/**
 * Claim a specific demiplane instance
 */
void DevUIViewServerBrowser::ClaimPlane(const Demiplane& plane, const String& sName, const String& sUsername)
{
	if (m_State != ServerBrowserState::kReady)
	{
		SEOUL_LOG("Not claiming demiplane: not in Ready state");
		return;
	}

	auto pGameMain = Main::Get();
	if (!pGameMain)
	{
		SEOUL_LOG("Not claiming demiplane: No GameMain");
		return;
	}

	auto& r = HTTP::Manager::Get()->CreateRequest();
	r.SetMethod(HTTP::Method::ksPost);
	r.SetURL(String::Printf("%s/v1/demiplane/claim", plane.GetServerBaseURL().CStr()));
	// TODO: If the game shuts down with a pending request, it could crash
	r.SetCallback(SEOUL_BIND_DELEGATE(&DevUIViewServerBrowser::OnClaimPlane, this));
	r.SetResendOnFailure(false);
	if (!AuthorizeRequest(r))
	{
		return;
	}

	r.AddPostData("Name", sName);
	r.AddPostData("Owner", sUsername);

	r.Start();

	SetState(ServerBrowserState::kClaiming);
}

/**
 * Response handler for claiming a specific demiplane instance
 */
HTTP::CallbackResult DevUIViewServerBrowser::OnClaimPlane(HTTP::Result eResult, HTTP::Response* pResponse)
{
	if (m_State == ServerBrowserState::kClaiming)
	{
		SetState(ServerBrowserState::kReady);
	}

	if (pResponse->GetStatus() == 409)
	{
		SEOUL_WARN("Plane already claimed by another developer; please try again.");
		QueryDemiplaneList();
		return HTTP::CallbackResult::kSuccess;
	}

	DemiplaneClaimResult result;
	if (!DeserializeResponseJSON(eResult, pResponse, &result))
	{
		SEOUL_WARN("Couldn't parse demiplane response");
		return HTTP::CallbackResult::kSuccess;
	}

	m_vAvailablePlanes.Swap(result.m_vAvailable);
	m_vClaimedPlanes.Swap(result.m_vClaimed);

	UsePlane(result.m_Plane);

	return HTTP::CallbackResult::kSuccess;
}

/** Shared helper for resetting your client hostname and restarting to the patcher. */
void DevUIViewServerBrowser::ChangeHostname(const String& sHostname)
{
	auto pGameMain = Main::Get();
	if (!pGameMain)
	{
		return;
	}

	SEOUL_LOG("Overriding hostname from %s to %s", pGameMain->GetServerBaseURL().CStr(), sHostname.CStr());
	pGameMain->ServerBrowserFriend_SetServerBaseURL(sHostname);
	UI::Manager::Get()->TriggerRestart(true);
}

/**
 * Open the browser admin UI for the given ServerBaseURL
 */
void DevUIViewServerBrowser::OpenAdmin(const String& sBaseUrl, Bool bAttemptAutoLogin)
{
	String sLoginUrl = "%s/admin";
	if (bAttemptAutoLogin)
	{
		sLoginUrl = String::Printf("%s/demiplane/login?Username=%s&ApiKey=%s", sLoginUrl.CStr(), GetUsername().CStr(), GetDemiplaneApiKey().CStr());
	}
	Engine::Get()->OpenURL(String::Printf(sLoginUrl.CStr(), sBaseUrl.CStr()));
}

/**
 * Disconnect from the current plane and return to the base server (QA/Staging). Resets the game to the patcher.
 */
void DevUIViewServerBrowser::DisconnectFromPlane()
{
	m_CurrentPlane = Demiplane();
	ChangeHostname(ClientSettings::GetServerBaseURL());
	auto pDevUI = DevUIRoot::Get();
	if (pDevUI.IsValid())
	{
		pDevUI->SetDemiplaneName(m_CurrentPlane.m_Config.m_sName);
	}
}

/**
 * Connect to a demiplane. Resets the game to the patcher.
 */
void DevUIViewServerBrowser::UsePlane(const Demiplane& plane)
{
	m_CurrentPlane = plane;
	ChangeHostname(plane.GetServerBaseURL().CStr());

	auto pDevUI = DevUIRoot::Get();
	if (pDevUI.IsValid())
	{
		pDevUI->SetDemiplaneName(m_CurrentPlane.m_Config.m_sName);
	}
}

/**
 * Should the "claim" button be clickable?
 */
Bool DevUIViewServerBrowser::CanClaimNewPlane() const
{
	if (m_State != ServerBrowserState::kReady)
	{
		return false;
	}

	return !m_sNewPlaneName.IsEmpty() && !m_vAvailablePlanes.IsEmpty() && CanChangePlane();
}

/**
 * Should the "change" button for existing planes be clickable?
 */
Bool DevUIViewServerBrowser::CanChangePlane() const
{
	static HString const kGameLoaded("GameLoaded");
	return m_State == ServerBrowserState::kReady && UI::Manager::Get()->GetCondition(kGameLoaded);
}

/**
 * Should the "push config data" button be clickable?
 */
Bool DevUIViewServerBrowser::CanPushConfigData() const
{
	if (m_State != ServerBrowserState::kReady)
	{
		return false;
	}

	return !m_CurrentPlane.m_sHost.IsEmpty() || ClientSettings::GetServerType() == ServerType::kLocal;
}

/**
 * Claims a random demiplane from the "available" list. On success, resets the game to the patcher.
 */
void DevUIViewServerBrowser::ClaimRandomPlane(const String& sName)
{
	if (m_vAvailablePlanes.IsEmpty())
	{
		SEOUL_WARN("Can't claim new plane: none available");
		return;
	}

	auto idx = GlobalRandom::UniformRandomUInt32n(m_vAvailablePlanes.GetSize());
	auto plane = m_vAvailablePlanes[idx];
	m_vAvailablePlanes.Erase(m_vAvailablePlanes.Begin() + idx);

	String sUsername = GetUsername();
	SEOUL_LOG("Claiming plane for %s: %s", sUsername.CStr(), plane.m_sHost.CStr());
	ClaimPlane(plane, sName, sUsername);
}

} // namespace Game

} // namespace Seoul

#endif // /(SEOUL_WITH_SERVER_BROWSER && SEOUL_ENABLE_DEV_UI && !SEOUL_SHIP)
