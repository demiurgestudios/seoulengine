/**
 * \file PackageCookTask.cpp
 * \brief Cooking tasks to generate .sar files given
 * a package configuration.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "BuildChangelistPublic.h"
#include "BuildVersion.h"
#include "Compress.h"
#include "CookDatabase.h"
#include "CookPriority.h"
#include "FalconFCNFile.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "ICookContext.h"
#include "Logger.h"
#include "Material.h"
#include "PackageCookConfig.h"
#include "PackageFileSystem.h"
#include "Path.h"
#include "Prereqs.h"
#include "ReflectionDefine.h"
#include "SccIClient.h"
#include "ScopedAction.h"
#include "SeoulCrc32.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"
#include "SeoulFileWriters.h"
#include "SeoulPugiXml.h"
#include "SeoulWildcard.h"
#include "SettingsManager.h"
#include "SoundUtil.h"
#include "StackOrHeapArray.h"
#include "ZipFile.h"

#if SEOUL_WITH_ANIMATION_2D
#include "Animation2DDataDefinition.h"
#include "Animation2DReadWriteUtil.h"
#endif

namespace Seoul
{

namespace Cooking
{

static const Int64 kiFileEntryAlignment = 8;
static const HString kFilePath("FilePath");
static const HString kSkins("skins");

namespace
{

void LogError(Byte const* s)
{
	SEOUL_LOG_COOKING("%s", s);
}

struct FileEntry SEOUL_SEALED
{
	String m_sFileName;
	PackageFileEntry m_Entry;
};

struct DeltaKey SEOUL_SEALED
{
	static DeltaKey Create(FilePath filePath, UInt64 uSize, UInt32 uCrc32)
	{
		DeltaKey ret;
		ret.m_uFileSize = uSize;
		ret.m_FilePath = filePath;
		ret.m_uCrc32 = uCrc32;
		return ret;
	}

	Bool operator==(const DeltaKey& b) const
	{
		return
			(m_FilePath == b.m_FilePath) &&
			(m_uFileSize == b.m_uFileSize) &&
			(m_uCrc32 == b.m_uCrc32);
	}

	Bool operator!=(const DeltaKey& b) const
	{
		return !(*this == b);
	}

	UInt64 m_uFileSize{};
	FilePath m_FilePath{};
	UInt32 m_uCrc32{};
};

static inline UInt32 GetHash(const DeltaKey& key)
{
	UInt32 uReturn = 0u;
	IncrementalHash(uReturn, Seoul::GetHash(key.m_uFileSize));
	IncrementalHash(uReturn, key.m_FilePath.GetHash());
	IncrementalHash(uReturn, key.m_uCrc32);
	return uReturn;
}

} // namespace

} // namespace Cooking

template <>
struct DefaultHashTableKeyTraits<Cooking::DeltaKey>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static Cooking::DeltaKey GetNullKey()
	{
		return Cooking::DeltaKey();
	}

	static const Bool kCheckHashBeforeEquals = false;
};

namespace Cooking
{

/**
 * @return An XorKey used to Obfuscate.
 */
inline static UInt32 GenerateKey(const String& s)
{
	UInt32 uXorKey = 0x54007b47;  // "shoot bot", roughly
	for (UInt i = 0; i < s.GetSize(); i++)
	{
		uXorKey = uXorKey * 33 + tolower(s[i]);
	}
	return uXorKey;
}

/**
 * Obfuscate a block of data based on provided parameters.
 */
static inline void Obfuscate(UInt32 uXorKey, void* pData, size_t zDataSize)
{
	UByte *pDataPtr = (UByte *)pData;
	UByte *pEndPtr = pDataPtr + zDataSize;

	Int32 i = 0;
	while (pDataPtr < pEndPtr)
	{
		*pDataPtr ^= (UByte)((uXorKey >> ((i % 4) << 3)) + (i / 4) * 101);
		pDataPtr++;
		i++;
	}
}

static inline Bool PadToAlignment(SyncFile& r, Int64 iAlignment)
{
	Int64 iPosition = 0;
	if (!r.GetCurrentPositionIndicator(iPosition))
	{
		SEOUL_LOG_COOKING("%s: failed getting position indicator for alignment padding.", r.GetAbsoluteFilename().CStr());
		return false;
	}

	Int64 const iNewPosition = RoundUpToAlignment(iPosition, iAlignment);
	Int64 const iDiff = (iNewPosition - iPosition);

	StackOrHeapArray<Byte, 16, MemoryBudgets::Cooking> aPadding((UInt32)iDiff);
	aPadding.Fill((Byte)0);
	if (aPadding.GetSizeInBytes() != r.WriteRawData(aPadding.Data(), aPadding.GetSizeInBytes()))
	{
		SEOUL_LOG_COOKING("%s: failed writing %u bytes for alignment padding.", r.GetAbsoluteFilename().CStr(), aPadding.GetSize());
		return false;
	}

	return true;
}

static inline String GetMemoryUsageString(UInt64 uSizeInBytes)
{
	// Handling for difference thresholds.
	if (uSizeInBytes > 1024 * 1024) { return ToString((uSizeInBytes / (1024 * 1024))) + " MBs"; }
	else if (uSizeInBytes > 1024) { return ToString((uSizeInBytes / 1024)) + " KBs"; }
	else { return ToString(uSizeInBytes) + " Bs"; }
}

class PackageCookTask SEOUL_SEALED : public BaseCookTask
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(PackageCookTask);

	PackageCookTask()
	{
	}

	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) SEOUL_OVERRIDE
	{
		auto pConfig = rContext.GetPackageCookConfig();

		// Early out, package cooking is disabled.
		if (!pConfig)
		{
			return true;
		}

		// Tracking.
		m_iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();
		Bool bMissingFiles = false;

		// Progress.
		rContext.AdvanceProgress(
			GetProgressType(rContext),
			(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - m_iStartTimeInTicks),
			0.0f,
			0u,
			pConfig->m_vPackages.GetSize());

		// Accumulate roots (files in Data/Config).
		if (!GatherConfigFiles(rContext, *pConfig))
		{
			goto fail;
		}

		// Starting from roots, trace files and gather dependencies/files
		// referenced by the application.
		if (!GatherDependencies(rContext, bMissingFiles) || bMissingFiles)
		{
			goto fail;
		}

		// Now process the set of packages.
		if (!ProcessPackages(rContext, *pConfig))
		{
			goto fail;
		}

		rContext.CompleteProgress(
			GetProgressType(rContext),
			(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - m_iStartTimeInTicks),
			true);
		return true;

	fail:
		rContext.CompleteProgress(
			GetProgressType(rContext),
			(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - m_iStartTimeInTicks),
			false);
		return false;
	}

	virtual Int32 GetPriority() const SEOUL_OVERRIDE
	{
		return CookPriority::kPackage;
	}

	virtual Bool ValidateContentEnvironment(ICookContext& rContext) SEOUL_OVERRIDE
	{
		// Early out if no config.
		if (nullptr == rContext.GetPackageCookConfig())
		{
			return true;
		}

		// Context platform must match the config platform.
		if (rContext.GetPlatform() != rContext.GetPackageCookConfig()->m_ePlatform)
		{
			SEOUL_LOG_COOKING("Context platform %s does not match package platform %s.",
				EnumToString<Platform>(rContext.GetPlatform()),
				EnumToString<Platform>(rContext.GetPackageCookConfig()->m_ePlatform));
			return false;
		}

		return true;
	}

