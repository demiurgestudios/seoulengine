/**
 * \file AppPCRunScript.cpp
 * \brief Defines the main function for a build run that will execute
 * an arbitrary Lua script and then exit.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AppPCCommandLineArgs.h"
#include "CrashManager.h"
#include "DataStoreParser.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Logger.h"
#include "NullPlatformEngine.h"
#include "PackageFileSystem.h"
#include "Platform.h"
#include "Prereqs.h"
#include "ScopedAction.h"
#include "ScriptManager.h"
#include "ScriptVm.h"
#include "SeoulUtil.h"
#include "StringUtil.h"
#include "UIManager.h"

namespace Seoul
{

void PCSendCustomCrash(const CustomCrashErrorState& errorState);
Int AutomatedTestsExceptionFilter(DWORD uExceptionCode, LPEXCEPTION_POINTERS pExceptionInfo);

#if !SEOUL_SHIP

namespace
{

#if SEOUL_LOGGING_ENABLED
/**
* Hook for print() output from Lua.
*/
void LuaLog(Byte const* sTextLine)
{
	SEOUL_LOG_SCRIPT("%s", sTextLine);
}
#endif // /#if SEOUL_LOGGING_ENABLED

} // namespace anonymous

static void OnInitializeFileSystems()
{
	// We need content .sar files always but we expect
	// direct access to config files.
	auto const sPlatformName = kaPlatformNames[(UInt32)keCurrentPlatform];
	for (auto const& s : kasStandardContentPackageFmts)
	{
		auto const sPath(Path::Combine(GamePaths::Get()->GetBaseDir(), "Data", String::Printf(s, sPlatformName)));
		if (DiskSyncFile::FileExists(sPath))
		{
			FileManager::Get()->RegisterFileSystem<PackageFileSystem>(sPath);
		}
	}

	FileManager::Get()->RegisterFileSystem<DiskFileSystem>();
}

namespace Reflection { Bool EmitScriptApi(const String& sPath); }

static Int AppPCRunScriptImplLevel2(NullPlatformEngine& engine, Bool bGenerateReflectionDef, String sFileName)
{
#if SEOUL_ENABLE_MEMORY_TOOLING
	// Output memory leak info to stdout instead of a file.
	MemoryManager::SetMemoryLeaksFilename(String());
#endif // /SEOUL_ENABLE_MEMORY_TOOLING

	// Get the script filename.
	sFileName = Path::GetExactPathName(sFileName);

	ScopedPtr<UI::Manager> pUI;
	{
		if (!AppPCCommandLineArgs::GetUIManager().IsEmpty())
		{
			FilePath filePath;
			if (!DataStoreParser::StringAsFilePath(AppPCCommandLineArgs::GetUIManager(), filePath))
			{
				SEOUL_WARN("-ui_manager argument has invalid gui FilePath: %s", AppPCCommandLineArgs::GetUIManager().CStr());
				return 1;
			}
			pUI.Reset(SEOUL_NEW(MemoryBudgets::UIRuntime) UI::Manager(filePath, UI::StackFilter::kAlways));
		}
	}

	if (bGenerateReflectionDef)
	{
		return Reflection::EmitScriptApi(sFileName) ? 0 : 1;
	}
	else
	{
		Script::VmSettings settings;

		// Only hookup to CrashManager if custom crashes are supported.
		if (CrashManager::Get()->CanSendCustomCrashes())
		{
			settings.m_ErrorHandler = SEOUL_BIND_DELEGATE(&PCSendCustomCrash);
		}
		// In non-ship builds, fall back to default handling.
#if !SEOUL_SHIP
		else
		{
			settings.m_ErrorHandler = SEOUL_BIND_DELEGATE(&CrashManager::DefaultErrorHandler);
		}
#endif // /#if !SEOUL_SHIP
		settings.SetStandardBasePaths();
#if SEOUL_LOGGING_ENABLED
		settings.m_StandardOutput = SEOUL_BIND_DELEGATE(LuaLog);
#endif // /#if SEOUL_LOGGING_ENABLED

		Script::Manager manager;
		Script::Vm vm(settings);
		return vm.RunScript(sFileName) ? 0 : 1;
	}
}

static Int AppPCRunScriptImplLevel1(NullPlatformEngine& engine, Bool bGenerateReflectionDef, const String& sFileName)
{
	__try
	{
		return AppPCRunScriptImplLevel2(engine, bGenerateReflectionDef, sFileName);
	}
	__except (AutomatedTestsExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
	{
		return 1;
	}
}

static Int AppPCRunScriptImplLevel0(Bool bGenerateReflectionDef, const String& sFileName)
{
	g_pInitializeFileSystemsCallback = OnInitializeFileSystems;

	// Initialize SeoulTime
	SeoulTime::MarkGameStartTick();

	// Mark that we're now in the main function.
	auto const inMain(MakeScopedAction(&BeginMainFunction, &EndMainFunction));

	GamePaths::SetUserConfigJsonFileName("game_config.json");

	SetMainThreadId(Thread::GetThisThreadId());

	// Configure booleans for automated testing.
	g_bRunningAutomatedTests = true;
	g_bHeadless = true;
	g_bShowMessageBoxesOnFailedAssertions = false;
	g_bEnableMessageBoxes = false;

	// Disable timestamping in the logger.
	Logger::GetSingleton().SetOutputTimestamps(false);

	// Disable a few noisy channels.
	Logger::GetSingleton().EnableAllChannels(true);
	Logger::GetSingleton().EnableChannel(LoggerChannel::Commerce, false);
	Logger::GetSingleton().EnableChannel(LoggerChannel::Engine, false);
	Logger::GetSingleton().EnableChannel(LoggerChannel::FileIO, false);
	Logger::GetSingleton().EnableChannel(LoggerChannel::Network, false);

	// If requested, enable warning channel.
	if (AppPCCommandLineArgs::GetEnableWarnings())
	{
		Logger::GetSingleton().EnableChannel(LoggerChannel::Warning, true);
	}

	// Startup, run, and shutdown.
	Int iInnerResult = 0;
	{
		NullCrashManager crashManager;
		NullPlatformEngineSettings settings;
		NullPlatformEngine engine(settings);
		engine.Initialize();
		iInnerResult = AppPCRunScriptImplLevel1(engine, bGenerateReflectionDef, sFileName);
		engine.Shutdown();
	}

	return iInnerResult;
}

Int AppPCRunScript(Bool bGenerateReflectionDef, const String& sFilePath)
{
	__try
	{
		return AppPCRunScriptImplLevel0(bGenerateReflectionDef, sFilePath);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		fprintf(stderr, "Unhandled x64 Exception (likely null pointer dereference or heap corruption)\n");
		return 1;
	}
}

#endif // /#if !SEOUL_SHIP

} // namespace Seoul
