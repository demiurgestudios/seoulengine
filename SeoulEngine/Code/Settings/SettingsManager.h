/**
 * \file SettingsManager.h
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

#pragma once
#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include "ContentLoadManager.h"
#include "ContentHandle.h"
#include "ContentStore.h"
#include "DataStore.h"
#include "FilePath.h"
#include "Prereqs.h"
#include "ReflectionWeakAny.h"
#include "Settings.h"
#include "Singleton.h"
namespace Seoul { class DataStoreSchemaCache; }

namespace Seoul
{

/**
 * Singleton manager for caching settings
 * with a FilePath.
 */
class SettingsManager SEOUL_SEALED : public Singleton<SettingsManager>
{
public:
	SettingsManager();
	~SettingsManager();

	// Async retrieval of settings (check the returned IsLoaded() method for completion).
	SettingsContentHandle GetSettings(FilePath filePath)
	{
		return m_Settings.GetContent(filePath, false);
	}

	// Busy wait until the specified settings are loaded then
	// return a strong pointer to them.
	SharedPtr<DataStore> WaitForSettings(FilePath filePath);

	// Utility method - deserialize a settings file into an arbitrary C++ data structure
	// using reflection - objectPtr should be a read-write pointer to the object.
	//
	// The rootSectionIdentifier parameter is optional - if specified, a specific section of the
	// settings file will be used as the root of deserialization - otherwise the entire
	// file will be used.
	Bool DeserializeObject(
		FilePath filePath,
		const Reflection::WeakAny& objectPtr,
		HString rootSectionIdentifier = HString(),
		Bool bSkipPostSerialize = false);

	// Return true if any settings are currently loading, false otherwise.
	Bool AreSettingsLoading() const;

	/**
	 * Called immediately after Content::LoadManager construction by Engine, to place
	 * SettingsManager in normal operation mode, out of "bootstrap" mode.
	 */
	void OnInitializeContentLoader()
	{
		m_Settings.OnInitializeContentLoader();
	}

	/**
	 * Called immediately before destruction of the Content::LoadManager by Engine, to place
	 * SettingsManager back in "bootstrap" mode.
	 */
	void OnShutdownContentLoader()
	{
		m_Settings.OnShutdownContentLoader();
	}

	/**
	 * Begin suppression of unloading against the LRU in the Settings Content::Store<>. Typically,
	 * used to allow preloading of settings.
	 */
	void BeginUnloadSuppress()
	{
		m_Settings.BeginUnloadSuppress();
	}

	/**
	 * End suppression of unloading in the Settings Content::Store<>.
	 */
	void EndUnloadSuppress()
	{
		m_Settings.EndUnloadSuppress();
	}

	/**
	 * Update the threshold at which old settings are unloaded, based on an LRU.
	 */
	void SetUnloadLRUThresholdInBytes(UInt32 uUnloadLRUThresholdInBytes)
	{
		m_Settings.SetUnloadLRUThresholdInBytes(uUnloadLRUThresholdInBytes);
	}

	/** Allow manual override of the current state of settings data (currently used by in-engine tools). */
	void SetSettings(FilePath filePath, const SharedPtr<DataStore>& dataStore);

#if !SEOUL_SHIP
	DataStoreSchemaCache* GetSchemaCache() const
	{
		return m_pSchemaCache;
	}
#endif // /#if !SEOUL_SHIP

#if !SEOUL_SHIP
	Bool ValidateSettings(const String& sExcludeWildcard, Bool bCheckDependencies, UInt32& ruNumChecked);
#endif // /!SEOUL_SHIP

private:
	/** Internal utility, encapsulates the bootstrap vs. normal SettingsManager operation mode. */
	class SettingsContainer SEOUL_SEALED
	{
	public:
		SettingsContainer();
		~SettingsContainer();

		// Return true if any settings are actively loading, false otherwise.
		Bool AreSettingsLoading() const;

		// Return a Content::Handle from either the bootstrap or normal system, as appropriate.
		SettingsContentHandle GetContent(FilePath filePath, Bool bSyncLoad);

		// force overwrite settings.
		void SetSettings(FilePath filePath, const SharedPtr<DataStore>& dataStore);

		// Access points for post Content::LoadManager construction and
		// pre Content::LoadManager destruction.
		void OnInitializeContentLoader();
		void OnShutdownContentLoader();
		void BeginUnloadSuppress();
		void EndUnloadSuppress();
		void SetUnloadLRUThresholdInBytes(UInt32 uUnloadLRUThresholdInBytes);

	private:
		typedef HashTable<FilePath, SharedPtr<DataStore>, MemoryBudgets::Content> BoostrapTable;
		Mutex m_BootstrapMutex;
		BoostrapTable m_tBoostrapTable;
		ScopedPtr< Content::Store<DataStore> > m_pSettings;
		Atomic32 m_UnloadSuppress;
		UInt32 m_uUnloadLRUThresholdInBytes;
		Atomic32Value<Bool> m_bHasSettings;
	}; // class SettingsContainer
	SettingsContainer m_Settings;

#if !SEOUL_SHIP
	DataStoreSchemaCache* m_pSchemaCache;
#endif // /#if !SEOUL_SHIP

	Bool InternalDeserializeObject(
		FilePath filePath,
		const Reflection::WeakAny& objectPtr,
		HString rootSectionIdentifier = HString(),
		Bool bSkipPostSerialize = false);

	SEOUL_DISABLE_COPY(SettingsManager);
}; // class SettingsManager

} // namespace Seoul

#endif // include guard
