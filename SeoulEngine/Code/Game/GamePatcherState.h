/**
 * \file GamePatcherState.h
 * \brief Tracking of patcher state and progress.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_PATCHER_STATE_H
#define GAME_PATCHER_STATE_H

#include "DownloadablePackageFileSystemStats.h"
#include "FixedArray.h"
#include "HTTPStats.h"
#include "Prereqs.h"
#include "SeoulTime.h"

namespace Seoul::Game
{

enum class PatcherState : Int32
{
	/**
	 * The first thing we want to do is make sure the player has seen the GDPR message
	 * and agreed to it.
	 */
	kGDPRCheck,

	/**
	 * Pending patch - makes sure UI is in the appropriate state, then moves Game::Main
	 * into the pre-game tier.
	 */
	kInitial,

	/** Waiting for the game auth manager to report that auth information is available. */
	kWaitForAuth,

	/**
	 * Waiting for the required version check to pass - mostly this is instantaneous,
	 * but when a required version has been specified, this is indefinite.
	 */
	kWaitForRequiredVersion,

	/** Waiting for all startup conditions to be fulfilled before applying patch .sar files. */
	kWaitForPatchApplyConditions,

	/** When dowloading additional content, a write error occured. */
	kInsufficientDiskSpace,

	/** When downloading the patch files, a write error occured. */
	kInsufficientDiskSpacePatchApply,

	/** The patcher is actively attempting to swap in the patchable .sar files. */
	kPatchApply,

	/** Waiting for texture cache purge so we can being content reload. */
	kWaitingForTextureCachePurge,

	/**
	 * Waiting for the settings - make sure all settings files are done loading / reloading
	 * before advancing to the soft reboot.
	 */
	kWaitingForContentReload,

	/**
	 * Waiting for the settings after a patch error occured - we essentially flush state
	 * and start the patching process over when this happens.
	 */
	kWaitingForContentReloadAfterError,

	/** Waiting for the app's ConfigManager to load. */
	kWaitingForGameConfigManager,

#if SEOUL_WITH_GAME_PERSISTENCE
	/** Waiting for the app's PersistenceManager to load. */
	kWaitingForGamePersistenceManager,
#endif // /#if SEOUL_WITH_GAME_PERSISTENCE

	/** Waiting to pre cache URLs. */
	kWaitingForPrecacheUrls,

	/** Waiting for the script virtual machine to reload. */
	kWaitingForGameScriptManager,

	/** Tell Game::Main to complete initialization of the game tier. */
	kGameInitialize,

	/** The patching flow is complete. */
	kDone,

	/**
	 * A few unexpected events(e.g.auth change) will force
	 * the patcher to restart. This state waits for any
	 * pending operations to complete, then resets
	 * simple state and returns to the kInitial state.
	 */
	kRestarting,

	COUNT
};

struct PatcherDisplayStat SEOUL_SEALED
{
	UInt32 m_uCount{};
	Float m_fTimeSecs{};
}; // struct Game::PatcherDisplayStat

struct PatcherDisplayStats SEOUL_SEALED
{
	typedef FixedArray<PatcherDisplayStat, (UInt32)PatcherState::COUNT> PerState;
	PerState m_aPerState;
	typedef HashTable<HString, PatcherDisplayStat, MemoryBudgets::Game> ApplySubStats;
	ApplySubStats m_tApplySubStats;
	UInt32 m_uReloadedFiles{};

	HTTP::Stats m_AuthLoginRequest;
	DownloadablePackageFileSystemStats m_AdditionalStats;
	DownloadablePackageFileSystemStats m_ConfigStats;
	DownloadablePackageFileSystemStats m_ContentStats;
}; // struct Game::PatcherDisplayStats

} // namespace Seoul::Game

#endif // include guard
