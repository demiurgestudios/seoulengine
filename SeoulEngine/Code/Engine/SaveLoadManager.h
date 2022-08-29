/**
 * \file SaveLoadManager.h
 * \brief Global singleton that handles encrypted storage
 * of data blobs to disk and (optionally) remote cloud storage.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SAVE_LOAD_MANAGER_H
#define SAVE_LOAD_MANAGER_H

#include "AtomicRingBuffer.h"
#include "Delegate.h"
#include "FilePath.h"
#include "HashTable.h"
#include "Prereqs.h"
#include "SaveLoadManagerSettings.h"
#include "SaveLoadResult.h"
#include "SaveLoadUtil.h"
#include "ScopedPtr.h"
#include "SeoulTime.h"
#include "SeoulUUID.h"
#include "Singleton.h"
namespace Seoul { class DataNode; }
namespace Seoul { class DataStore; }
namespace Seoul { class SaveApi; }
namespace Seoul { class Signal; }
namespace Seoul { class StreamBuffer; }
namespace Seoul { class Thread; }
namespace Seoul { namespace Reflection { class Type; } }
namespace Seoul { namespace Reflection { class WeakAny; } }

namespace Seoul
{

class ISaveLoadOnComplete SEOUL_ABSTRACT
{
public:
	virtual ~ISaveLoadOnComplete()
	{
	}
	
	/** Can be overriden. By default, OnLoadComplete() or OnSaveComplete() will be invoked on the main thread. */
	virtual Bool DispatchOnMainThread() const
	{
		return true;
	}

	/*
	 * @param[in] eLocalResult Specific result of loading from local storage.
	 * @param[in] eCloudResult Specific result of loading from remote(cloud) storage.
	 * @param[in] eFinalResult End result - kSuccess if operation was considered successful, otherwise should be considered a failure.
	 * @param[in] pData On successful load, an instance of the desired object type.
	 */
	virtual void OnLoadComplete(
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult,
		const Reflection::WeakAny& pData)
	{
	}

	/*
	 * @param[in] eLocalResult Specific result of saving to local storage.
	 * @param[in] eCloudResult Specific result of saving to remote(cloud) storage.
	 * @param[in] eFinalResult End result - kSuccess if operation was considered successful, otherwise should be considered a failure.
	 */
	virtual void OnSaveComplete(
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult)
	{
	}

protected:
	SEOUL_REFERENCE_COUNTED(ISaveLoadOnComplete);

	ISaveLoadOnComplete()
	{
	}

private:
	SEOUL_DISABLE_COPY(ISaveLoadOnComplete);
}; // class IOnSaveComplete

/** Cached state of a save by the SaveLoadMananger at any given time, keyed on local save path. */
struct SaveFileState SEOUL_SEALED
{
	SaveFileState()
		: m_iLastSaveUptimeInMilliseconds(0)
		, m_Metadata()
		, m_pCheckpoint()
#if SEOUL_UNIT_TESTS
		, m_iUnitTestLoadCount(0)
		, m_bRanFirstTimeLoadTests(false)
		, m_bRanFirstTimeSaveTests(false)
#endif // /#if SEOUL_UNIT_TESTS
	{
	}

	/**
	 * Used to rate limit cloud saving. Uses uptime as we want a world clock
	 * value that is monotonically increasing (can't be affected by a user's
	 * wall clock setting), and we don't need it to be serialized (a new
	 * gameplay session effectively resets the cloud save rate limiting).
	 */
	Int64 m_iLastSaveUptimeInMilliseconds;

	/**
	 * Tracking of save metadata. Mostly used around logic for cloud saving,
	 * such as delta generation.
	 */
	SaveLoadUtil::SaveFileMetadata m_Metadata;

	/**
	 * This is the last confirmed state of the server save data. It is
	 * the base save state that is used for delta transaction generation
	 * (we send deltas, not entire save blobs, to the server).
	 */
	SharedPtr<DataStore> m_pCheckpoint;

#if SEOUL_UNIT_TESTS
	/** In developer builds only when enabled, tracks the number of loads. */
	Int32 m_iUnitTestLoadCount;

	/**
	 * In developer builds only and when enabled, marks whether a suite
	 * of tests have been run. Mainly used to test certain edge cases
	 * with cloud save and load.
	 */
	Bool m_bRanFirstTimeLoadTests;
	Bool m_bRanFirstTimeSaveTests;
#endif // /#if SEOUL_UNIT_TESTS
}; // struct SaveFileState

