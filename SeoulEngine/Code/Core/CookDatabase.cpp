/**
 * \file CookDatabase.cpp
 * \brief Utility class that monitors cookable files. Used to
 * determine if a file needs to be recooked, as well as to
 * query certain properties about cookable files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CookDatabase.h"
#include "DataStore.h"
#include "DataStoreParser.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "ScopedAction.h"
#include "SeoulFileReaders.h"
#include "SeoulFileWriters.h"

namespace Seoul
{

/** Placeholder for an empty String. */
static const String ksEmpty;

/**
 * Version of the cooker tool. Advance if the tool changes completely,
 * in which case all assets will need to be recooked.
 * NOTE: there are now per-type settings for this, you probably want to
 * change those instead.
 */
static const UInt32 kuCookerVersion = 39u;

/**
 * Data tracking version. Not typically in-sync with any
 * version internal to its file format, used for
 * cook checking specifically.
 */
static const UInt32 kauDataVersions[(UInt32)FileType::FILE_TYPE_COUNT] =
{
	1u,  // kUnknown,
	10u, // kAnimation2D,
	1u,  // kCsv,
	1u,  // kEffect,
	1u,  // kEffectHeader,
	1u,  // kExe,
	7u,  // kFont,
	3u,  // kFxBank,
	1u,  // kHtml,
	1u,  // kJson,
	1u,  // kPEMCertificate,
	1u,  // kProtobuf,
	1u,  // kSaveGame,
	2u,  // kSceneAsset,
	2u,  // kScenePrefab,
	7u,  // kScript,
	13u, // kSoundBank,
	13u, // kSoundProject,
	3u,  // kTexture0,
	1u,  // kTexture1,
	1u,  // kTexture2,
	1u,  // kTexture3,
	1u,  // kTexture4,
	1u,  // kText,
	9u,  // kUIMovie,
	1u,  // kWav,
	1u,  // kXml,
	// Force a version change on branching to factor in new compilation macros.
	(SEOUL_BUILD_FOR_DISTRIBUTION << 16u) | 7u, // kScriptProject,
	1u,  // kCs,
	1u,  // kVideo,
};

/**
 * The vast majority of cooked files are "one-to-one" - a single source
 * file generates a single cooked file. For these files, there is no metadata
 * file. The modified times necessary to determine whether a cooked file
 * is up-to-date is always read directly from the source and cooked files (assuming
 * file system caching in Seoul Engine and at the OS level, which makes this
 * a significantly faster check than reading an additional metadata file).
 *
 * To maintain and apply versioning at the cooker level, there is a global "versions"
 * file that the cooker database loads and maintains for each platform. This file
 * is local only (it is not stored in any .sar nor submitted to source control) and
 * is handled as follows:
 * - if it does not exist at all, it is generated from current compiled constants.
 * - if it does exist but is invalid or corrupt, it is treated in the manner of the
 *   prior bullet (generally, we assume files are up-to-date rather than not, since
 *   the consequences of an accidental recook are generally worse than a lack of a recook,
 *   since the latter can be fixed by just changing the data version again).
 * - if the file exists:
 *   - if all versions match, nothing occurs.
 *   - if there is a cooker or data mismatch for a file type, all on-disk files of that type
 *     are deleted from disk. If this operation is successful, then the version is updated
 *     and the file is re-saved.
 */
Bool CookDatabase::IsOneToOneType(FileType eType)
{
	// Only script projects, audio projects, and shader FX files are not one-to-one types.
	switch (eType)
	{
		// NOTE: Conceptually, an effect header also has dependencies, but cooked Effect metadata
		// "flattens" the entire dependency graph - it includes not only any direct includes, but
		// also any includes of those includes, and so on. As a result, only the root Effect (.fx)
		// file itself is a many-to-one dependency type.
	case FileType::kEffect:
		return false;
		// Script projects depend on many .cs files.
	case FileType::kScriptProject:
		return false;
		// Sound projects are the most complex, since the "sound project" is really a stub for the
		// entire directly of Xml and Wav files in an FMOD Studio folder. Effectively, sound projects
		// are many-to-many that we identify as one-to-many by tieing the project to its "sibling"
		// cooked files, which are .bank files.
	case FileType::kSoundProject:
		return false;
		// UIMovies have a source dependency on their generated .png files (the generated .png files
		// are extracted from .swf files, image data is internal to .swf). Under ideal conditions
		// this dependency is not necessary, but if a .png is removed outside the Cooker's control
		// (e.g. due to a source control submission failure), then it is necessary for us to
		// recook any dependent UIMovie files.
	case FileType::kUIMovie:
		return false;

	default:
		return true;
	};
};

UInt32 CookDatabase::GetCookerVersion()
{
	return kuCookerVersion;
}

UInt32 CookDatabase::GetDataVersion(FileType eType)
{
	auto const iIndex = (Int32)eType;
	if (iIndex >= 0 && iIndex < (Int32)SEOUL_ARRAY_COUNT(kauDataVersions))
	{
		return kauDataVersions[iIndex];
	}

	return 0u;
}

