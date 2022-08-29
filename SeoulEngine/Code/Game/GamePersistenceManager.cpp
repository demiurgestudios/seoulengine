/**
 * \file GamePersistenceManager.cpp
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

#include "GamePersistenceManager.h"
#include "JobsManager.h"
#include "Logger.h"
#include "MemoryBarrier.h"
#include "ReflectionDefine.h"
#include "ReflectionRegistry.h"
#include "ReflectionType.h"
#include "SaveLoadManager.h"

#if SEOUL_WITH_GAME_PERSISTENCE

namespace Seoul::Game
{

using namespace Reflection;

class PersistenceLoadUtility SEOUL_SEALED : public ISaveLoadOnComplete
{
public:
	SEOUL_DELEGATE_TARGET(PersistenceLoadUtility);

	PersistenceLoadUtility(
		const Type& persistenceDataType,
		const PersistenceSettings& settings)
		: m_Type(persistenceDataType)
		, m_Settings(settings)
		, m_bRunning(true)
		, m_bSuccess(false)
		, m_bNew(false)
	{
	}
	~PersistenceLoadUtility() = default;

	const PersistenceSettings& GetSettings() const { return m_Settings; }
	Bool IsNew() const { return m_bNew; }
	Bool IsRunning() const { return m_bRunning; }
	Bool WasSuccessful() const { return m_bSuccess; }

	Reflection::WeakAny m_pPersistenceData;

private:
	SEOUL_DISABLE_COPY(PersistenceLoadUtility);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(PersistenceLoadUtility);

	virtual Bool DispatchOnMainThread() const SEOUL_OVERRIDE
	{
		// Safe and desirable to find out about load or save
		// completion immediately without waiting for the main thread.
		return false;
	}

	virtual void OnLoadComplete(
		SaveLoadResult eLocalResult,
		SaveLoadResult eCloudResult,
		SaveLoadResult eFinalResult,
		const Reflection::WeakAny& pData) SEOUL_OVERRIDE
	{
		using namespace SaveLoadResult;

		if (kSuccess == eFinalResult)
		{
			m_pPersistenceData = pData;
			m_bNew = false;
			m_bSuccess = true;
		}
		// If the result was kCloudCancelled, it means we're shutting down,
		// so the job must fail.
		else if (kCloudCancelled == eFinalResult)
		{
			m_pPersistenceData.Reset();
			m_bNew = false;
			m_bSuccess = false;
		}
		// Otherwise, the job is always successful, even in the error case, unless
		// type instantiation fails. We suppress warnings on file not found.
		else
		{
			if (kErrorFileNotFound != eFinalResult)
			{
				SEOUL_WARN("Unexpected save data load result: %s(%s, %s, %s)\n",
					m_Settings.m_FilePath.CStr(),
					EnumToString<SaveLoadResult>(eLocalResult),
					EnumToString<SaveLoadResult>(eCloudResult),
					EnumToString<SaveLoadResult>(eFinalResult));
			}

			m_pPersistenceData = m_Type.New(MemoryBudgets::Saving);
			m_bNew = true;
			m_bSuccess = m_pPersistenceData.IsValid();
		}

		m_bRunning = false;
	}

	const Type& m_Type;
	const PersistenceSettings& m_Settings;
	Atomic32Value<Bool> m_bRunning;
	Atomic32Value<Bool> m_bSuccess;
	Atomic32Value<Bool> m_bNew;
}; // class Game::PersistenceLoadUtility

SEOUL_TYPE(PersistenceManager);

Mutex PersistenceManager::s_Mutex;

PersistenceManager::PersistenceManager()
{
}

PersistenceManager::~PersistenceManager()
{
}

PersistenceLock::PersistenceLock()
{
	// Try lock until success - let the Jobs::Manager do work while we're waiting.
	while (!PersistenceManager::s_Mutex.TryLock())
	{
		Jobs::Manager::Get()->YieldThreadTime();
	}
}

PersistenceLock::~PersistenceLock()
{
	PersistenceManager::s_Mutex.Unlock();
}

PersistenceManagerLoadJob::PersistenceManagerLoadJob(const PersistenceSettings& settings)
	: m_Settings(settings)
	, m_pPersistenceManager()
{
}

PersistenceManagerLoadJob::~PersistenceManagerLoadJob()
{
}

void PersistenceManagerLoadJob::InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId)
{
	// Get the attribute which defines our persistence manager instantiator.
	auto pCreatePersistenceManager = m_Settings.m_pPersistenceManagerType->GetAttribute<Attributes::CreatePersistenceManager>();
	if (nullptr == pCreatePersistenceManager)
	{
		SEOUL_LOG("Failed loading persistence manager, no CreatePersistenceManager attribute.");
		reNextState = Jobs::State::kError;
		return;
	}

	// Get the attribute which defines the root persistence data type.
	auto pRootPersistenceDataType = m_Settings.m_pPersistenceManagerType->GetAttribute<Attributes::RootPersistenceDataType>();
	if (nullptr == pRootPersistenceDataType)
	{
		SEOUL_LOG("Failed loading persistence manager, no RootPersistenceDataType attribute.");
		reNextState = Jobs::State::kError;
		return;
	}

	// Now acquire the persistence data type from the registry.
	auto pPersistenceDataType = Registry::GetRegistry().GetType(pRootPersistenceDataType->m_Name);
	if (nullptr == pPersistenceDataType)
	{
		SEOUL_LOG("Failed loading persistence manager, \"%s\" is not a valid persistence data type.", pRootPersistenceDataType->m_Name.CStr());
		reNextState = Jobs::State::kError;
		return;
	}

	// Create the utility.
	SharedPtr<PersistenceLoadUtility> pUtility(SEOUL_NEW(MemoryBudgets::Persistence) PersistenceLoadUtility(*pPersistenceDataType, m_Settings));
	// Now start the load - this needs to be down outside pUtility, because if the load operation is rejected immediately (e.g. during system shutdown),
	// ref counting of this coudl result in Game::PersistenceUtility being deleted from within its constructor.
	SaveLoadManager::Get()->QueueLoad(
		*pPersistenceDataType,
		m_Settings.m_FilePath,
		m_Settings.m_sCloudLoadURL,
		m_Settings.m_iVersion,
		pUtility,
		m_Settings.m_tMigrations,
		// Load of player save resets the session guid.
		true);

	// Now wait for the job to complete.
	//
	// Swith to the appropriate interval while waiting.
	SetJobQuantum(Jobs::Quantum::kWaitingForDependency);
	while (pUtility->IsRunning())
	{
		Jobs::Manager::Get()->YieldThreadTime();
	}
	// Restore priority to default..
	SetJobQuantum(Min(GetJobQuantum(), Jobs::Quantum::kDefault));

	// On failure, return immediately.
	if (!pUtility->WasSuccessful())
	{
		pPersistenceDataType->Delete(pUtility->m_pPersistenceData);
		SEOUL_LOG("Failed loading persistence data.");
		reNextState = Jobs::State::kError;
		return;
	}

	// If defined, apply the post load attribute.
	{
		auto pPostLoad = m_Settings.m_pPersistenceManagerType->GetAttribute<Attributes::PersistencePostLoad>();
		if (nullptr != pPostLoad)
		{
			// Error from this function indicates a load failure.
			if (!pPostLoad->m_pPersistencePostLoadFunc(
				m_Settings,
				pUtility->m_pPersistenceData,
				pUtility->IsNew()))
			{
				pPersistenceDataType->Delete(pUtility->m_pPersistenceData);
				SEOUL_LOG("Persistence post load returned false, data load fails.");
				reNextState = Jobs::State::kError;
				return;
			}
		}
	}

	// Now instantiate the persistence manager.
	m_pPersistenceManager.Reset(pCreatePersistenceManager->m_pCreatePersistenceManager(
		m_Settings,
		false, /* TODO: */
		pUtility->m_pPersistenceData));
	if (!m_pPersistenceManager.IsValid())
	{
		pPersistenceDataType->Delete(pUtility->m_pPersistenceData);
		SEOUL_LOG("Failed instantiating concrete persistence manager of type \"%s\".", m_Settings.m_pPersistenceManagerType->GetName().CStr());
		reNextState = Jobs::State::kError;
		return;
	}

	reNextState = Jobs::State::kComplete;
}