/**
 * Handles threaded save and load of persistent data.
 */
class SaveLoadManager SEOUL_SEALED : public Singleton<SaveLoadManager>
{
public:
	/**
	 * Defines a migration hook.
	 *
	 * On load, if the body data version does not match the expected version,
	 * migration hooks will be invoked until:
	 * - the new version is the desired version (successful load).
	 * - a migration function fails (fail load).
	 * - the SaveLoadManager runs out of migration functions to run (fail load).
	 *
	 * @param[in] rDataStore The DataStore body to migrate.
	 * @param[in] root The DataNode of the root to migrate from.
	 * @param[out] rNewVersion After callback function runs, the version of the modified data.
	 *
	 * @return true if migration was successful, false otherwise.
	 */
	typedef Delegate<Bool(DataStore& rDataStore, const DataNode& root, Int32& riNewVersion)> MigrationCallback;

	/** Table of migrations defined with a load operation. */
	typedef HashTable<Int32, MigrationCallback, MemoryBudgets::Saving> Migrations;

	/** Table of save file state, used mostly for cloud saving and loading. */
	typedef HashTable<FilePath, SaveFileState, MemoryBudgets::Saving> StateTable;

	// Save/load low-level.
	static SaveLoadResult LoadLocalData(StreamBuffer& data, DataStore& rSaveData, DataStore& rPendingDelta, SaveLoadUtil::SaveFileMetadata& rMetadata);
	static SaveLoadResult SaveLocalData(StreamBuffer& data, const DataStore& saveData, const DataStore& pendingDelta, const SaveLoadUtil::SaveFileMetadata& metadata);

	SEOUL_DELEGATE_TARGET(SaveLoadManager);

	// In non-Ship builds, enable testing mode. This mode will intentionally hit edge cases, particularly in cloud save/load,
	// including sending bad data to the server.
	SaveLoadManager(const SaveLoadManagerSettings& settings);
	~SaveLoadManager();

	/** @return The randomly generated UUID corresponding to the current game session. */
	const String& GetSessionGuid() const
	{
		return m_sSessionGuid;
	}

	/** @return The settings used to configure this SaveLoadManager. */
	const SaveLoadManagerSettings& GetSettings() const
	{
		return m_Settings;
	}

	/** @return The total current size of the work queue. */
	Atomic32Type GetWorkQueueCount() const { return m_WorkQueue.GetCount(); }

	void QueueLoad(
		const Reflection::Type& objectType,
		FilePath savePath,
		const String& sCloudURL,
		Int32 iExpectedVersion,
		const SharedPtr<ISaveLoadOnComplete>& pCallback,
		const Migrations& tMigrations,
		Bool bResetSession);

	void QueueSave(
		FilePath savePath,
		const String& sCloudURL,
		const Reflection::WeakAny& pObject,
		Int32 iDataVersion,
		const SharedPtr<ISaveLoadOnComplete>& pCallback,
		Bool bForceImmediateCloudSave);

#if SEOUL_ENABLE_CHEATS
	void QueueSaveReset(FilePath savePath, const String& sCloudURL, Bool bResetSession);
#endif // /#if SEOUL_ENABLE_CHEATS

#if SEOUL_UNIT_TESTS
	Bool IsFirstTimeTestingComplete() const;
#endif // /#if SEOUL_UNIT_TESTS

private:
	/**
	 * Type of save/load operation for a queued request
	 */
	enum EntryType
	{
		kNone,
		kSave,
		kLoadNoSessionChange,
		kLoadSessionChange,
#if SEOUL_ENABLE_CHEATS
		kSaveResetNoSessionChange,
		kSaveResetSessionChange,
#endif // /#if SEOUL_ENABLE_CHEATS
	};

	/**
	 * Internal tracking of a save operation.
	 */
	struct Entry SEOUL_SEALED
	{
		Entry()
			: m_pLoadDataType(nullptr)
			, m_tMigrations()
			, m_pCallback()
			, m_pSaveData()
			, m_iVersion(0)
			, m_Path()
			, m_eEntryType(kNone)
			, m_sCloudURL()
			, m_bForceImmediateCloudSave(false)
		{
		}

		Reflection::Type const* m_pLoadDataType;
		Migrations m_tMigrations;
		SharedPtr<ISaveLoadOnComplete> m_pCallback;
		SharedPtr<DataStore> m_pSaveData;
		Int32 m_iVersion;
		FilePath m_Path;
		EntryType m_eEntryType;
		String m_sCloudURL;
		Bool m_bForceImmediateCloudSave;
	};

