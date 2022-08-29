/**
 * \file GameClientSettings.h
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

#pragma once
#ifndef GAME_CLIENT_SETTINGS_H
#define GAME_CLIENT_SETTINGS_H

#include "DataStore.h"
#include "Prereqs.h"
#include "SharedPtr.h"

namespace Seoul::Game
{

enum class ServerType
{
	kUnknown,
	kLocal,
	kSandbox,
	kQa,
	kStaging,
	kProd,
};

namespace ClientSettings
{

// Load the Game::Client DataStore (applies some additional handling around the load).
SharedPtr<DataStore> Load();

// Retrieve the analytics API key from the game's client configuration.
String GetAnalyticsApiKey();

// Retrieve the base filename to use for the game's save data.
String GetSaveGameFilename();

// Retrieve the root/base URL (with scheme) for connecting to the game's HTTP server.
String GetServerBaseURL();

// Retrieve the current server type that this client communicates with.
ServerType GetServerType();

} // namespace Game::ClientSettings

} // namespace Seoul::Game

#endif // include guard
