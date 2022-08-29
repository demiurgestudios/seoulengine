/**
 * \file GamePatcherState.cpp
 * \brief Tracking of patcher state and progress.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GamePatcherState.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(Game::PatcherState)
	SEOUL_ENUM_N("GDPRCheck", Game::PatcherState::kGDPRCheck)
	SEOUL_ENUM_N("Initial", Game::PatcherState::kInitial)
	SEOUL_ENUM_N("WaitForAuth", Game::PatcherState::kWaitForAuth)
	SEOUL_ENUM_N("WaitForRequiredVersion", Game::PatcherState::kWaitForRequiredVersion)
	SEOUL_ENUM_N("WaitForPatchApplyConditions", Game::PatcherState::kWaitForPatchApplyConditions)
	SEOUL_ENUM_N("InsufficientDiskSpace", Game::PatcherState::kInsufficientDiskSpace)
	SEOUL_ENUM_N("InsufficientDiskSpacePatchApply", Game::PatcherState::kInsufficientDiskSpacePatchApply)
	SEOUL_ENUM_N("PatchApply", Game::PatcherState::kPatchApply)
	SEOUL_ENUM_N("WaitingForTextureCachePurge", Game::PatcherState::kWaitingForTextureCachePurge)
	SEOUL_ENUM_N("WaitingForContentReload", Game::PatcherState::kWaitingForContentReload)
	SEOUL_ENUM_N("WaitingForContentReloadAfterError", Game::PatcherState::kWaitingForContentReloadAfterError)
	SEOUL_ENUM_N("WaitingForGameConfigManager", Game::PatcherState::kWaitingForGameConfigManager)
#if SEOUL_WITH_GAME_PERSISTENCE
	SEOUL_ENUM_N("WaitingForGamePersistenceManager", Game::PatcherState::kWaitingForGamePersistenceManager)
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE
	SEOUL_ENUM_N("WaitingForPrecacheUrls", Game::PatcherState::kWaitingForPrecacheUrls)
	SEOUL_ENUM_N("WaitingForGameScriptManager", Game::PatcherState::kWaitingForGameScriptManager)
	SEOUL_ENUM_N("GameInitialize", Game::PatcherState::kGameInitialize)
	SEOUL_ENUM_N("Done", Game::PatcherState::kDone)
	SEOUL_ENUM_N("Restarting", Game::PatcherState::kRestarting)
SEOUL_END_ENUM()

} // namespace Seoul
