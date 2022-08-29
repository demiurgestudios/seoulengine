/**
 * \file SteamEngine.cpp
 * \brief Specialization of Engine for the PC platform which
 * uses the Steamworks API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BuildChangelistPublic.h"
#include "Logger.h"
#include "SteamAchievementManager.h"
#include "SteamCommerceManager.h"
#include "SteamEngine.h"
#include "SteamPrivateApi.h"
#include "SteamSaveApi.h"

#if SEOUL_WITH_STEAM

// Make sure the Steam redistributable library is included.
#ifdef _MSC_VER
#	pragma comment(lib, "steam_api64.lib")
#endif

namespace Seoul
{

/** Maximum size of a Steam authentication ticket */
static const UInt kSteamMaxAuthTicketSize = 1024;

class SteamEnginePimpl SEOUL_SEALED
{
public:
	SteamEnginePimpl()
#pragma warning(disable:4355)  // 'this': used in base member initializer list
		: m_ReceiveAuthTicketCallback(this, &SteamEnginePimpl::OnReceiveAuthTicket)
#pragma warning(default:4355)
		, m_hAuthTicket(k_HAuthTicketInvalid)
	{
	}

	~SteamEnginePimpl()
	{
	}

	const Engine::AuthTicket& GetAuthTicket() const { return m_vAuthTicket; }

	void OnSteamInit()
	{
		// Get an authentication ticket. If we don't have a ticket yet, we'll
		// get a callback later when it's ready.
		m_vAuthTicket.Resize(kSteamMaxAuthTicketSize);
		UInt32 uTicketSize = m_vAuthTicket.GetSize();
		m_hAuthTicket = SteamUser()->GetAuthSessionTicket(m_vAuthTicket.Data(), uTicketSize, &uTicketSize);
		m_vAuthTicket.Resize(uTicketSize);
	}

	void Shutdown()
	{
		if (SteamUser() && m_hAuthTicket != k_HAuthTicketInvalid)
		{
			SteamUser()->CancelAuthTicket(m_hAuthTicket);
		}
	}

private:
	SEOUL_DISABLE_COPY(SteamEnginePimpl);

	STEAM_CALLBACK(SteamEnginePimpl, OnReceiveAuthTicket, GetAuthSessionTicketResponse_t, m_ReceiveAuthTicketCallback);

	// Handle to the steam auth ticket
	HAuthTicket m_hAuthTicket;

	// Authentication ticket data
	Engine::AuthTicket m_vAuthTicket;
};

/**
 * Callback called when we receive an authentication ticket from the Steam
 * servers, if we requested a ticket earlier but did not yet have one.
 */
void SteamEnginePimpl::OnReceiveAuthTicket(GetAuthSessionTicketResponse_t* pCallback)
{
	// The docs are not really clear on when this gets called.  So just to be
	// safe, avoid calling GetAuthSessionTicket if we already have a ticket.
	if (!m_vAuthTicket.IsEmpty())
	{
		return;
	}

	// If we got a ticket, store it now
	if (pCallback->m_eResult == k_EResultOK)
	{
		m_vAuthTicket.Resize(kSteamMaxAuthTicketSize);
		UInt32 uTicketSize = m_vAuthTicket.GetSize();
		m_hAuthTicket = SteamUser()->GetAuthSessionTicket(m_vAuthTicket.Get(0), uTicketSize, &uTicketSize);
		m_vAuthTicket.Resize(uTicketSize);
	}
}

Bool SteamEngine::RestartAppIfNecessary(UInt32 uAppID)
{
#if SEOUL_SHIP
	if (SteamAPI_RestartAppIfNecessary(uAppID))
	{
		return true;
	}
#endif // /#if SEOUL_SHIP

	return false;
}

void SteamEngine::WriteMiniDump(UInt32 uExceptionCode, void* pExceptionInfo)
{
	SteamAPI_WriteMiniDump(uExceptionCode, pExceptionInfo, (UInt32)g_iBuildChangelist);
}

