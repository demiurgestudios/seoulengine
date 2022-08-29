/**
 * \file PackageCookConfig.cpp
 * \brief Data structure into which package configuration data
 * is serialized. Controls the PackageCookTask.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CookerSettings.h"
#include "Directory.h"
#include "FileManager.h"
#include "Logger.h"
#include "PackageCookConfig.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionDeserialize.h"
#include "ScopedAction.h"
#include "SeoulWildcard.h"
#include "Vector.h"

namespace Seoul
{

namespace Cooking
{

struct OverflowTrainingDataEntry SEOUL_SEALED
{
	FilePath m_FilePath{};
}; // struct OverflowTrainingDataEntry
typedef Vector<OverflowTrainingDataEntry, MemoryBudgets::Cooking> OverflowTrainingDataEntries;

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::PackageConfig, TypeFlags::kDisableCopy)
	SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_ATTRIBUTE(PostSerializeType, "PostSerialize")
	SEOUL_PROPERTY_N("AdditionalIncludes", m_vAdditionalIncludes)
	SEOUL_PROPERTY_N("CompressFiles", m_bCompressFiles)
	SEOUL_PROPERTY_N("CookJson", m_bCookJson)
	SEOUL_PROPERTY_N("CustomSarExtension", m_sCustomSarExtension)
	SEOUL_PROPERTY_N("DeltaArchives", m_vDeltaArchives)
	SEOUL_PROPERTY_N("ExcludeExemptions", m_vExcludeExemptions)
	SEOUL_PROPERTY_N("ExcludeFiles", m_vExcludeFiles)
	SEOUL_PROPERTY_N("Extensions", m_vExtensions)
	SEOUL_PROPERTY_N("GameDirectoryType", m_eGameDirectoryType)
	SEOUL_PROPERTY_N("IncludeFiles", m_vIncludeFiles)
	SEOUL_PROPERTY_N("LocaleBaseArchive", m_sLocaleBaseArchive)
	SEOUL_PROPERTY_N("LocaleBaseFilename", m_sLocaleBaseFilename)
	SEOUL_PROPERTY_N("LocalePatchFilename", m_sLocalePatchFilename)
	SEOUL_PROPERTY_N("Name", m_sName)
	SEOUL_PROPERTY_N("NonDependencySearchPatterns", m_vNonDependencySearchPatterns)
	SEOUL_PROPERTY_N("Obfuscate", m_bObfuscate)
	SEOUL_PROPERTY_N("PopulateFromDependencies", m_bPopulateFromDependencies)
	SEOUL_PROPERTY_N("Root", m_sRoot)
	SEOUL_PROPERTY_N("RootDistro", m_sRootDistro)
	SEOUL_PROPERTY_N("SupportDirectoryQueries", m_bSupportDirectoryQueries)
	SEOUL_PROPERTY_N("ZipArchive", m_bZipArchive)
	SEOUL_PROPERTY_N("SortByModifiedTime", m_bSortByModifiedTime)
	SEOUL_PROPERTY_N("MinifyJson", m_bMinifyJson)
	SEOUL_PROPERTY_N("IncludeInSourceControl", m_bIncludeInSourceControl)
	SEOUL_PROPERTY_N("CompressionDictionarySize", m_uCompressionDictionarySize)
	SEOUL_PROPERTY_N("UseCompressionDictionary", m_bUseCompressionDictionary)
	SEOUL_PROPERTY_N("Overflow", m_sOverflow)
	SEOUL_PROPERTY_N("OverflowTargetBytes", m_uOverflowTargetBytes)
	SEOUL_PROPERTY_N("OverflowTrainingData", m_OverflowTrainingDataFilePath)
	SEOUL_PROPERTY_N("OverflowConsider", m_vOverflowConsider)
	SEOUL_PROPERTY_N("Variations", m_vVariations)
	SEOUL_PROPERTY_N("ExcludeFromLocal", m_bExcludeFromLocal)

	SEOUL_METHOD(PostSerialize)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Cooking::PackageCookConfig, TypeFlags::kDisableNew)
	SEOUL_ATTRIBUTE(PostSerializeType, "PostSerialize")
	SEOUL_PROPERTY_N("ConfigDirectoryExcludes", m_vConfigDirectoryExcludes)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("Platform", m_ePlatform)
	SEOUL_PROPERTY_N("Packages", m_vPackages)

	SEOUL_METHOD(PostSerialize)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(Cooking::OverflowTrainingDataEntry)
	SEOUL_PROPERTY_N("Path", m_FilePath)
SEOUL_END_TYPE()

SEOUL_SPEC_TEMPLATE_TYPE(CheckedPtr<Cooking::PackageConfig>)
SEOUL_SPEC_TEMPLATE_TYPE(CheckedPtr<Cooking::PackageCookConfig>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<CheckedPtr<Cooking::PackageConfig>, 9>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<CheckedPtr<Cooking::PackageCookConfig>, 9>)
SEOUL_SPEC_TEMPLATE_TYPE(Vector<Cooking::OverflowTrainingDataEntry, 9>)

namespace Cooking
{

static const String ksGeneratedPC("GeneratedPC");

static inline void Fill(const PackageConfig::StringVector& v, PackageConfig::Wildcards& rv)
{
	SafeDeleteVector(rv);
	rv.Reserve(v.GetSize());

	for (auto const& s : v)
	{
		rv.PushBack(SEOUL_NEW(MemoryBudgets::Cooking) Wildcard(s));
	}
}

static inline Bool MatchesAnyWildcard(const PackageConfig::Wildcards& v, FilePath filePath)
{
	for (auto const pWildcard : v)
	{
		if (pWildcard->IsExactMatch(filePath.GetRelativeFilename().CStr()))
		{
			return true;
		}
	}

	return false;
}

PackageConfig::PackageConfig()
{
}

PackageConfig::~PackageConfig()
{
	SafeDeleteVector(m_vIncludeFileWildcards);
	SafeDeleteVector(m_vExcludeFileWildcards);
	SafeDeleteVector(m_vExcludeExemptionWildcards);
}

Bool PackageConfig::ComputeOverflowExclusionSet(
	Bool bIncludeLocal,
	Platform ePlatform,
	PackageConfig::OverflowExclusionSet& rSet) const
{
	// Load overflow data, if it exists.
	if (!m_OverflowTrainingDataFilePath.IsValid())
	{
		PackageConfig::OverflowExclusionSet emptySet;
		rSet.Swap(emptySet);
		return true;
	}

	// Read from source.
	auto const sFileName(m_OverflowTrainingDataFilePath.GetAbsoluteFilenameInSource());
	String sBody;
	if (!FileManager::Get()->ReadAll(sFileName, sBody))
	{
		SEOUL_LOG_COOKING("%s: failed reading overflow training data from '%s'.",
			m_sName.CStr(),
			sFileName.CStr());
		return false;
	}

	// Deserialize.
	OverflowTrainingDataEntries vEntries;
	if (!Reflection::DeserializeFromString(sBody, &vEntries))
	{
		SEOUL_LOG_COOKING("%s: failed deserializing overflow training data from '%s'.",
			m_sName.CStr(),
			sFileName.CStr());
		return false;
	}

	// Migrate all entires into our set.
	OverflowExclusionSet set;
	{
		Bool bHasUITextures = false;
		UITextures tUITextures;

		auto const sGeneratedPrefix(Path::Combine(String(),
			String::Printf("Generated%s", EnumToString<Platform>(ePlatform))));
		for (auto const& e : vEntries)
		{
			auto filePath = e.m_FilePath;

			// TODO: Perform this conversion as part of the deserialization so
			// we're not creating unnecessary HStrings.
			//
			// Handled conversion of GeneratedPC to the current platform target.
			if (ePlatform != Platform::kPC)
			{
				auto h = filePath.GetRelativeFilenameWithoutExtension();
				if (STRNCMP_CASE_INSENSITIVE(h.CStr(), ksGeneratedPC.CStr(), ksGeneratedPC.GetSize()) == 0)
				{
					// If an image, we need to search the current platform's
					// UI images folder  for the matching target.
					if (IsTextureFileType(filePath.GetType()))
					{
						if (!bHasUITextures)
						{
							bHasUITextures = true;
							tUITextures.Clear();
							AppendUITextures(ePlatform, sGeneratedPrefix, tUITextures);
							if (bIncludeLocal)
							{
								AppendUITextures(ePlatform, "GeneratedLocal", tUITextures);
							}
						}

						// Remap the file path from the GeneratedPC/GeneratedLocal path to
						// the platform specific path (the PC trainer will emit paths
						// to PC data, and we need to remap those for data for the current
						// platform cook).
						RemapUITexture(tUITextures, ePlatform, filePath);
					}
					// Otherwise, just remap to the target platform.
					else
					{
						h = FilePathRelativeFilename(sGeneratedPrefix + (h.CStr() + ksGeneratedPC.GetSize()));
						filePath.SetRelativeFilenameWithoutExtension(h);
					}
				}
			}

			if (IsTextureFileType(filePath.GetType()))
			{
				auto e(filePath);
				for (Int i = (Int)FileType::FIRST_TEXTURE_TYPE; i <= (Int)FileType::LAST_TEXTURE_TYPE; ++i)
				{
					e.SetType((FileType)i);
					set.Insert(e);
				}
			}
			else
			{
				set.Insert(filePath);
			}
		}
	}

	// Apply, complete.
	rSet.Swap(set);
	return true;
}

Bool PackageConfig::ShouldIncludeFile(FilePath filePath) const
{
	if (!m_FileTypeSet.HasKey(filePath.GetType()))
	{
		return false;
	}

	auto const eResult = TestFileAgainstFilters(filePath);
	return (kPass == eResult || kPassWithExemption == eResult);
}

Bool PackageCookConfig::IsExcludedFromConfigs(FilePath filePath) const
{
	return MatchesAnyWildcard(m_vConfigDirectoryExcludeWildcards, filePath);
}

static inline Bool CompareSourceFiles(const String& sA, FilePath b)
{
	auto const sB(b.GetAbsoluteFilenameInSource());

	void* pA = nullptr;
	UInt32 uA = 0u;
	void* pB = nullptr;
	UInt32 uB = 0u;

	auto const scoped(MakeScopedAction(
	[&]()
	{
		(void)FileManager::Get()->ReadAll(sA, pA, uA, 0u, MemoryBudgets::Cooking);
		(void)FileManager::Get()->ReadAll(sB, pB, uB, 0u, MemoryBudgets::Cooking);
	},
	[&]()
	{
		MemoryManager::Deallocate(pB);
		MemoryManager::Deallocate(pA);
	}));

	if (nullptr == pA || nullptr == pB)
	{
		return false;
	}
	if (uA != uB)
	{
		return false;
	}

	return (0 == memcmp(pA, pB, uA));
}

void PackageConfig::RemapUITexture(
	const UITextures& tUITextures,
	Platform ePlatform,
	FilePath& rFilePath) const
{
	auto const sSource(rFilePath.GetAbsoluteFilenameInSource());
	auto const uFileSize = FileManager::Get()->GetFileSize(sSource);
	if (0u == uFileSize)
	{
		// Does not exist, nothing to remap.
		return;
	}

	auto pvRemaps = tUITextures.Find((UInt32)uFileSize);
	if (nullptr == pvRemaps)
	{
		// TODO: Lesser of two evils. We don't build every platform with every build. As a result,
		// it is possible for a generated file to be removed but not yet removed from the PC configuration.
		// We can't tell the difference between this, and a bug where the file is present in the PC build
		// but missing from another platform (without performing a full dependency scan to determine if
		// the PC file on disk is actually in-use or not). I'm making the call to not introduce the complexity/time
		// of a full dependency scan, which opens up the risk that we may miss dependency data if there is a cooker
		// bug and a file detected as needed by the PC trainer cannot be matched to an (e.g.) Android or iOS file.
		return;
	}

	for (auto const& e : (*pvRemaps))
	{
		if (CompareSourceFiles(sSource, e))
		{
			rFilePath = e;
			return;
		}
	}

	// TODO: Lesser of two evils. We don't build every platform with every build. As a result,
	// it is possible for a generated file to be removed but not yet removed from the PC configuration.
	// We can't tell the difference between this, and a bug where the file is present in the PC build
	// but missing from another platform (without performing a full dependency scan to determine if
	// the PC file on disk is actually in-use or not). I'm making the call to not introduce the complexity/time
	// of a full dependency scan, which opens up the risk that we may miss dependency data if there is a cooker
	// bug and a file detected as needed by the PC trainer cannot be matched to an (e.g.) Android or iOS file.
	return;
}

namespace
{

Bool OnTexture(void* pUserData, Directory::DirEntryEx& entry)
{
	if (Path::GetExtension(entry.m_sFileName) != ".png")
	{
		return true;
	}

	auto& t = *((PackageConfig::UITextures*)pUserData);

	auto const filePath = FilePath::CreateContentFilePath(entry.m_sFileName);
	auto const& e = t.Insert(entry.m_uFileSize, Vector<FilePath, MemoryBudgets::Cooking>());
	e.First->Second.PushBack(filePath);
	return true;
}

} // namespace

Bool PackageConfig::AppendUITextures(
	Platform ePlatform,
	const String& sGeneratedPrefix,
	UITextures& rt) const
{
	FilePath dirPath;
	dirPath.SetDirectory(GameDirectory::kContent);
	dirPath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(sGeneratedPrefix + Path::DirectorySeparatorChar() + "UIImages"));

	auto const sDir(dirPath.GetAbsoluteFilenameInSource());

	UITextures t;
	if (Directory::DirectoryExists(sDir))
	{
		// TODO: Bind GetDirectoryListingEx into FileManager.
		if (!Directory::GetDirectoryListingEx(sDir, SEOUL_BIND_DELEGATE(OnTexture, &t)))
		{
			SEOUL_LOG_COOKING("%s: failed listing UI textures for reconciling overflow data.",
				m_sName.CStr());
			return false;
		}
	}

	for (auto const& e : t)
	{
		auto const res = rt.Insert(e.First, e.Second);
		if (!res.Second)
		{
			res.First->Second.Append(e.Second);
		}
	}
	return true;
}

Bool PackageConfig::PostSerialize(Reflection::SerializeContext* pContext)
{
	// Get cooker settings.
	if (nullptr == pContext ||
		!pContext->GetUserData().IsOfType<CookerSettings const*>())
	{
		SEOUL_LOG_COOKING("%s: programmer error, context is not CookerSettings.", m_sName.CStr());
		return false;
	}

	auto const& context = *pContext->GetUserData().Cast<CookerSettings const*>();

	// If we have a config structure, apply certain filtering now.
	// In particular, use the presence of m_bLocal to disable
	// some slow operations (such as dictionary based compression).
	auto const bLocal = context.m_bLocal;
	if (bLocal)
	{
		// Compression dictionary compression is slow, so we turn it off
		// in fast builds.
		m_bUseCompressionDictionary = false;
		m_uCompressionDictionarySize = 0u;

		// Set compression level to fast.
		m_eCompressionLevel = ZSTDCompressionLevel::kFastest;
	}
	else
	{
		// Set compression level to best.
		m_eCompressionLevel = ZSTDCompressionLevel::kBest;
	}

	// Local handling - we convert the ExcludeFromLocal setting
	// to a runtime determiner during load (it will only be
	// true if the file should be excluded).
	if (m_bExcludeFromLocal && !bLocal)
	{
		m_bExcludeFromLocal = false;
	}

	m_FileTypeSet.Clear();
	for (auto const& s : m_vExtensions)
	{
		(void)m_FileTypeSet.Insert(ExtensionToFileType(s));
	}

	if (m_sLocaleBaseFilename.IsEmpty()) { m_sLocaleBaseFilenameNoExtension = String(); }
	else { m_sLocaleBaseFilenameNoExtension = Path::ReplaceExtension(m_sLocaleBaseFilename, String()); }

	if (m_sLocalePatchFilename.IsEmpty()) { m_sLocalePatchFilenameNoExtension = String(); }
	else { m_sLocalePatchFilenameNoExtension = Path::ReplaceExtension(m_sLocalePatchFilename, String()); }

	Fill(m_vIncludeFiles, m_vIncludeFileWildcards);
	Fill(m_vExcludeFiles, m_vExcludeFileWildcards);
	Fill(m_vExcludeExemptions, m_vExcludeExemptionWildcards);

	return true;
}

PackageConfig::FilterResult PackageConfig::TestFileAgainstFilters(FilePath filePath) const
{
	// If the include list is empty or if the file passes the include pattern.
	if (m_vIncludeFileWildcards.IsEmpty() || MatchesAnyWildcard(m_vIncludeFileWildcards, filePath))
	{
		// If the exclude list is empty or if the file does not match any exclude pattern,
		// it passes without exemption.
		if (m_vExcludeFileWildcards.IsEmpty() || !MatchesAnyWildcard(m_vExcludeFileWildcards, filePath))
		{
			return kPass;
		}
		// If the file was excluded but is then exempted,
		// it passes with an exemption.
		else if (MatchesAnyWildcard(m_vExcludeExemptionWildcards, filePath))
		{
			return kPassWithExemption;
		}
		// File was explicitly excluded.
		else
		{
			return kIncludedButExcluded;
		}
	}

	// File is not included at all.
	return kNotIncluded;
}

PackageCookConfig::PackageCookConfig(const String& sAbsoluteConfigFilename)
	: m_sAbsoluteConfigFilename(sAbsoluteConfigFilename)
{
}

PackageCookConfig::~PackageCookConfig()
{
	SafeDeleteVector(m_vConfigDirectoryExcludeWildcards);
	SafeDeleteVector(m_vPackages);
}

Bool PackageCookConfig::PostSerialize(Reflection::SerializeContext* pContext)
{
	Fill(m_vConfigDirectoryExcludes, m_vConfigDirectoryExcludeWildcards);

	return true;
}

} // namespace Cooking

} // namespace Seoul