// Fields in the metadata .json file.
static const HString kCookedTimestamp("CookedTimestamp");
static const HString kCookerVersion("CookerVersion");
static const HString kDataVersion("DataVersion");
static const HString kDirectorySources("DirectorySources");
static const HString kFileCount("FileCount");
static const HString kSiblings("Siblings");
static const HString kSource("Source");
static const HString kSources("Sources");
static const HString kTimestamp("Timestamp");

/** Utility for equality comparisons between dependent and cooked files. */
static inline FilePath Normalize(FilePath filePath)
{
	if (IsTextureFileType(filePath.GetType()))
	{
		filePath.SetType(FileType::kTexture0);
	}
	return filePath;
}

static inline FilePath GetMetadataPath(FilePath filePath)
{
	FilePath ret;
	ret.SetDirectory(filePath.GetDirectory());
	// NOTE: include entire filename, *with* extension, than convert to a kJson
	// file type to resolve the metadata filename.
	ret.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(filePath.GetRelativeFilename()));
	ret.SetType(FileType::kJson);
	return ret;
}

/** Single file that stores version data for all one-to-one content files. */
static inline FilePath GetOneToOneVersionDataFilePath()
{
	return FilePath::CreateContentFilePath("version_data.dat");
}

/** Utility, used to count the number of files (recursively) in a given directory path. */
static inline UInt32 GetDirectoryFileCount(FilePath inDirPath)
{
	// We cheat a bit and allow these to have a type - that's
	// used to select the extension. We have to strip it
	// from the dirPath prior to issuing
	// the listing though, or it will be misinterpreted
	// as an extension.
	auto dirPath = inDirPath;
	auto const eType = dirPath.GetType();
	dirPath.SetType(FileType::kUnknown);

	Vector<String> vs;
	if (!FileManager::Get()->GetDirectoryListing(
		dirPath.GetAbsoluteFilenameInSource(),
		vs,
		false,
		true,
		(FileType::kUnknown == eType ? String() : FileTypeToSourceExtension(eType))))
	{
		return 0u;
	}

	return vs.GetSize();
}

// Configuration.
static const UInt32 kuFilePathFileTypeBits = 5u;
static const UInt32 kuFilePathGameDirectoryBits = 3u;
static const UInt32 kuFilePathRelativeFilenameBits = 19u;

namespace
{

struct FilePathConverter SEOUL_SEALED
{
	struct Parts
	{
		UInt32 m_uKeyType : 2;
		UInt32 m_uDirectory : 3;
		UInt32 m_uRelativeFilename : 19;
		UInt32 m_uType : 5;
		UInt32 m_uReserved : 3;
	};

	union
	{
		Parts m_Parts;
		UInt32 m_uBits;
	};
};
SEOUL_STATIC_ASSERT(sizeof(FilePathConverter) == sizeof(UInt32));

} // namespace

// Sanity checks.
SEOUL_STATIC_ASSERT((UInt32)FileType::FILE_TYPE_COUNT <= (1 << kuFilePathFileTypeBits));
SEOUL_STATIC_ASSERT((UInt32)GameDirectory::GAME_DIRECTORY_COUNT <= (1 << kuFilePathGameDirectoryBits));
SEOUL_STATIC_ASSERT(HStringDataProperties<HStringData::InternalIndexType>::GlobalArraySize <= (1 << kuFilePathRelativeFilenameBits));

CookDatabase::CookDatabase(Platform ePlatform, Bool bProcessOneToOneVersions)
	: m_ePlatform(ePlatform)
	, m_Mutex()
	, m_tUpToDate()
	, m_tDep()
	, m_tDepDir()
	, m_tMetadata()
	, m_pContentNotifier()
{
	// Before hooking up a notifier, since we may potentially delete a bunch
	// of files, process one-to-one version data.
	if (bProcessOneToOneVersions)
	{
		ProcessOneToOneVersions();
	}

	m_pContentNotifier.Reset(SEOUL_NEW(MemoryBudgets::Cooking) FileChangeNotifier(
		GamePaths::Get()->GetSourceDir(),
		SEOUL_BIND_DELEGATE(&CookDatabase::OnFileChange, this),
		FileChangeNotifier::kChangeFileName |
		FileChangeNotifier::kChangeSize |
		FileChangeNotifier::kChangeLastWrite |
		FileChangeNotifier::kChangeCreation));
}

CookDatabase::~CookDatabase()
{
	m_pContentNotifier.Reset();
}

/**
 * Return true if the cooked version of the given
 * file is up-to-date.
 *
 * NOTE: This function may read or re-read metadata
 * from disk (synchronously), so it is not recommended
 * to call it from within time sensitive functions.
 */
