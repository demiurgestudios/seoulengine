/**
 * \file CompilerSettings.cpp
 * \brief Shared configuration of the SlimCS compiler.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ApplicationJson.h"
#include "BuildDistroPublic.h"
#include "CompilerSettings.h"
#include "CookDatabase.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Path.h"

namespace Seoul::CompilerSettings
{

static const HString kScriptProject("ScriptProject");
static const char* kSlimCSExeFilename = "SlimCS.exe";

/**
 * @return the global script project for the app.
 *
 * Can be an invalid FilePath if one is not
 * configured.
 */
FilePath GetApplicationScriptProjectFilePath()
{
	// Check that the application has a configured scripts
	// project. If not, no compiler service.
	FilePath projectFilePath;
	if (!GetApplicationJsonValue(kScriptProject, projectFilePath))
	{
		return FilePath();
	}

	return projectFilePath;
}

/**
 * Runtime value that indicates whether the compiler
 * is running in debug mode or not. Not a useful
 * value in tools, only at runtime in applications.
 */
Bool ApplicationIsUsingDebug()
{
#if SEOUL_SHIP
	return false;
#else
	return true;
#endif
}

/**
 * Populate rvArgs with the arguments necessary
 * to invoke the SlimCS compiler.
 */
void GetCompilerProcessArguments(
	Platform ePlatform,
	FilePath projectFilePath,
	Bool bDebug,
	ProcessArguments& rvsArgs,
	String sOutputPath /*= String()*/)
{
	String sRootAuthored;
	String sRootGenerated;
	String sRootGeneratedDebug;
	GetRootPaths(ePlatform, projectFilePath, sRootAuthored, sRootGenerated, sRootGeneratedDebug);

	if (sOutputPath.IsEmpty())
	{
		sOutputPath = (bDebug ? sRootGeneratedDebug : sRootGenerated);
	}

	ProcessArguments vs;
	vs.PushBack(sRootAuthored);
	vs.PushBack(sOutputPath);
	vs.PushBack(bDebug ? "-DDEBUG" : "-DNDEBUG");
#if SEOUL_WITH_ANIMATION_2D
	vs.PushBack("-DSEOUL_WITH_ANIMATION_2D");
#endif // /#if SEOUL_WITH_ANIMATION_2D
	vs.PushBack(String::Printf("-D%s", kaPlatformMacroNames[(UInt32)ePlatform]));
	vs.PushBack(g_kbBuildForDistribution ? "-DSEOUL_BUILD_FOR_DISTRIBUTION" : "-DSEOUL_BUILD_NOT_FOR_DISTRIBUTION");
	rvsArgs.Swap(vs);
}

/**
 * Get the path to the SlimCS compiler process. May return
 * an invalid FilePath if one is not available.
 */
FilePath GetCompilerProcessFilePath()
{
	auto const filePath(FilePath::CreateToolsBinFilePath(kSlimCSExeFilename));
	if (!FileManager::Get()->Exists(filePath))
	{
		return FilePath();
	}

	return filePath;
}

static inline UInt Filter(UInt u) { return (String::NPos == u ? 0u : u); }

/**
* Utility, based on a project path, derives input path, output path, and output path
* for debug generation.
*/
void GetRootPaths(
	Platform ePlatform,
	FilePath projectFilePath,
	String& rsRootAuthored)
{
	// Base relative path.
	auto const sRootRelativeAuthored(Path::GetDirectoryName(
		projectFilePath.GetRelativeFilenameWithoutExtension().ToString()));

	// Output as absolute paths - we terminate with the slash to make relative
	// operations easier later.
	auto const& sSourceRoot = GamePaths::Get()->GetSourceDir();
	rsRootAuthored = Path::Combine(sSourceRoot, sRootRelativeAuthored + Path::DirectorySeparatorChar());
}
void GetRootPaths(
	Platform ePlatform,
	FilePath projectFilePath,
	String& rsRootAuthored,
	String& rsRootGenerated,
	String& rsRootGeneratedDebug)
{
	// Base relative path.
	auto const sRootRelativeAuthored(Path::GetDirectoryName(
		projectFilePath.GetRelativeFilenameWithoutExtension().ToString()));

	// Replace Authored with the Generated<Platform> folder.
	auto const sGenerated(String(GamePaths::GetGeneratedContentDirName(ePlatform)));
	auto const uFirstSlash = Filter(sRootRelativeAuthored.Find(Path::DirectorySeparatorChar()));
	auto const sRootRelativeLua(sGenerated + (sRootRelativeAuthored.CStr() + uFirstSlash));

	// Finally, append "Debug" to the relative Lua path for debug scripts.
	auto const sRootRelativeLuaDebug = sRootRelativeLua + "Debug";

	// Output as absolute paths - we terminate with the slash to make relative
	// operations easier later.
	auto const& sSourceRoot = GamePaths::Get()->GetSourceDir();
	rsRootAuthored = Path::Combine(sSourceRoot, sRootRelativeAuthored + Path::DirectorySeparatorChar());
	rsRootGenerated = Path::Combine(sSourceRoot, sRootRelativeLua + Path::DirectorySeparatorChar());
	rsRootGeneratedDebug = Path::Combine(sSourceRoot, sRootRelativeLuaDebug + Path::DirectorySeparatorChar());
}

