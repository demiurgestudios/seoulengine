/**
 * \file UICookTask.cpp
 * \brief Cooking tasks for cooking UI .swf files into runtime
 * Falcon .fcn files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "Compress.h"
#include "CookDatabase.h"
#include "CookPriority.h"
#include "FalconFCNFile.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "Logger.h"
#include "ICookContext.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "SccIClient.h"
#include "ScopedAction.h"
#include "SeoulFileReaders.h"
#include "ToString.h"

namespace Seoul
{

namespace Cooking
{

typedef HashSet<FilePath, MemoryBudgets::Cooking> DepSet;

// Our manually set SWF minimum version. Falcon supports versions lower than
// this, we use this to force upgrade to newer version of Flash Animate.
static const Int32 kiMinimumSwfVersion = SEOUL_MIN_SWF_VERSION;

/** Relative path from tools directory for FalconCooker. */
#if SEOUL_PLATFORM_WINDOWS
static Byte const* ksFalconCooker = "FalconCooker.exe";
#else
static Byte const* ksFalconCooker = "FalconCooker";
#endif

static Bool SkipRectangleInSwf(SyncFile& r)
{
	UInt8 uNextByte = 0;
	if (!ReadUInt8(r, uNextByte))
	{
		SEOUL_LOG_COOKING("%s: dependency scan, failed reading first byte to skip reactangle of cooked SWF.", r.GetAbsoluteFilename().CStr());
		return false;
	}

	// The rectangle record is a little complex - the first 5 bits
	// are the # of bits used for each of the next 4 components of
	// the rectangle record, and the result is round up to be byte aligned.
	Int32 iShift = 7;
	Int32 iBit = 4;
	Int32 iBits = 0;
	while (iBit >= 0)
	{
		Bool bBit = ((((UInt32)uNextByte) >> iShift) & 1) == 1;
		if (bBit)
		{
			iBits |= (1 << iBit);
		}

		--iBit;
		--iShift;
	}

	// The size of the rectangle is 5 bits + 4 elements, which are
	// each the size of the # value in the first 5 bits. We've
	// read 1 byte, so the remaining is that total minus 8.
	Int32 iRemainingBits = (5 + (4 * iBits)) - 8;

	// Now skip the remaining bytes.
	while (iRemainingBits > 0)
	{
		UInt8 uUnused = 0;
		if (!ReadUInt8(r, uUnused))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed reading additional bytes to skip reactangle of cooked SWF.", r.GetAbsoluteFilename().CStr());
			return false;
		}
		iRemainingBits -= 8;
	}

	return true;
}

static Bool GatherDependenciesUIMovie(Platform ePlatform, FilePath filePath, DepSet& rSet)
{
	void* pU = nullptr;
	UInt32 uU = 0u;
	auto const deferredU(MakeDeferredAction([&]() { MemoryManager::Deallocate(pU); pU = nullptr; uU = 0u; }));
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		auto const deferredC(MakeDeferredAction([&]() { MemoryManager::Deallocate(pC); pC = nullptr; uC = 0u; }));
		if (!FileManager::Get()->ReadAll(filePath, pC, uC, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: GatherDependenciesUIMovie: failed reading UI Movie data from disk.", filePath.CStr());
			return false;
		}

		if (!ZSTDDecompress(pC, uC, pU, uU))
		{
			SEOUL_LOG_COOKING("%s: GatherDependenciesUIMovie: failed decompressing UI Movie data.", filePath.CStr());
			return false;
		}
	}

	Falcon::FCNFile::FCNDependencies vDependencies;
	if (!Falcon::FCNFile::GetFCNFileDependencies(filePath, pU, uU, vDependencies))
	{
		return false;
	}

	DepSet set;
	for (auto const& f : vDependencies)
	{
		if (!IsTextureFileType(f.GetType()))
		{
			continue;
		}

		set.Insert(f);
	}

	rSet.Swap(set);
	return true;
}

class UICookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(UICookTask);

	UICookTask()
	{
	}

	virtual Bool CanCook(FilePath filePath) const SEOUL_OVERRIDE
	{
		return (filePath.GetType() == FileType::kUIMovie);
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		ContentFiles v;
		if (!DefaultOutOfDateCook(rContext, FileType::kUIMovie, v))
		{
			return false;
		}

		return UpdateSourceImagesInSourceControl(rContext, !v.IsEmpty());
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kUI;
	}

