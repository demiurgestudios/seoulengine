/**
 * \file AppLinuxCommandLineArgs.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AppLinuxCommandLineArgs.h"
#include "ReflectionAttributes.h"
#include "ReflectionDefine.h"

namespace Seoul
{

Bool AppLinuxCommandLineArgs::AcceptGDPR{};
Bool AppLinuxCommandLineArgs::EnableWarnings{};
Bool AppLinuxCommandLineArgs::FMODHeadless{};
Bool AppLinuxCommandLineArgs::FreeDiskAccess{};
Bool AppLinuxCommandLineArgs::DownloadablePackageFileSystemsEnabled{};
Bool AppLinuxCommandLineArgs::PersistentTest{};
CommandLineArgWrapper<String> AppLinuxCommandLineArgs::RunUnitTests{};
String AppLinuxCommandLineArgs::RunAutomatedTest{};
String AppLinuxCommandLineArgs::UIManager{};
Bool AppLinuxCommandLineArgs::VerboseMemoryTooling{};
String AppLinuxCommandLineArgs::VideoDir{};

SEOUL_BEGIN_TYPE(AppLinuxCommandLineArgs, TypeFlags::kDisableNew | TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(ScriptPreprocessorDirective, "SEOUL_PLATFORM_LINUX")
	SEOUL_CMDLINE_PROPERTY(AcceptGDPR, "accept_gdpr")
	SEOUL_CMDLINE_PROPERTY(EnableWarnings, "enable_warnings")
	SEOUL_CMDLINE_PROPERTY(FMODHeadless, "fmod_headless")
	SEOUL_CMDLINE_PROPERTY(FreeDiskAccess, "free_disk_access")
	SEOUL_CMDLINE_PROPERTY(DownloadablePackageFileSystemsEnabled, "dpkg_enable")
	SEOUL_CMDLINE_PROPERTY(PersistentTest, "persistent_test")
	SEOUL_CMDLINE_PROPERTY(RunUnitTests, "run_unit_tests", "test-options")
	SEOUL_CMDLINE_PROPERTY(RunAutomatedTest, "run_automated_test")
	SEOUL_CMDLINE_PROPERTY(UIManager, "ui_manager")
	SEOUL_CMDLINE_PROPERTY(VerboseMemoryTooling, "verbose_memory_tooling")
	SEOUL_CMDLINE_PROPERTY(VideoDir, "video_dir")
SEOUL_END_TYPE()

} // namespace Seoul
