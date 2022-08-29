/**
 * \file SettingsManager.cpp
 * \brief A manager for game config files and data, contained in
 * JSON files, uniquely identified by a FilePath and
 * stored in DataStore objects.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "CookManager.h"
#include "DataStoreParser.h"
#include "FileManager.h"
#include "JobsManager.h"
#include "Logger.h"
#include "ReflectionDeserialize.h"
#include "ScopedAction.h"
#include "SettingsContentLoader.h"
#include "SettingsManager.h"
#include "SeoulUtil.h"
#include "SeoulWildcard.h"
#include "ThreadId.h"

namespace Seoul
{

SettingsManager::SettingsManager()
	: m_Settings()
#if !SEOUL_SHIP
	, m_pSchemaCache(nullptr)
#endif // /#if !SEOUL_SHIP
{
}

SettingsManager::~SettingsManager()
{
}

/**
 * Utility method, equivalent to GetSettings(), except
 * busy waits until the settings have completed loading. This method
 * may still return nullptr if the load failed.
 *
 * @return A non-null SharedPtr<DataStore> to the settings associated
 * with filePath if loading succeeded, a nullptr pointer otherwise.
 */
SharedPtr<DataStore> SettingsManager::WaitForSettings(FilePath filePath)
{
	// Special case - particularly on iOS (due to recent changes to their
	// thread scheduler), WaitForSettings() is far more efficient if
	// done synchronously, since the waiting thread has a tendency to
	// starve the loader threads in this case.
	SettingsContentHandle hSettings = m_Settings.GetContent(filePath, true);
	if (Content::LoadManager::Get())
	{
		Content::LoadManager::Get()->WaitUntilLoadIsFinished(hSettings);
	}
	return hSettings.GetPtr();
}

/**
 * Deserialize the contents of filePath into the object pointed at by objectPtr using
 * Reflection.
 *
 * @return True if deserialization was successful, false if any error occured.
 *
 * @param[in] rootSectionIdentifier If defined to the non-empty string, this is the name of the section
 * that will be used as the root when deserializing the object, otherwise the entire file will be used.
 *
 * @param[in] bSkipPostSerialize If defined to true, and if the object being serialized has
 * a Reflection::Attributes::PostSerializeType attribute, the method defined with that attribute
 * will *not* be executed once serialization of the object has completed successfully. Otherwise, it will be.
 */
Bool SettingsManager::DeserializeObject(
	FilePath filePath,
	const Reflection::WeakAny& objectPtr,
	HString rootSectionIdentifier /* = HString() */,
	Bool bSkipPostSerialize /* = false */)
{
	return InternalDeserializeObject(filePath, objectPtr, rootSectionIdentifier, bSkipPostSerialize);
}

/** Utility class used by AreSettingsLoading(). */
struct AreSettingsLoadingUtility SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(AreSettingsLoadingUtility);

	AreSettingsLoadingUtility()
		: m_bIsLoading(false)
	{
	}

	Bool m_bIsLoading;

	Bool Apply(const SettingsContentHandle& h)
	{
		if (h.IsLoading())
		{
			m_bIsLoading = true;
			return true;
		}

		return false;
	}
}; // struct AreSettingsLoadingUtility

/** Return true if any settings are currently loading, false otherwise. */
Bool SettingsManager::AreSettingsLoading() const
{
	return m_Settings.AreSettingsLoading();
}

/** Allow manual override of the current state of settings data (currently used by in-engine tools). */
void SettingsManager::SetSettings(FilePath filePath, const SharedPtr<DataStore>& dataStore)
{
	m_Settings.SetSettings(filePath, dataStore);
}

/**
 * Internal function used by DeserializeObject(), performs the actual deserialization
 * attempt.
 */
Bool SettingsManager::InternalDeserializeObject(
	FilePath filePath,
	const Reflection::WeakAny& objectPtr,
	HString rootSectionIdentifier /*= HString()*/,
	Bool bSkipPostSerialize /*= false*/)
{
	SharedPtr<DataStore> pDataStore(WaitForSettings(filePath));
	if (!pDataStore.IsValid())
	{
		SEOUL_WARN("InternalDeserializeObject: Unable to find file at %s", filePath.CStr());
		return false;
	}

	DataNode root = pDataStore->GetRootNode();
	if (!rootSectionIdentifier.IsEmpty())
	{
		if (!pDataStore->GetValueFromTable(root, rootSectionIdentifier, root))
		{
			SEOUL_WARN("InternalDeserializeObject: Unable to get root node in file %s", filePath.CStr());
			return false;
		}
	}

	Reflection::DefaultSerializeContext context(filePath, *pDataStore, root, objectPtr.GetTypeInfo(), rootSectionIdentifier);
	return Seoul::Reflection::DeserializeObject(context, *pDataStore, root, objectPtr, bSkipPostSerialize);
}


