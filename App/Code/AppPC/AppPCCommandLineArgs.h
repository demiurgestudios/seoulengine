/**
 * \file AppPCCommandLineArgs.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef APP_PC_COMMAND_LINE_ARGS_H
#define APP_PC_COMMAND_LINE_ARGS_H

#include "CommandLineArgWrapper.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"

namespace Seoul
{

/**
 * Root PC command-line arguments - handled by reflection, can be
 * configured via the literal command-line, environment variables, or
 * a configuration file.
 */
class AppPCCommandLineArgs SEOUL_SEALED
{
public:
	static Bool GetAcceptGDPR() { return AcceptGDPR; }
	static Bool GetD3D11Headless() { return D3D11Headless; }
	static Bool GetEnableWarnings() { return EnableWarnings; }
	static Bool GetFMODHeadless() { return FMODHeadless; }
	static Bool GetFreeDiskAccess() { return FreeDiskAccess; }
	static const String& GetRunScript() { return RunScript; }
	static Bool GetDownloadablePackageFileSystemsEnabled() { return DownloadablePackageFileSystemsEnabled; }
	static Bool GetPersistentTest() { return PersistentTest; }
	static const CommandLineArgWrapper<String>& GetRunUnitTests() { return RunUnitTests; }
	static const String& GetRunAutomatedTest() { return RunAutomatedTest; }
	static const String& GetGenerateScriptBindings() { return GenerateScriptBindings; }
	static const String& GetTrainerFile() { return TrainerFile; }
	static const String& GetUIManager() { return UIManager; }
	static Bool GetVerboseMemoryTooling() { return VerboseMemoryTooling; }

private:
	SEOUL_REFLECTION_FRIENDSHIP(AppPCCommandLineArgs);

	static Bool AcceptGDPR;
	static Bool D3D11Headless;
	static Bool EnableWarnings;
	static Bool FMODHeadless;
	static Bool FreeDiskAccess;
	static String RunScript;
	static Bool DownloadablePackageFileSystemsEnabled;
	static Bool PersistentTest;
	static CommandLineArgWrapper<String> RunUnitTests;
	static String RunAutomatedTest;
	static String GenerateScriptBindings;
	static String TrainerFile;
	static String UIManager;
	static Bool VerboseMemoryTooling;
	static String VideoDir;

	AppPCCommandLineArgs() = delete; // Static class.
	SEOUL_DISABLE_COPY(AppPCCommandLineArgs);
};

} // namespace Seoul

#endif // include guard
