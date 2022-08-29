/**
 * \file ScriptProjectCookTask.cpp
 * \brief Cooking tasks for cooking SlimCS .csproj files into runtime
 * SeoulEnigne .csp files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "CompilerSettings.h"
#include "CookDatabase.h"
#include "CookPriority.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "ICookContext.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SccIClient.h"

namespace Seoul
{

namespace Cooking
{

class ScriptProjectCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScriptProjectCookTask);

	ScriptProjectCookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kScriptProject);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kScriptProject, v))
		{
			return false;
		}

		return true;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kScriptProject;
	}

private:
	SEOUL_DISABLE_COPY(ScriptProjectCookTask);

	virtual Bool GetSources(ICookContext& rContext, FilePath filePath, Sources& rv) const SEOUL_OVERRIDE
	{
		return CompilerSettings::GetSources(false, rContext.GetPlatform(), filePath, rv);
	}

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		// Skip file cooking if the only dependency that changed was the project file itself,
		// since the project file is just a placeholder.
		auto bCookFiles = true;
		CookDatabase::Dependents vDetails;
		if (rContext.GetDatabase().CheckUpToDateWithDetails(filePath, vDetails) ||
			(vDetails.GetSize() == 1u && vDetails.Front() == filePath))
		{
			bCookFiles = false;
		}

		// Cook files unless the only change was to the project file itself.
		if (bCookFiles)
		{
			// Tracking.
			auto const bDebugOnly = rContext.GetCookDebugOnly();

			// Operations.
			Vector<String> vsDirsLocal, vsDirsSourceControl;
			// Get the roots.
			String sRootCS;
			String sRootLua;
			String sRootLuaDebug;
			CompilerSettings::GetRootPaths(rContext.GetPlatform(), filePath, sRootCS, sRootLua, sRootLuaDebug);
			if (!bDebugOnly)
			{
				vsDirsLocal.PushBack(sRootLua);
				vsDirsSourceControl.PushBack(sRootLua + "...");
			}
			vsDirsLocal.PushBack(sRootLuaDebug);
			vsDirsSourceControl.PushBack(sRootLuaDebug + "...");

			// Source control modifications, open for edit now.
			if (!rContext.GetSourceControlClient().OpenForEdit(
				vsDirsSourceControl.Begin(),
				vsDirsSourceControl.End(),
				Scc::FileTypeOptions(),
				Scc::IClient::ErrorOutDelegate(),
				// We don't sync first, the Cooker handles syncing
				// Generated*/ source to head prior to all cooking start.
				false))
			{
				SEOUL_LOG_COOKING("%s: failed opening files for edit during SlimCS compile.", filePath.CStr());
				return false;
			}

			// Perform the cooks.
			for (auto const& sRoot : vsDirsLocal)
			{
				if (!CookSlimCS(rContext, filePath, (sRoot == sRootLuaDebug), sRoot)) { return false; }
			}

			// Finish source control and local caching operations now.

			// Revert unchanged current status.
			if (!rContext.GetSourceControlClient().RevertUnchanged(
				vsDirsSourceControl.Begin(),
				vsDirsSourceControl.End()))
			{
				SEOUL_LOG_COOKING("%s: failed revert unchanged during SlimCS compile.", filePath.CStr());
				return false;
			}

			// Gather all .cs files.
			Vector<String> vsCsharp;
			if (FileManager::Get()->IsDirectory(sRootCS) &&
				!FileManager::Get()->GetDirectoryListing(
					sRootCS,
					vsCsharp,
					false,
					true,
					FileTypeToSourceExtension(FileType::kCs)))
			{
				SEOUL_LOG_COOKING("%s: failed gathering source .cs for remove processing.", filePath.CStr());
				return false;
			}

			// Build a query set.
			HashSet<FilePath, MemoryBudgets::Cooking> set;
			for (auto const& s : vsCsharp)
			{
				// For each dir.
				for (auto const& sRoot : vsDirsLocal)
				{
					// .cs to .lua conversion in generated folders.
					auto filePath(FilePath::CreateContentFilePath(
						Path::Combine(sRoot, s.Substring(sRootCS.GetSize()))));
					filePath.SetType(FileType::kScript);
					(void)set.Insert(filePath);
				}
			}

			// Special case, SlimCSFiles.lua, purely compiled generated.
			for (auto const& sRoot : vsDirsLocal)
			{
				(void)set.Insert(FilePath::CreateContentFilePath(Path::Combine(sRoot, "SlimCSFiles.lua")));
			}

			// Now iterate over all .lua files in the two generate folders, and track any
			// that don't exist in set for removal and any that do exist for add.
			//
			// NOTE: We lean on Perforce to both ignore redundant adds and AmendSourceFiles()
			// to do the same (we just add every file that exist, even those that may
			// already be opened for edit).
			Vector<String> vsToAdd;
			Vector<String> vsToRemove;
			// For each dir.
			for (auto const& sRoot : vsDirsLocal)
			{
				Vector<String> vs;
				if (FileManager::Get()->IsDirectory(sRoot) &&
					!FileManager::Get()->GetDirectoryListing(
						sRoot,
						vs,
						false,
						true,
						FileTypeToSourceExtension(FileType::kScript)))
				{
					SEOUL_LOG_COOKING("%s: failed gathering .lua in %s for remove processing.", filePath.CStr(), sRoot.CStr());
					return false;
				}

				// No process.
				for (auto const& s : vs)
				{
					if (!set.HasKey(FilePath::CreateContentFilePath(s)))
					{
						vsToRemove.PushBack(s);
					}
					else
					{
						vsToAdd.PushBack(s);
					}
				}
			}

			// Issue if we have any files at all (to potentially add, will be filtered
			// for redundant adds.
			if (!vsToAdd.IsEmpty())
			{
				if (!rContext.GetSourceControlClient().OpenForAdd(
					vsToAdd.Begin(),
					vsToAdd.End()))
				{
					SEOUL_LOG_COOKING("%s: failed opening some files for add during SlimCS compile.", filePath.CStr());
					return false;
				}

				// Also add these files from the context's working set of source files.
				// We rely on rContext to filter.
				if (!rContext.AmendSourceFiles(
					vsToAdd.Begin(),
					vsToAdd.End()))
				{
					SEOUL_LOG_COOKING("%s: failed amending some files to the working set during SlimCS compile.", filePath.CStr());
					return false;
				}
			}

			// Issue if we have some files to remove.
			if (!vsToRemove.IsEmpty())
			{
				if (!rContext.GetSourceControlClient().OpenForDelete(
					vsToRemove.Begin(),
					vsToRemove.End(),
					Scc::IClient::ErrorOutDelegate(),
					// We don't sync first, the Cooker handles syncing
					// Generated*/ source to head prior to all cooking start.
					false))
				{
					SEOUL_LOG_COOKING("%s: failed opening some files for remove during SlimCS compile.", filePath.CStr());
					return false;
				}

				// Also remove these files from the context's working set of source files.
				if (!rContext.RemoveSourceFiles(
					vsToRemove.Begin(),
					vsToRemove.End()))
				{
					SEOUL_LOG_COOKING("%s: failed removing some files from the working set during SlimCS compile.", filePath.CStr());
					return false;
				}
			}
		}

		// Final step, write the actual "project file". This is just an empty file
		// at present, it's entirely a placeholder for loading and hot loading.
		return AtomicWriteFinalOutput(rContext, nullptr, 0u, filePath);
	}

	Bool CookSlimCS(
		ICookContext& rContext,
		FilePath filePath,
		Bool bDebug,
		const String& sOutputRoot)
	{
		// Gather arguments.
		ProcessArguments vsArguments;
		CompilerSettings::GetCompilerProcessArguments(
			rContext.GetPlatform(),
			filePath,
			bDebug,
			vsArguments,
			sOutputRoot);

		// Run the process.
		if (!RunCommandLineProcess(
			CompilerSettings::GetCompilerProcessFilePath().GetAbsoluteFilename(),
			vsArguments))
		{
			return false;
		}

		return true;
	}
}; // class ScriptProjectCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::ScriptProjectCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