private:
	SEOUL_DISABLE_COPY(UICookTask);

 	virtual Bool GetSources(ICookContext& rContext, FilePath filePath, Sources& rv) const SEOUL_OVERRIDE
	{
		// Gather image dependencies.
		DepSet set;
		if (!GatherDependenciesUIMovie(rContext.GetPlatform(), filePath, set))
		{
			SEOUL_LOG_COOKING("%s: failed gathering UI movie image dependencies for GetSources()", filePath.CStr());
			return false;
		}

		// Assemble sources.
		Sources v;
		v.Reserve(set.GetSize() + 1);

		// Add the base file.
		v.PushBack(CookSource{ filePath });

		// Source images files, extracted from .swf files and located in the
		// Generated*/UIImages folder.
		for (auto const& e : set)
		{
			v.PushBack(CookSource{ e });
		}

		// Out.
		rv.Swap(v);
		return true;
	}

	ProcessArguments GetFCNConvertArguments(
		Platform ePlatform,
		const String& sInput,
		const String& sOutput,
		const String& sUIImagesDirectory,
		const String& sInputOnlyUIImagesDirectory,
		const String& sImagePrefix,
		const String& sInOnlyImagePrefix) const
	{
		ProcessArguments vs;
		vs.PushBack(sInput);
		vs.PushBack("-o");
		vs.PushBack(sOutput);
		vs.PushBack("-img_dir");
		vs.PushBack(sUIImagesDirectory);
		if (!sInputOnlyUIImagesDirectory.IsEmpty())
		{
			vs.PushBack("-in_only_img_dir");
			vs.PushBack(sInputOnlyUIImagesDirectory);
		}
		vs.PushBack("-image_prefix");
		vs.PushBack(sImagePrefix);
		if (!sInOnlyImagePrefix.IsEmpty())
		{
			vs.PushBack("-in_only_image_prefix");
			vs.PushBack(sInOnlyImagePrefix);
		}
		vs.PushBack("-no_lossy");
		vs.PushBack("-min_swf_version");
		vs.PushBack(ToString(kiMinimumSwfVersion));

		return vs;
	}

	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath) SEOUL_OVERRIDE
	{
		auto const sFalconCooker(Path::Combine(rContext.GetToolsDirectory(), ksFalconCooker));
		auto const sTemporaryFile(Path::GetTempFileAbsoluteFilename());
		auto const scoped(MakeScopedAction([]() {}, [&]()
		{
			(void)FileManager::Get()->Delete(sTemporaryFile);
		}));

		auto const sGeneratedPrefix(Path::Combine(String(),
			String::Printf("Generated%s/UIImages/", EnumToString<Platform>(rContext.GetPlatform()))));
		auto const sGenerated(Path::Combine(GamePaths::Get()->GetSourceDir(), sGeneratedPrefix));

		String sImagePrefix;
		String sUIImagesDirectory;
		String sInOnlyImagePrefix;
		String sInputOnlyUIImagesDirectory;

		// If source control is enabled, we are the one true cooker,
		// so we only interact with the generated platform folder.
		if (!rContext.GetSourceControlClient().IsNull())
		{
			sImagePrefix = sGeneratedPrefix;
			sUIImagesDirectory = sGenerated;
		}
		// If source control is disabled, then we're a local cooker.
		// We cook to generate local but use the generated folder as an
		// input only source.
		else
		{
			sImagePrefix = Path::Combine(String(), "GeneratedLocal/UIImages/");
			sUIImagesDirectory = Path::Combine(GamePaths::Get()->GetSourceDir(), sImagePrefix);
			sInOnlyImagePrefix = sGeneratedPrefix;
			sInputOnlyUIImagesDirectory = sGenerated;
		}

		Bool bReturn = RunCommandLineProcess(
			sFalconCooker,
			GetFCNConvertArguments(
				rContext.GetPlatform(),
				filePath.GetAbsoluteFilenameInSource(),
				sTemporaryFile,
				sUIImagesDirectory,
				sInputOnlyUIImagesDirectory,
				sImagePrefix,
				sInOnlyImagePrefix));
		if (bReturn)
		{
			bReturn = WriteOutput(rContext, sTemporaryFile, filePath);
		}

		return bReturn;
	}

	/**
	 * This function searches all FCN files in the cooked folder defined
	 * in commandLine for texture dependencies, and then cross references
	 * those with the UI source image directory. Any images in the UI source image
	 * directory which are not in the dependency set of FCN files are marked for
	 * deleted in Perforce. All remaining images are marked for add, and are then
	 * amended to the cooker's active set.
	 *
	 * @return True if the dependency check completed without error, false otherwise.
	 * If this method returns true, rsDeletedImages will contain the list of
	 * deleted images. This list may be a 0-sized array if no images were deleted,
	 * but it will never be null.
	 *
	 * - a true value from this method does not mean images were deleted, it only
	 *   means no error was encountered while traversing for dependencies.
	 * - rasDeletedImages will be modified whether this method returns true or false. If this
	 *   method returns false, it will always be set to a zero-length array.
	 * - due to the nature of this method, it is best to run it at the very end
	 *   of a full directory cook before pruning cooked files in general.
	 */
	Bool UpdateSourceImagesInSourceControl(ICookContext& rContext, Bool bPossiblyHasNewImages) const
	{
		// Early out if source control  is null and bPossiblyHasNewImages is false.
		if (rContext.GetSourceControlClient().IsNull() && !bPossiblyHasNewImages)
		{
			return true;
		}

		// Options for source control operations.
		auto const& opt = rContext.GetSourceControlFileTypeOptions();

		// Cache the UI image directory - if no source control client, then
		// it's the local folder, otherwise it's platform determined.
		auto const sUIImageDirectoryRelative(rContext.GetSourceControlClient().IsNull()
			? Path::Combine("GeneratedLocal", "UIImages")
			: Path::Combine(GamePaths::GetGeneratedContentDirName(rContext.GetPlatform()), "UIImages"));
		auto const sUIImageDirectoryAbsolute(Path::Combine(
			GamePaths::Get()->GetSourceDir(),
			sUIImageDirectoryRelative));

		// Get a list of all FCN files in the cooked directory.
		auto const sFCNPath(GamePaths::Get()->GetContentDir());
		Vector<String> vsFCNFiles;
		if (!FileManager::Get()->GetDirectoryListing(
			sFCNPath,
			vsFCNFiles,
			false,
			true,
			FileTypeToCookedExtension(FileType::kUIMovie)))
		{
			SEOUL_LOG_COOKING("failed listing FCN files in directory \"%s\" for stale UI source image remove.", sFCNPath.CStr());
			return false;
		}

		// Now walk the list of FCN files and extract image dependencies from each.
		DepSet set;
		for (auto const& sFCNFile : vsFCNFiles)
		{
			auto const filePath(FilePath::CreateContentFilePath(sFCNFile));
			if (!GatherDependenciesUIMovie(rContext.GetPlatform(), filePath, set))
			{
				SEOUL_LOG_COOKING("%s: failed getting image dependencies for UI movie.", filePath.CStr());
				return false;
			}
		}

		// Generate a list of PNG files in the UI source folder.
		Vector<String> vsUIImages;
		if (!FileManager::Get()->GetDirectoryListing(
			sUIImageDirectoryAbsolute,
			vsUIImages,
			false,
			true,
			FileTypeToSourceExtension(FileType::kTexture0)))
		{
			SEOUL_LOG_COOKING("failed listing images in generated UI images directory \"%s\".", sUIImageDirectoryAbsolute.CStr());
			return false;
		}

		// Now update the list - move to delete entries to the head,
		// leave add entries at the end.
		Int32 iToDelete = (Int32)vsUIImages.GetSize();

		for (Int32 i = 0; i < iToDelete; ++i)
		{
			auto const filePath = FilePath::CreateContentFilePath(vsUIImages[i]);
			if (set.HasKey(filePath))
			{
				Swap(vsUIImages[i], vsUIImages[iToDelete - 1]);
				--iToDelete;
				--i;
			}
		}

		// If there are any entries to delete, mark them for delete in source control.
		if (iToDelete > 0)
		{
			// If the SCC operation fails, then the whole operation fails.
			if (!rContext.GetSourceControlClient().OpenForDelete(
				vsUIImages.Begin(),
				vsUIImages.Begin() + iToDelete,
				Scc::IClient::ErrorOutDelegate(),
				// We don't sync first, the Cooker handles syncing
				// Generated*/ source to head prior to all cooking start.
				false))
			{
				SEOUL_LOG_COOKING("UI image cleanup failed source control open for delete operation.");
				return false;
			}

			// Also remove these files from the context's working set of source files.
			if (!rContext.RemoveSourceFiles(vsUIImages.Begin(), vsUIImages.Begin() + iToDelete))
			{
				SEOUL_LOG_COOKING("UI image cleanup failed removing files from the context's working set.");
				return false;
			}
		}

		// Any add entries, process.
		if (iToDelete < (Int32)vsUIImages.GetSize())
		{
			// If the SCC operation fails, then the whole operation fails.
			if (!rContext.GetSourceControlClient().OpenForAdd(vsUIImages.Begin() + iToDelete, vsUIImages.End(), opt))
			{
				SEOUL_LOG_COOKING("UI image cleanup failed source control open for add operation.");
				return false;
			}

			// Also amend the add to the context's working set of source files.
			if (!rContext.AmendSourceFiles(vsUIImages.Begin() + iToDelete, vsUIImages.End()))
			{
				SEOUL_LOG_COOKING("UI image cleanup failed amending files to the context's working set.");
				return false;
			}
		}

		return true;
	}

	Bool WriteOutput(ICookContext& rContext, const String& sInput, FilePath filePath) const
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		auto const deferredC(MakeDeferredAction([&]()
			{
				MemoryManager::Deallocate(pC);
				pC = nullptr;
				uC = 0u;
			}));

		{
			void* p = nullptr;
			UInt32 u = 0u;
			auto const deferredU(MakeDeferredAction([&]()
				{
					MemoryManager::Deallocate(p);
					p = nullptr;
					u = 0u;
				}));

			if (!FileManager::Get()->ReadAll(sInput, p, u, 0u, MemoryBudgets::Cooking))
			{
				SEOUL_LOG_COOKING("%s: failed reading final file for UI cook", sInput.CStr());
				return false;
			}

			if (!ZSTDCompress(p, u, pC, uC, ZSTDCompressionLevel::kBest, MemoryBudgets::Cooking))
			{
				SEOUL_LOG_COOKING("%s: failed compressing UI data for UI cook.", sInput.CStr());
				return false;
			}
		}

		return AtomicWriteFinalOutput(rContext, pC, uC, filePath);
	}
}; // class UICookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::UICookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