private:
	SEOUL_DISABLE_COPY(PackageCookTask);

	Int64 m_iStartTimeInTicks = 0;
	typedef Vector<FilePath, MemoryBudgets::Cooking> Settings;
	Settings m_vSettings;
	typedef HashTable< FilePath,  SharedPtr<DataStore>, MemoryBudgets::Cooking > ResolvedSettings;
	ResolvedSettings m_tResolvedSettings;
	typedef HashSet< FilePath, MemoryBudgets::Cooking > DependencySet;
	typedef Vector< FilePath, MemoryBudgets::Cooking > DependencyVector;
	DependencySet m_DependencySet;
	DependencyVector m_vDependencyVector;

	Bool GatherConfigFiles(ICookContext& rContext, const PackageCookConfig& config)
	{
		FilePath dir;
		dir.SetDirectory(GameDirectory::kConfig);
		Vector<String> vsFiles;
		if (FileManager::Get()->IsDirectory(dir) &&
			!FileManager::Get()->GetDirectoryListing(
			dir,
			vsFiles,
			false,
			true,
			FileTypeToCookedExtension(FileType::kJson)))
		{
			SEOUL_LOG_COOKING("Failed listing .json files for tracing package roots.");
			return false;
		}

		// Reserve.
		m_vSettings.Clear();
		m_vSettings.Reserve(vsFiles.GetSize());
		m_tResolvedSettings.Reserve(vsFiles.GetSize());
		for (auto const& s : vsFiles)
		{
			// Skip if excluded.
			auto const filePath = FilePath::CreateConfigFilePath(s);
			if (config.IsExcludedFromConfigs(filePath))
			{
				continue;
			}

			DataStoreSchemaCache* pCache = nullptr;
#if !SEOUL_SHIP
			pCache = SettingsManager::Get()->GetSchemaCache();
#endif // /#if !SEOUL_SHIP

			// Load settings.
			SharedPtr<DataStore> p(SEOUL_NEW(MemoryBudgets::Cooking) DataStore);
			if (!DataStoreParser::FromFile(pCache, filePath, *p, DataStoreParserFlags::kLogParseErrors))
			{
				SEOUL_LOG_COOKING("Failed loading config file \"%s\", for dependencies and packaging.", filePath.CStr());
				return false;
			}

			m_vSettings.PushBack(filePath);
			SEOUL_VERIFY(m_tResolvedSettings.Insert(filePath, p).Second);
		}

		return true;
	}

	Bool GatherDependenciesSceneAsset(FilePath filePath, Bool& rbMissingFiles)
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		if (!FileManager::Get()->ReadAll(filePath, pC, uC, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed reading scene asset from disk.", filePath.CStr());
			return false;
		}

		void* pU = nullptr;
		UInt32 uU = 0u;
		Bool const bSuccess = ZSTDDecompress(pC, uC, pU, uU);
		MemoryManager::Deallocate(pC);

		if (!bSuccess)
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed decompressing scene asset data.", filePath.CStr());
			return false;
		}

		FullyBufferedSyncFile file(pU, uU, true);

		auto const uFileSize = file.GetSize();
		while (true)
		{
			// Get current position, terminate with failure
			// on failure to acquire.
			Int64 iGlobalOffset = 0;
			if (!file.GetCurrentPositionIndicator(iGlobalOffset) ||
				iGlobalOffset < 0 ||
				(UInt64)iGlobalOffset > uFileSize)
			{
				return false;
			}

			// If offset is at the end, break out of the loop
			if ((UInt64)iGlobalOffset == uFileSize)
			{
				break;
			}

			// Get the tag chunk.
			Int32 iTag = 0;
			if (!ReadInt32(file, iTag))
			{
				SEOUL_LOG_COOKING("%s: dependency scan, failed reading tag identifier.", filePath.CStr());
				return false;
			}

			UInt32 uSizeInBytes = 0u;
			if (!ReadUInt32(file, uSizeInBytes))
			{
				SEOUL_LOG_COOKING("%s: dependency scan, failed reading tag size.", filePath.CStr());
				return false;
			}

			// Process - all but material libraries are skipped.
			switch (iTag)
			{
			case DataTypeAnimationClip:
			case DataTypeAnimationSkeleton:
			case DataTypeMesh:
				// Skip.
				if (!file.Seek((Int64)uSizeInBytes, File::kSeekFromCurrent))
				{
					SEOUL_LOG_COOKING("%s: dependency scan, failed seeking to skip chunk.", filePath.CStr());
					return false;
				}
				break;
			case DataTypeMaterialLibrary:
			{
				if (!VerifyDelimiter(DataTypeMaterialLibrary, file))
				{
					SEOUL_LOG_COOKING("%s: dependency scan, material library delimiter is invalid.", filePath.CStr());
					return false;
				}

				// Read and process.
				UInt32 uMaterials = 0u;
				if (!ReadUInt32(file, uMaterials))
				{
					SEOUL_LOG_COOKING("%s: dependency scan, material library count is invalid.", filePath.CStr());
					return false;
				}

				String sUnused;
				for (UInt32 u = 0u; u < uMaterials; ++u)
				{
					if (!VerifyDelimiter(DataTypeMaterial, file))
					{
						SEOUL_LOG_COOKING("%s: dependency scan, material delimiter is invalid.", filePath.CStr());
						return false;
					}
					if (!ReadString(file, sUnused))
					{
						SEOUL_LOG_COOKING("%s: dependency scan, failed reading material technique name.", filePath.CStr());
						return false;
					}

					UInt32 uParameters = 0u;
					if (!ReadUInt32(file, uParameters))
					{
						SEOUL_LOG_COOKING("%s: dependency scan, failed reading material parameter count.", filePath.CStr());
						return false;
					}

					for (UInt32 uParam = 0u; uParam < uParameters; ++uParam)
					{
						if (!VerifyDelimiter(DataTypeMaterialParameter, file))
						{
							SEOUL_LOG_COOKING("%s: dependency scan, parameter %u delimiter is invalid.", filePath.CStr(), uParam);
							return false;
						}
						if (!ReadString(file, sUnused))
						{
							SEOUL_LOG_COOKING("%s: dependency scan, failed reading material parameter semantic name.", filePath.CStr());
							return false;
						}

						UInt32 uType = 0u;
						if (!ReadUInt32(file, uType))
						{
							SEOUL_LOG_COOKING("%s: dependency scan, failed reading material parameter type enum.", filePath.CStr());
							return false;
						}

						switch ((MaterialParameterType)uType)
						{
						case MaterialParameterType::kTexture:
						{
							FilePath depFilePath;
							if (!ReadFilePath(file, GameDirectory::kContent, depFilePath))
							{
								SEOUL_LOG_COOKING("%s: dependency scan, failed reading material texture parameter file path.", filePath.CStr());
								return false;
							}

							if (!GatherDependencies(filePath, depFilePath, rbMissingFiles))
							{
								return false;
							}
						}
						break;
						case MaterialParameterType::kFloat:
						{
							Float fUnused;
							if (!ReadSingle(file, fUnused))
							{
								SEOUL_LOG_COOKING("%s: dependency scan, failed reading material float parameter.", filePath.CStr());
								return false;
							}
						}
						break;
						case MaterialParameterType::kVector4D:
						{
							Vector4D vUnused;
							if (!ReadVector4D(file, vUnused))
							{
								SEOUL_LOG_COOKING("%s: dependency scan, failed reading material float4 parameter.", filePath.CStr());
								return false;
							}
						}
						default:
							SEOUL_LOG_COOKING("%s: dependency scan, failed reading material parameters, encountered invalid type value '%u'.", filePath.CStr(), uType);
							return false;
						};
					}
				}
			}
			break;

			default:
				SEOUL_LOG_COOKING("%s: dependency scan, encountered invalid asset chunk tag in asset '%d'.", filePath.CStr(), iTag);
				return false;
			};
		}

		return true;
	}

	Bool GatherDependenciesScenePrefab(FilePath filePath, Bool& rbMissingFiles)
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		if (!FileManager::Get()->ReadAll(filePath, pC, uC, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed reading scene prefab from disk.", filePath.CStr());
			return false;
		}

		void* pU = nullptr;
		UInt32 uU = 0u;
		Bool const bSuccess = ZSTDDecompress(pC, uC, pU, uU);
		MemoryManager::Deallocate(pC);

		if (!bSuccess)
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed decompressing scene prefab data.", filePath.CStr());
			return false;
		}

		FullyBufferedSyncFile file(pU, uU, true);

		DataStore dataStore;
		if (!dataStore.Load(file))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed loading scene prefab data into data store.", filePath.CStr());
			return false;
		}

		return GatherDependencies(filePath, dataStore, dataStore.GetRootNode(), rbMissingFiles);
	}

	Bool GatherDependenciesSoundProject(FilePath filePath, Bool& rbMissingFiles)
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		if (!FileManager::Get()->ReadAll(filePath, pC, uC, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed reading sound project from disk.", filePath.CStr());
			return false;
		}

		void* pU = nullptr;
		UInt32 uU = 0u;
		Bool const bSuccess = ZSTDDecompress(pC, uC, pU, uU);
		MemoryManager::Deallocate(pC);

		if (!bSuccess)
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed decompressing sound project data.", filePath.CStr());
			return false;
		}

		// Populate a stream buffer for reading.
		StreamBuffer buffer;
		{
			Byte* p = (Byte*)pU;
			pU = nullptr;
			buffer.TakeOwnership(p, uU);
			uU = 0u;
		}

		// Read
		auto const sBase(Path::GetDirectoryName(filePath.GetAbsoluteFilename()));
		SoundUtil::BankFiles vFiles;
		SoundUtil::EventDependencies tEvents;
		if (!SoundUtil::ReadBanksAndEvents(sBase, buffer, vFiles, tEvents))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed reading project body.", filePath.CStr());
			return false;
		}

		// Track.
		for (auto const& e : vFiles)
		{
			if (!GatherDependencies(filePath, e, rbMissingFiles))
			{
				return false;
			}
		}
		for (auto const& evt : tEvents)
		{
			for (auto const& e : evt.Second)
			{
				if (!GatherDependencies(filePath, e, rbMissingFiles))
				{
					return false;
				}
			}
		}

		return true;
	}

	Bool ShouldReportMissing(FilePath from, FilePath to) const
	{
		// Missing textures are allowed when referenced
		// from FxBank or Json content.
		if (IsTextureFileType(to.GetType()))
		{
			if (from.GetType() == FileType::kFxBank ||
				from.GetType() == FileType::kJson)
			{
				return false;
			}
		}

		// Otherwise, must exist.
		return true;
	}

	Bool GatherDependencies(FilePath from, FilePath to, Bool& rbMissingFiles)
	{
		// General purpose missing file handling.
		if (!FileManager::Get()->Exists(to))
		{
			if (ShouldReportMissing(from, to))
			{
				SEOUL_LOG_COOKING("%s: dependency \"%s\" does not exist on disk.",
					from.GetAbsoluteFilenameInSource().CStr(),
					to.GetAbsoluteFilenameInSource().CStr());
				rbMissingFiles = true;
			}

			return true; // Want the scan to continue so we can report multiple missing files.
		}

		// Root add.
		{
			FilePath innerFilePath = to;
			if (IsTextureFileType(innerFilePath.GetType()))
			{
				innerFilePath.SetType(FileType::FIRST_TEXTURE_TYPE);
			}

			do
			{
				// In this case, filePath is a dependency.
				if (!m_DependencySet.Insert(innerFilePath).Second)
				{
					// Already processed, early out.
					return true;
				}

				// Append to vector also.
				m_vDependencyVector.PushBack(innerFilePath);

				if (IsTextureFileType(innerFilePath.GetType()))
				{
					if (innerFilePath.GetType() == FileType::LAST_TEXTURE_TYPE)
					{
						break;
					}
					else
					{
						innerFilePath.SetType((FileType)((Int32)innerFilePath.GetType() + 1));
					}
				}
			} while (IsTextureFileType(innerFilePath.GetType()));
		}

		// Some types can have sub dependencies.
		switch (to.GetType())
		{
#if SEOUL_WITH_ANIMATION_2D
		case FileType::kAnimation2D:
			return GatherDependenciesAnimation2D(to, rbMissingFiles);
#endif // /#if SEOUL_WITH_ANIMATION_2D
		case FileType::kFxBank:
			return GatherDependenciesFxBank(to, rbMissingFiles);
		case FileType::kSceneAsset:
			return GatherDependenciesSceneAsset(to, rbMissingFiles);
		case FileType::kScenePrefab:
			return GatherDependenciesScenePrefab(to, rbMissingFiles);
		case FileType::kSoundProject:
			return GatherDependenciesSoundProject(to, rbMissingFiles);
		case FileType::kUIMovie:
			return GatherDependenciesUIMovie(to, rbMissingFiles);
		default:
			// Nop
			return true;
		};
	}

	/**
	 * Our animation pipeline supports
	 * "palettes". These are defines as additional directories
	 * that are siblings to the base (typically named images/) directory,
	 * that have exact replacements of the images defined in the
	 * base directory.
	 *
	 * These images need to be added as dependencies in addition
	 * to those in the base.
	 */
	Bool AddPalettes(
		const HashSet<String, MemoryBudgets::Cooking> baseFilenameSet,
		FilePath animationFilePath,
		Bool& rbMissingFiles)
	{
		// Get the images base folder from one existing entry. If existing
		// is empty, we're done.
		if (baseFilenameSet.IsEmpty())
		{
			return true;
		}

		// List PNG images in the animation's directory.
		Vector<String> vsFiles;
		String const sBaseSourcePath(
			Path::GetDirectoryName(animationFilePath.GetAbsoluteFilenameInSource()));
		if (FileManager::Get()->IsDirectory(sBaseSourcePath) &&
			!FileManager::Get()->GetDirectoryListing(
			sBaseSourcePath,
			vsFiles,
			false,
			true,
			".png"))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed listing .png files to gather palette of animation data.", animationFilePath.CStr());
			return false;
		}

		// Iterate and accumulate.
		for (auto const& s : vsFiles)
		{
			auto const filePath = FilePath::CreateContentFilePath(s);

			// Treat any file that matches base filename as a dependency.
			if (baseFilenameSet.HasKey(Path::GetFileName(
				filePath.GetRelativeFilenameInSource()).ToLowerASCII()))
			{
				if (!GatherDependencies(animationFilePath, filePath, rbMissingFiles))
				{
					return false;
				}
			}
		}

		return true;
	}

#if SEOUL_WITH_ANIMATION_2D
	Bool GatherDependenciesAnimation2D(FilePath filePath, Bool& rbMissingFiles)
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		if (!FileManager::Get()->ReadAll(filePath, pC, uC, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed reading 2D animation data from disk.", filePath.CStr());
			return false;
		}

		// Deobfuscate.
		Animation2D::Obfuscate((Byte*)pC, uC, filePath);

		void* pU = nullptr;
		UInt32 uU = 0u;
		Bool bSuccess = ZSTDDecompress(pC, uC, pU, uU);
		MemoryManager::Deallocate(pC);

		if (!bSuccess)
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed decompressing 2D animation data.", filePath.CStr());
			return false;
		}

		SharedPtr<Animation2D::DataDefinition> pData(SEOUL_NEW(MemoryBudgets::Rendering) Animation2D::DataDefinition(filePath));
		StreamBuffer buffer;
		buffer.TakeOwnership((Byte*)pU, uU);
		Animation2D::ReadWriteUtil util(buffer);
		if (!util.BeginRead())
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed loading 2D animation data into a data definition (begin read failed).", filePath.CStr());
			return false;
		}
		if (!pData->Load(util))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed loading 2D animation data into a data definition.", filePath.CStr());
			return false;
		}

		HashSet<String, MemoryBudgets::Cooking> baseFilenameSet;
		auto const& tSkins = pData->GetSkins();
		for (auto const& skin : tSkins)
		{
			for (auto const& slot : skin.Second)
			{
				for (auto const& attach : slot.Second)
				{
					auto const& pAttach = attach.Second;
					auto const eType = pAttach->GetType();
					FilePath innerFilePath;
					using namespace Animation2D;
					switch (eType)
					{
					case AttachmentType::kBitmap: innerFilePath = ((BitmapAttachment const*)pAttach.GetPtr())->GetFilePath(); break;
					case AttachmentType::kLinkedMesh: innerFilePath = ((LinkedMeshAttachment const*)pAttach.GetPtr())->GetFilePath(); break;
					case AttachmentType::kMesh: innerFilePath = ((MeshAttachment const*)pAttach.GetPtr())->GetFilePath(); break;
					default:
						break;
					};

					if (innerFilePath.IsValid())
					{
						if (!GatherDependencies(filePath, innerFilePath, rbMissingFiles))
						{
							return false;
						}
						else
						{
							(void)baseFilenameSet.Insert(Path::GetFileName(innerFilePath.GetRelativeFilenameInSource()).ToLowerASCII());
						}
					}
				}
			}
		}

		// Now append any palette dependencies.
		return AddPalettes(baseFilenameSet, filePath, rbMissingFiles);
	}