Bool CookDatabase::CheckUpToDate(FilePath filePath)
{
	Lock lock(m_Mutex);

	// Early out if we've already verified that the given path is
	// up to date or not.
	{
		Bool bUpToDate = false;
		if (m_tUpToDate.GetValue(filePath, bUpToDate))
		{
			return bUpToDate;
		}
	}

	// Simple case.
	if (IsOneToOneType(filePath.GetType()))
	{
		auto const uCooked = FileManager::Get()->GetModifiedTimeForPlatform(m_ePlatform, filePath);
		auto const uSource = FileManager::Get()->GetModifiedTimeInSource(filePath);
		if (uCooked == uSource)
		{
			(void)m_tUpToDate.Overwrite(filePath, true);
			return true;
		}
		else
		{
			return false;
		}
	}

	auto const& metadata = InsideLockResolveMetadata(filePath);

	// Check for cooker consistency.
	if (metadata.m_uCookerVersion != kuCookerVersion)
	{
		return false;
	}

	// Check for data version consistency.
	if (metadata.m_uDataVersion != kauDataVersions[(UInt32)filePath.GetType()])
	{
		return false;
	}

	// Check cooked timestamp.
	if (metadata.m_uCookedTimestamp != FileManager::Get()->GetModifiedTimeForPlatform(m_ePlatform, filePath))
	{
		return false;
	}

	// Check timestamps of siblings.
	for (auto const& e : metadata.m_vSiblings)
	{
		if (e.m_uTimestamp != FileManager::Get()->GetModifiedTimeForPlatform(m_ePlatform, e.m_Source))
		{
			return false;
		}
	}

	// Check source timestamps.
	for (auto const& e : metadata.m_vSources)
	{
		if (e.m_uTimestamp != FileManager::Get()->GetModifiedTimeInSource(e.m_Source))
		{
			return false;
		}
	}

	// Check source directory counts.
	for (auto const& e : metadata.m_vDirectorySources)
	{
		if (e.m_uFileCount != GetDirectoryFileCount(e.m_Source))
		{
			return false;
		}
	}

	// Success, up-to-date - insert the FilePath in the set to speed
	// up future up-to-date calls.
	(void)m_tUpToDate.Overwrite(filePath, true);
	return true;
}

/**
 * Equivalent to CheckUpToDate(), except also populates
 * rvOut with the list of FilePaths that have changed
 * when not up to date. Note that this makes this function
 * quite a bit more expensive than CheckUpToDate(),
 * so it should only be called when the extra information
 * is actually needed.
 *
 * NOTE: rvOut may be empty, in which case a version mismatch
 * or some other "global" case has occured, in which case
 * the caller should assume all dependencies are out-of-date.
 */
Bool CookDatabase::CheckUpToDateWithDetails(FilePath filePath, Dependents& rvOut)
{
	Lock lock(m_Mutex);

	// Early out if we have data and we're up to date, since
	// we don't need to populate rvOut in this case.
	{
		Bool bUpToDate = false;
		if (m_tUpToDate.GetValue(filePath, bUpToDate) && bUpToDate)
		{
			return bUpToDate;
		}
	}

	// Simple case.
	if (IsOneToOneType(filePath.GetType()))
	{
		auto const uCooked = FileManager::Get()->GetModifiedTimeForPlatform(m_ePlatform, filePath);
		auto const uSource = FileManager::Get()->GetModifiedTimeInSource(filePath);
		auto const b = (uCooked == uSource);
		if (!b)
		{
			rvOut.PushBack(filePath);
		}
		else
		{
			// Success, up-to-date - insert the FilePath in the set to speed
			// up future up-to-date calls.
			(void)m_tUpToDate.Overwrite(filePath, true);
		}

		return b;
	}

	// Start populate.
	Dependents vOut;

	auto const& metadata = InsideLockResolveMetadata(filePath);

	// Check for cooker consistency.
	if (metadata.m_uCookerVersion != kuCookerVersion)
	{
		rvOut.Swap(vOut);
		return false;
	}

	// Check for data version consistency.
	if (metadata.m_uDataVersion != kauDataVersions[(UInt32)filePath.GetType()])
	{
		rvOut.Swap(vOut);
		return false;
	}

	// Check cooked timestamp.
	if (metadata.m_uCookedTimestamp != FileManager::Get()->GetModifiedTimeForPlatform(m_ePlatform, filePath))
	{
		rvOut.Swap(vOut);
		return false;
	}

	// From here on, we need to populate, so track return value.
	auto bReturn = true;

	// Check timestamps of siblings.
	for (auto const& e : metadata.m_vSiblings)
	{
		if (e.m_uTimestamp != FileManager::Get()->GetModifiedTimeForPlatform(m_ePlatform, e.m_Source))
		{
			vOut.PushBack(e.m_Source);
			bReturn = false;
		}
	}

	// Check source timestamps.
	for (auto const& e : metadata.m_vSources)
	{
		if (e.m_uTimestamp != FileManager::Get()->GetModifiedTimeInSource(e.m_Source))
		{
			vOut.PushBack(e.m_Source);
			bReturn = false;
		}
	}

	// Check source directory counts.
	for (auto const& e : metadata.m_vDirectorySources)
	{
		if (e.m_uFileCount != GetDirectoryFileCount(e.m_Source))
		{
			vOut.PushBack(e.m_Source);
			bReturn = false;
		}
	}

	if (bReturn)
	{
		// Success, up-to-date - insert the FilePath in the set to speed
		// up future up-to-date calls.
		(void)m_tUpToDate.Overwrite(filePath, true);
	}

	// Populate and return.
	rvOut.Swap(vOut);
	return bReturn;
}