Bool GetSources(Bool bLuaInAuthored, Platform ePlatform, FilePath filePath, Sources& rv)
{
	// Dependencies are the project file itself, as well as
	// any .cs files in its directory structure.
	//
	// The project also depends on the generated output paths,
	// which are mapped to the Generated[Platform] root path for
	// and either the base path, or the base path + "Debug" for
	// debug generated scripts.
	String sRootAuthored;
	String sRootGenerated;
	String sRootGeneratedDebug;
	CompilerSettings::GetRootPaths(ePlatform, filePath, sRootAuthored, sRootGenerated, sRootGeneratedDebug);

	auto const eAuthoredType = (bLuaInAuthored ? FileType::kScript : FileType::kCs);
	Vector<String> vsAuthored;
	if (FileManager::Get()->IsDirectory(sRootAuthored) &&
		!FileManager::Get()->GetDirectoryListing(
			sRootAuthored,
			vsAuthored,
			false,
			true,
			FileTypeToSourceExtension(eAuthoredType)))
	{
		SEOUL_LOG_COOKING("%s: failed enumerating root directory to get SlimCS project authored sources list.",
			filePath.CStr());
		return false;
	}

	Vector<String> vsGenerated;
	if (FileManager::Get()->IsDirectory(sRootGenerated) &&
		!FileManager::Get()->GetDirectoryListing(
			sRootGenerated,
			vsGenerated,
			false,
			true,
			FileTypeToSourceExtension(FileType::kScript)))
	{
		SEOUL_LOG_COOKING("%s: failed enumerating root directory to get SlimCS project generated sources list.",
			filePath.CStr());
		return false;
	}

	Vector<String> vsGeneratedDebug;
	if (FileManager::Get()->IsDirectory(sRootGeneratedDebug) &&
		!FileManager::Get()->GetDirectoryListing(
			sRootGeneratedDebug,
			vsGeneratedDebug,
			false,
			true,
			FileTypeToSourceExtension(FileType::kScript)))
	{
		SEOUL_LOG_COOKING("%s: failed enumerating root directory to get SlimCS project generated (debug) sources list.",
			filePath.CStr());
		return false;
	}

	rv.Reserve(vsAuthored.GetSize() + vsGenerated.GetSize() + vsGeneratedDebug.GetSize() + 4u);

	// Project file.
	rv.PushBack(CookSource{ filePath, false });

	// We add the root paths as directory sources. Appropriate
	// extensions in each.
	{
		auto dirPath = FilePath::CreateContentFilePath(sRootAuthored);
		dirPath.SetType(eAuthoredType);
		rv.PushBack(CookSource{ dirPath, true });
	}
	{
		auto dirPath = FilePath::CreateContentFilePath(sRootGenerated);
		dirPath.SetType(FileType::kScript);
		rv.PushBack(CookSource{ dirPath, true });
	}
	{
		auto dirPath = FilePath::CreateContentFilePath(sRootGeneratedDebug);
		dirPath.SetType(FileType::kScript);
		rv.PushBack(CookSource{ dirPath, true });
	}

	// All authored files, generated files, and generated (debug) files.
	for (auto const& s : vsAuthored)
	{
		// Currently, all lua sources in authored are considered debug only.
		rv.PushBack(CookSource{ FilePath::CreateContentFilePath(s), false, bLuaInAuthored });
	}
	for (auto const& s : vsGenerated)
	{
		rv.PushBack(CookSource{ FilePath::CreateContentFilePath(s), false, false });
	}
	for (auto const& s : vsGeneratedDebug)
	{
		rv.PushBack(CookSource{ FilePath::CreateContentFilePath(s), false, true });
	}

	return true;
}

} // namespace Seoul::CompilerSettings
