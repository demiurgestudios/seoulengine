/**
 * \file GamePersistenceManager.h
 * \brief Game::PersistenceManager is an encapsulated, concrete representation
 * of game save data. The schema of the save data is defined by C++ classes.
 *
 * As such, an App must define a concrete subclass of GamePersistenceManager.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_PERSISTENCE_MANAGER_H
#define GAME_PERSISTENCE_MANAGER_H

#include "Delegate.h"
#include "JobsJob.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "ReflectionAttribute.h"
#include "ReflectionWeakAny.h"
#include "SaveLoadManager.h"
#include "SeoulString.h"
#include "Singleton.h"
namespace Seoul { namespace Game { class PersistenceManager; } }
namespace Seoul { namespace Sound { class Settings; } }

#if SEOUL_WITH_GAME_PERSISTENCE

namespace Seoul::Game
{

struct PersistenceSettings SEOUL_SEALED
{
	PersistenceSettings()
		: m_tMigrations()
		, m_pPersistenceManagerType(nullptr)
		, m_FilePath()
		, m_sCloudLoadURL()
		, m_sCloudResetURL()
		, m_sCloudSaveURL()
		, m_iVersion(0)
	{
	}

	SaveLoadManager::Migrations m_tMigrations;
	Reflection::Type const* m_pPersistenceManagerType;
	FilePath m_FilePath;
	String m_sCloudLoadURL;
	String m_sCloudResetURL;
	String m_sCloudSaveURL;
	Int32 m_iVersion;
}; // struct Game::PersistenceSettings

/**
 * Abstract Game::PersistenceManager base class.
 */
class PersistenceManager SEOUL_ABSTRACT : public Singleton<PersistenceManager>
{
public:
	virtual ~PersistenceManager();

	virtual void QueueSave(
		Bool bForceCloudSave,
		const SharedPtr<ISaveLoadOnComplete>& pSaveComplete = SharedPtr<ISaveLoadOnComplete>()) = 0;

	virtual void Update() = 0;

	virtual void GetSoundSettings(Sound::Settings&) const = 0;

protected:
	PersistenceManager();

private:
	friend class PersistenceLock;
	friend class PersistenceTryLock;
	static Mutex s_Mutex;

	SEOUL_DISABLE_COPY(PersistenceManager);
}; // class Game::PersistenceManager

/**
 * Lock for synchronizing access to GamePersistence. The lock
 * is acquired in a Jobs::Manager aware fashion, so that contention
 * should not result in deadlock due to Job starvation.
 */
class PersistenceLock SEOUL_SEALED
{
public:
	PersistenceLock();
	~PersistenceLock();

private:
	SEOUL_DISABLE_COPY(PersistenceLock);
}; // class Game::PersistenceLock

/**
 * Lock for synchronizing access to GamePersistence. The lock
 * is acquired in a Jobs::Manager aware fashion, so that contention
 * should not result in deadlock due to Job starvation. Locking
 * is optional.
 */
class PersistenceTryLock SEOUL_SEALED
{
public:
	PersistenceTryLock()
		: m_TryLock(PersistenceManager::s_Mutex)
	{
	}

	~PersistenceTryLock()
	{
	}

	/** @return Whether the lock was acquired or not. */
	Bool IsLocked() const
	{
		return m_TryLock.IsLocked();
	}

private:
	TryLock const m_TryLock;

	SEOUL_DISABLE_COPY(PersistenceTryLock);
}; // class Game::PersistenceTryLock

namespace Reflection
{

namespace Attributes
{

/**
 * Attribute used to construct the concrete PersistenceManager
 * subclass.
 *
 * This is a workaround for the lack of New<>() with arguments
 * support in the reflection system.
 */
class CreatePersistenceManager SEOUL_SEALED : public Attribute
{
public:
	typedef CheckedPtr<PersistenceManager> (*CreatePersistenceManagerFunc)(
		const PersistenceSettings& settings,
		Bool bDisableSaving,
		const Reflection::WeakAny& pPersistenceData);

	CreatePersistenceManager(CreatePersistenceManagerFunc pCreatePersistenceManager)
		: m_pCreatePersistenceManager(pCreatePersistenceManager)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("CreatePersistenceManager");
		return kId;
	}

