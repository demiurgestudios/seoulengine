/**
 * \file CompilerSettings.h
 * \brief Shared configuration of the SlimCS compiler.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COMPILER_SETTINGS_H
#define COMPILER_SETTINGS_H

#include "FilePath.h"
#include "Prereqs.h"
#include "SeoulString.h"
namespace Seoul { struct CookSource; }

namespace Seoul::CompilerSettings
{

// Return the global script project for the app.
// Can be an invalid FilePath if one is not
// configured.
FilePath GetApplicationScriptProjectFilePath();

// Runtime value that indicates whether the compiler
// is running in debug mode or not. Not a useful
// value in tools, only at runtime in applications.
Bool ApplicationIsUsingDebug();

// Populate rvArgs with the arguments necessary
// to invoke the SlimCS compiler.
typedef Vector<String, MemoryBudgets::TBD> ProcessArguments;
void GetCompilerProcessArguments(
	Platform ePlatform,
	FilePath projectFilePath,
	Bool bDebug,
	ProcessArguments& rvsArgs,
	String sOutputPath = String());

// Get the path to the SlimCS compiler process. May return
// an invalid FilePath if one is not available.
FilePath GetCompilerProcessFilePath();

void GetRootPaths(
	Platform ePlatform,
	FilePath projectFilePath,
	String& rsRootAuthored);
void GetRootPaths(
	Platform ePlatform,
	FilePath projectFilePath,
	String& rsRootAuthored,
	String& rsRootGenerated,
	String& rsRootGeneratedDebug);

typedef Vector<CookSource, MemoryBudgets::Cooking> Sources;
Bool GetSources(
	Bool bLuaInAuthored,
	Platform ePlatform,
	FilePath filePath,
	Sources& rv);

} // namespace Seoul::CompilerSettings

#endif  // include guard
