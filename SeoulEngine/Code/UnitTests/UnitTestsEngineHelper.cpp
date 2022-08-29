/**
 * \file UnitTestsEngineHelper.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CrashManager.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "NullPlatformEngine.h"
#include "SharedPtr.h"
#include "UnitTestsEngineHelper.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

extern InitializeFileSystemsCallback g_pUnitTestsFileSystemsCallback;
extern String g_sUnitTestsBaseDirectoryPath;
static InitializeFileSystemsCallback s_pUnitTestsCustomCallback = nullptr;

static NullPlatformEngineSettings GetUnitTestsEngineHelperEngineSettings()
{
	NullPlatformEngineSettings ret;
	ret.m_sBaseDirectoryPath = g_sUnitTestsBaseDirectoryPath;
	return ret;
}

static void UnitTestsEngineHelperFileSystemsCallback()
{
	if (nullptr != g_pUnitTestsFileSystemsCallback)
	{
		g_pUnitTestsFileSystemsCallback();
	}
	else
	{
		// Register a default disk file system.
		FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
	}

	if (nullptr != s_pUnitTestsCustomCallback)
	{
		s_pUnitTestsCustomCallback();
	}
}

UnitTestsEngineHelper::UnitTestsEngineHelper(
	void (*customFileSystemInitialize)() /*= nullptr*/)
	: m_pCrashManager()
	, m_pEngine()
{
	s_pUnitTestsCustomCallback = customFileSystemInitialize;
	g_pInitializeFileSystemsCallback = UnitTestsEngineHelperFileSystemsCallback;

	m_pCrashManager.Reset(SEOUL_NEW(MemoryBudgets::Developer) NullCrashManager);
	m_pEngine.Reset(SEOUL_NEW(MemoryBudgets::Developer) NullPlatformEngine(GetUnitTestsEngineHelperEngineSettings()));
	m_pEngine->Initialize();
}

UnitTestsEngineHelper::UnitTestsEngineHelper(
	void (*customFileSystemInitialize)(),
	const NullPlatformEngineSettings& settings)
	: m_pCrashManager()
	, m_pEngine()
{
	s_pUnitTestsCustomCallback = customFileSystemInitialize;
	g_pInitializeFileSystemsCallback = UnitTestsEngineHelperFileSystemsCallback;

	auto settingsCopy(settings);
	if (settingsCopy.m_sBaseDirectoryPath.IsEmpty() &&
		!g_sUnitTestsBaseDirectoryPath.IsEmpty())
	{
		settingsCopy.m_sBaseDirectoryPath = g_sUnitTestsBaseDirectoryPath;
	}
	m_pCrashManager.Reset(SEOUL_NEW(MemoryBudgets::Developer) NullCrashManager);
	m_pEngine.Reset(SEOUL_NEW(MemoryBudgets::Developer) NullPlatformEngine(settingsCopy));
	m_pEngine->Initialize();

	// Cleanup save directory after startup.
	auto const sSave(GetUnitTestingSaveDir());
	Directory::Delete(sSave, true);
}

UnitTestsEngineHelper::~UnitTestsEngineHelper()
{
	// On shutdown, delete any files in the user's save folder.
	auto const sSave(GetUnitTestingSaveDir());

	m_pEngine->Shutdown();
	m_pEngine.Reset();
	m_pCrashManager.Reset();

	g_pInitializeFileSystemsCallback = nullptr;
	s_pUnitTestsCustomCallback = nullptr;

	// Now delete files in the save directory.
	Directory::Delete(sSave, true);
}

void UnitTestsEngineHelper::Tick()
{
	(void)m_pEngine->Tick();
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
