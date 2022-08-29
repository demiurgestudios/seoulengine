/**
 * \file GamePatcherTest.h
 * \brief Test for the GamePatcher flow, makes sure that
 * all patchable types are applied correctly.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_PATCHER_TEST_H
#define GAME_PATCHER_TEST_H

#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulHString.h"
namespace Seoul { namespace Game { struct PatcherDisplayStats; } }
namespace Seoul { namespace HTTP { class Server; } }
namespace Seoul { class UnitTestsGameHelper; }

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class GamePatcherTest SEOUL_SEALED
{
public:
	GamePatcherTest();
	~GamePatcherTest();

	void TestNoPatch() { TestNoPatchImpl(false); }
	void TestPatchA() { TestPatchAImpl(false); }
	void TestPatchB() { TestPatchBImpl(false); }
	void TestMulti();
	void TestRestartingAfterContentReload();
	void TestRestartingAfterGameConfigManager();
	void TestRestartingAfterPrecacheUrls();

private:
	ScopedPtr<UnitTestsGameHelper> m_pHelper;
	ScopedPtr<HTTP::Server> m_pServer;

	void TestNoPatchImpl(Bool bAllowRestart, UInt32 uMinExpectedReloadCount = 7u, UInt32 uMaxExpectedReloadCount = 9u);
	void TestPatchAImpl(Bool bAllowRestart, UInt32 uMinExpectedReloadCount = 7u, UInt32 uMaxExpectedReloadCount = 9u);
	void TestPatchBImpl(Bool bAllowRestart, UInt32 uMinExpectedReloadCount = 7u, UInt32 uMaxExpectedReloadCount = 9u);
	void TestRestartingImpl(Int iPatcherState);

	void CheckAnimation2DData(Byte const* sName);
	void CheckAnimation2DNetwork(Byte const* sName);
	void CheckEffect(Byte const* sName, Double fTimeOutSeconds = 1.0);
	void CheckFx(Float fExpectedDuration, Double fTimeOutSeconds = 1.0);
	void CheckMovie(Byte const* sName);
	void CheckScript(Byte const* sName);
	void CheckScriptSetting(Byte const* sName);
	void CheckSound(Byte const* sName);
	void CheckTexture(UInt32 uExpectedWidth, UInt32 uExpectedHeight);
	void InitServer(const String& sLoginRoot,  const String& sRefreshRoot);
	void WaitForUIState(Game::PatcherDisplayStats& rStats, Byte const* sMachine, Byte const* sState, Bool bAllowRestart, Double fTimeOutSeconds = 10.0);

	SEOUL_DISABLE_COPY(GamePatcherTest);
}; // class GamePatcherTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