	CreatePersistenceManagerFunc const m_pCreatePersistenceManager;
}; // class CreatePersistenceManager

/**
 * Attribute to put on the concrete PersistenceManager subclass,
 * optional. If defined, must provide a function that will be called
 * on the (successfully) loaded data, prior to its commit to the
 * persistence manager.
 */
class PersistencePostLoad SEOUL_SEALED : public Attribute
{
public:
	// Function to define and that will be called. Must return true
	// to indicate a successful load (this function can both post-process
	// and prune the data, and also provide final app-level verification
	// of the data).
	typedef Bool (*PersistencePostLoadFunc)(
		const PersistenceSettings& settings,
		const Reflection::WeakAny& pPersistenceData,
		Bool bNew);

	PersistencePostLoad(PersistencePostLoadFunc pPersistencePostLoadFunc)
		: m_pPersistencePostLoadFunc(pPersistencePostLoadFunc)
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("PersistencePostLoad");
		return kId;
	}

	PersistencePostLoadFunc const m_pPersistencePostLoadFunc;
}; // class PersistencePostLoad

/**
 * Attribute to put on the concrete PersistenceManager subclass,
 * defines the type of the root persistence data object used
 * by the class.
 */
class RootPersistenceDataType SEOUL_SEALED : public Attribute
{
public:
	template <size_t zStringArrayLengthInBytes>
	RootPersistenceDataType(Byte const (&sName)[zStringArrayLengthInBytes])
		: m_Name(CStringLiteral(sName))
	{
	}

	virtual HString GetId() const SEOUL_OVERRIDE
	{
		return StaticId();
	}

	static HString StaticId()
	{
		static const HString kId("RootPersistenceDataType");
		return kId;
	}

	HString m_Name;
}; // class RootPersistenceDataType

} // namespace Attribute

} // namespace Reflection

/**
 * Utility to asynchronously load a root persistence data
 * object, later used to construct the concrete
 * PersistenceManager.
 */
class PersistenceManagerLoadJob SEOUL_SEALED : public Jobs::Job
{
public:
	PersistenceManagerLoadJob(const PersistenceSettings& settings);
	~PersistenceManagerLoadJob();

	ScopedPtr<PersistenceManager>& GetPersistenceManager()
	{
		return m_pPersistenceManager;
	}

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(PersistenceManagerLoadJob);

	const PersistenceSettings m_Settings;
	ScopedPtr<PersistenceManager> m_pPersistenceManager;

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(PersistenceManagerLoadJob);
}; // class Game::PersistenceManagerLoadJob

class NullPersistenceData SEOUL_SEALED
{
public:
	NullPersistenceData();
	~NullPersistenceData();

private:
	SEOUL_REFERENCE_COUNTED(NullPersistenceData);
	SEOUL_DISABLE_COPY(NullPersistenceData);
}; // class NullPersistenceData

class NullPersistenceManager SEOUL_SEALED : public PersistenceManager
{
public:
	static CheckedPtr<NullPersistenceManager> Get()
	{
		return (NullPersistenceManager*)PersistenceManager::Get().Get();
	}

	static CheckedPtr<const NullPersistenceManager> GetConst()
	{
		return (NullPersistenceManager*)PersistenceManager::Get().Get();
	}

	static CheckedPtr<PersistenceManager> CreateNullPersistenceManager(
		const PersistenceSettings& settings,
		Bool bDisableSaving,
		const Reflection::WeakAny& pPersistenceData);
	static Bool PersistencePostLoad(
		const PersistenceSettings& settings,
		const Reflection::WeakAny& pPersistenceData,
		Bool bNew);

	~NullPersistenceManager();

	virtual void QueueSave(
		Bool bForceCloudSave,
		const SharedPtr<ISaveLoadOnComplete>& pSaveComplete = SharedPtr<ISaveLoadOnComplete>()) SEOUL_OVERRIDE;
	virtual void Update() SEOUL_OVERRIDE;

	virtual void GetSoundSettings(Sound::Settings&) const SEOUL_OVERRIDE;

private:
	NullPersistenceManager(
		const PersistenceSettings& settings,
		Bool bDisableSaving,
		const SharedPtr<NullPersistenceData>& pPersistenceData);

	SEOUL_DISABLE_COPY(NullPersistenceManager);
}; // class NullPersistenceManager

} // namespace Seoul::Game

#endif // /SEOUL_WITH_GAME_PERSISTENCE

#endif // include guard
