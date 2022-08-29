/**
 * \file UnitTestsGameHelper.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UNIT_TESTS_GAME_HELPER_H
#define UNIT_TESTS_GAME_HELPER_H

#include "Prereqs.h"
#include "ScopedPtr.h"
#include "Singleton.h"
namespace Seoul { namespace Game { class Main; } }
namespace Seoul { namespace Sound { class Manager; } }
namespace Seoul { class UnitTestsEngineHelper; }

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class UnitTestsGameHelper SEOUL_SEALED : public Singleton<UnitTestsGameHelper>
{
public:
	UnitTestsGameHelper(
		const String& sServerBaseURL,
		const String& sRelativeConfigUpdatePath,
		const String& sRelativeContentPath,
		const String& sRelativeContentUpdatePath,
		Sound::Manager* (*soundManagerCreate)() = nullptr);
	~UnitTestsGameHelper();

	void Tick();

private:
	ScopedPtr<UnitTestsEngineHelper> m_pEngineHelper;
	ScopedPtr<Game::Main> m_pGameMain;
}; // class UnitTestsGameHelper

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
