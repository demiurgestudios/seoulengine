/**
 * \file GameClientSettings.cpp
 * \brief Utilities for accessing the packaging/server specific
 * client configuration (QA, Staging, Prod). Client settings
 * are not directly accessed by the Game project, but this
 * is a set of shared utilities for accessing them.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ApplicationJson.h"
#include "GameClientSettings.h"
#include "Logger.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(Game::ServerType)
	SEOUL_ENUM_N("Unknown", Game::ServerType::kUnknown)
	SEOUL_ENUM_N("LOCAL", Game::ServerType::kLocal)
	SEOUL_ENUM_N("SANDBOX", Game::ServerType::kSandbox)
	SEOUL_ENUM_N("QA", Game::ServerType::kQa)
	SEOUL_ENUM_N("STAGING", Game::ServerType::kStaging)
	SEOUL_ENUM_N("PROD", Game::ServerType::kProd)
SEOUL_END_ENUM()

namespace Game
{

namespace ClientSettings
{

// Shared constants.
static Byte const* ksAnalyticsApiKeyFormat = "AnalyticsApiKey%s";
static const HString ksClientSettings("ClientSettings");
static const HString ksSaveGameFilename("SaveGameFilename");
static const HString ksServerBaseURL("ServerBaseURL");
static const HString ksServerType("ServerType");

/**
 * Developer utility, handles loading the appropriate client settings in
 * various developer configurations.
 */
SharedPtr<DataStore> Load()
{
#if !SEOUL_SHIP
	// In developer builds, check for ClientSettings field
	// in application.json. If it exists, use that file instead.
	{
		FilePath clientSettings;
		if (GetApplicationJsonValue(ksClientSettings, clientSettings))
		{
			auto pDataStore(SettingsManager::Get()->WaitForSettings(clientSettings));
			if (pDataStore.IsValid())
			{
				return pDataStore;
			}
		}
	}
#endif

	static FilePath const kClientSettingsFilePath(FilePath::CreateConfigFilePath("ClientSettings.json"));
	auto pDataStore(SettingsManager::Get()->WaitForSettings(kClientSettingsFilePath));

#if !SEOUL_SHIP
	// TODO: When we have a branch/deploy configuration, this should fallback to Staging
	// instead of QA.
	static FilePath const kClientSettingsQAFilePath(FilePath::CreateConfigFilePath("ClientSettingsQA/ClientSettings.json"));

	if (!pDataStore.IsValid())
	{
		// Try QA.
		pDataStore = SettingsManager::Get()->WaitForSettings(kClientSettingsQAFilePath);
	}
#endif // /#if !SEOUL_SHIP

	return pDataStore;
}

/** Utility to get a particular key-value pair from ClientSettings*.json. */
template <typename T>
inline static Bool GetClientSettingsIniValue(HString sName, T& rValue)
{
	SharedPtr<DataStore> pDataStore(Load());
	if (!pDataStore.IsValid())
	{
		SEOUL_WARN("Failed loading ClientSettings.json.");
		return false;
	}

	DataStoreTableUtil defaultSection(*pDataStore, pDataStore->GetRootNode(), HString());
	if (!defaultSection.GetValue(sName, rValue))
	{
		SEOUL_WARN("Failed looking up \"%s\" in ClientSettings.json.", sName.CStr());
		return false;
	}

	return true;
}

String GetAnalyticsApiKey()
{
	String sReturn;
	HString const key(String::Printf(ksAnalyticsApiKeyFormat, GetCurrentPlatformName()));
	GetClientSettingsIniValue(key, sReturn);
	return sReturn;
}

String GetSaveGameFilename()
{
	String sReturn;
	GetClientSettingsIniValue(ksSaveGameFilename, sReturn);
	return sReturn;
}

String GetServerBaseURL()
{
	String sReturn;
	GetClientSettingsIniValue(ksServerBaseURL, sReturn);
	return sReturn;
}

// Retrieve the current server type that this client communicates with.
ServerType GetServerType()
{
	ServerType eType = ServerType::kUnknown;
	GetClientSettingsIniValue(ksServerType, eType);
	return eType;
}

} // namespace Game::ClientSettings

} // namespace Game

} // namespace Seoul