SEOUL_TYPE(NullPersistenceData, TypeFlags::kDisableCopy)

NullPersistenceData::NullPersistenceData()
{
}

NullPersistenceData::~NullPersistenceData()
{
}

SEOUL_BEGIN_TYPE(NullPersistenceManager, TypeFlags::kDisableNew)
	SEOUL_PARENT(PersistenceManager)
	SEOUL_ATTRIBUTE(CreatePersistenceManager, NullPersistenceManager::CreateNullPersistenceManager)
	SEOUL_ATTRIBUTE(PersistencePostLoad, NullPersistenceManager::PersistencePostLoad)
	SEOUL_ATTRIBUTE(RootPersistenceDataType, "NullPersistenceData")
SEOUL_END_TYPE()

CheckedPtr<PersistenceManager> NullPersistenceManager::CreateNullPersistenceManager(
	const PersistenceSettings& settings,
	Bool bDisableSaving,
	const Reflection::WeakAny& pPersistenceData)
{
	return SEOUL_NEW(MemoryBudgets::Persistence) NullPersistenceManager(
		settings,
		bDisableSaving,
		SharedPtr<NullPersistenceData>(pPersistenceData.Cast<NullPersistenceData*>()));
}

Bool NullPersistenceManager::PersistencePostLoad(const PersistenceSettings& settings, const Reflection::WeakAny& pPersistenceData, Bool bNew)
{
	return true;
}

NullPersistenceManager::NullPersistenceManager(const PersistenceSettings& settings, Bool bDisableSaving, const SharedPtr<NullPersistenceData>& pPersistenceData)
{
}

NullPersistenceManager::~NullPersistenceManager()
{
}

void NullPersistenceManager::QueueSave(
	Bool bForceCloudSave,
	const SharedPtr<ISaveLoadOnComplete>& pSaveComplete /*= SharedPtr<ISaveLoadOnComplete>()*/)
{
}

void NullPersistenceManager::Update()
{
}

void NullPersistenceManager::GetSoundSettings(Sound::Settings& soundSettings) const
{
}

} // namespace Seoul::Game

#endif // /SEOUL_WITH_GAME_PERSISTENCE
