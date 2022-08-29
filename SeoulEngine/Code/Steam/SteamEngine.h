/**
 * \file SteamEngine.h
 * \brief Specialization of Engine for the PC platform which
 * uses the Steamworks API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STEAM_ENGINE_H
#define STEAM_ENGINE_H

#include "PCEngine.h"
#include "ScopedPtr.h"
namespace Seoul { class SteamEnginePimpl; }
namespace Seoul { class SteamProfiles; }
namespace Seoul { class SteamSession; }

#if SEOUL_WITH_STEAM

namespace Seoul
{

/**
 * Specialization of the PC engine which uses the Steamworks API
 */
class SteamEngine SEOUL_SEALED : public PCEngine
{
public:
	static Bool RestartAppIfNecessary(UInt32 uAppID);
	static void WriteMiniDump(UInt32 uExceptionCode, void* pExceptionInfo);

	static CheckedPtr<SteamEngine> Get()
	{
		if (Engine::Get() && Engine::Get()->GetType() == EngineType::kSteam)
		{
			return (SteamEngine*)Engine::Get().Get();
		}
		else
		{
			return CheckedPtr<SteamEngine>();
		}
	}

	SteamEngine(const PCEngineSettings& settings);
	~SteamEngine();

	virtual EngineType GetType() const SEOUL_OVERRIDE { return EngineType::kSteam; }

	virtual void Initialize() SEOUL_OVERRIDE;
	virtual void Shutdown() SEOUL_OVERRIDE;

	virtual Bool Tick() SEOUL_OVERRIDE;

	virtual SaveApi* CreateSaveApi() SEOUL_OVERRIDE;

	virtual String GetSystemLanguage() const SEOUL_OVERRIDE;

	// Gets the current authentication ticket.
	virtual const AuthTicket& GetAuthenticationTicket() const SEOUL_OVERRIDE;

	// TODO: Move this into PlatformData and also implement for other
	// platforms that have a similar concept (e.g. the first party app id
	// on iOS).

	// Title app identifier on platforms which support a unique app identifier.
	virtual UInt32 GetTitleAppID() const { return m_uAppID; }

private:
	virtual AchievementManager* InternalCreateAchievementManager() SEOUL_OVERRIDE;
	virtual CommerceManager* InternalCreateCommerceManager() SEOUL_OVERRIDE;

	static void SteamWarningHook(Int32 iSeverity, Byte const* sMessag);

	ScopedPtr<SteamEnginePimpl> m_pImpl;
	UInt32 m_uAppID;
	UInt64 m_uSteamID;

	SEOUL_DISABLE_COPY(SteamEngine);
}; // class SteamEngine

} // namespace Seoul

#endif // /#if SEOUL_WITH_STEAM

#endif // include guard
