/**
 * \file SlimCSTest.cpp
 * \brief Unit tests of the SlimCS compiler and toolchain.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FileManager.h"
#include "GamePaths.h"
#include "Logger.h"
#include "Path.h"
#include "ReflectionDefine.h"
#include "SeoulProcess.h"
#include "SlimCSTest.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

// Number of expected files in our test set.
static const Int32 kiExpectedCount = 125;

SEOUL_BEGIN_TYPE(SlimCSTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestFeatures)
	SEOUL_METHOD(TestMono)
	SEOUL_METHOD(TestSlimCSToLua)
	SEOUL_METHOD(TestSlimCSToLuaDebug)
	SEOUL_METHOD(TestSlimCSToLuaJapan)
	SEOUL_METHOD(TestSlimCSToLuaKorea)
	SEOUL_METHOD(TestSlimCSToLuaPoland)
	SEOUL_METHOD(TestSlimCSToLuaQatar)
	SEOUL_METHOD(TestSlimCSToLuaRussia)
SEOUL_END_TYPE()

namespace
{

void OnLog(Byte const* s)
{
	SEOUL_LOG_UNIT_TEST("%s", s);
}

}

/**
 * Test-ception - this runs the SlimCS compiler with the -t argument,
 * which is itself a testing harness. It reats all .cs files in the input
 * folder as unit tests, executing the main function from each.
 */