	SaveLoadManagerSettings const m_Settings;
	String m_sSessionGuid;
	ScopedPtr<SaveApi> m_pSaveApi;
	AtomicRingBuffer<Entry*, MemoryBudgets::Saving> m_WorkQueue;
	ScopedPtr<Thread> m_pWorkerThread;
	ScopedPtr<Signal> m_pSignal;
	Atomic32Value<Bool> m_bRunning;

	// Note that due to the heavy amount of threading in SaveLoadManager,
	// functions below are static unless they need access to m_pSaveApi,
	// in which case they are const. The goal here is to isolate
	// and encapsulate logic to eliminate as many opportunities for
	// thread synchronization bugs as possible.

	// On loads, apply any migrations to migrate the save's
	// schema version to a newer schema version.
	static SaveLoadResult WorkerThreadApplyMigrations(
		const Migrations& tMigrations,
		Int32 iTargetVersion,
		DataStore& rDataStore,
		const DataNode& rootDataNode,
		Int32 iCurrentVersion);

	// Wrapper around invoking save or load callbacks.
	static void DispatchLoadCallback(
		const SharedPtr<ISaveLoadOnComplete>& pCallback,
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult,
		const Reflection::WeakAny& pData);
	static void DispatchSaveCallback(
		const SharedPtr<ISaveLoadOnComplete>& pCallback,
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult);

	// Utilities involved in loading.
	void WorkerThreadLoad(SaveFileState& rState, const Entry& entry) const;
	SaveLoadResult WorkerThreadLoadCloudData(
		const String& sURL,
		const SaveLoadUtil::SaveFileMetadata& targetMetadata,
		const DataStore& targetSaveData,
		const SaveLoadUtil::SaveFileMetadata& pendingDeltaMetadata,
		const DataStore& pendingDelta,
		SaveLoadUtil::SaveFileMetadata& rOutMetadata,
		DataStore& rOutSaveData,
		Bool bTestOnlyNoEmail = false,
		Bool bTestOnlyNoVerify = false) const;
	SaveLoadResult WorkerThreadLoadLocalData(
		FilePath filePath,
		DataStore& rSaveData,
		DataStore& rPendingDelta,
		SaveLoadUtil::SaveFileMetadata& rMetadata) const;
	static SaveLoadResult WorkerThreadCreateObject(
		const Entry& entry,
		const DataStore& dataStore,
		Reflection::WeakAny& rpObject);

	// Utilities involved in saving.
	void WorkerThreadSave(SaveFileState& rState, const Entry& entry) const;
	SaveLoadResult WorkerThreadSaveCloudData(
		const SaveFileState& state,
		const Entry& entry,
		const SaveLoadUtil::SaveFileMetadata& pendingDeltaMetadata,
		const DataStore& pendingDelta,
		const String& sTargetMD5,
		Bool bEnableTests,
		Bool bTestOnlyNoEmail = false,
		Bool bTestOnlyNoVerify = false) const;
	SaveLoadResult WorkerThreadSaveLocalData(
		FilePath filePath,
		const DataStore& saveData,
		const DataStore& pendingDelta,
		const SaveLoadUtil::SaveFileMetadata& metadata) const;

	Int WorkerThreadMain(const Thread&);

#if SEOUL_UNIT_TESTS
	void RunFirstTimeLoadTests(SaveFileState& rState, const Entry& entry);
	void RunFirstTimeSaveTests(SaveFileState& rState, const Entry& entry);

	public:
		FilePath UnitTestingHook_GetFilePathOfActiveSaveLoadEntry() const
		{
			FilePath ret;
			{
				Lock lock(m_UnitTestActiveFilePathMutex);
				ret = m_UnitTestActiveFilePath;
			}
			return ret;
		}

	private:
		FilePath m_UnitTestActiveFilePath{};
		Mutex m_UnitTestActiveFilePathMutex{};
		Atomic32Value<Bool> m_bFirstTimeLoadTestsComplete;
		Atomic32Value<Bool> m_bFirstTimeSaveTestsComplete;
#endif // /#if SEOUL_UNIT_TESTS

	SEOUL_DISABLE_COPY(SaveLoadManager);
}; // class SaveLoadManager

} // namespace Seoul

#endif // include guard