/**
 * Output a vector of any output files that
 * need to be recooked if the give source file changes.
 */
void CookDatabase::GetDependents(FilePath filePath, Dependents& rvDependents) const
{
	Lock lock(m_Mutex);
	InsideLockGetDependents(filePath, rvDependents);
}

#if SEOUL_UNIT_TESTS
Bool CookDatabase::UnitTestHook_GetMetadata(FilePath filePath, CookMetadata& rMetadata)
{
	Lock lock(m_Mutex);
	rMetadata = InsideLockResolveMetadata(filePath);
	return (0 != rMetadata.m_uMetadataTimestamp);

}
#endif // /SEOUL_UNIT_TEST

/**
 * Update the metdata for a given file. Commits the data to disk.
 */
void CookDatabase::UpdateMetadata(
	FilePath filePath,
	UInt64 uCookedTimestamp,
	CookSource const* pBeginSources,
	CookSource const* pEndSources)
{
	// Simple case, no tracking metadata for a one-to-one type.
	if (IsOneToOneType(filePath.GetType()))
	{
		return;
	}

	Lock lock(m_Mutex);

	CookMetadata metadata;
	metadata.m_uCookedTimestamp = uCookedTimestamp;
	metadata.m_uCookerVersion = kuCookerVersion;
	metadata.m_uDataVersion = kauDataVersions[(UInt32)filePath.GetType()];
	for (auto p = pBeginSources; pEndSources != p; ++p)
	{
		// Directory source.
		auto tmpFilePath = p->m_FilePath;
		// Normalize for consistency sake.
		if (IsTextureFileType(tmpFilePath.GetType())) { tmpFilePath.SetType(FileType::kTexture0); }
		if (p->m_bDirectory)
		{
			CookMetadata::DirectorySource entry;
			entry.m_Source = tmpFilePath;
			entry.m_uFileCount = GetDirectoryFileCount(tmpFilePath);
			metadata.m_vDirectorySources.PushBack(entry);
		}
		else
		{
			CookMetadata::Source entry;
			entry.m_Source = tmpFilePath;

			// Sibling dependency, in same directory as output.
			if (p->m_bSibling)
			{
				entry.m_uTimestamp = FileManager::Get()->GetModifiedTimeForPlatform(m_ePlatform, tmpFilePath);
				metadata.m_vSiblings.PushBack(entry);
			}
			// Typical source dependency.
			else
			{
				entry.m_uTimestamp = FileManager::Get()->GetModifiedTimeInSource(tmpFilePath);
				metadata.m_vSources.PushBack(entry);
			}
		}
	}

	DataStore ds;
	ds.MakeTable();
	if (CommitSingleMetadata(metadata, ds, ds.GetRootNode()) &&
		WriteSingleMetadataToDisk(m_ePlatform, filePath, ds, ds.GetRootNode()))
	{
		metadata.m_uMetadataTimestamp = FileManager::Get()->GetModifiedTimeForPlatform(m_ePlatform, GetMetadataPath(filePath));

		// Remove filePath from the existing metadata.
		InsideLockRemoveDependents(filePath);
		SEOUL_VERIFY(m_tMetadata.Overwrite(filePath, metadata).Second);
		InsideLockAddDependents(filePath, metadata, m_tDep, m_tDepDir);

		// A successful written metadata is also up-to-date.
		(void)m_tUpToDate.Overwrite(filePath, true);
	}
}

