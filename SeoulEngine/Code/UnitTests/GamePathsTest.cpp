/**
 * \file GamePathsTest.cpp
 * \brief Unit test implementations for GamePaths class
 *
 * This file contains the unit tests for the GamePaths class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "GamePaths.h"
#include "GamePathsSettings.h"
#include "GamePathsTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(GamePathsTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestMethods)
	SEOUL_METHOD(TestValues)
SEOUL_END_TYPE()

extern String g_sUnitTestsBaseDirectoryPath;

/**
 * Tests the GamePaths Get/Set methods
 */
void GamePathsTest::TestMethods(void)
{
	if (!GamePaths::Get())
	{
		GamePathsSettings settings;
		settings.m_sBaseDirectoryPath = g_sUnitTestsBaseDirectoryPath;
		GamePaths::Initialize(settings);
	}

	GamePaths::Get()->SetBaseDir("base dir");
	String baseDir = GamePaths::Get()->GetBaseDir();
	SEOUL_UNITTESTING_ASSERT(baseDir == String("base dir") + Path::DirectorySeparatorChar());

	GamePaths::Get()->SetConfigDir("config dir");
	String configDir = GamePaths::Get()->GetConfigDir();
	SEOUL_UNITTESTING_ASSERT(configDir == String("config dir") + Path::DirectorySeparatorChar());

	GamePaths::Get()->SetContentDir("content dir");
	String dataDir = GamePaths::Get()->GetContentDir();
	SEOUL_UNITTESTING_ASSERT(dataDir == String("content dir") + Path::DirectorySeparatorChar());

	GamePaths::Get()->SetLogDir("log dir");
	String logDir = GamePaths::Get()->GetLogDir();
	SEOUL_UNITTESTING_ASSERT(logDir == String("log dir") + Path::DirectorySeparatorChar());
	GamePaths::ShutDown();
}

void GamePathsTest::TestValues(void)
{
	if (!GamePaths::Get())
	{
		GamePathsSettings settings;
		settings.m_sBaseDirectoryPath = g_sUnitTestsBaseDirectoryPath;
		GamePaths::Initialize(settings);
	}

	auto const& paths = *GamePaths::Get();

	SEOUL_UNITTESTING_ASSERT_EQUAL(Path::Combine(paths.GetBaseDir(), "Data/Config/"), paths.GetConfigDir());
	SEOUL_UNITTESTING_ASSERT_EQUAL(paths.GetContentDir(), paths.GetContentDirForPlatform(keCurrentPlatform));
#if SEOUL_PLATFORM_IOS
	SEOUL_UNITTESTING_ASSERT_EQUAL(Path::Combine(paths.GetUserDir(), "Data/Log/"), paths.GetLogDir());
#else
	SEOUL_UNITTESTING_ASSERT_EQUAL(Path::Combine(paths.GetBaseDir(), "Data/Log/"), paths.GetLogDir());
#endif
	SEOUL_UNITTESTING_ASSERT_EQUAL(Path::Combine(paths.GetBaseDir(), "Source/"), paths.GetSourceDir());

	GamePaths::ShutDown();
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