SteamEngine::SteamEngine(const PCEngineSettings& settings)
	: PCEngine(settings)
	, m_pImpl(SEOUL_NEW(MemoryBudgets::TBD) SteamEnginePimpl)
	, m_uAppID(0u)
	, m_uSteamID(k_steamIDNil.ConvertToUint64())
{
}

SteamEngine::~SteamEngine()
{
}

void SteamEngine::Initialize()
{
	// In order for the Steam overlay to work properly, Steam needs to be
	// initialized before we create the render device.
	if (SteamAPI_Init())
	{
		// Cache the app ID and user ID once at startup to avoid extra IPC
		// later since these never change while the game is running
		m_uAppID = SteamUtils()->GetAppID();
		m_uSteamID = SteamUser()->GetSteamID().ConvertToUint64();

		SEOUL_LOG_ENGINE("[Steam]: Steam API initialized: AppID=%u Persona=%s SteamID=%llu", m_uAppID, SteamFriends()->GetPersonaName(), m_uSteamID);
		SteamUtils()->SetWarningMessageHook(&SteamEngine::SteamWarningHook);

		// Initialize the pimpl.
		m_pImpl->OnSteamInit();
	}
	else
	{
		// This used to be a SEOUL_WARN() to remind developers to start Steam, but
		// it's more annoying than useful since we're not using a lot
		// of Steam features, so it's a SEOUL_LOG() for now.
		SEOUL_LOG_ENGINE("[Steam] Steam client is not running (SteamAPI_Init failed).  Steam functionality will be disabled.");
	}

	PCEngine::Initialize();
}

void SteamEngine::Shutdown()
{
	PCEngine::Shutdown();
	m_pImpl->Shutdown();
	SteamAPI_Shutdown();
}

String SteamEngine::GetSystemLanguage() const
{
	if (SteamApps() != nullptr)
	{
		String sLowercaseLanguage = String(SteamApps()->GetCurrentGameLanguage());

		// Special case Korean since Seoul expects Korean, but steam returns koreana
		if (sLowercaseLanguage == "koreana")
		{
			return "Korean";
		}

		if (!sLowercaseLanguage.IsEmpty())
		{
			return String(sLowercaseLanguage[0]).ToUpperASCII() + sLowercaseLanguage.Substring(1);
		}
		else
		{
			SEOUL_LOG("Steam returned empty language; has your Steam account been added to own the game on the App Data Admin?");
			return PCEngine::GetSystemLanguage();
		}
	}
	else
	{
		return PCEngine::GetSystemLanguage();
	}
}

/** Gets the current authentication ticket */
const Engine::AuthTicket& SteamEngine::GetAuthenticationTicket() const
{
	return m_pImpl->GetAuthTicket();
}

/** Ticks Steam */
Bool SteamEngine::Tick()
{
	auto const bReturn = PCEngine::Tick();

	// Dispatch Steam callbacks
	SteamAPI_RunCallbacks();

	return bReturn;
}

SaveApi* SteamEngine::CreateSaveApi()
{
	return SEOUL_NEW(MemoryBudgets::TBD) SteamSaveApi();
}

AchievementManager* SteamEngine::InternalCreateAchievementManager()
{
	return SEOUL_NEW(MemoryBudgets::TBD) SteamAchievementManager();
}

CommerceManager* SteamEngine::InternalCreateCommerceManager()
{
	return SEOUL_NEW(MemoryBudgets::TBD) SteamCommerceManager(
		m_uAppID,
		m_uSteamID);
}

/**
 * Steam API warning handler
 *
 * @param[in] nSeverity Severity: 0 for message, 1 for warning
 * @param[in] sMessage Message text
 */
void SteamEngine::SteamWarningHook(Int32 iSeverity, Byte const* sMessage)
{
	// By default, only warnings will get sent to this function.  If you run
	// with the command line argument "-debug_steamapi", then debug messages
	// will also appear.
	if (iSeverity > 0)
	{
		SEOUL_WARN("[Steam (severity %d)]: %s", iSeverity, sMessage);
	}
	else
	{
		SEOUL_LOG_ENGINE("[Steam]: %s", sMessage);
	}
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_STEAM
