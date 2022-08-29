/**
 * \file AppPCCommandLineArgs.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AppPCCommandLineArgs.h"
#include "ReflectionAttributes.h"
#include "ReflectionDefine.h"

namespace Seoul
{

Bool AppPCCommandLineArgs::AcceptGDPR{};
Bool AppPCCommandLineArgs::D3D11Headless{};
Bool AppPCCommandLineArgs::EnableWarnings{};
Bool AppPCCommandLineArgs::FMODHeadless{};
Bool AppPCCommandLineArgs::FreeDiskAccess{};
String AppPCCommandLineArgs::RunScript{};
Bool AppPCCommandLineArgs::DownloadablePackageFileSystemsEnabled{};
Bool AppPCCommandLineArgs::PersistentTest{};
CommandLineArgWrapper<String> AppPCCommandLineArgs::RunUnitTests{};
String AppPCCommandLineArgs::RunAutomatedTest{};
String AppPCCommandLineArgs::GenerateScriptBindings{};
String AppPCCommandLineArgs::TrainerFile{};
String AppPCCommandLineArgs::UIManager{};
Bool AppPCCommandLineArgs::VerboseMemoryTooling{};
String AppPCCommandLineArgs::VideoDir{};

SEOUL_BEGIN_TYPE(AppPCCommandLineArgs, TypeFlags::kDisableNew | TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(ScriptPreprocessorDirective, "SEOUL_PLATFORM_WINDOWS")
	SEOUL_CMDLINE_PROPERTY(AcceptGDPR, "accept_gdpr")
	SEOUL_CMDLINE_PROPERTY(D3D11Headless, "d3d11_headless")
	SEOUL_CMDLINE_PROPERTY(EnableWarnings, "enable_warnings")
	SEOUL_CMDLINE_PROPERTY(FMODHeadless, "fmod_headless")
	SEOUL_CMDLINE_PROPERTY(FreeDiskAccess, "free_disk_access")
	SEOUL_CMDLINE_PROPERTY(RunScript, "run_script")
	SEOUL_CMDLINE_PROPERTY(DownloadablePackageFileSystemsEnabled, "dpkg_enable")
	SEOUL_CMDLINE_PROPERTY(PersistentTest, "persistent_test")
	SEOUL_CMDLINE_PROPERTY(RunUnitTests, "run_unit_tests", "test-options")
	SEOUL_CMDLINE_PROPERTY(RunAutomatedTest, "run_automated_test")
	SEOUL_CMDLINE_PROPERTY(GenerateScriptBindings, "generate_script_bindings")
	SEOUL_CMDLINE_PROPERTY(TrainerFile, "trainer_file")
	SEOUL_CMDLINE_PROPERTY(UIManager, "ui_manager")
	SEOUL_CMDLINE_PROPERTY(VerboseMemoryTooling, "verbose_memory_tooling")
	SEOUL_CMDLINE_PROPERTY(VideoDir, "video_dir")
SEOUL_END_TYPE()

} // namespace Seoul
