/**
 * \file UnitTestsFileManagerHelper.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Directory.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "GamePathsSettings.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

InitializeFileSystemsCallback g_pUnitTestsFileSystemsCallback = nullptr;
String g_sUnitTestsBaseDirectoryPath;

UnitTestsFileManagerHelper::UnitTestsFileManagerHelper()
	: m_bShutdownFileManager(false)
	, m_bShutdownGamePaths(false)
{
	if (!GamePaths::Get().IsValid())
	{
		GamePathsSettings settings;
		settings.m_sBaseDirectoryPath = g_sUnitTestsBaseDirectoryPath;
		GamePaths::Initialize(settings);
		m_bShutdownGamePaths = true;
	}

	if (!FileManager::Get().IsValid())
	{
		FileManager::Initialize();
		if (nullptr != g_pUnitTestsFileSystemsCallback)
		{
			g_pUnitTestsFileSystemsCallback();
		}
		else
		{
			FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
		}
		m_bShutdownFileManager = true;
	}
}

UnitTestsFileManagerHelper::~UnitTestsFileManagerHelper()
{
	if (m_bShutdownFileManager)
	{
		FileManager::ShutDown();
	}

	if (m_bShutdownGamePaths)
	{
		// On shutdown, delete any files in the user's save folder.
		auto const sSave(GetUnitTestingSaveDir());

		GamePaths::ShutDown();

		// Now delete files in the save directory.
		Directory::Delete(sSave, true);
	}
}

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul
