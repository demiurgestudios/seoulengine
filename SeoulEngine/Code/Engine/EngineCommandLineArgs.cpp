/**
 * \file EngineCommandLineArgs.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EngineCommandLineArgs.h"
#include "ReflectionAttributes.h"
#include "ReflectionDefine.h"

namespace Seoul
{

String EngineCommandLineArgs::AutomationScript{};
String EngineCommandLineArgs::MoriartyServer{};
Bool EngineCommandLineArgs::NoCooking{};
Bool EngineCommandLineArgs::PreferUsePackageFiles{};

SEOUL_BEGIN_TYPE(EngineCommandLineArgs, TypeFlags::kDisableNew | TypeFlags::kDisableCopy)
	SEOUL_CMDLINE_PROPERTY(AutomationScript, "automation_script")
		SEOUL_ATTRIBUTE(Description, "builds with head, run with given automation behavior")
	SEOUL_CMDLINE_PROPERTY(MoriartyServer, "moriarty_server")
		SEOUL_ATTRIBUTE(Description, "connect to Moriarty server with given hostname")
	SEOUL_CMDLINE_PROPERTY(NoCooking, "no_cooking")
		SEOUL_ATTRIBUTE(Description, "disable all on-the-fly cooking functionality")
	SEOUL_CMDLINE_PROPERTY(PreferUsePackageFiles, "use_package_files")
		SEOUL_ATTRIBUTE(Description, "for non-Ship builds, use .sar files over loose files on disk")
SEOUL_END_TYPE()

} // namespace Seoul