#if !SEOUL_SHIP
namespace
{

static void CheckDep(FilePath from, HashSet<FilePath>& set, FilePath to, Bool& rbOk);

static void CheckFxBank(HashSet<FilePath>& set, FilePath filePath, Bool& rbOk)
{
	void* pU = nullptr;
	UInt32 uU = 0u;
	auto const deferredU(MakeDeferredAction([&]() { MemoryManager::Deallocate(pU); pU = nullptr; }));
	{
		void* pC = nullptr;
		UInt32 uC = 0u;
		auto const deferredC(MakeDeferredAction([&]() { MemoryManager::Deallocate(pC); pC = nullptr; }));

		if (!FileManager::Get()->ReadAll(filePath, pC, uC, 0u, MemoryBudgets::Cooking))
		{
			SEOUL_WARN("%s: failed reading Fx bank data from disk.", filePath.CStr());
			rbOk = false;
			return;
		}

		if (!ZSTDDecompress(pC, uC, pU, uU))
		{
			SEOUL_WARN("%s: failed decompressing Fx bank data.", filePath.CStr());
			rbOk = false;
			return;
		}
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

			String const str(sStart, (UInt)(s - sStart));

			FilePath depFilePath;
			if (!DataStoreParser::StringAsFilePath(str, depFilePath))
			{
				depFilePath = FilePath::CreateContentFilePath(str);
			}

			CheckDep(filePath, set, depFilePath, rbOk);
		}
	}
}

static void CheckDataStore(HashSet<FilePath>& set, const FilePath& filePath, const DataStore& dataStore, const DataNode& dataNode, Bool& rbOk)
{
	switch (dataNode.GetType())
	{
	case DataNode::kArray:
	{
		UInt32 uCount = 0u;
		(void)dataStore.GetArrayCount(dataNode, uCount);
		for (UInt32 i = 0u; i < uCount; ++i)
		{
			DataNode child;
			(void)dataStore.GetValueFromArray(dataNode, i, child);
			CheckDataStore(set, filePath, dataStore, child, rbOk);
		}
	} break;

	case DataNode::kFilePath:
	{
		FilePath child;
		(void)dataStore.AsFilePath(dataNode, child);
		CheckDep(filePath, set, child, rbOk);
	} break;

	case DataNode::kTable:
	{
		for (auto i = dataStore.TableBegin(dataNode), iEnd = dataStore.TableEnd(dataNode); iEnd != i; ++i)
		{
			CheckDataStore(set, filePath, dataStore, i->Second, rbOk);
		}
	} break;

	default:
		break;
	};
}

static void CheckJson(HashSet<FilePath>& set, FilePath to, Bool& rbOk)
{
	DataStore dataStore;
	if (!DataStoreParser::FromFile(
		SettingsManager::Get()->GetSchemaCache(),
		to,
		dataStore,
		DataStoreParserFlags::kLogParseErrors))
	{
		SEOUL_WARN("%s: parse failed.", to.GetRelativeFilenameInSource().CStr());
		rbOk = false;
		return;
	}

	CheckDataStore(set, to, dataStore, dataStore.GetRootNode(), rbOk);
}

static void CheckDep(FilePath from, HashSet<FilePath>& set, FilePath to, Bool& rbOk)
{
	// Insertion failure implies we've already checked the file path.
	if (!set.Insert(to).Second)
	{
		return;
	}

	CookManager::Get()->Cook(to);
	if (!FileManager::Get()->Exists(to))
	{
		SEOUL_WARN("%s: dependency \"%s\" does not exist on disk.",
			from.GetRelativeFilenameInSource().CStr(),
			to.GetRelativeFilenameInSource().CStr());
		rbOk = false;
	}

	// We also nest check kFxBank or kJson,
	// as the nested dependencies of these file types
	// can be hand authored (and corrected by users).
	//
	// Put another way, this is the inverse of the exception
	// granted in PackageCookTask.cpp, ShouldReportMissing().
	switch (to.GetType())
	{
	case FileType::kFxBank:
		CheckFxBank(set, to, rbOk);
		break;
	case FileType::kJson:
		CheckJson(set, to, rbOk);
		break;
	default:
		break;
	};
}

} // namespace anonymous

