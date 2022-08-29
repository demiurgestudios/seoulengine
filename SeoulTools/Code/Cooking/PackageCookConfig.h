/**
 * \file PackageCookConfig.h
 * \brief Data structure into which package configuration data
 * is serialized. Controls the PackageCookTask.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PACKAGE_COOK_CONFIG
#define PACKAGE_COOK_CONFIG

#include "BuildDistroPublic.h"
#include "Compress.h"
#include "FilePath.h"
#include "HashSet.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "Vector.h"
namespace Seoul { class Wildcard; }
namespace Seoul { namespace Cooking { struct CookerSettings; } }
namespace Seoul { namespace Reflection { struct SerializeContext; } }

namespace Seoul::Cooking
{

class PackageConfig SEOUL_SEALED
{
public:
	typedef HashSet<FileType, MemoryBudgets::Cooking> FileTypeSet;
	typedef Vector<String, MemoryBudgets::Cooking> StringVector;
	typedef HashTable<UInt32, Vector<FilePath, MemoryBudgets::Cooking>, MemoryBudgets::Cooking> UITextures;
	typedef Vector<Wildcard*, MemoryBudgets::Cooking> Wildcards;

	PackageConfig();
	~PackageConfig();

	Vector<String, MemoryBudgets::Cooking> m_vVariations;
	String m_sOverflow{};
	UInt64 m_uOverflowTargetBytes{};
	FilePath m_OverflowTrainingDataFilePath{};
	Vector<String, MemoryBudgets::Cooking> m_vOverflowConsider{};
	StringVector m_vAdditionalIncludes{};
	Bool m_bCompressFiles{};
	Bool m_bCookJson{};
	String m_sCustomSarExtension{};
	StringVector m_vDeltaArchives{};
	StringVector m_vExcludeExemptions{};
	StringVector m_vExcludeFiles{};
	StringVector m_vExtensions{};
	GameDirectory m_eGameDirectoryType{};
	StringVector m_vIncludeFiles{};
	String m_sLocaleBaseArchive{};
	String m_sLocaleBaseFilename{};
	String m_sLocalePatchFilename{};
	String m_sName{};
	StringVector m_vNonDependencySearchPatterns{};
	Bool m_bObfuscate{};
	Bool m_bPopulateFromDependencies{};
	Bool m_bSupportDirectoryQueries{};
	Bool m_bZipArchive{};
	Bool m_bSortByModifiedTime{};
	Bool m_bMinifyJson{};
	Bool m_bIncludeInSourceControl{};
	Bool m_bUseCompressionDictionary{};
	Bool m_bExcludeFromLocal{};
	UInt32 m_uCompressionDictionarySize{};
	ZSTDCompressionLevel GetCompressionLevel() const { return m_eCompressionLevel; }

	const String& GetLocaleBaseFilenameNoExtension() const { return m_sLocaleBaseFilenameNoExtension; }
	const String& GetLocalePatchFilenameNoExtension() const { return m_sLocalePatchFilenameNoExtension; }

	typedef HashSet<FilePath, MemoryBudgets::Cooking> OverflowExclusionSet;
	Bool ComputeOverflowExclusionSet(
		Bool bIncludeLocal,
		Platform ePlatform,
		PackageConfig::OverflowExclusionSet& rSet) const;

	Bool ShouldIncludeFile(FilePath filePath) const;

	const String& GetRoot() const
	{
		// When in distribution branches, prefer m_sRootDistro if it is defined.
		if (g_kbBuildForDistribution)
		{
			return (m_sRootDistro.IsEmpty() ? m_sRoot : m_sRootDistro);
		}
		else
		{
			return m_sRoot;
		}
	}

private:
	SEOUL_DISABLE_COPY(PackageConfig);
	SEOUL_REFLECTION_FRIENDSHIP(PackageConfig);

	String m_sRoot{};
	String m_sRootDistro{};
	FileTypeSet m_FileTypeSet;
	String m_sLocaleBaseFilenameNoExtension;
	String m_sLocalePatchFilenameNoExtension;
	Wildcards m_vIncludeFileWildcards;
	Wildcards m_vExcludeFileWildcards;
	Wildcards m_vExcludeExemptionWildcards;
	ZSTDCompressionLevel m_eCompressionLevel = ZSTDCompressionLevel::kBest;

	/**
	 * Possible results of applying include, exclude, and exclud-exemption
	 * filters to a file.
	 */
	enum FilterResult
	{
		/** File does not pass the include filters at all. */
		kNotIncluded,

		/** File passed the include filters but was explicitly excluded. */
		kIncludedButExcluded,

		/** File passed all filters, was not explicitly excluded. */
		kPass,

		/** File passed include filter, was explicitly excluded, but then was exempted from the exclude. */
		kPassWithExemption,
	};

	Bool AppendUITextures(
		Platform ePlatform,
		const String& sGeneratedPrefix,
		UITextures& rt) const;
	void RemapUITexture(
		const UITextures& tUITextures,
		Platform ePlatform,
		FilePath& rFilePath) const;

	Bool PostSerialize(Reflection::SerializeContext* pContext);
	FilterResult TestFileAgainstFilters(FilePath filePath) const;
}; // class PackageConfig

class PackageCookConfig SEOUL_SEALED
{
public:
	typedef Vector<String, MemoryBudgets::Cooking> ConfigDirectoryExcludes;
	typedef Vector<CheckedPtr<PackageConfig>, MemoryBudgets::Cooking> Packages;
	typedef Vector<Wildcard*, MemoryBudgets::Cooking> Wildcards;

	PackageCookConfig(const String& sAbsoluteConfigFilename);
	~PackageCookConfig();

	String const m_sAbsoluteConfigFilename;
	ConfigDirectoryExcludes m_vConfigDirectoryExcludes;
	Platform m_ePlatform{};
	Packages m_vPackages;

	Bool IsExcludedFromConfigs(FilePath filePath) const;

private:
	SEOUL_DISABLE_COPY(PackageCookConfig);
	SEOUL_REFLECTION_FRIENDSHIP(PackageCookConfig);

	Wildcards m_vConfigDirectoryExcludeWildcards;

	Bool PostSerialize(Reflection::SerializeContext* pContext);
}; // class PackageCookConfig

} // namespace Seoul::Cooking

#endif // include guard