Bool CookDatabase::CommitSingleMetadata(const CookMetadata& metadata, DataStore& ds, const DataNode& root)
{
	Bool bOk = true;
	bOk = bOk && ds.SetUInt64ValueToTable(root, kCookedTimestamp, metadata.m_uCookedTimestamp);
	bOk = bOk && ds.SetUInt32ValueToTable(root, kCookerVersion, metadata.m_uCookerVersion);
	bOk = bOk && ds.SetUInt32ValueToTable(root, kDataVersion, metadata.m_uDataVersion);

	// Dir sources is optional, so it doesn't affect bOk unless
	// we try to write it and that fails. We only write if metadata.m_vDirectoyrSources
	// is not empty.
	if (!metadata.m_vDirectorySources.IsEmpty())
	{
		DataNode dirSources;
		bOk = bOk && ds.SetArrayToTable(root, kDirectorySources);
		bOk = bOk && ds.GetValueFromTable(root, kDirectorySources, dirSources);

		UInt32 const uDirectorySources = metadata.m_vDirectorySources.GetSize();
		for (UInt32 i = 0u; i < uDirectorySources; ++i)
		{
			DataNode entryNode;
			bOk = bOk && ds.SetTableToArray(dirSources, i);
			bOk = bOk && ds.GetValueFromArray(dirSources, i, entryNode);

			auto const& entry = metadata.m_vDirectorySources[i];
			bOk = bOk && ds.SetUInt32ValueToTable(entryNode, kFileCount, entry.m_uFileCount);
			bOk = bOk && ds.SetFilePathToTable(entryNode, kSource, entry.m_Source);
		}
	}

	// Siblings are optional, so it doesn't affect bOk unless
	// we try to write it and that fails. We only write if metadata.m_vSiblings
	// is not empty.
	if (!metadata.m_vSiblings.IsEmpty())
	{
		DataNode siblings;
		bOk = bOk && ds.SetArrayToTable(root, kSiblings);
		bOk = bOk && ds.GetValueFromTable(root, kSiblings, siblings);

		UInt32 const uSiblings = metadata.m_vSiblings.GetSize();
		for (UInt32 i = 0u; i < uSiblings; ++i)
		{
			DataNode entryNode;
			bOk = bOk && ds.SetTableToArray(siblings, i);
			bOk = bOk && ds.GetValueFromArray(siblings, i, entryNode);

			auto const& entry = metadata.m_vSiblings[i];
			bOk = bOk && ds.SetFilePathToTable(entryNode, kSource, entry.m_Source);
			bOk = bOk && ds.SetUInt64ValueToTable(entryNode, kTimestamp, entry.m_uTimestamp);
		}
	}

	// Sources are required.
	{
		DataNode sources;
		bOk = bOk && ds.SetArrayToTable(root, kSources);
		bOk = bOk && ds.GetValueFromTable(root, kSources, sources);

		UInt32 const uSources = metadata.m_vSources.GetSize();
		for (UInt32 i = 0u; i < uSources; ++i)
		{
			DataNode entryNode;
			bOk = bOk && ds.SetTableToArray(sources, i);
			bOk = bOk && ds.GetValueFromArray(sources, i, entryNode);

			auto const& entry = metadata.m_vSources[i];
			bOk = bOk && ds.SetFilePathToTable(entryNode, kSource, entry.m_Source);
			bOk = bOk && ds.SetUInt64ValueToTable(entryNode, kTimestamp, entry.m_uTimestamp);
		}
	}

	// Done.
	return bOk;
}

/**
 * Commit a single file's metadata to an appropriate disk location
 * (the cooked path + .json)
 */
Bool CookDatabase::WriteSingleMetadataToDisk(Platform ePlatform, FilePath filePath, const DataStore& ds, const DataNode& dataNode)
{
	auto const metadataFilePath(GetMetadataPath(filePath));

	String s;
	ds.ToString(dataNode, s, true, 0, true);
	return FileManager::Get()->WriteAllForPlatform(ePlatform, metadataFilePath, s.CStr(), s.GetSize());
}

/**
 * For special cases and situations where you know
 * that you will query file attributes immediately
 * after a mutation.
 */
void CookDatabase::ManualOnFileChange(FilePath filePath)
{
	Lock lock(m_Mutex);
	// Mark this file as changed - when changed, we won't request
	// the file from the network cache, since it changed locally.
	(void)m_Changed.Insert(filePath);
	// Propagate the change.
	InsideLockOnFileChange(filePath);
}

void CookDatabase::InsideLockAddDependents(FilePath filePath, const CookMetadata& metadata, DepTable& rtDep, DepTable& rtDepDir) const
{
	for (auto const& e : metadata.m_vSiblings)
	{
		auto const entry = rtDep.Insert(e.m_Source, DepSet());
		(void)entry.First->Second.Insert(filePath);
	}
	for (auto const& e : metadata.m_vSources)
	{
		auto const entry = rtDep.Insert(e.m_Source, DepSet());
		(void)entry.First->Second.Insert(filePath);
	}
	for (auto const& e : metadata.m_vDirectorySources)
	{
		auto const entry = rtDepDir.Insert(e.m_Source, DepSet());
		(void)entry.First->Second.Insert(filePath);
	}
}

