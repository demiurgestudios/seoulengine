/**
 * \file GameAuthConflictResolveData.h
 * \brief This data is returned from the server when an authentication
 * attempt (login) would create a conflict that needs to be manually
 * resolved by the player. The current case is an attempt to login
 * with platform auth that resolves to a player that is different from
 * device auth, which would cause the device auth player to be orphaned.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_AUTH_CONFLICT_RESOLVE_DATA_H
#define GAME_AUTH_CONFLICT_RESOLVE_DATA_H

#include "Prereqs.h"
#include "SeoulString.h"
#include "SeoulTime.h"

namespace Seoul::Game
{

struct AuthConflictPlayerData SEOUL_SEALED
{
	String m_sName;
	WorldTime m_CreatedAt;
	String m_sData;
};

struct AuthConflictResolveData SEOUL_SEALED
{
	AuthConflictPlayerData m_DevicePlayer;
	AuthConflictPlayerData m_PlatformPlayer;
}; // struct Game::AuthConflictResolveData

} // namespace Seoul::Game

#endif // include guard