void SlimCSTest::TestFeatures()
{
	UnitTestsFileManagerHelper helper;

	// Get input path.
	auto const sInCS(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/SlimCS/Features"));

	// Get temp path.
	auto const sOutLua(Path::Combine(Path::GetTempDirectory(), "SeoulUnitTestOutLua"));

	// Get path to compilers.
	auto const sLuaJIT(Path::Combine(GamePaths::Get()->GetToolsBinDir(), "LuaJIT", "luajit.exe"));
	auto const sSlimCS(Path::Combine(GamePaths::Get()->GetToolsBinDir(), "SlimCS.exe"));

	// Clean the output paths.
	FileManager::Get()->DeleteDirectory(sOutLua, true);

	{
		Int32 iCount = 0;
		SEOUL_UNITTESTING_ASSERT(TestDirCountFiles(sOutLua, ".lua", iCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iCount);
	}

	// Run unit tests against the testing SlimCS .cs feature set.
	Int32 iResult = -1;
	{
		auto const aIn = { sInCS, sOutLua, String("-t"), String("--luac"), sLuaJIT };
		Process process(sSlimCS, Process::ProcessArguments(aIn.begin(), aIn.end()), SEOUL_BIND_DELEGATE(OnLog), SEOUL_BIND_DELEGATE(OnLog));
		if (!process.Start())
		{
			iResult = -1;
		}
		else
		{
			iResult = process.WaitUntilProcessIsNotRunning();
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, iResult);
}

void SlimCSTest::TestMono()
{
	UnitTestsFileManagerHelper helper;

	// Get input path.
	auto const sInCS(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/SlimCS/Mono"));

	// Get temp path.
	auto const sOutLua(Path::Combine(Path::GetTempDirectory(), "SeoulUnitTestOutLua"));

	// Get path to compilers.
	auto const sLuaJIT(Path::Combine(GamePaths::Get()->GetToolsBinDir(), "LuaJIT", "luajit.exe"));
	auto const sSlimCS(Path::Combine(GamePaths::Get()->GetToolsBinDir(), "SlimCS.exe"));

	// Clean the output paths.
	FileManager::Get()->DeleteDirectory(sOutLua, true);

	{
		Int32 iCount = 0;
		SEOUL_UNITTESTING_ASSERT(TestDirCountFiles(sOutLua, ".lua", iCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iCount);
	}

	// Run unit tests against the testing SlimCS .cs feature set.
	Int32 iResult = -1;
	{
		auto const aIn = { sInCS, sOutLua, String("-t"), String("--luac"), sLuaJIT };
		Process process(sSlimCS, Process::ProcessArguments(aIn.begin(), aIn.end()), SEOUL_BIND_DELEGATE(OnLog), SEOUL_BIND_DELEGATE(OnLog));
		if (!process.Start())
		{
			iResult = -1;
		}
		else
		{
			iResult = process.WaitUntilProcessIsNotRunning();
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, iResult);
}

void SlimCSTest::TestSlimCSToLua()
{
	UnitTestsFileManagerHelper helper;

	// Get input path.
	auto const sInCS(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/SlimCS/ScriptsCS"));

	// Get temp path.
	auto const sOutLua(Path::Combine(Path::GetTempDirectory(), "SeoulUnitTestOutLua"));

	// Get expected path.
	auto const sExpectedLua(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/SlimCS/Scripts"));

	// Get path to compiler.
	auto const sSlimCS(Path::Combine(GamePaths::Get()->GetToolsBinDir(), "SlimCS.exe"));

	// Clean the output paths.
	FileManager::Get()->DeleteDirectory(sOutLua, true);

	{
		Int32 iCount = 0;
		SEOUL_UNITTESTING_ASSERT(TestDirCountFiles(sOutLua, ".lua", iCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iCount);
	}

	// Generate .lua from the .cs snapshot.
	Int32 iResult = -1;
	{
		auto const aIn =
		{
			sInCS,
			sOutLua,
			String("-DNDEBUG"),
			String("-DSEOUL_WITH_ANIMATION_2D"),
			String("-DSEOUL_PLATFORM_WINDOWS"),
			String("-DSEOUL_BUILD_NOT_FOR_DISTRIBUTION"),
		};

		Process process(sSlimCS, Process::ProcessArguments(aIn.begin(), aIn.end()), Process::OutputDelegate(), SEOUL_BIND_DELEGATE(OnLog));
		if (!process.Start())
		{
			iResult = -1;
		}
		else
		{
			iResult = process.WaitUntilProcessIsNotRunning();
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, iResult);

	// Test generated against expected snapshot.
	SEOUL_UNITTESTING_ASSERT(TestDirIdenticalRecursive(sExpectedLua, sOutLua, ".lua", kiExpectedCount));
}

void SlimCSTest::TestSlimCSToLuaDebug()
{
	UnitTestsFileManagerHelper helper;

	// Get input path.
	auto const sInCS(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/SlimCS/ScriptsCS"));

	// Get temp path.
	auto const sOutLua(Path::Combine(Path::GetTempDirectory(), "SeoulUnitTestOutLua"));

	// Get expected path.
	auto const sExpectedLua(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/SlimCS/ScriptsDebug"));

	// Get path to compiler.
	auto const sSlimCS(Path::Combine(GamePaths::Get()->GetToolsBinDir(), "SlimCS.exe"));

	// Clean the output paths.
	FileManager::Get()->DeleteDirectory(sOutLua, true);

	{
		Int32 iCount = 0;
		SEOUL_UNITTESTING_ASSERT(TestDirCountFiles(sOutLua, ".lua", iCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iCount);
	}

	// Generate .lua from the .cs snapshot.
	Int32 iResult = -1;
	{
		auto const aIn =
		{
			sInCS,
			sOutLua,
			String("-DDEBUG"),
			String("-DSEOUL_WITH_ANIMATION_2D"),
			String("-DSEOUL_PLATFORM_WINDOWS"),
			String("-DSEOUL_BUILD_NOT_FOR_DISTRIBUTION"),
		};

		Process process(sSlimCS, Process::ProcessArguments(aIn.begin(), aIn.end()), Process::OutputDelegate(), SEOUL_BIND_DELEGATE(OnLog));
		if (!process.Start())
		{
			iResult = -1;
		}
		else
		{
			iResult = process.WaitUntilProcessIsNotRunning();
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, iResult);

	// Test generated against expected snapshot.
	SEOUL_UNITTESTING_ASSERT(TestDirIdenticalRecursive(sExpectedLua, sOutLua, ".lua", kiExpectedCount));
}

void SlimCSTest::DoTestSlimCSToLuaCulture(Byte const* sCulture)
{
	UnitTestsFileManagerHelper helper;

	// Get input path.
	auto const sInCS(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/SlimCS/ScriptsCS"));

	// Get temp path.
	auto const sOutLua(Path::Combine(Path::GetTempDirectory(), "SeoulUnitTestOutLua"));

	// Get expected path.
	auto const sExpectedLua(Path::Combine(GamePaths::Get()->GetConfigDir(), "UnitTests/SlimCS/Scripts"));

	// Get path to compiler.
	auto const sSlimCS(Path::Combine(GamePaths::Get()->GetToolsBinDir(), "SlimCS.exe"));

	// Clean the output paths.
	FileManager::Get()->DeleteDirectory(sOutLua, true);

	{
		Int32 iCount = 0;
		SEOUL_UNITTESTING_ASSERT(TestDirCountFiles(sOutLua, ".lua", iCount));
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, iCount);
	}

	// Generate .lua from the .cs snapshot.
	Int32 iResult = -1;
	{
		auto const aIn =
		{
			sInCS,
			sOutLua,
			String("-DNDEBUG"),
			String("-DSEOUL_WITH_ANIMATION_2D"),
			String("-DSEOUL_PLATFORM_WINDOWS"),
			String("-DSEOUL_BUILD_NOT_FOR_DISTRIBUTION"),
			String("--tcul"),
			String(sCulture),
		};

		Process process(sSlimCS, Process::ProcessArguments(aIn.begin(), aIn.end()), Process::OutputDelegate(), SEOUL_BIND_DELEGATE(OnLog));
		if (!process.Start())
		{
			iResult = -1;
		}
		else
		{
			iResult = process.WaitUntilProcessIsNotRunning();
		}
	}
	SEOUL_UNITTESTING_ASSERT_EQUAL(0, iResult);

	// Test generated against expected snapshot.
	SEOUL_UNITTESTING_ASSERT(TestDirIdenticalRecursive(sExpectedLua, sOutLua, ".lua", kiExpectedCount));
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