void CookDatabase::InsideLockGetDependents(FilePath filePath, Dependents& rvDependents) const
{
	rvDependents.Clear();

	auto const normalized = Normalize(filePath);
	auto pDeps = m_tDep.Find(filePath);
	if (nullptr != pDeps)
	{
		for (auto const& e : *pDeps)
		{
			// Filter out filePath itself.
			if (Normalize(e) != normalized)
			{
				rvDependents.PushBack(e);
			}
		}
	}

	// Dependent directory checks are more complex - we need to enumerate the entire table,
	// and check if any key is a prefix
	auto const iBegin = m_tDepDir.Begin();
	auto const iEnd = m_tDepDir.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		auto const& dir = i->First;
		auto const& deps = i->Second;

		// If the dir type is not unknown, make sure the type matches.
		if (dir.GetType() != FileType::kUnknown &&
			dir.GetType() != filePath.GetType())
		{
			continue;
		}

		// Check if the dir is a prefix of the filePath.
		auto const dirName = dir.GetRelativeFilenameWithoutExtension();
		auto const fileName = filePath.GetRelativeFilenameWithoutExtension();
		if (0 == STRNCMP_CASE_INSENSITIVE(dirName.CStr(), fileName.CStr(), dirName.GetSizeInBytes()))
		{
			// Add all the dependents.
			for (auto const& e : deps)
			{
				// Filter out filePath itself.
				if (Normalize(e) != normalized)
				{
					rvDependents.PushBack(e);
				}
			}
		}
	}
}

void CookDatabase::InsideLockOnFileChange(FilePath filePath)
{
	// Special handling for some one-to-many types.
	if (IsTextureFileType(filePath.GetType()))
	{
		for (Int i = (Int)FileType::FIRST_TEXTURE_TYPE; i <= (Int)FileType::LAST_TEXTURE_TYPE; ++i)
		{
			filePath.SetType((FileType)i);
			InsideLockHandleFileChange(filePath);
		}
	}
	// Otherwise, dispatch normally.
	else
	{
		InsideLockHandleFileChange(filePath);
	}
}

void CookDatabase::InsideLockHandleFileChange(FilePath filePath)
{
	// Remove the file itself.
	InsideLockRemoveFromCaches(filePath);
	// Remove this file's cached metadata.
	(void)m_tMetadata.Erase(filePath);

	// Get any output files dependent on this source file.
	Dependents v;
	InsideLockGetDependents(filePath, v);

	// Enumerate and remove from the up-to-date set.
	for (auto const& dependentFilePath : v)
	{
		InsideLockRemoveFromCaches(dependentFilePath);
		// Remove this file's cached metadata.
		(void)m_tMetadata.Erase(dependentFilePath);
	}
}

void CookDatabase::InsideLockRemoveFromCaches(FilePath filePath)
{
	// Remove this file from the up-to-date set
	(void)m_tUpToDate.Erase(filePath);
}

const CookMetadata& CookDatabase::InsideLockResolveMetadata(FilePath filePath)
{
	// Query existing metadata.
	auto pReturn = m_tMetadata.Find(filePath);
	// Reread metadata if not found.
	if (nullptr == pReturn)
	{
		auto const metadata = InsideLockReadMetadata(filePath);

		// Update the cache metadata entry.
		InsideLockRemoveDependents(filePath);
		auto const e = m_tMetadata.Overwrite(filePath, metadata);
		SEOUL_ASSERT(e.Second);
		InsideLockAddDependents(filePath, metadata, m_tDep, m_tDepDir);

		// On metadata update, also clear the up-to-date state.
		// Need to recheck on read.
		InsideLockRemoveFromCaches(filePath);

		// Done.
		pReturn = &(e.First->Second);
	}

	// Sanity.
	SEOUL_ASSERT(nullptr != pReturn);

	return *pReturn;
}

void CookDatabase::OnFileChange(
	const String& sOldPath,
	const String& sNewPath,
	FileChangeNotifier::FileEvent eEvent)
{
	// Get paths.
	auto const oldPath = FilePath::CreateContentFilePath(sOldPath);
	auto const newPath = FilePath::CreateContentFilePath(sNewPath);

	// Early out if nothing to do.
	if (!oldPath.IsValid() && !newPath.IsValid())
	{
		return;
	}

	// Exclusive access.
	Lock lock(m_Mutex);

	// Check for dependencies for old path and new path and remove those.
	if (oldPath.IsValid())
	{
		// Mark this file as changed - when changed, we won't request
		// the file from the network cache, since it changed locally.
		(void)m_Changed.Insert(oldPath);
		InsideLockOnFileChange(oldPath);
	}

	// Also trigger new path unless it's the same as old.
	if (newPath != oldPath && newPath.IsValid())
	{
		// Mark this file as changed - when changed, we won't request
		// the file from the network cache, since it changed locally.
		(void)m_Changed.Insert(newPath);
		InsideLockOnFileChange(newPath);
	}
}

void CookDatabase::InsideLockRemoveDependents(FilePath filePath)
{
	auto pExisting = m_tMetadata.Find(filePath);
	if (nullptr == pExisting)
	{
		return;
	}

	for (auto const& e : pExisting->m_vSiblings)
	{
		auto pDep = m_tDep.Find(e.m_Source);
		if (nullptr != pDep)
		{
			pDep->Erase(filePath);
		}
	}

	for (auto const& e : pExisting->m_vSources)
	{
		auto pDep = m_tDep.Find(e.m_Source);
		if (nullptr != pDep)
		{
			pDep->Erase(filePath);
		}
	}

	for (auto const& e : pExisting->m_vDirectorySources)
	{
		auto pDep = m_tDepDir.Find(e.m_Source);
		if (nullptr != pDep)
		{
			pDep->Erase(filePath);
		}
	}
}