Bool SettingsManager::ValidateSettings(const String& sExcludeWildcard, Bool bCheckDependencies, UInt32& ruNumChecked)
{
	Wildcard wildcard(sExcludeWildcard);

	Bool bReturn = true;

	HashSet<FilePath> checked;
	Vector<String> vs;
	FilePath dirPath;
	dirPath.SetDirectory(GameDirectory::kConfig);
	if (FileManager::Get()->GetDirectoryListing(dirPath, vs, false, true, FileTypeToSourceExtension(FileType::kJson)))
	{
		DataStore dataStore;
		for (auto const& s : vs)
		{
			auto const filePath = FilePath::CreateConfigFilePath(s);

			// Skip
			if (wildcard.IsExactMatch(filePath.GetRelativeFilenameInSource()))
			{
				continue;
			}

			// If checking dependencies, nested (and potentially recursive) check.
			if (bCheckDependencies)
			{
				CheckDep(FilePath(), checked, filePath, bReturn);
			}
			// Otherwise, just make sure we can parse the file.
			else
			{
				DataStore dataStore;
				if (!DataStoreParser::FromFile(
					SettingsManager::Get()->GetSchemaCache(),
					filePath,
					dataStore,
					DataStoreParserFlags::kLogParseErrors))
				{
					SEOUL_WARN("%s: parse failed.", filePath.GetRelativeFilenameInSource().CStr());
					bReturn = false;
				}
			}
		}
	}

	ruNumChecked = vs.GetSize();
	return bReturn;
}
#endif // /!SEOUL_SHIP

SettingsManager::SettingsContainer::SettingsContainer()
	: m_BootstrapMutex()
	, m_tBoostrapTable()
	, m_pSettings()
	, m_UnloadSuppress()
	, m_uUnloadLRUThresholdInBytes(20u * 1024u * 1024u)
	, m_bHasSettings(false)
{
}

SettingsManager::SettingsContainer::~SettingsContainer()
{
}

/** @return true if any settings are actively loading, false otherwise. */
Bool SettingsManager::SettingsContainer::AreSettingsLoading() const
{
	// If we don't have the normal content system yet, no files can be loading,
	// so immediately return false.
	if (!m_bHasSettings)
	{
		return false;
	}
	// Otherwise, query the content system.
	else
	{
		AreSettingsLoadingUtility utility;
		m_pSettings->Apply(SEOUL_BIND_DELEGATE(&AreSettingsLoadingUtility::Apply, &utility));

		return utility.m_bIsLoading;
	}
}

/** Allow manual override of the current state of settings data (currently used by in-engine tools). */
void SettingsManager::SettingsContainer::SetSettings(FilePath filePath, const SharedPtr<DataStore>& dataStore)
{
	if (m_bHasSettings)
	{
		m_pSettings->SetContent(filePath, dataStore);
	}
}

/** @return a Content::Handle from either the bootstrap or normal system, as appropriate. */
SettingsContentHandle SettingsManager::SettingsContainer::GetContent(FilePath filePath, Bool bSyncLoad)
{
	// If the normal Settings content system has been initialized, fulfill the request
	// with that system.
	if (m_bHasSettings)
	{
		return m_pSettings->GetContent(filePath, bSyncLoad);
	}
	// Otherwise, use the bootstrap system.
	else
	{
		Lock lock(m_BootstrapMutex);

		// Check again after a successful mutex lock - if another thread has initialized
		// the normal Settings content system, use that to fulfill the request.
		if (m_bHasSettings)
		{
			return m_pSettings->GetContent(filePath, bSyncLoad);
		}
		// Otherwise, use the bootstrap system.
		else
		{
			// Check if the DataStore has already been loaded and cached - if so,
			// return it.
			SharedPtr<DataStore> pDataStore;
			if (!m_tBoostrapTable.GetValue(filePath, pDataStore))
			{
				// Instantiate a DataStore and synchronously load it - if this fails,
				// return immediately.
				DataStore dataStore;
				if (!DataStoreParser::FromFile(filePath, dataStore))
				{
					return SettingsContentHandle();
				}

				pDataStore.Reset(SEOUL_NEW(MemoryBudgets::DataStore) DataStore);
				pDataStore->Swap(dataStore);

				// Cache the loaded DataStore prior to returning it from
				// the bootstrap system.
				SEOUL_VERIFY(m_tBoostrapTable.Insert(filePath, pDataStore).Second);
			}

			// Done, the bootstrap system has fulfilled the request.
			return SettingsContentHandle(pDataStore.GetPtr());
		}
	}
}

