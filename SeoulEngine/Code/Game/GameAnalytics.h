/**
 * \file GameAnalytics.h
 * \brief Constants and utilities for analytics integration in Game.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_ANALYTICS_H
#define GAME_ANALYTICS_H

#include "Prereqs.h"
#include "FixedArray.h"
#include "GamePatcherState.h"

namespace Seoul { namespace Game { struct PatcherDisplayStats; } }
namespace Seoul { class TimeInterval; }
namespace Seoul { class WorldTime; }

namespace Seoul
{

namespace Analytics
{

typedef HashTable<String, Int32, MemoryBudgets::Analytics> ABTests;

void OnDiskWriteError();
void OnInstall();
void OnAppLaunch();
void OnPatcherClose(
	const TimeInterval& patcherUptime,
	Float32 fPatcherDisplayTimeInSeconds,
	const Game::PatcherDisplayStats& stats);
void OnPatcherOpen();
void SetAnalyticsABTests(const ABTests& tABTests);
void SetAnalyticsSandboxed(Bool bSandboxed);
void SetAnalyticsUserID(const String& sUserID);
void UpdateCreatedAt(const WorldTime& createdAt);

} // namespace Analytics

} //namespace Seoul

#endif // include guard
