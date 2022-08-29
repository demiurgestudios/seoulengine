/**
 * \file UnitTestsEngineHelper.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UNIT_TESTS_ENGINE_HELPER_H
#define UNIT_TESTS_ENGINE_HELPER_H

#include "Prereqs.h"
#include "ScopedPtr.h"
#include "Singleton.h"
namespace Seoul { struct GenericAnalyticsManagerSettings; }
namespace Seoul { class NullCrashManager; }
namespace Seoul { class NullPlatformEngine; }
namespace Seoul { struct NullPlatformEngineSettings; }

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class UnitTestsEngineHelper SEOUL_SEALED : public Singleton<UnitTestsEngineHelper>
{
public:
	UnitTestsEngineHelper(
		void (*customFileSystemInitialize)() = nullptr);
	UnitTestsEngineHelper(
		void (*customFileSystemInitialize)(),
		const NullPlatformEngineSettings& settings);
	~UnitTestsEngineHelper();

	void Tick();

private:
	ScopedPtr<NullCrashManager> m_pCrashManager;
	ScopedPtr<NullPlatformEngine> m_pEngine;
}; // class UnitTestsEngineHelper

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
