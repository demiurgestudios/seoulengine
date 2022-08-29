/**
 * \file EngineCommandLineArgs.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Prereqs.h"
#include "ReflectionDeclare.h"

namespace Seoul
{

/**
 * Engine level command-line arguments - handled by reflection, can be
 * configured via the literal command-line, environment variables, or
 * a configuration file.
 */
class EngineCommandLineArgs SEOUL_SEALED
{
public:
	static const String& GetAutomationScript() { return AutomationScript; }
	static const String& GetMoriartyServer() { return MoriartyServer; }
	static Bool GetNoCooking() { return NoCooking; }
	static Bool GetPreferUsePackageFiles() { return PreferUsePackageFiles; }

private:
	SEOUL_REFLECTION_FRIENDSHIP(EngineCommandLineArgs);

	static String AutomationScript;
	static String MoriartyServer;
	static Bool NoCooking;
	static Bool PreferUsePackageFiles;

	EngineCommandLineArgs() = delete; // Static class.
	SEOUL_DISABLE_COPY(EngineCommandLineArgs);
};

} // namespace Seoul
