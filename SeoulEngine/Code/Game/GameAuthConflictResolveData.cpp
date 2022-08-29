/**
 * \file GameAuthConflictResolveData.cpp
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

#include "GameAuthConflictResolveData.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Game::AuthConflictPlayerData)
	SEOUL_PROPERTY_N("Name", m_sName)
	SEOUL_PROPERTY_N("CreatedAt", m_CreatedAt)
	SEOUL_PROPERTY_N("Data", m_sData)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Game::AuthConflictResolveData)
	SEOUL_PROPERTY_N("DevicePlayer", m_DevicePlayer)
	SEOUL_PROPERTY_N("PlatformPlayer", m_PlatformPlayer)
SEOUL_END_TYPE()

} // namespace Seoul