#endif

	Bool GatherDependenciesFxBank(FilePath filePath, Bool& rbMissingFiles)
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		if (!FileManager::Get()->ReadAll(filePath, pC, uC, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed reading fx bank data from disk.", filePath.CStr());
			return false;
		}

		void* pU = nullptr;
		UInt32 uU = 0u;
		Bool const bSuccess = ZSTDDecompress(pC, uC, pU, uU);
		MemoryManager::Deallocate(pC);

		if (!bSuccess)
		{
			SEOUL_LOG_COOKING("%s: dependency scan, failed decompressing fx bank data.", filePath.CStr());
			return false;
		}

		// Scan for '.' characters.
		Byte const* s = (Byte const*)pU;
		Byte const* const sBegin = s;
		Byte const* const sEnd = (s + uU);
		while (s < sEnd)
		{
			if (*s != '.')
			{
				++s;
				continue;
			}

			auto sStart = s;
			while (s < sEnd)
			{
				if (*s == 0 || *s == '"')
				{
					break;
				}
				++s;
			}

			auto const eType = ExtensionToFileType(String(sStart, (UInt)(s - sStart)));
			if (FileType::kUnknown != eType)
			{
				// Possible dependency, find start.
				while (sStart > sBegin)
				{
					if (*sStart == 0 || *sStart == '"')
					{
						++sStart;
						break;
					}
					--sStart;
				}

				String const sDep(sStart, (UInt)(s - sStart));

				FilePath depFilePath;
				if (!DataStoreParser::StringAsFilePath(sDep, depFilePath))
				{
					depFilePath = FilePath::CreateContentFilePath(sDep);
				}

				if (!GatherDependencies(filePath, depFilePath, rbMissingFiles))
				{
					MemoryManager::Deallocate(pU);
					return false;
				}
			}
		}

		MemoryManager::Deallocate(pU);
		return true;
	}

	Bool GatherDependenciesUIMovie(FilePath filePath, Bool& rbMissingFiles)
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
				SEOUL_LOG_COOKING("%s: dependency scan, failed reading UI Movie data from disk.", filePath.CStr());
				return false;
			}

			if (!ZSTDDecompress(pC, uC, pU, uU))
			{
				SEOUL_LOG_COOKING("%s: dependency scan, failed decompressing UI Movie data.", filePath.CStr());
				return false;
			}
		}

		Falcon::FCNFile::FCNDependencies v;
		if (!Falcon::FCNFile::GetFCNFileDependencies(filePath, pU, uU, v))
		{
			return false;
		}

		for (auto const& otherFilePath : v)
		{
			if (!GatherDependencies(filePath, otherFilePath, rbMissingFiles))
			{
				return false;
			}
		}

		return true;
	}

	Bool GatherDependencies(FilePath filePath, const DataStore& dataStore, const DataNode& dataNode, Bool& rbMissingFiles)
	{
		switch (dataNode.GetType())
		{
		case DataNode::kArray:
		{
			UInt32 uArrayCount = 0u;
			SEOUL_VERIFY(dataStore.GetArrayCount(dataNode, uArrayCount));

			for (UInt32 i = 0u; i < uArrayCount; ++i)
			{
				DataNode child;
				SEOUL_VERIFY(dataStore.GetValueFromArray(dataNode, i, child));
				if (!GatherDependencies(filePath, dataStore, child, rbMissingFiles))
				{
					return false;
				}
			}
		}
		break;
		case DataNode::kFilePath:
		{
			FilePath depFilePath;
			SEOUL_VERIFY(dataStore.AsFilePath(dataNode, depFilePath));

			if (!FileManager::Get()->Exists(depFilePath))
			{
				if (ShouldReportMissing(filePath, depFilePath))
				{
					SEOUL_LOG_COOKING("%s: dependency \"%s\" does not exist on disk.",
						filePath.GetAbsoluteFilenameInSource().CStr(),
						depFilePath.GetAbsoluteFilenameInSource().CStr());
					rbMissingFiles = true;
				}
				return true;
			}

			if (!GatherDependencies(filePath, depFilePath, rbMissingFiles))
			{
				return false;
			}
		}
		break;
		case DataNode::kTable:
		{
			auto const iBegin = dataStore.TableBegin(dataNode);
			auto const iEnd = dataStore.TableEnd(dataNode);

			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (!GatherDependencies(filePath, dataStore, i->Second, rbMissingFiles))
				{
					return false;
				}
			}
		}
		break;
		default:
			break;
		};

		return true;
	}

	Bool GatherLocDependencies(FilePath filePath, const DataStore& dataStore, const DataNode& dataNode, Bool& rbMissingFiles)
	{
		auto const eType = dataNode.GetType();
		switch (eType)
		{
		case DataNode::kString:
		{
			Byte const* s = nullptr;
			UInt32 u = 0u;
			SEOUL_VERIFY(dataStore.AsString(dataNode, s, u));

			// Check for possible XML tags, then parse and process.
			auto const p = strchr(s, '<');
			if (nullptr == p || (UInt32)(p - s) >= u)
			{
				return true;
			}

			pugi::xml_document rootDocument;
			pugi::xml_parse_result result = rootDocument.load_buffer(
				(void const*)s,
				(size_t)u,
				pugi::parse_default | pugi::parse_fragment | pugi::parse_ws_pcdata,
				pugi::encoding_utf8);

			// Process all fragments
			Bool errorFound = false;

			// Now search for an "img" tag.
			auto node = rootDocument.find_node([&](const pugi::xml_node& node) {
				if (0 != strcmp(node.name(), "img"))
				{
					return false; // tell predicate to keep going. e.g: continue
				}

				if (node.empty())
				{
					return false; // tell predicate to keep going. e.g: continue
				}

				auto src = node.attribute("src");
				if (src.empty())
				{
					return false; // tell predicate to keep going. e.g: continue
				}

				// Src with a substitution pattern is not expected to exist.
				if (nullptr != strstr(src.value(), "${"))
				{
					return false; // tell predicate to keep going. e.g: continue
				}

				FilePath depFilePath;
				if (!DataStoreParser::StringAsFilePath(src.value(), depFilePath))
				{
					return false; // tell predicate to keep going. e.g: continue
				}

				// error cases
				if (!FileManager::Get()->Exists(depFilePath))
				{
					if (ShouldReportMissing(filePath, depFilePath))
					{
						SEOUL_LOG_COOKING("%s: dependency \"%s\" does not exist on disk.",
											filePath.GetAbsoluteFilenameInSource().CStr(),
											depFilePath.GetAbsoluteFilenameInSource().CStr());
						rbMissingFiles = true;
					}
					return false; // break predicate
				}

				if (!GatherDependencies(filePath, depFilePath, rbMissingFiles))
				{
					errorFound = true; // break graph traversal
					return true; // break predicate
				}

				return false;
			});

			return !errorFound;
		}
		break;
		case DataNode::kTable:
		{
			auto const iBegin = dataStore.TableBegin(dataNode);
			auto const iEnd = dataStore.TableEnd(dataNode);

			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (!GatherLocDependencies(filePath, dataStore, i->Second, rbMissingFiles))
				{
					return false;
				}
			}
		}
		break;
		default:
			SEOUL_LOG_COOKING("%s: unexpected data node type in locale file: %s", filePath.CStr(), EnumToString<DataNode::Type>(eType));
			return false;
		};

		return true;
	}

	Bool GatherDependencies(ICookContext& rContext, Bool& rbMissingFiles)
	{
		for (auto const& filePath : m_vSettings)
		{
			auto pp = m_tResolvedSettings.Find(filePath);
			SEOUL_ASSERT(nullptr != pp);
			auto const& p = *pp;

			if (0 == strncmp(filePath.GetRelativeFilenameWithoutExtension().CStr(), "Loc" SEOUL_DIR_SEPARATOR, 4))
			{
				if (!GatherLocDependencies(filePath, *p, p->GetRootNode(), rbMissingFiles))
				{
					return false;
				}
			}
			else
			{
				if (!GatherDependencies(filePath, *p, p->GetRootNode(), rbMissingFiles))
				{
					return false;
				}
			}
		}

		return true;
	}

	struct FileListEntry SEOUL_SEALED
	{
	private:
		FilePath m_FilePath{};
		String m_sSortKey{};

	public:
		FilePath GetFilePath() const { return m_FilePath; }
		void SetFilePath(FilePath filePath)
		{
			m_FilePath = filePath;
			m_sSortKey = filePath.GetRelativeFilename().ToLowerASCII();
		}

		UInt64 m_uModifiedTime{};
		UInt64 m_uUncompressedSize{};

		Bool operator<(const FileListEntry& b) const
		{
			// Sort by timestamp first.
			if (m_uModifiedTime != b.m_uModifiedTime)
			{
				return (m_uModifiedTime < b.m_uModifiedTime);
			}

			// Next by type - "reverse" since we want mip levels of
			// lower resolution first in the .sar
			auto const eTypeA = m_FilePath.GetType();
			auto const eTypeB = b.m_FilePath.GetType();
			if (eTypeA != eTypeB)
			{
				return (Int)eTypeA > (Int)eTypeB;
			}

			// Name key.
			return m_sSortKey < b.m_sSortKey;
		}
	};
	typedef HashSet<DeltaKey, MemoryBudgets::Cooking> DeltaSet;
	typedef Vector<FileListEntry, MemoryBudgets::Cooking> FileList;

	struct DefaultSarSorter SEOUL_SEALED
	{
		Bool operator()(const FileListEntry& a, const FileListEntry& b) const
		{
			auto const bA = IsTextureFileType(a.GetFilePath().GetType());
			auto const bB = IsTextureFileType(b.GetFilePath().GetType());
			if (!bA && bB)
			{
				return true;
			}
			else if (bA && !bB)
			{
				return false;
			}
			else if (bA && bB)
			{
				return (Int32)a.GetFilePath().GetType() > (Int32)b.GetFilePath().GetType();
			}
			else
			{
				return false;
			}
		}
	};

	struct OverflowAddressSorter SEOUL_SEALED
	{
		Bool operator()(const FileList::Iterator& a, const FileList::Iterator& b) const
		{
			return (a < b);
		}
	};

	struct OverflowSizeSorter SEOUL_SEALED
	{
		typedef Pair<Int64, FileList::Iterator> Entry;

		Bool operator()(const Entry& a, const Entry& b) const
		{
			// In all other cases, just sort by size.
			// Sort by the computed size.
			return (a.First > b.First);
		}
	};

	Bool GetDeltaFileCrc32Set(const PackageCookConfig& config, const PackageConfig& pkg, DeltaSet& r) const
	{
		DeltaSet set;
		for (auto const& s : pkg.m_vDeltaArchives)
		{
			// Open the base archive and reach the base path from it.
			ScopedPtr<PackageFileSystem> pBase;
			if (!ResolveBaseArchiveForPatch(s, config, pkg, pBase))
			{
				return false;
			}

			auto const& t = pBase->GetFileTable();
			auto const iBegin = t.Begin();
			auto const iEnd = t.End();

			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (!set.Insert(DeltaKey::Create(i->First, i->Second.m_Entry.m_uCompressedFileSize, i->Second.m_Entry.m_uCrc32Pre)).Second)
				{
					SEOUL_LOG_COOKING("%s: expected failure inserting \"%s\" into delta set, invalid duplicate entry.", pkg.m_sName.CStr(), i->First.CStr());
					return false;
				}
			}
		}

		r.Swap(set);
		return true;
	}

	typedef HashSet<FilePath, MemoryBudgets::Cooking> FileSet;
	Bool ShouldIncludeFile(const PackageConfig& pkg, const FileSet& set, FilePath filePath) const
	{
		if (set.HasKey(filePath))
		{
			return false;
		}

		return pkg.ShouldIncludeFile(filePath);
	}

	Bool ResolveBaseArchiveForPatch(
		const String& sName,
		const PackageCookConfig& config,
		const PackageConfig& pkg,
		ScopedPtr<PackageFileSystem>& rp) const
	{
		auto const sPath(Path::ReplaceExtension(
			Path::Combine(Path::GetDirectoryName(config.m_sAbsoluteConfigFilename), sName),
			".sar"));

		ScopedPtr<PackageFileSystem> p(SEOUL_NEW(MemoryBudgets::Cooking) PackageFileSystem(sPath));
		if (!p->IsOk())
		{
			SEOUL_LOG_COOKING("%s: Locale base archive \"%s\" is invalid, failed reading timestamp for base locale file during locale patch.", pkg.m_sName.CStr(), sName.CStr());
			return false;
		}

		rp.Swap(p);
		return true;
	}

	Bool ResolveLocaleBaseArchive(
		const PackageCookConfig& config,
		const PackageConfig& pkg,
		ScopedPtr<PackageFileSystem>& rp) const
	{
		return ResolveBaseArchiveForPatch(pkg.m_sLocaleBaseArchive, config, pkg, rp);
	}

	Bool GetFileList(
		ICookContext& rContext,
		const PackageCookConfig& config,
		const PackageConfig& pkg,
		FileList& rv) const
	{
		FileSet set;

		// Add additional includes.
		for (auto const& s : pkg.m_vAdditionalIncludes)
		{
			auto const filePath = FilePath::CreateFilePath(pkg.m_eGameDirectoryType, s);

			// We don't use ShouldIncludeFile() here, only a check against set.
			// Our logic is that if an entry is in additional includes, it should
			// *always* be included and ignore any pattern filtering rules.
			if (set.HasKey(filePath))
			{
				continue;
			}

			// Query attributes from the database (provides some
			// caching/optimization over raw file system checks).
			auto const uModTime = FileManager::Get()->GetModifiedTimeForPlatform(rContext.GetPlatform(), filePath);
			auto const uFileSize = FileManager::Get()->GetFileSizeForPlatform(rContext.GetPlatform(), filePath);

			if (0u == uModTime)
			{
				SEOUL_LOG_COOKING("AdditionalInclude \"%s\" does not exist.", s.CStr());
				return false;
			}

			FileListEntry entry;
			entry.SetFilePath(filePath);
			entry.m_uModifiedTime = uModTime;
			entry.m_uUncompressedSize = uFileSize;
			SEOUL_VERIFY(set.Insert(filePath).Second);
			rv.PushBack(entry);
		}

		// Add dependencies if specified.
		if (pkg.m_bPopulateFromDependencies)
		{
			for (auto const& filePath : m_vDependencyVector)
			{
				if (!ShouldIncludeFile(pkg, set, filePath))
				{
					continue;
				}

				FileListEntry entry;
				entry.SetFilePath(filePath);
				entry.m_uModifiedTime = FileManager::Get()->GetModifiedTimeForPlatform(rContext.GetPlatform(), filePath);
				entry.m_uUncompressedSize = FileManager::Get()->GetFileSizeForPlatform(rContext.GetPlatform(), filePath);

				// A few special cases:
				// - the locale path will have the timestamp of the locale file on disk.
				// - the locale file keeps its timestamp in the base archive.
				auto eClass = GetFileClass(pkg, entry.GetFilePath());
				if (FileClass::kLocalePatchFile == eClass)
				{
					// Cache the FilePath used for reading the locale base
					// file.
					auto const baseFilePath = FilePath::CreateConfigFilePath(Path::Combine(
						Path::GetDirectoryName(entry.GetFilePath().GetRelativeFilenameWithoutExtension().ToString()),
						pkg.m_sLocaleBaseFilename));
					auto const uBaseModifiedTime = FileManager::Get()->GetModifiedTime(baseFilePath);
					if (0u == uBaseModifiedTime)
					{
						SEOUL_LOG_COOKING("%s: failed reading modified time for locale base file '%s' on disk when generating locale patch.", pkg.m_sName.CStr(), baseFilePath.CStr());
						return false;
					}

					// Update the modified time.
					entry.m_uModifiedTime = uBaseModifiedTime;
				}
				else if (FileClass::kLocaleBaseFile == eClass)
				{
					// Open the base archive and reach the base path from it.
					ScopedPtr<PackageFileSystem> pLocalePkg;
					if (!ResolveLocaleBaseArchive(config, pkg, pLocalePkg))
					{
						return false;
					}

					UInt64 uBaseModifiedTime = 0u;
					if (!pLocalePkg->GetModifiedTime(entry.GetFilePath(), uBaseModifiedTime))
					{
						SEOUL_LOG_COOKING("%s: failed reading modified time for locale base file '%s' in base locale .sar file when generating locale patch.", pkg.m_sName.CStr(), entry.GetFilePath().CStr());
						return false;
					}

					// Update the modified time.
					entry.m_uModifiedTime = uBaseModifiedTime;
				}

				SEOUL_VERIFY(set.Insert(filePath).Second);
				rv.PushBack(entry);
			}
		}

		// Add any files found with a search pattern if specified.
		if (!pkg.m_vNonDependencySearchPatterns.IsEmpty())
		{
			for (auto const& sPattern : pkg.m_vNonDependencySearchPatterns)
			{
				// TODO: Update data schema so we don't need to do this cleanup.
				String sExtension;
				if ("*.*" != sPattern)
				{
					if (!sPattern.IsEmpty() && sPattern[0] == '*')
					{
						sExtension = sPattern.CStr() + 1;
					}
					else
					{
						sExtension = sPattern;
					}
				}

				Vector<String> vs;
				auto const sDirectory(Path::Combine(GameDirectoryToString(pkg.m_eGameDirectoryType), pkg.GetRoot()));
				if (!FileManager::Get()->GetDirectoryListing(
					sDirectory,
					vs,
					false,
					true,
					sExtension))
				{
					SEOUL_LOG_COOKING("Failed listing directory \"%s\" (root \"%s\"), for search pattern \"%s\"", sDirectory.CStr(), pkg.GetRoot().CStr(), sPattern.CStr());
					return false;
				}

				for (auto const& s : vs)
				{
					auto const filePath = FilePath::CreateFilePath(pkg.m_eGameDirectoryType, s);
					if (!ShouldIncludeFile(pkg, set, filePath))
					{
						continue;
					}

					// For all inclusive queries of content, we need to check if the file
					// exists in source and exclude it if it does not, with some specific exceptions.
					if (GameDirectory::kContent == pkg.m_eGameDirectoryType)
					{
						auto const eType = filePath.GetType();
						// Special handling for sound banks (which don't have metadata) and
						// json (which is the metadata inside the kContent folder).
						switch (eType)
						{
						case FileType::kSoundBank: // TODO:
							break;
						case FileType::kJson:
						{
							// Get the filePath that the metadata is for - this is the relative filename with the extension remove,
							// as a filePath.
							auto const base(FilePath::CreateContentFilePath(filePath.GetRelativeFilenameWithoutExtension().ToString()));
							// This is stale metadata from when we generated an explicit json metadata file for all
							// content files (going forward, we do not generate an explicit file for "one-to-one" files,
							// since it greatly improves cooking performance if we can just do a file-to-file (source-to-content)
							// modified timestamp check).
							if (CookDatabase::IsOneToOneType(base.GetType()))
							{
								continue;
							}
							// Otherwise, we can remove the file if the source of the base no longer exists.
							if (!FileManager::Get()->ExistsInSource(base))
							{
								continue;
							}
						}
						break;
						default:
							// For all other types in content, if the source file
							// does not exist, we can exclude it from the archive
							// as it is now stale.
							if (!FileManager::Get()->ExistsInSource(filePath))
							{
								continue;
							}
							break;
						};
					}

					FileListEntry entry;
					entry.SetFilePath(filePath);
					entry.m_uModifiedTime = FileManager::Get()->GetModifiedTimeForPlatform(rContext.GetPlatform(), filePath);
					entry.m_uUncompressedSize = FileManager::Get()->GetFileSizeForPlatform(rContext.GetPlatform(), filePath);
					SEOUL_VERIFY(set.Insert(filePath).Second);
					rv.PushBack(entry);
				}
			}
		}

		// Handle final ordering.
		if (pkg.m_bSortByModifiedTime)
		{
			QuickSort(rv.Begin(), rv.End());
		}
		else
		{
			DefaultSarSorter sorter;
			StableSort(rv.Begin(), rv.End(), sorter);
		}

		// Resolve the compression dictionary if requested.
		{
			auto const dictPathFile = FilePath::CreateFilePath(
				pkg.m_eGameDirectoryType,
				String::Printf(ksPackageCompressionDictNameFormat, kaPlatformNames[(UInt32)config.m_ePlatform]));

			if (pkg.m_bCompressFiles && pkg.m_bUseCompressionDictionary)
			{
				FileListEntry dictEntry;
				dictEntry.SetFilePath(dictPathFile);
				dictEntry.m_uModifiedTime = FileManager::Get()->GetModifiedTimeForPlatform(rContext.GetPlatform(), dictPathFile);
				dictEntry.m_uUncompressedSize = FileManager::Get()->GetFileSizeForPlatform(rContext.GetPlatform(), dictPathFile);
				rv.Insert(rv.Begin(), dictEntry);
			}
		}

		return true;
	}

	Bool ProcessPackages(ICookContext& rContext, const PackageCookConfig& config)
	{
		UInt32 uComplete = 0u;
		for (auto const& pkg : config.m_vPackages)
		{
			// Progress.
			rContext.AdvanceProgress(
				GetProgressType(rContext),
				(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - m_iStartTimeInTicks),
				(Float)uComplete / (Float)config.m_vPackages.GetSize(),
				1u,
				config.m_vPackages.GetSize() - uComplete);

			if (!ProcessPackage(rContext, config, *pkg))
			{
				return false;
			}

			// Tracking.
			++uComplete;
		}

		return true;
	}

	Bool ProcessPackage(
		ICookContext& rContext,
		const PackageCookConfig& config,
		const PackageConfig& pkg)
	{
		// Non-local packages are excluded from local builds.
		// Post serialize will have converted this from an opt-in
		// to enabled/disabled based on -local command-line argument.
		if (pkg.m_bExcludeFromLocal)
		{
			return true;
		}

		// Process based on type.
		if (pkg.m_bZipArchive)
		{
			return ProcessZipArchive(rContext, config, pkg);
		}
		else
		{
			return ProcessSarArchive(rContext, config, pkg);
		}
	}

	Bool WriteManifest(
		ICookContext& rContext,
		const PackageConfig& pkg,
		const String& sPackageFilename,
		const String& sPackageManifestFilename) const
	{
		StreamBuffer buffer;
		{
			// Open the package file so we can read the header and file table.
			ScopedPtr<SyncFile> pFile;
			if (!FileManager::Get()->OpenFile(sPackageFilename, File::kRead, pFile))
			{
				SEOUL_LOG_COOKING("%s: failed opening package .sar for manifest generation.", sPackageFilename.CStr());
				return false;
			}

			// Read the initial header.
			PackageFileHeader header;
			if (sizeof(header) != pFile->ReadRawData(&header, sizeof(header)))
			{
				SEOUL_LOG_COOKING("%s: failed reading header from package .sar for manifest generation.", sPackageFilename.CStr());
				return false;
			}
			// Check it.
			if (!PackageFileSystem::ReadPackageHeader(&header, sizeof(header), header))
			{
				SEOUL_LOG_COOKING("%s: header is corrupt or invalid as part of manifest generation.", sPackageFilename.CStr());
				return false;
			}

			// Add to buffer.
			buffer.Write(&header, sizeof(header));

			// Now capture the offset and size of the file table.
			auto const iOffset = header.GetOffsetToFileTableInBytes();
			auto const uSize = header.GetSizeOfFileTableInBytes();

			// Now read the file table into place.
			buffer.PadTo(buffer.GetOffset() + uSize, false);
			if (!pFile->Seek(iOffset, File::kSeekFromStart) ||
				uSize != pFile->ReadRawData(buffer.GetBuffer() + buffer.GetOffset() - uSize, uSize))
			{
				SEOUL_LOG_COOKING("%s: failed reading file table from package .sar for manifest generation.", sPackageFilename.CStr());
				return false;
			}
		}

		// Last bit - if the package was configured to use a compression dict, append that after the header
		// and file table.
		if (pkg.m_bCompressFiles && pkg.m_bUseCompressionDictionary)
		{
			PackageFileSystem fs(sPackageFilename, false, false, true);
			auto const filePath(fs.GetCompressionDictFilePath());
			if (filePath.IsValid())
			{
				// Get the entry.
				auto pEntry = fs.GetFileTable().Find(filePath);
				if (nullptr == pEntry)
				{
					SEOUL_LOG_COOKING("%s: failed reading compression dict entry from .sar for manifest generation.", sPackageFilename.CStr());
					return false;
				}

				// Get the values.
				auto const uOffset = pEntry->m_Entry.m_uOffsetToFile;
				auto const uSize = (UInt32)pEntry->m_Entry.m_uCompressedFileSize;

				// Pad and add.
				buffer.PadTo(buffer.GetOffset() + uSize, false);
				if (!fs.ReadRaw(uOffset, buffer.GetBuffer() + buffer.GetOffset() - uSize, uSize))
				{
					SEOUL_LOG_COOKING("%s: failed reading compression dict from .sar for manifest generation.", sPackageFilename.CStr());
					return false;
				}
			}
		}

		// Finally, commit all to the manifest file.
		return AtomicWriteFinalOutput(
			rContext,
			buffer.GetBuffer(),
			buffer.GetTotalDataSizeInBytes(),
			sPackageManifestFilename);
	}

	Bool FinalizeWrittenArchive(
		ICookContext& rContext,
		const PackageConfig& pkg,
		const String& sTempFile,
		const String& sOutputFilename) const
	{
		// Commit the archive to its final location.
		auto& scc = rContext.GetSourceControlClient();
		auto const& opt = rContext.GetSourceControlFileTypeOptions(true /* bNeedsExclusiveLock */, true /* bLongLife */);
		Bool bReturn = true;

		bReturn = bReturn && (!pkg.m_bIncludeInSourceControl || scc.OpenForEdit(&sOutputFilename, &sOutputFilename + 1, opt, SEOUL_BIND_DELEGATE(LogError)));
		bReturn = bReturn && AtomicWriteFinalOutput(rContext, sTempFile, sOutputFilename);
		bReturn = bReturn && (!pkg.m_bIncludeInSourceControl || scc.OpenForAdd(&sOutputFilename, &sOutputFilename + 1, opt, SEOUL_BIND_DELEGATE(LogError)));
		bReturn = bReturn && (!pkg.m_bIncludeInSourceControl || scc.RevertUnchanged(&sOutputFilename, &sOutputFilename + 1));

		return bReturn;
	}

	Bool WriteSarArchive(
		ICookContext& rContext,
		const PackageCookConfig& config,
		const PackageConfig& pkg,
		const FileList& vFiles,
		const String& sOutputFilename,
		UInt32 uPackageVariation,
		HashTable<FilePath, String, MemoryBudgets::Cooking> const* ptVariations = nullptr,
		PackageFileSystem* pVariationBase = nullptr) const
	{
		// Generate a temporary file to write the package data.
		auto const sTempFile(Path::GetTempFileAbsoluteFilename());
		auto const scoped(MakeScopedAction([]() {}, [&]()
		{
			(void)FileManager::Get()->Delete(sTempFile);
		}));

		// Write the body - must be scoped so we close and flush the file.
		{
			ScopedPtr<SyncFile> pFile;
			if (!FileManager::Get()->OpenFile(sTempFile, File::kWriteTruncate, pFile))
			{
				SEOUL_LOG_COOKING("Failed opening output temp file \"%s\" for writing .sar file.", sTempFile.CStr());
				return false;
			}

			// TODO: Add big endian support.
			SEOUL_STATIC_ASSERT(SEOUL_LITTLE_ENDIAN);

			// Cache some runtime values. We clamp both
			// to a minimum of 1, as 0 has special meaning
			// and may result in incorrect behavior
			// at runtime (particularly with the patching system).
			UInt32 const uBuildChangelist = Max(g_iBuildChangelist, 1);
			UInt32 const uBuildVersionMajor = Max(BUILD_VERSION_MAJOR, 1);

			auto& f = *pFile;

			// The rest of this body depends on a known PackageFileHeader size.
			SEOUL_STATIC_ASSERT(sizeof(PackageFileHeader) == 48);

			// Ensure we update this block if the version changes.
			SEOUL_STATIC_ASSERT(21 == kuPackageVersion);

			// Signature
			if (!WriteUInt32(f, kuPackageSignature))
			{
				SEOUL_LOG_COOKING("%s: failed writing package signature.", pkg.m_sName.CStr());
				return false;
			}

			// Current version - must match what the game expects.
			if (!WriteUInt32(f, kuPackageVersion))
			{
				SEOUL_LOG_COOKING("%s: failed writing package version.", pkg.m_sName.CStr());
				return false;
			}

			// Mark position for later fixup.
			Int64 iTotalPackageFileSizeFixupOffset = 0;
			if (!f.GetCurrentPositionIndicator(iTotalPackageFileSizeFixupOffset))
			{
				SEOUL_LOG_COOKING("%s: Could not get file position while writing .sar file.", pkg.m_sName.CStr());
				return false;
			}

			// Total size of the package file in bytes - write out 0 as a placeholder
			// for now, will be fixed up after the archive is otherwise complete.
			if (!WriteUInt64(f, (UInt64)0))
			{
				SEOUL_LOG_COOKING("%s: failed writing placeholder package size.", pkg.m_sName.CStr());
				return false;
			}

			// Mark the position that we will seek to at the end, to fixup
			// the offset to the package's file table.
			Int64 iPackageFileTableOffsetFixupOffset = 0;
			if (!f.GetCurrentPositionIndicator(iPackageFileTableOffsetFixupOffset))
			{
				SEOUL_LOG_COOKING("%s: Could not get file position while writing .sar file.", pkg.m_sName.CStr());
				return false;
			}

			// Offset to file table - write out 0 as a placeholder for now,
			// will be fixed up after the archive is otherwise complete.
			if (!WriteUInt64(f, (UInt64)0))
			{
				SEOUL_LOG_COOKING("%s: failed writing placeholder package file table offset.", pkg.m_sName.CStr());
				return false;
			}

			// Mark the position that we will seek to at the end, to fixup
			// the number of entries in the package's file table.
			Int64 iPackageFileTableNumberOfEntriesFixupOffset = 0;
			if (!f.GetCurrentPositionIndicator(iPackageFileTableNumberOfEntriesFixupOffset))
			{
				SEOUL_LOG_COOKING("%s: Could not get file position while writing .sar file.", pkg.m_sName.CStr());
				return false;
			}

			// Number of entries in the file table - write out 0 as a placeholder,
			// will be fixed up after the archive is otherwise complete.
			if (!WriteUInt32(f, (UInt32)0))
			{
				SEOUL_LOG_COOKING("%s: failed writing placeholder package file table entry count.", pkg.m_sName.CStr());
				return false;
			}

			// Write out the game directory code.
			if (!WriteUInt16(f, (UInt16)pkg.m_eGameDirectoryType))
			{
				SEOUL_LOG_COOKING("%s: failed writing package game directory type entry.", pkg.m_sName.CStr());
				return false;
			}

			// Whether the file table is compressed or not - always compressed.
			if (!WriteUInt16(f, (UInt16)1))
			{
				SEOUL_LOG_COOKING("%s: failed writing package file table compression mode.", pkg.m_sName.CStr());
				return false;
			}

			// Mark the position that we will seek to at the end, to fixup
			// the size of the package's file table.
			Int64 iPackageFileTableSizeFixupOffset = 0;
			if (!f.GetCurrentPositionIndicator(iPackageFileTableSizeFixupOffset))
			{
				SEOUL_LOG_COOKING("%s: Could not get file position while writing .sar file.", pkg.m_sName.CStr());
				return false;
			}

			// Size of the file table in bytes - write out 0 as a placeholder for now,
			// will be fixed up after the archive is otherwise complete.
			if (!WriteUInt32(f, (UInt32)0))
			{
				SEOUL_LOG_COOKING("%s: failed writing package file table placeholder size in bytes.", pkg.m_sName.CStr());
				return false;
			}

			// Package variation, or 0 if non was specified.
			if (!WriteUInt16(f, uPackageVariation))
			{
				SEOUL_LOG_COOKING("%s: failed writing package variation.", pkg.m_sName.CStr());
				return false;
			}

			// Build major version of the content, or 0 if none was specified.
			if (!WriteUInt16(f, uBuildVersionMajor))
			{
				SEOUL_LOG_COOKING("%s: failed writing package build version major.", pkg.m_sName.CStr());
				return false;
			}

			// Changelist of the content, or 0 if none was specified.
			if (!WriteUInt32(f, uBuildChangelist))
			{
				SEOUL_LOG_COOKING("%s: failed writing package build changelist.", pkg.m_sName.CStr());
				return false;
			}

			// Whether or not the package should support directory queries after load.
			if (!WriteUInt16(f, pkg.m_bSupportDirectoryQueries ? (UInt16)1 : (UInt16)0))
			{
				SEOUL_LOG_COOKING("%s: failed writing package support directory queries flag.", pkg.m_sName.CStr());
				return false;
			}

			// Whether or not the package is obfuscated or not.
			if (!WriteUInt8(f, pkg.m_bObfuscate ? (UInt8)1 : (UInt8)0))
			{
				SEOUL_LOG_COOKING("%s: failed writing package obfuscation flag.", pkg.m_sName.CStr());
				return false;
			}

			// Platform of the package.
			if (!WriteUInt8(f, (UInt8)config.m_ePlatform))
			{
				SEOUL_LOG_COOKING("%s: failed writing package platform value.", pkg.m_sName.CStr());
				return false;
			}

			// Pad to the header size.
			if (!PadToAlignment(f, (Int64)sizeof(PackageFileHeader)))
			{
				SEOUL_LOG_COOKING("%s: failed padding after package file header.", pkg.m_sName.CStr());
				return false;
			}

			// File table entries.
			Vector<FileEntry, MemoryBudgets::Cooking> vEntries;

			// Write out the file entries - this will also populate the
			// file entry table
			if (!WriteSarFileEntries(
				rContext,
				config,
				pkg,
				vFiles,
				f,
				vEntries,
				uPackageVariation,
				ptVariations,
				pVariationBase)) { return false; }

			// Values for file table offset and size.
			Int64 iOffsetToFileTable = -1;
			UInt32 uFileTableSizeInBytes = 0;
			UInt32 uFileTableNumberOfEntries = 0;

			// Output stream for file table data.
			{
				MemorySyncFile fileTable;
				for (auto const& e : vEntries)
				{
					if (!WriteUInt64(fileTable, e.m_Entry.m_uOffsetToFile))
					{
						SEOUL_LOG_COOKING("%s: failed writing file \"%s\" offset.", pkg.m_sName.CStr(), e.m_sFileName.CStr());
						return false;
					}
					if (!WriteUInt64(fileTable, e.m_Entry.m_uCompressedFileSize))
					{
						SEOUL_LOG_COOKING("%s: failed writing file \"%s\" compressed file size.", pkg.m_sName.CStr(), e.m_sFileName.CStr());
						return false;
					}
					if (!WriteUInt64(fileTable, e.m_Entry.m_uUncompressedFileSize))
					{
						SEOUL_LOG_COOKING("%s: failed writing file \"%s\" uncompressed file size.", pkg.m_sName.CStr(), e.m_sFileName.CStr());
						return false;
					}
					if (!WriteUInt64(fileTable, e.m_Entry.m_uModifiedTime))
					{
						SEOUL_LOG_COOKING("%s: failed writing file \"%s\" modification time.", pkg.m_sName.CStr(), e.m_sFileName.CStr());
						return false;
					}
					if (!WriteUInt32(fileTable, e.m_Entry.m_uCrc32Pre))
					{
						SEOUL_LOG_COOKING("%s: failed writing file \"%s\" crc32 pre.", pkg.m_sName.CStr(), e.m_sFileName.CStr());
						return false;
					}
					if (!WriteUInt32(fileTable, e.m_Entry.m_uCrc32Post))
					{
						SEOUL_LOG_COOKING("%s: failed writing file \"%s\" crc32 post.", pkg.m_sName.CStr(), e.m_sFileName.CStr());
						return false;
					}
					if (!WriteString(fileTable, e.m_sFileName))
					{
						SEOUL_LOG_COOKING("%s: failed writing file \"%s\" filename.", pkg.m_sName.CStr(), e.m_sFileName.CStr());
						return false;
					}
				}

				// Compress the file table.
				UInt32 uPlaceholder = 0u;
				auto const& buffer = fileTable.GetBuffer();
				void* pC = nullptr;
				UInt32 uC = 0u;
				auto const deferredC(MakeDeferredAction([&]()
					{
						MemoryManager::Deallocate(pC);
						pC = nullptr;
						uC = 0u;
					}));

				if (!ZSTDCompress(
					(nullptr == buffer.GetBuffer() ? (void const*)&uPlaceholder : (void const*)buffer.GetBuffer()),
					buffer.GetTotalDataSizeInBytes(),
					pC,
					uC,
					pkg.GetCompressionLevel()))
				{
					SEOUL_LOG_COOKING("%s: failed compressing file table.", pkg.m_sName.CStr());
					return false;
				}

				// Obfuscate the file table.
				String const sFileTablePsuedoFileName = ToString(uBuildVersionMajor) + ToString(uBuildChangelist);
				Obfuscate(GenerateKey(sFileTablePsuedoFileName), pC, uC);

				// Now that the file table is compressed and obfuscated, compute a post crc32 value
				// for checking its validity at read time.
				auto const uFileTablePostCRC32 = GetCrc32((UInt8 const*)pC, uC);

				// Pad to support efficient reads on some platforms (Android), for
				// the file table.
				if (!PadToAlignment(f, kiFileEntryAlignment))
				{
					SEOUL_LOG_COOKING("%s: failed padding prior to package file table write.", pkg.m_sName.CStr());
					return false;
				}

				// Record relevant values of the file table - must occur here,
				// immediately before writing the file table, so the
				// offset is correct.
				if (!f.GetCurrentPositionIndicator(iOffsetToFileTable))
				{
					SEOUL_LOG_COOKING("%s: failed getting file position to package file table.", pkg.m_sName.CStr());
					return false;
				}

				// Total size in bytes is the compressed data size plus size of the crc32 value.
				uFileTableSizeInBytes = (uC + sizeof(uFileTablePostCRC32));
				uFileTableNumberOfEntries = vEntries.GetSize();

				// Now write the file table data to the archive. Body
				// first followed by its CRC32 value in a "footer".
				if (uC != f.WriteRawData(pC, uC) ||
					!WriteUInt32(f, uFileTablePostCRC32))
				{
					SEOUL_LOG_COOKING("%s: failed writing the package file table to disk.", pkg.m_sName.CStr());
					return false;
				}
			}

			// Compute the total package file size.
			UInt64 zTotalPackageFileSize = 0;
			{
				Int64 i = 0;
				if (!f.GetCurrentPositionIndicator(i))
				{
					SEOUL_LOG_COOKING("%s: failed querying position offset to compute package size.", pkg.m_sName.CStr());
					return false;
				}
				zTotalPackageFileSize = (UInt64)i;
			}

			// Now seek back and fixup file table and size values.
			if (!f.Seek(iTotalPackageFileSizeFixupOffset, File::kSeekFromStart))
			{
				SEOUL_LOG_COOKING("%s: failed seeking to write package total size.", pkg.m_sName.CStr());
				return false;
			}
			if (!WriteUInt64(f, zTotalPackageFileSize))
			{
				SEOUL_LOG_COOKING("%s: failed writing package total size.", pkg.m_sName.CStr());
				return false;
			}

			if (!f.Seek(iPackageFileTableOffsetFixupOffset, File::kSeekFromStart))
			{
				SEOUL_LOG_COOKING("%s: failed seeking to write package file table offset.", pkg.m_sName.CStr());
				return false;
			}
			if (!WriteUInt64(f, (UInt64)iOffsetToFileTable))
			{
				SEOUL_LOG_COOKING("%s: failed writing package file table offset.", pkg.m_sName.CStr());
				return false;
			}

			if (!f.Seek(iPackageFileTableNumberOfEntriesFixupOffset, File::kSeekFromStart))
			{
				SEOUL_LOG_COOKING("%s: failed seeking to write package file table entry count.", pkg.m_sName.CStr());
				return false;
			}
			if (!WriteUInt32(f, (UInt32)uFileTableNumberOfEntries))
			{
				SEOUL_LOG_COOKING("%s: failed writing package file table entry count.", pkg.m_sName.CStr());
				return false;
			}

			if (!f.Seek(iPackageFileTableSizeFixupOffset, File::kSeekFromStart))
			{
				SEOUL_LOG_COOKING("%s: failed seeking to write package file table size in bytes.", pkg.m_sName.CStr());
				return false;
			}
			if (!WriteUInt32(f, (UInt32)uFileTableSizeInBytes))
			{
				SEOUL_LOG_COOKING("%s: failed writing package file table size in bytes.", pkg.m_sName.CStr());
				return false;
			}
		}

		// Done, write the .sar to its final location.
		return FinalizeWrittenArchive(
			rContext,
			pkg,
			sTempFile,
			sOutputFilename);
	}

	Bool GatherVariations(
		const String& sBaseFilename,
		const String& sVariationFile,
		HashTable<FilePath, String, MemoryBudgets::Cooking>& rt) const
	{
		auto const sInput(Path::Combine(Path::GetDirectoryName(sBaseFilename), sVariationFile));

		ScopedPtr<SyncFile> pFile;
		if (!FileManager::Get()->OpenFile(sInput, File::kRead, pFile) || !pFile->CanRead())
		{
			SEOUL_LOG_COOKING("%s: failed opening variation file '%s' for processing.",
				sBaseFilename.CStr(),
				sVariationFile.CStr());
			return false;
		}

		BufferedSyncFile reader(pFile.Get(), false);

		String sLine;
		FilePath filePath;
		String sBody;
		UInt32 uLine = 1u;

		auto const finishBlock = [&]()
		{
			if (filePath.IsValid() && !sBody.IsEmpty())
			{
				// Insertion failure means an additional append to existing append.
				auto const e = rt.Insert(filePath, sBody);
				if (!e.Second)
				{
					e.First->Second.Append(sBody);
				}
			}

			filePath = FilePath();
			sBody.Clear();
		};

		while (reader.ReadLine(sLine))
		{
			if (sLine.StartsWith("@@append_to"))
			{
				finishBlock();

				auto uBegin = sLine.Find('"');
				if (String::NPos == uBegin)
				{
					SEOUL_LOG_COOKING("%s(%u): invalid append_to line.",
						sVariationFile.CStr(),
						uLine);
					return false;
				}
				++uBegin;

				auto const uEnd = sLine.Find('"', uBegin);
				if (String::NPos == uEnd)
				{
					SEOUL_LOG_COOKING("%s(%u): invalid append_to line.",
						sVariationFile.CStr(),
						uLine);
					return false;
				}

				auto const sTarget(sLine.Substring(uBegin, (uEnd - uBegin)));
				filePath = FilePath::CreateConfigFilePath(sTarget);
				if (!filePath.IsValid())
				{
					SEOUL_LOG_COOKING("%s: variation '%s' contains invalid append_to target '%s'",
						sBaseFilename.CStr(),
						sVariationFile.CStr(),
						sTarget.CStr());
					return false;
				}

				if (!FileManager::Get()->Exists(filePath))
				{
					SEOUL_LOG_COOKING("%s: variation '%s'  append_to targets '%s' but this file does not exist.",
						sBaseFilename.CStr(),
						sVariationFile.CStr(),
						filePath.CStr());
					return false;
				}
			}
			else
			{
				sBody.Append(sLine);
			}

			++uLine;
		}

		finishBlock();
		return true;
	}

	Bool WriteVariationArchive(
		ICookContext& rContext,
		const PackageCookConfig& config,
		const PackageConfig& pkg,
		const FileList& vFiles,
		const String& sBaseFilename,
		PackageFileSystem& rBase,
		const String& sVariationFile,
		UInt32 uVariation) const
	{
		auto const sVariationOutputFilename(Path::GetPathWithoutExtension(sBaseFilename) + String::Printf("_Variation_%u%s", uVariation, Path::GetExtension(sBaseFilename).CStr()));

		HashTable<FilePath, String, MemoryBudgets::Cooking> tVariations;
		if (!GatherVariations(sBaseFilename, sVariationFile, tVariations))
		{
			return false;
		}

		return WriteSarArchive(
			rContext,
			config,
			pkg,
			vFiles,
			sVariationOutputFilename,
			uVariation,
			&tVariations,
			&rBase);
	}

	Bool ResolveOverflow(
		ICookContext& rContext,
		const PackageCookConfig& config,
		const PackageConfig& pkg,
		FileList& rvBase,
		FileList& rvOverflow) const
	{
		// Sanity check overflow settings - no overflow,
		// return immediately. Mismatched settings, return
		// immediately as a failure.
		if (pkg.m_sOverflow.IsEmpty()) { return true; }

		if (0 == pkg.m_uOverflowTargetBytes)
		{
			SEOUL_LOG_COOKING("%s: overflow archive \"%s\" specified with 0 overflow bytes.",
				pkg.m_sName.CStr(),
				pkg.m_sOverflow.CStr());
			return false;
		}

		// TODO: Should take into account m_bCompress of the archive,
		// but that would require pre-emptively compressing all the files
		// to determine their final size.

		// First, sum the total size. Check if we need any overflow processing at all.
		UInt64 uTotal = 0u;
		for (auto const& e : rvBase)
		{
			uTotal += e.m_uUncompressedSize;
		}

		// Add in any consider archives.
		{
			auto const sBaseDir(Path::GetDirectoryName(config.m_sAbsoluteConfigFilename));
			for (auto const& e : pkg.m_vOverflowConsider)
			{
				auto const s(Path::Combine(sBaseDir, e + ".sar"));

				// Report error if does not exist.
				auto const u = FileManager::Get()->GetFileSize(s);
				if (0u == u)
				{
					SEOUL_LOG_COOKING("%s: overflow archive \"%s\" includes overflow consider entry "
						"\"%s\" but that file does not exist on disk.",
						pkg.m_sName.CStr(),
						pkg.m_sOverflow.CStr(),
						e.CStr());
					return false;
				}

				uTotal += u;
			}
		}

		// Done early if under the target.
		if (uTotal <= pkg.m_uOverflowTargetBytes)
		{
			rvOverflow.Clear();
			return true;
		}

		// First assemble the list, include mip0, mip1, mip2, and .pbank files.
		FixedArray<UInt32, (UInt32)FileType::FILE_TYPE_COUNT> aCanOverflow;
		FixedArray<UInt64, (UInt32)FileType::FILE_TYPE_COUNT> aCanOverflowBytes;
		FixedArray<UInt32, (UInt32)FileType::FILE_TYPE_COUNT> aExcluded;
		FixedArray<UInt64, (UInt32)FileType::FILE_TYPE_COUNT> aExcludedBytes;

		PackageConfig::OverflowExclusionSet exclusions;
		if (!pkg.ComputeOverflowExclusionSet(
			rContext.GetSourceControlClient().IsNull(),
			config.m_ePlatform,
			exclusions))
		{
			SEOUL_LOG_COOKING("%s: failed resolving overflow exclusions '%s'.",
				pkg.m_sName.CStr(),
				pkg.m_sOverflow.CStr());
			return false;
		}

		UInt64 uOverflowTotalBytes = 0u;
		UInt32 uCanOverflowTotal = 0u;
		UInt64 uCanOverflowTotalBytes = 0u;
		UInt32 uExcludedTotal = 0u;
		UInt64 uExcludedTotalBytes = 0u;
		{
			Vector<FileList::Iterator, MemoryBudgets::Cooking> vOverflow;
			{
				Vector< Pair<Int64, FileList::Iterator>, MemoryBudgets::Cooking> vCanOverflow;
				{
					HashTable<HString, UInt64, MemoryBudgets::Cooking> tMip1Sizes;
					for (auto i = rvBase.Begin(); rvBase.End() != i; ++i)
					{
						auto const eType = i->GetFilePath().GetType();
						if (FileType::kTexture0 == eType ||
							FileType::kTexture1 == eType)
						{
							// Initial size estimate is the file size.
							//
							// Later, we will replace mip0 with the the difference
							// between mip0 and mip1 (how much overflow will increase
							// by switching mip1 for mip0).
							vCanOverflow.PushBack(MakePair((Int64)i->m_uUncompressedSize, i));
						}
						else if (FileType::kTexture2 == eType || FileType::kTexture3 == eType)
						{
							vCanOverflow.PushBack(MakePair((Int64)i->m_uUncompressedSize, i));
						}
						else if (FileType::kSoundBank == eType)
						{
							vCanOverflow.PushBack(MakePair((Int64)i->m_uUncompressedSize, i));
						}
					}

					// Track.
					uCanOverflowTotal = vCanOverflow.GetSize();

					// Now fixup mip 0 sizes to be the delta.
					for (auto& e : vCanOverflow)
					{
						// Track.
						uCanOverflowTotalBytes += e.Second->m_uUncompressedSize;
						aCanOverflow[(UInt32)e.Second->GetFilePath().GetType()]++;
						aCanOverflowBytes[(UInt32)e.Second->GetFilePath().GetType()] += e.Second->m_uUncompressedSize;
					}
				}

				// This will sort results as:
				// - all mip1
				// - then mip0 and .pbank files sorted by size (where .mip0 files
				//   are the size *increase*, since adding a mip0 will remove its
				//   corresponding mip1).
				{
					OverflowSizeSorter sorter;
					QuickSort(vCanOverflow.Begin(), vCanOverflow.End(), sorter);
				}

				// Find the target, then find how many files we need to transfer
				// to overflow. This is also where we handle the ping-pong
				// of mip1 vs. mip0. As soon as we start including mip0,
				// we need to remove the corresponding mip1 that we included
				// previously.
				{
					// Cache input and prepare to build output.
					HashTable<FilePath, FileList::Iterator, MemoryBudgets::Cooking> tOverflow;

					auto const uTarget = (uTotal - pkg.m_uOverflowTargetBytes);
					UInt64 uCurrent = 0u;
					for (auto const& e : vCanOverflow)
					{
						// Done if we're over the target.
						if (uCurrent >= uTarget)
						{
							break;
						}

						// Cache entry for further use.
						auto const iSize = e.First;
						auto const& entry = *e.Second;

						// Check if this entry's size is negative (this happens
						// if a mip0 texture is *smaller* than its mip1, which
						// can happen because we apply crunch compression only to mip0).
						// If so, exclude.
						if (iSize < 0) { continue; }

						// Check if this entry is in the exclusion set - if
						// so, skip it.
						if (exclusions.HasKey(entry.GetFilePath()))
						{
							++uExcludedTotal;
							uExcludedTotalBytes += entry.m_uUncompressedSize;
							aExcluded[(UInt32)entry.GetFilePath().GetType()]++;
							aExcludedBytes[(UInt32)entry.GetFilePath().GetType()] += entry.m_uUncompressedSize;
							continue;
						}

						// Add this entry's size to current.
						uCurrent += (UInt64)iSize;

						// Track.
						SEOUL_VERIFY(tOverflow.Insert(entry.GetFilePath(), e.Second).Second);
					}

					// If we get here without achieving the target, report a failure.
					if (uCurrent < uTarget)
					{
						SEOUL_LOG_COOKING("%s: overflow archive \"%s\" has only %" PRIu64 " bytes available, "
							"need at least %" PRIu64 " bytes to achieve target base size of %" PRIu64 " bytes.",
							pkg.m_sName.CStr(),
							pkg.m_sOverflow.CStr(),
							uCurrent,
							uTarget,
							pkg.m_uOverflowTargetBytes);
						return false;
					}

					// Now rebuild vCanOverflow from the table.
					{
						Vector<FileList::Iterator, MemoryBudgets::Cooking> vNew;
						for (auto const& pair : tOverflow)
						{
							vNew.PushBack(pair.Second);
						}

						// Done.
						vOverflow.Swap(vNew);
					}
				}
			}

			// Resort by address.
			{
				OverflowAddressSorter sorter;
				QuickSort(vOverflow.Begin(), vOverflow.End(), sorter);
			}

			// Populate rvOverflow.
			rvOverflow.Clear();
			rvOverflow.Reserve(vOverflow.GetSize());
			for (auto const& e : vOverflow)
			{
				rvOverflow.PushBack(*e);
				uOverflowTotalBytes += e->m_uUncompressedSize;
			}

			// TODO: Only valid because we know an erase
			// will never re-allocate memory.
			//
			// Remove from rvBase.
			for (Int32 i = (Int32)vOverflow.GetSize() - 1; i >= 0; --i)
			{
				rvBase.Erase(vOverflow[i]);
			}
		}

		{
			// Base archive stats.
			SEOUL_LOG_COOKING("\t%s base stats", pkg.m_sName.CStr());
			SEOUL_LOG_COOKING("\t\tDistribution:");

			FixedArray<UInt32, (UInt32)FileType::FILE_TYPE_COUNT> aBase;
			FixedArray<UInt64, (UInt32)FileType::FILE_TYPE_COUNT> aBaseBytes;
			for (auto const& e : rvBase)
			{
				aBase[(UInt32)e.GetFilePath().GetType()]++;
				aBaseBytes[(UInt32)e.GetFilePath().GetType()] += e.m_uUncompressedSize;
			}

			for (UInt32 i = 0u; i < aBase.GetSize(); ++i)
			{
				if (aBase[i] == 0) { continue; }
				SEOUL_LOG_COOKING("\t\t\t%s: %u (%s)", EnumToString<FileType>(i), aBase[i], GetMemoryUsageString(aBaseBytes[i]).CStr());
			}

			// Overflowed archive stats.
			SEOUL_LOG_COOKING("\t%s overflow stats", pkg.m_sName.CStr());
			SEOUL_LOG_COOKING("\t\tOverflow total (count): %u of %u with %u excluded", rvOverflow.GetSize(), uCanOverflowTotal, uExcludedTotal);
			SEOUL_LOG_COOKING("\t\tOverflow total (bytes): %s of %s with %s excluded", GetMemoryUsageString(uOverflowTotalBytes).CStr(), GetMemoryUsageString(uCanOverflowTotalBytes).CStr(), GetMemoryUsageString(uExcludedTotalBytes).CStr());
			SEOUL_LOG_COOKING("\t\tDistribution:");

			FixedArray<UInt32, (UInt32)FileType::FILE_TYPE_COUNT> a;
			FixedArray<UInt64, (UInt32)FileType::FILE_TYPE_COUNT> aBytes;
			for (auto const& e : rvOverflow)
			{
				a[(UInt32)e.GetFilePath().GetType()]++;
				aBytes[(UInt32)e.GetFilePath().GetType()] += e.m_uUncompressedSize;
			}

			for (UInt32 i = 0u; i < a.GetSize(); ++i)
			{
				if (a[i] == 0) { continue; }
				SEOUL_LOG_COOKING("\t\t\t%s (count): %u of %u with %u excluded", EnumToString<FileType>(i), a[i], aCanOverflow[i], aExcluded[i]);
				SEOUL_LOG_COOKING("\t\t\t%s (bytes): %s of %s with %s excluded", EnumToString<FileType>(i), GetMemoryUsageString(aBytes[i]).CStr(), GetMemoryUsageString(aCanOverflowBytes[i]).CStr(), GetMemoryUsageString(aExcludedBytes[i]).CStr());
			}
		}

		// Resort according to package specification prior to return.
		if (pkg.m_bSortByModifiedTime)
		{
			QuickSort(rvBase.Begin(), rvBase.End());
			QuickSort(rvOverflow.Begin(), rvOverflow.End());
		}
		else
		{
			DefaultSarSorter sorter;
			StableSort(rvBase.Begin(), rvBase.End(), sorter);
			StableSort(rvOverflow.Begin(), rvOverflow.End(), sorter);
		}

		// Done, success.
		return true;
	}

	Bool ProcessSarArchive(
		ICookContext& rContext,
		const PackageCookConfig& config,
		const PackageConfig& pkg) const
	{
		// Generate the list of files to write.
		FileList vFiles;
		if (!GetFileList(rContext, config, pkg, vFiles)) { return false; }

		// If an overflow archive is specified, find overflow
		// files.
		FileList vOverflow;
		if (!ResolveOverflow(rContext, config, pkg, vFiles, vOverflow)) { return false; }

		// Write the base archive.
		auto const sBaseOut(
			Path::Combine(
				Path::GetDirectoryName(config.m_sAbsoluteConfigFilename),
				pkg.m_sName) +
			(pkg.m_sCustomSarExtension.IsEmpty() ? ".sar" : pkg.m_sCustomSarExtension));
		if (!WriteSarArchive(
			rContext,
			config,
			pkg,
			vFiles,
			sBaseOut,
			0u /* Base variation, no variation */))
		{
			return false;
		}

		// If specified, write the overflow archive. We always
		// write the overflow archive, even if no files overflowed.
		if (!pkg.m_sOverflow.IsEmpty())
		{
			auto const sOverflowOut(
				Path::Combine(
					Path::GetDirectoryName(config.m_sAbsoluteConfigFilename),
					pkg.m_sOverflow) +
				(pkg.m_sCustomSarExtension.IsEmpty() ? ".sar" : pkg.m_sCustomSarExtension));
			if (!WriteSarArchive(
				rContext,
				config,
				pkg,
				vOverflow,
				sOverflowOut,
				0u /* Overflow has no variation */))
			{
				return false;
			}
		}

		// Generate variations.
		if (!pkg.m_vVariations.IsEmpty())
		{
			PackageFileSystem basePkg(sBaseOut, true);
			if (!basePkg.IsOk())
			{
				SEOUL_LOG_COOKING("%s: trying to process variation, base package failed to load.",
					sBaseOut.CStr());
				return false;
			}

			UInt32 uVariation = 1u;
			for (auto const& s : pkg.m_vVariations)
			{
				if (!WriteVariationArchive(
					rContext,
					config,
					pkg,
					vFiles,
					sBaseOut,
					basePkg,
					s,
					uVariation))
				{
					return false;
				}

				++uVariation;
			}
		}

		// Done, success.
		return true;
	}

	Bool ProcessZipArchive(
		ICookContext& rContext,
		const PackageCookConfig& config,
		const PackageConfig& pkg) const
	{
		// Generate a temporary file to write the package data.
		auto const sTempFile(Path::GetTempFileAbsoluteFilename());
		auto const scoped(MakeScopedAction([]() {}, [&]()
		{
			(void)FileManager::Get()->Delete(sTempFile);
		}));

		// Sanity check - we don't support overflow for zip archives yet.
		if (!pkg.m_sOverflow.IsEmpty())
		{
			SEOUL_LOG_COOKING("%s: overflow archive \"%s\" is not supported for .zip archives.", pkg.m_sName.CStr(), pkg.m_sOverflow.CStr());
			return false;
		}

		// Get files to include in the archive.
		FileList vFiles;
		if (!GetFileList(rContext, config, pkg, vFiles)) { return false; }

		{
			ScopedPtr<SyncFile> pFile;
			if (!FileManager::Get()->OpenFile(sTempFile, File::kWriteTruncate, pFile))
			{
				SEOUL_LOG_COOKING("%s: failed opening temp file \"%s\" for .zip archive write.", pkg.m_sName.CStr(), sTempFile.CStr());
				return false;
			}

			// Open the archive for writing.
			ZipFileWriter zip;
			if (!zip.Init(*pFile))
			{
				SEOUL_LOG_COOKING("%s: failed initialize zip file writer.", pkg.m_sName.CStr());
				return false;
			}

			// Cache common values, then enumerate.
			auto const ePlatform = rContext.GetPlatform();
			auto const eCompressionLevel = (pkg.m_bCompressFiles ? ZlibCompressionLevel::kBest : ZlibCompressionLevel::kNone);
			for (auto const& entry : vFiles)
			{
				void* p = nullptr;
				UInt32 u = 0u;
				auto const deferred(MakeDeferredAction([&]()
				{
					MemoryManager::Deallocate(p);
					p = nullptr;
					u = 0u;
				}));

				if (!ReadFileData(rContext, config, pkg, entry, p, u))
				{
					return false;
				}

				// Relative filename.
				String const sName(entry.GetFilePath().GetRelativeFilename());
				if (!zip.AddFileBytes(
					sName,
					p,
					u,
					eCompressionLevel,
					entry.m_uModifiedTime))
				{
					SEOUL_LOG_COOKING("%s: failed writing file data \"%s\" into zip archive.", pkg.m_sName.CStr(), sName.CStr());
					return false;
				}
			}

			// Finish the archive, this will write the central directory
			// to the archive.
			if (!zip.Finalize())
			{
				SEOUL_LOG_COOKING("%s: failed finalizing zip file.", pkg.m_sName.CStr());
				return false;
			}
		}

		// Done, write the .zip to its final location.
		return FinalizeWrittenArchive(
			rContext,
			pkg,
			sTempFile,
			Path::Combine(Path::GetDirectoryName(config.m_sAbsoluteConfigFilename), pkg.m_sName) + (pkg.m_sCustomSarExtension.IsEmpty() ? ".zip" : pkg.m_sCustomSarExtension));
	}

	// Utility and enum for checking for certain special classes of a file based on its name.
	enum class FileClass
	{
		kNormal,
		kLocaleBaseFile,
		kLocalePatchFile,
	};
	FileClass GetFileClass(const PackageConfig& pkg, FilePath filePath) const
	{
		// Early out if an easy case - all special cases only apply to JSON files.
		if (filePath.GetType() != FileType::kJson)
		{
			return FileClass::kNormal;
		}

		// Early out if both empty.
		if (pkg.GetLocaleBaseFilenameNoExtension().IsEmpty() &&
			pkg.GetLocalePatchFilenameNoExtension().IsEmpty())
		{
			return FileClass::kNormal;
		}

		// Need relative filename for both remaining checks.
		auto const h = filePath.GetRelativeFilenameWithoutExtension();
		auto const u = h.GetSizeInBytes();

		// If base filename is not empty, check.
		{
			auto const& s = pkg.GetLocaleBaseFilenameNoExtension();
			auto const us = s.GetSize();
			if (!s.IsEmpty() &&
				u >= us &&
				0 == strcmp(h.CStr() + u - us, s.CStr()))
			{
				return FileClass::kLocaleBaseFile;
			}
		}

		// If patch filename is not empty, check.
		{
			auto const& s = pkg.GetLocalePatchFilenameNoExtension();
			auto const us = s.GetSize();
			if (!s.IsEmpty() &&
				u >= us &&
				0 == strcmp(h.CStr() + u - us, s.CStr()))
			{
				return FileClass::kLocalePatchFile;
			}
		}

		// Fallback to normal.
		return FileClass::kNormal;
	}

	Bool ReadFileData(
		ICookContext& rContext,
		const PackageCookConfig& config,
		const PackageConfig& pkg,
		const FileListEntry& entry,
		void*& rpData,
		UInt32& ruData) const
	{
		// Get the file class.
		auto eClass = GetFileClass(pkg, entry.GetFilePath());

		// If a locale.json file, read the contents from the locale base archive.
		if (FileClass::kLocaleBaseFile == eClass)
		{
			// Open the base archive and reach the base path from it.
			ScopedPtr<PackageFileSystem> pLocalePkg;
			if (!ResolveLocaleBaseArchive(config, pkg, pLocalePkg))
			{
				return false;
			}

			// Read the raw data.
			if (!pLocalePkg->ReadAll(entry.GetFilePath(), rpData, ruData, 0u, MemoryBudgets::Cooking))
			{
				SEOUL_LOG_COOKING("Failed reading base locale file \"%s\" from archive \"%s\".", entry.GetFilePath().CStr(), pkg.m_sLocaleBaseArchive.CStr());
				return false;
			}

			// Success - handle according to configuration.
			if (pkg.m_bCookJson || pkg.m_bMinifyJson)
			{
				// Read the data into a DataStore.
				DataStore dataStore;
				Bool const bSuccess = DataStoreParser::FromString((Byte const*)rpData, ruData, dataStore, DataStoreParserFlags::kLogParseErrors, entry.GetFilePath());
				MemoryManager::Deallocate(rpData);
				rpData = nullptr;
				ruData = 0u;

				if (!bSuccess)
				{
					SEOUL_LOG_COOKING("Failed parsing base locale file \"%s\" into a DataStore.", entry.GetFilePath().CStr());
					return false;
				}

				if (pkg.m_bCookJson)
				{
					MemorySyncFile file;
					if (!dataStore.Save(file, config.m_ePlatform))
					{
						SEOUL_LOG_COOKING("Failed cooking base locale file: %s", entry.GetFilePath().CStr());
						return false;
					}

					file.GetBuffer().RelinquishBuffer(rpData, ruData);
					return true;
				}
				else
				{
					String s;
					dataStore.ToString(dataStore.GetRootNode(), s, false, 0, true);
					s.RelinquishBuffer(rpData, ruData);
					return true;
				}
			}
			// Done, just return the values already in the output parameters.
			else
			{
				return true;
			}
		}
		// If a locale_patch.json file, generate a patch for its body.
		else if (FileClass::kLocalePatchFile == eClass)
		{
			// Open the base archive and reach the base path from it.
			ScopedPtr<PackageFileSystem> pLocalePkg;
			if (!ResolveLocaleBaseArchive(config, pkg, pLocalePkg))
			{
				return false;
			}

			// Cache the FilePath used for reading the locale base
			// file.
			auto const filePath = FilePath::CreateConfigFilePath(Path::Combine(
				Path::GetDirectoryName(entry.GetFilePath().GetRelativeFilenameWithoutExtension().ToString()),
				pkg.m_sLocaleBaseFilename));

			// Read the base version of the locale file (this
			// is the version in the base .sar archive).
			DataStore base;
			{
				void* pBase = nullptr;
				UInt32 uBase = 0u;
				if (!pLocalePkg->ReadAll(filePath, pBase, uBase, 0u, MemoryBudgets::Cooking))
				{
					SEOUL_LOG_COOKING("Failed reading locale base \"%s\" from base archive \"%s\" to generate patch.", filePath.CStr(), pkg.m_sLocaleBaseArchive.CStr());
					return false;
				}

				Bool const bBase = DataStoreParser::FromString((Byte const*)pBase, uBase, base, DataStoreParserFlags::kLogParseErrors, filePath);
				MemoryManager::Deallocate(pBase);
				pBase = nullptr;
				uBase = 0u;

				if (!bBase)
				{
					SEOUL_LOG_COOKING("Failed converting locale base \"%s\" from base archive \"%s\" to a DataStore for patch generation.", filePath.CStr(), pkg.m_sLocaleBaseArchive.CStr());
					return false;
				}
			}

			// Read the target version of the locale file (this
			// is the version on disk).
			DataStore target;
			{
				void* pTarget = nullptr;
				UInt32 uTarget = 0u;
				if (!FileManager::Get()->ReadAll(filePath, pTarget, uTarget, 0u, MemoryBudgets::Cooking))
				{
					SEOUL_LOG_COOKING("Failed reading locale target \"%s\" from disk to generate patch.", filePath.CStr());
					return false;
				}

				Bool const bTarget = DataStoreParser::FromString((Byte const*)pTarget, uTarget, target, DataStoreParserFlags::kLogParseErrors, filePath);
				MemoryManager::Deallocate(pTarget);
				pTarget = nullptr;
				uTarget = 0u;

				if (!bTarget)
				{
					SEOUL_LOG_COOKING("Failed converting locale target \"%s\" from disk to a DataStore for patch generation.", filePath.CStr());
					return false;
				}
			}

			// Generate the diff.
			DataStore diff;
			if (!ComputeDiff(base, target, diff))
			{
				SEOUL_LOG_COOKING("Failed generating diff between base and target locale file \"%s\".", filePath.CStr());
				return false;
			}

			// Output the diff version as appropriate.
			if (pkg.m_bCookJson)
			{
				MemorySyncFile file;
				if (!diff.Save(file, config.m_ePlatform))
				{
					SEOUL_LOG_COOKING("Failed converting locale diff to binary output.");
					return false;
				}

				// Done success.
				file.GetBuffer().RelinquishBuffer(rpData, ruData);
				return true;
			}
			else
			{
				// TODO: Do we want to consider MinifyJson
				// here and return a human readable output if
				// that flag is not set?

				// Return the string version of the diff as output.
				String s;
				diff.ToString(diff.GetRootNode(), s, false, 0, true);
				s.RelinquishBuffer(rpData, ruData);
				return true;
			}
		}
		else
		{
			if ((pkg.m_bCookJson || pkg.m_bMinifyJson) && entry.GetFilePath().GetType() == FileType::kJson)
			{
				SharedPtr<DataStore> p;
				if (!m_tResolvedSettings.GetValue(entry.GetFilePath(), p))
				{
					p.Reset(SEOUL_NEW(MemoryBudgets::Cooking) DataStore);
					if (!DataStoreParser::FromFile(nullptr, entry.GetFilePath(), *p, DataStoreParserFlags::kLogParseErrors))
					{
						SEOUL_LOG_COOKING("Attempting to minify or cook an invalid .json file: %s", entry.GetFilePath().CStr());
						return false;
					}
				}

				if (pkg.m_bCookJson)
				{
					MemorySyncFile file;
					if (!p->Save(file, config.m_ePlatform, true))
					{
						SEOUL_LOG_COOKING("Failed cooking .json file: %s", entry.GetFilePath().CStr());
						return false;
					}

					file.GetBuffer().RelinquishBuffer(rpData, ruData);
					return true;
				}
				else
				{
					String s;
					p->ToString(p->GetRootNode(), s, false, 0, true);
					s.RelinquishBuffer(rpData, ruData);
					return true;
				}
			}
			else
			{
				if (!FileManager::Get()->ReadAll(entry.GetFilePath(), rpData, ruData, 0u, MemoryBudgets::Cooking))
				{
					SEOUL_LOG_COOKING("%s: failed reading file data from \"%s\" for package generation.", pkg.m_sName.CStr(), entry.GetFilePath().CStr());
					return false;
				}

				return true;
			}
		}
	}

	Bool AppendVariation(
		UInt32 uPackageVariation,
		FilePath filePath,
		const String& sVariationData,
		void*& rp,
		UInt32& ru) const
	{
		DataStore ds;
		if (!DataStoreParser::FromString((Byte const*)rp, ru, ds))
		{
			SEOUL_LOG_COOKING("Variation %u: failed parsing base data of '%s' to apply variation.",
				uPackageVariation,
				filePath.CStr());
			return false;
		}

		// Parse the chunk.
		DataStore chunk;
		if (!DataStoreParser::FromString((Byte const*)sVariationData.CStr(), sVariationData.GetSize(), chunk, DataStoreParserFlags::kLogParseErrors))
		{
			SEOUL_LOG_COOKING("Variation %u: failed parsing variation data of '%s' to apply variation.",
				uPackageVariation,
				filePath.CStr());
			return false;
		}

		// Chunk must be a commands file.
		if (!DataStoreParser::IsJsonCommandFile(chunk))
		{
			SEOUL_LOG_COOKING("Variation %u: variation data for '%s' is not a JSON command file.",
				uPackageVariation,
				filePath.CStr());
			return false;
		}

		// If target is a commands file, then we just append the chunk to that file.
		if (DataStoreParser::IsJsonCommandFile(ds))
		{
			// Append commands to existing commands array.
			UInt32 uExistingCommands = 0u;
			if (!ds.GetArrayCount(ds.GetRootNode(), uExistingCommands))
			{
				SEOUL_LOG_COOKING("Variation %u: failed getting count of JSON commands in base file '%s' to apply variation.",
					uPackageVariation,
					filePath.CStr());
				return false;
			}

			UInt32 uNewCommands = 0u;
			if (!chunk.GetArrayCount(chunk.GetRootNode(), uNewCommands))
			{
				SEOUL_LOG_COOKING("Variation %u: failed getting count of JSON commands in variation file '%s' to apply variation.",
					uPackageVariation,
					filePath.CStr());
				return false;
			}

			for (UInt32 i = 0u; i < uNewCommands; ++i)
			{
				DataNode arrayElem;
				if (!chunk.GetValueFromArray(chunk.GetRootNode(), i, arrayElem))
				{
					SEOUL_LOG_COOKING("Variation %u: failed getting JSON command '%u' in variation file '%s' to apply variation.",
						uPackageVariation,
						i,
						filePath.CStr());
					return false;
				}

				if (!ds.DeepCopyToArray(chunk, arrayElem, ds.GetRootNode(), i + uExistingCommands))
				{
					SEOUL_LOG_COOKING("Variation %u: failed deep copy of JSON command '%u' in variation file '%s' to apply variation.",
						uPackageVariation,
						i,
						filePath.CStr());

					return false;
				}
			}
		}
		// Otherwise, we apply it to that file "in place". The initial state is
		// the initial state of the data store and we apply any appended commands
		// to that state.
		else
		{
			DataNode target = ds.GetRootNode();
			if (!DataStoreParser::ResolveCommandFileInPlace(
				DataStoreParser::Resolver() /* TODO: */,
				String(),
				chunk,
				ds,
				target))
			{
				SEOUL_LOG_COOKING("Variation %u: failed resolution of variation file '%s' to apply variation.",
					uPackageVariation,
					filePath.CStr());

				return false;
			}
		}

		// Output.
		String sOutput;
		ds.ToString(ds.GetRootNode(), sOutput, true, 0, true);

		MemoryManager::Deallocate(rp);
		sOutput.RelinquishBuffer(rp, ru);
		return true;
	}

	Bool WriteSarFileEntries(
		ICookContext& rContext,
		const PackageCookConfig& config,
		const PackageConfig& pkg,
		const FileList& vFiles,
		SyncFile& r,
		Vector<FileEntry, MemoryBudgets::Cooking>& rvEntries,
		UInt32 uPackageVariation,
		HashTable<FilePath, String, MemoryBudgets::Cooking> const* ptVariations,
		PackageFileSystem* pVariationBase) const
	{
		// Get a hash set of delta archives - this will be empty if
		// we're not generating a delta archive.
		DeltaSet deltaFiles;
		if (!GetDeltaFileCrc32Set(config, pkg, deltaFiles)) { return false; }

		// Resolve the compression dictionary if requested.
		auto const dictPathFile = FilePath::CreateFilePath(
			pkg.m_eGameDirectoryType,
			String::Printf(ksPackageCompressionDictNameFormat, kaPlatformNames[(UInt32)config.m_ePlatform]));

		// Prefetch all files to improve fetch from the NFS shared cache.
		for (auto const& entry : vFiles)
		{
			FileManager::Get()->NetworkPrefetch(entry.GetFilePath());
		}

		Vector<Byte> vDict;
		if (pkg.m_bCompressFiles && pkg.m_bUseCompressionDictionary)
		{
			// Cache filename.
			auto const sDictFilename(dictPathFile.GetAbsoluteFilename());

			// If a size was specified, and generation is enabled,
			// we're generating the dictionary fresh, unless we're
			// generating a variation of a base that already
			// generated the dict.
			if (nullptr == pVariationBase &&
				(rContext.GetForceCompressionDictGeneration() || !FileManager::Get()->Exists(sDictFilename)) &&
				pkg.m_uCompressionDictionarySize > 0u)
			{
				Vector<Byte, MemoryBudgets::Cooking> vAll;
				Vector<size_t, MemoryBudgets::Cooking> vSizes;

				// Accumulate all files to generate the dictionary.
				for (auto const& entry : vFiles)
				{
					// Skip the dict file itself.
					if (entry.GetFilePath() == dictPathFile)
					{
						continue;
					}

					// Read in the file data.
					void* p = nullptr;
					UInt32 u = 0u;
					if (!ReadFileData(rContext, config, pkg, entry, p, u)) { return false; }

					vAll.Append((Byte*)p, (Byte*)p + u);
					vSizes.PushBack(u);
				}

				// Now generate the dictionary.
				vDict.Resize(pkg.m_uCompressionDictionarySize);
				if (!ZSTDPopulateDict(vAll.Data(), vSizes.GetSize(), vSizes.Data(), vDict.Data(), vDict.GetSizeInBytes()))
				{
					SEOUL_LOG_COOKING("%s: failed generation of compression dictionary.", pkg.m_sName.CStr());
					return false;
				}

				// Open the dict file for add.
				auto& scc = rContext.GetSourceControlClient();
				auto const& opt = rContext.GetSourceControlFileTypeOptions();
				if (!scc.OpenForEdit(&sDictFilename, &sDictFilename + 1, opt, SEOUL_BIND_DELEGATE(LogError)))
				{
					SEOUL_LOG_COOKING("%s: failed opening compression dictionary for edit.", pkg.m_sName.CStr());
					return false;
				}

				if (!AtomicWriteFinalOutput(rContext, vDict.Data(), vDict.GetSizeInBytes(), dictPathFile))
				{
					SEOUL_LOG_COOKING("%s: failed writing compression dictionary to disk.", pkg.m_sName.CStr());
					return false;
				}

				if (!scc.OpenForAdd(&sDictFilename, &sDictFilename + 1, opt, SEOUL_BIND_DELEGATE(LogError)))
				{
					SEOUL_LOG_COOKING("%s: failed opening compression dictionary for add.", pkg.m_sName.CStr());
					return false;
				}

				// Finally, revert unchanged if the file has changed.
				if (!scc.RevertUnchanged(&sDictFilename, &sDictFilename + 1, SEOUL_BIND_DELEGATE(&LogError)))
				{
					SEOUL_LOG_COOKING("%s: reverting unchanged dictionary file.", pkg.m_sName.CStr());
					return false;
				}
			}
			// Otherwise, we're using an existing dictionary. This is used
			// for patch generation.
			else
			{
				// Before continuing, read the generated dictionary from disk.
				if (!FileManager::Get()->ReadAll(sDictFilename, vDict))
				{
					SEOUL_LOG_COOKING("%s: failed reading compression dictionary from disk.", pkg.m_sName.CStr());
					return false;
				}
			}
		}

		// Populate the compression dictionary for further processing,
		// if one was defined.
		ZSTDCompressionDict* pCompressionDict = nullptr;
		auto const action(MakeScopedAction(
			[&]()
			{
				if (!vDict.IsEmpty())
				{
					pCompressionDict = ZSTDCreateCompressionDictWeak(
						vDict.Data(),
						vDict.GetSizeInBytes());
				}
			},
			[&]()
			{
				ZSTDFreeCompressionDict(pCompressionDict);
			}));

		// Enumerate and process.
		for (auto const& entry : vFiles)
		{
			// Normalized path for obfuscation and storage.
			auto const sNormalizedPath = entry.GetFilePath().GetRelativeFilename().ReplaceAll("/", "\\").Substring(pkg.GetRoot().GetSize());

			// Variation handling - defined and not a variation.
			void* p = nullptr;
			UInt32 u = 0u;
			UInt32 uUncompressedFileSize = 0u;
			UInt32 uCrc32Pre = 0u;
			UInt32 uCrc32Post = 0u;
			if (nullptr != pVariationBase && !ptVariations->HasValue(entry.GetFilePath()))
			{
				// Get the entry - if it does not exist, there's been a programmer error (the variation
				// system does not enerate new files, only modifies a subst of base files).
				auto pEntry = pVariationBase->GetFileTable().Find(entry.GetFilePath());
				if (nullptr == pEntry)
				{
					SEOUL_LOG_COOKING("%s: failed generating variation file '%s' does not exist in base.",
						pkg.m_sName.CStr(),
						entry.GetFilePath().CStr());
					return false;
				}

				u = (UInt32)pEntry->m_Entry.m_uCompressedFileSize;
				p = MemoryManager::Allocate(u, MemoryBudgets::Cooking);
				uUncompressedFileSize = (UInt32)pEntry->m_Entry.m_uUncompressedFileSize;
				uCrc32Pre = pEntry->m_Entry.m_uCrc32Pre;
				uCrc32Post = pEntry->m_Entry.m_uCrc32Post;
				if (!pVariationBase->ReadRaw(pEntry->m_Entry.m_uOffsetToFile, p, u))
				{
					MemoryManager::Deallocate(p);
					SEOUL_LOG_COOKING("%s: failed reading data for file '%s' when processing variation.",
						pkg.m_sName.CStr(),
						entry.GetFilePath().CStr());
					return false;
				}
			}
			// Otherwise, standard read, or a read that we must apply a variation to.
			else
			{
				// Read in the file data.
				if (!ReadFileData(rContext, config, pkg, entry, p, u)) { return false; }

				// Apply variation now, if there is one.
				if (nullptr != ptVariations)
				{
					auto psVariationAppend = ptVariations->Find(entry.GetFilePath());
					if (nullptr != psVariationAppend)
					{
						if (!AppendVariation(
							uPackageVariation,
							entry.GetFilePath(),
							*psVariationAppend,
							p,
							u))
						{
							SEOUL_LOG_COOKING("%s: could not append variation to '%s'.",
								pkg.m_sName.CStr(),
								entry.GetFilePath().CStr());
							return false;
						}
					}
				}

				// Record.
				uUncompressedFileSize = u;

				// Cache the CRC-32 - the "pre" CRC-32 is computed before compression and obfuscation.
				uCrc32Pre = GetCrc32((UInt8 const*)p, u);

				// Apply lossless compression if specified for the package.
				Bool bCrc32Match = true;
				if (pkg.m_bCompressFiles)
				{
					void* pC = nullptr;
					UInt32 uC = 0u;

					// Use the compression dictionary, unless we're compressing the dictionary
					// itself.
					if (nullptr != pCompressionDict && entry.GetFilePath() != dictPathFile)
					{
						if (!ZSTDCompressWithDict(pCompressionDict, p, u, pC, uC))
						{
							SEOUL_LOG_COOKING("%s: failed ZSTD compression of package file entry \"%s\".", pkg.m_sName.CStr(), entry.GetFilePath().CStr());
							MemoryManager::Deallocate(p);
							return false;
						}
					}
					else
					{
						if (!ZSTDCompress(p, u, pC, uC, pkg.GetCompressionLevel()))
						{
							SEOUL_LOG_COOKING("%s: failed ZSTD compression of package file entry \"%s\".", pkg.m_sName.CStr(), entry.GetFilePath().CStr());
							MemoryManager::Deallocate(p);
							return false;
						}
					}

					// Don't use the compressed version if it's larger than or equal to the original. This can happen with some file
					// data that is already compressed, or tiny files with little redundancy.
					if (uC < u)
					{
						MemoryManager::Deallocate(p);
						p = pC;
						u = uC;
						bCrc32Match = false;
					}
					else
					{
						MemoryManager::Deallocate(pC);
						pC = nullptr;
						uC = 0u;
					}
				}

				// Weakly obfuscate the file data just so it isn't trivially
				// readable in notepad.  A better system would be to encrypt
				// the data with a real cipher and use some kind of a hash or
				// checksum for data integrity, but there are a lot of
				// complications with that -- each file becomes non-seekable,
				// because of the crypto and checksum state, and figuring out
				// when to check the checksum is tricky.  FMOD does do
				// non-trivial seeks, so audio data at the very least cannot
				// be non-trivially encrypted (or at least not without
				// figuring out if we can tell FMOD not to seek etc.).
				if (pkg.m_bObfuscate)
				{
					Obfuscate(GenerateKey(sNormalizedPath), p, u);
					bCrc32Match = false;
				}

				// Post CRC32 is either equal to pre, if
				// no obfuscation or compression. Otherwise,
				// compute the new value after these
				// operations have been applied.
				uCrc32Post = uCrc32Pre;
				if (!bCrc32Match)
				{
					uCrc32Post = Seoul::GetCrc32((UInt8 const*)p, u);
				}
			}

			// Check if the file already exists in the delta set - if so, skip it.
			if (deltaFiles.HasKey(DeltaKey::Create(entry.GetFilePath(), u, uCrc32Pre)))
			{
				MemoryManager::Deallocate(p);
				continue;
			}

			// Pad to support efficient reads on some platforms (Android)
			if (!PadToAlignment(r, kiFileEntryAlignment))
			{
				SEOUL_LOG_COOKING("%s: failed alignment padding for writing file entry \"%s\".", pkg.m_sName.CStr(), entry.GetFilePath().CStr());
				MemoryManager::Deallocate(p);
				return false;
			}

			Int64 iPosition = 0;
			if (!r.GetCurrentPositionIndicator(iPosition))
			{
				SEOUL_LOG_COOKING("%s: failed getting file position indicator for package file entry \"%s\".", pkg.m_sName.CStr(), entry.GetFilePath().CStr());
				MemoryManager::Deallocate(p);
				return false;
			}

			// Insert the data into the package, and add an entry
			// to the file table.
			FileEntry pkgEntry;
			pkgEntry.m_Entry.m_uOffsetToFile = (UInt64)iPosition;
			pkgEntry.m_Entry.m_uCompressedFileSize = (UInt64)u;
			pkgEntry.m_Entry.m_uUncompressedFileSize = (UInt64)uUncompressedFileSize;
			pkgEntry.m_Entry.m_uModifiedTime = entry.m_uModifiedTime;
			pkgEntry.m_Entry.m_uCrc32Pre = uCrc32Pre;
			pkgEntry.m_Entry.m_uCrc32Post = uCrc32Post;
			pkgEntry.m_sFileName = sNormalizedPath;

			// Write out file data to the package.
			if (u != r.WriteRawData(p, u))
			{
				SEOUL_LOG_COOKING("%s: failed writing file data to package for package file entry \"%s\".", pkg.m_sName.CStr(), entry.GetFilePath().CStr());
				MemoryManager::Deallocate(p);
				return false;
			}
			MemoryManager::Deallocate(p);

			// Insert the header data into the entries table.
			rvEntries.PushBack(pkgEntry);
		}

		return true;
	}
}; // class PackageCookTask

} // namespace Cooking

SEOUL_BEGIN_TYPE(Cooking::PackageCookTask, TypeFlags::kDisableCopy)
	SEOUL_PARENT(Cooking::BaseCookTask)
SEOUL_END_TYPE()

} // namespace Seoul