/** Access point for post Content::LoadManager construction. */
void SettingsManager::SettingsContainer::OnInitializeContentLoader()
{
	// Nothing to do if we already have settings.
	if (m_bHasSettings)
	{
		return;
	}

	Lock lock(m_BootstrapMutex);

	// Check the m_bHasSettings flag again now that we've successfully locked
	// the mutex.
	if (m_bHasSettings)
	{
		return;
	}

	// TODO: Conditionally enable on mobile?
	// TODO: Don't hard code the path to SchemaMapping.json.
#if (!SEOUL_SHIP && SEOUL_PLATFORM_WINDOWS)
	// Create out schema cache in developer builds.
	SettingsManager::Get()->m_pSchemaCache = DataStoreParser::CreateSchemaCache(FilePath::CreateConfigFilePath("Schema/SchemaMapping.json"));
#endif // /#if !SEOUL_SHIP

	// Create the normal mode content system.
	m_pSettings.Reset(SEOUL_NEW(MemoryBudgets::Content) Content::Store<DataStore>(false));

	// Set unloading threshold.
	if (0 == m_UnloadSuppress)
	{
		m_pSettings->SetUnloadLRUThresholdInBytes(m_uUnloadLRUThresholdInBytes);
	}
	else
	{
		m_pSettings->SetUnloadLRUThresholdInBytes(0u);
	}

	// TODO: Ideally, we'd carry through any entries in the Bootstrap table,
	// but this would mark them as "dynamic or non-loadable" loads, which will do
	// the wrong thing later (those entries will not correctly hot load or patch).
	m_tBoostrapTable.Clear();

	// Done - no longer need bootstrapping.
	SeoulMemoryBarrier();
	m_bHasSettings = true;
}

/** Access point for pre Content::LoadManager destruction. */
void SettingsManager::SettingsContainer::OnShutdownContentLoader()
{
	// Nothing to do if we don't have settings yet.
	if (!m_bHasSettings)
	{
		return;
	}

	Lock lock(m_BootstrapMutex);

	// Check the m_bHasSettings flag to see if another thread has
	// already shutdown settings.
	if (!m_bHasSettings)
	{
		return;
	}

	// Destroy settings.
	m_pSettings.Reset();

#if !SEOUL_SHIP
	// Cleanup the schema cache in developer builds.
	DataStoreParser::DestroySchemaCache(SettingsManager::Get()->m_pSchemaCache);
#endif

	// Done - no longer have settings.
	SeoulMemoryBarrier();
	m_bHasSettings = false;
}

/**
 * Begin suppression of unloading in the Settings Content::Store<>. Typically,
 * used to allow preloading of settings.
 */
void SettingsManager::SettingsContainer::BeginUnloadSuppress()
{
	if (1 == ++m_UnloadSuppress)
	{
		// Need to suppress.
		if (m_bHasSettings)
		{
			m_pSettings->SetUnloadLRUThresholdInBytes(0u);
		}
	}
}

/**
 * End suppression of unloading in the Settings Content::Store<>.
 */
void SettingsManager::SettingsContainer::EndUnloadSuppress()
{
	if (0 == --m_UnloadSuppress)
	{
		// Need to stop suppressing.
		if (m_bHasSettings)
		{
			m_pSettings->SetUnloadLRUThresholdInBytes(m_uUnloadLRUThresholdInBytes);
		}
	}
}

/**
 * Update the threshold at which old settings are unloaded, based on an LRU.
 */
void SettingsManager::SettingsContainer::SetUnloadLRUThresholdInBytes(UInt32 uUnloadLRUThresholdInBytes)
{
	m_uUnloadLRUThresholdInBytes = uUnloadLRUThresholdInBytes;

	// Need to update the value in the ContentStore.
	if (m_bHasSettings && 0 == m_UnloadSuppress)
	{
		m_pSettings->SetUnloadLRUThresholdInBytes(m_uUnloadLRUThresholdInBytes);
	}
}

} // namespace Seoul