CookMetadata CookDatabase::InsideLockReadMetadata(FilePath filePath)
{
	auto const metadataFilePath(GetMetadataPath(filePath));

	String s;
	{
		void* p = nullptr;
		UInt32 u = 0u;
		if (!FileManager::Get()->ReadAllForPlatform(m_ePlatform, metadataFilePath, p, u, 0u, MemoryBudgets::Strings))
		{
			return CookMetadata();
		}

		s.TakeOwnership(p, u);
	}

	DataStore ds;
	if (!DataStoreParser::FromString(s, ds))
	{
		return CookMetadata();
	}

	DataNode node;
	auto const root = ds.GetRootNode();

	CookMetadata ret;
	ret.m_uMetadataTimestamp = FileManager::Get()->GetModifiedTimeForPlatform(m_ePlatform, metadataFilePath);

	Bool bOk = true;
	bOk = bOk && ds.GetValueFromTable(root, kCookedTimestamp, node) && ds.AsUInt64(node, ret.m_uCookedTimestamp);
	bOk = bOk && ds.GetValueFromTable(root, kCookerVersion, node) && ds.AsUInt32(node, ret.m_uCookerVersion);
	bOk = bOk && ds.GetValueFromTable(root, kDataVersion, node) && ds.AsUInt32(node, ret.m_uDataVersion);

	// Dir sources is optional, so it doesn't affect bOk unless
	// it exists but then has an invalid format.
	{
		DataNode dirSources;
		if (bOk && ds.GetValueFromTable(root, kDirectorySources, dirSources))
		{
			UInt32 uDirSources = 0u;
			bOk = bOk && ds.GetArrayCount(dirSources, uDirSources);

			for (UInt32 i = 0u; i < uDirSources; ++i)
			{
				DataNode entryNode;
				bOk = bOk && ds.GetValueFromArray(dirSources, i, entryNode);

				CookMetadata::DirectorySource entry;
				bOk = bOk && ds.GetValueFromTable(entryNode, kFileCount, node) && ds.AsUInt32(node, entry.m_uFileCount);
				bOk = bOk && ds.GetValueFromTable(entryNode, kSource, node) && ds.AsFilePath(node, entry.m_Source);
				ret.m_vDirectorySources.PushBack(entry);
			}
		}
	}

	// Siblings are optional, so they don't affect bOk unless
	// they exist but have an invalid format.
	{
		DataNode siblings;
		if (bOk && ds.GetValueFromTable(root, kSiblings, siblings))
		{
			UInt32 uSiblings = 0u;
			bOk = bOk && ds.GetArrayCount(siblings, uSiblings);

			for (UInt32 i = 0u; i < uSiblings; ++i)
			{
				DataNode entryNode;
				bOk = bOk && ds.GetValueFromArray(siblings, i, entryNode);

				CookMetadata::Source entry;
				bOk = bOk && ds.GetValueFromTable(entryNode, kSource, node) && ds.AsFilePath(node, entry.m_Source);
				bOk = bOk && ds.GetValueFromTable(entryNode, kTimestamp, node) && ds.AsUInt64(node, entry.m_uTimestamp);

				// Normalize for consistency sake.
				if (IsTextureFileType(entry.m_Source.GetType())) { entry.m_Source.SetType(FileType::kTexture0); }

				ret.m_vSiblings.PushBack(entry);
			}
		}
	}

	// Typical sources are required.
	{
		DataNode sources;
		bOk = bOk && ds.GetValueFromTable(root, kSources, sources);
		UInt32 uSources = 0u;
		bOk = bOk && ds.GetArrayCount(sources, uSources);

		for (UInt32 i = 0u; i < uSources; ++i)
		{
			DataNode entryNode;
			bOk = bOk && ds.GetValueFromArray(sources, i, entryNode);

			CookMetadata::Source entry;
			bOk = bOk && ds.GetValueFromTable(entryNode, kSource, node) && ds.AsFilePath(node, entry.m_Source);
			bOk = bOk && ds.GetValueFromTable(entryNode, kTimestamp, node) && ds.AsUInt64(node, entry.m_uTimestamp);

			// Normalize for consistency sake.
			if (IsTextureFileType(entry.m_Source.GetType())) { entry.m_Source.SetType(FileType::kTexture0); }

			ret.m_vSources.PushBack(entry);
		}
	}

	if (bOk)
	{
		return ret;
	}
	else
	{
		return CookMetadata();
	}
}

namespace
{

/**
 * For performance, one-to-one file types shared a single global
 * cooker and data tracker version table. This struct represents
 * that value and handles save, load, and "fix", which occurs
 * on a version mismatch.
 */
struct OneToOneVersions
{
	SEOUL_DELEGATE_TARGET(OneToOneVersions);

	typedef FixedArray<UInt32, (UInt32)FileType::FILE_TYPE_COUNT> Versions;
	Versions m_Data; // Per type values.
	Versions m_Cooker; // Global cooker values, stored per-type, to represent last cook of that type.

	/**
	 * Assuming at least one file type has a version mismatch, delete any files
	 * of that type from disk, then bring this instance up-to-date.
	 */
	Bool Fix()
	{
		m_bFixStatus = true;
		(void)Directory::GetDirectoryListingEx(GamePaths::Get()->GetContentDir(), SEOUL_BIND_DELEGATE(&OneToOneVersions::CheckFile, this));
		return m_bFixStatus;
	}

	/** Checks if all recorded file types are up-to-date. */
	Bool Ok() const
	{
		for (UInt32 i = 0u; i < (UInt32)FileType::FILE_TYPE_COUNT; ++i)
		{
			// Skip types that are not one-to-one.
			if (!CookDatabase::IsOneToOneType((FileType)i))
			{
				continue;
			}

			if (m_Data[i] != kauDataVersions[i])
			{
				return false;
			}
			if (m_Cooker[i] != kuCookerVersion)
			{
				return false;
			}
		}

		return true;
	}

	/**
	 * Commit an up-to-date versions table to disk.
	 */
	static void Save(FilePath filePath)
	{
		OneToOneVersions data;
		Init(data);

		MemorySyncFile file;
		WriteUInt32(file, (UInt32)FileType::FILE_TYPE_COUNT);
		for (UInt32 i = 0u; i < (UInt32)FileType::FILE_TYPE_COUNT; ++i)
		{
			WriteUInt32(file, kauDataVersions[i]);
			WriteUInt32(file, kuCookerVersion);
		}

		DiskFileSystem system;
		system.WriteAll(filePath, file.GetBuffer().GetBuffer(), file.GetBuffer().GetOffset());
	}

	/**
	 * Reads the on-disk version table - on failure, r is still populated with
	 * up-to-date values.
	 */
	static Bool Load(FilePath filePath, OneToOneVersions& r)
	{
		// Only interact with disk, we don't want to interact with NFS, etc. for this file.
		UInt32 uCount = 0u;
		DiskSyncFile file(filePath, File::kRead);
		if (!file.CanRead()) { goto fail; }

		if (!ReadUInt32(file, uCount)) { goto fail; }
		if (uCount != (UInt32)FileType::FILE_TYPE_COUNT) { goto fail; }

		for (UInt32 i = 0u; i < uCount; ++i)
		{
			if (!ReadUInt32(file, r.m_Data[i])) { goto fail; }
			if (!ReadUInt32(file, r.m_Cooker[i])) { goto fail; }
		}

		return true;

	fail:
		Init(r);
		return false;
	}

private:
	Bool m_bFixStatus = false;

	/** Individual file check for the Fix loop. */
	Bool CheckFile(Directory::DirEntryEx& r)
	{
		auto const eType = ExtensionToFileType(Path::GetExtension(r.m_sFileName));

		// Skip non one-to-one or surprising types.
		if (FileType::kUnknown == eType ||
			!CookDatabase::IsOneToOneType(eType))
		{
			return true;
		}

		// Check if up-to-date.
		if (m_Cooker[(UInt32)eType] == kuCookerVersion &&
			m_Data[(UInt32)eType] == kauDataVersions[(UInt32)eType])
		{
			return true;
		}

		// Delete - on failure, stop processing.
		if (!DiskSyncFile::DeleteFile(r.m_sFileName))
		{
			m_bFixStatus = false;
			return false;
		}

		// Done.
		return true;
	}

	/** Establish r as up-to-date. */
	static void Init(OneToOneVersions& r)
	{
		for (UInt32 i = 0u; i < (UInt32)FileType::FILE_TYPE_COUNT; ++i)
		{
			r.m_Data[i] = kauDataVersions[i];
			r.m_Cooker[i] = kuCookerVersion;
		}
	}
};

} // namespace anonymous

/**
 * Load the one-to-one versions global table.
 *
 * On success, check and fix if needed. On failure to load,
 * save an up-to-date global table to disk. We take the
 * conservative assumption that a missing table means all
 * files are up-to-date, since the consequences of
 * an accidental recook are worse than an accidental not-cook
 * (in the latter case, mismatched files can just have their
 * data version bumped again).
 */
void CookDatabase::ProcessOneToOneVersions()
{
	auto const filePath(GetOneToOneVersionDataFilePath());

	// Load - on load failure, immediately save an up-to-date
	// version and return.
	OneToOneVersions data;
	if (!OneToOneVersions::Load(filePath, data))
	{
		OneToOneVersions::Save(filePath);
		return;
	}

	// Check status.
	if (!data.Ok())
	{
		// If we successfully fix the data, save
		// a new version with up-to-date values.
		if (data.Fix())
		{
			OneToOneVersions::Save(filePath);
		}
	}
}

} // namespace Seoul
