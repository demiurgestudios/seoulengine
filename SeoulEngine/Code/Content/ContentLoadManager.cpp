/**
 * \file ContentLoadManager.cpp
 * \brief Content::LoadManager handles queued loading of content data from permanent
 * storage.
 *
 * While Content::LoadManager is a single choke point for content loads,
 * content management is otherwise handled by disparate managers (i.e. TextureManager).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentChangeNotifier.h"
#include "ContentChangeNotifierLocal.h"
#include "ContentChangeNotifierMoriarty.h"
#include "ContentLoadManager.h"
#include "CookDatabase.h"
#include "CookManager.h"
#include "EventsManager.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JobsManager.h"
#include "Logger.h"
#include "MoriartyClient.h"
#include "Path.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_TYPE(Content::Reload);

#if SEOUL_HOT_LOADING
/**
 * @return The environment specific implementation of ContentChangeNotifier.
 */
static Content::ChangeNotifier* CreateContentChangeNotifier()
{
#if SEOUL_WITH_MORIARTY
	if (MoriartyClient::Get() && MoriartyClient::Get()->IsConnected())
	{
		return SEOUL_NEW(MemoryBudgets::Content) Content::ChangeNotifierMoriarty();
	}
#endif

#if SEOUL_HOT_LOADING
	return SEOUL_NEW(MemoryBudgets::Content) Content::ChangeNotifierLocal();
#else
	return SEOUL_NEW(MemoryBudgets::Content) NullChangeNotifier();
#endif
}
#endif // /#if SEOUL_HOT_LOADING

namespace Content
{

LoadManager::LoadManager()
	: m_HotLoadSuppress(0)
	, m_AllLoadWait(0)
	, m_eHotLoadMode(LoadManagerHotLoadMode::kNoAction)
#if SEOUL_HOT_LOADING
	, m_pContentChangeNotifier(CreateContentChangeNotifier())
	, m_tHotLoadingSuppressSpecific()
#endif
{
	SEOUL_ASSERT(IsMainThread());
}

LoadManager::~LoadManager()
{
	SEOUL_ASSERT(IsMainThread());

#if SEOUL_HOT_LOADING
	m_pContentChangeNotifier.Reset();
#endif
}

/** @return True if load operations are active, false otherwise. */
Bool LoadManager::HasActiveLoads() const
{
	return (LoaderBase::GetActiveLoaderCount() > 0);
}

/**
 * Specify a list of files to reload or an empty set to reload all files.
 * Files reloaded will be added to the output set, to allow monitoring of
 * load progress.
 */
void LoadManager::Reload(Content::Reload& rReload)
{
	SEOUL_ASSERT(IsMainThread());

	rReload.m_vReloaded.Clear();
	Events::Manager::Get()->TriggerEvent(ReloadEventId, &rReload);
}

/**
 * Apply an immediate and aggressive unload to all ContentStores. This
 * call both unloads any loaded candidate content immediately and it
 * also unloads candidate content that would not normally unload under
 * normal use conditions (e.g. Content::Stores which deliberately keep
 * all loaded content in memory to reduce future loading pressure).
 */
void LoadManager::UnloadAll()
{
	SEOUL_ASSERT(IsMainThread());

	// Repeat until we hit a termination condition or until all
	// candidate assets have been unloaded.
	UInt32 uLastRemaining = 0u;
	while (true)
	{
		// Perform an iteration - uRemaining will be populated with
		// the total number of assets that were left loaded.
		UInt32 uRemaining = 0u;
		Events::Manager::Get()->TriggerEvent(UnloadEventId, &uRemaining);

		// If we hit zero, we're done.
		if (uRemaining == 0u) { return; }

		// Otherwise, if uRemaining is the same as last remaining, we're done.
		if (uRemaining == uLastRemaining) { return; }

		// Otherwise, update and loop.
		uLastRemaining = uRemaining;
	}
}

/**
 * Should be called once and only once per frame on the main thread.
 */
void LoadManager::PrePose()
{
	SEOUL_ASSERT(IsMainThread());

	// Process content changes.
#if SEOUL_HOT_LOADING
	InternalPollContentChanges();
#endif

	// Give dependents a chance to do some per-frame work.
	Events::Manager::Get()->TriggerEvent(PollEventId);
}

/**
 * Wait until all active content load ops are complete.
 */
void LoadManager::WaitUntilAllLoadsAreFinished()
{
	++m_AllLoadWait;
	while (LoaderBase::GetActiveLoaderCount() > 0)
	{
		// Give the Jobs::Manager some thread time.
		Jobs::Manager::Get()->YieldThreadTime();

		// Tick any systems that have registered for MainThreadTickWhileWait.
		if (IsMainThread())
		{
			Events::Manager::Get()->TriggerEvent(MainThreadTickWhileWaiting, 0.0f);
		}
	}
	--m_AllLoadWait;
}

/**
 * Returns true if the specified content path is either cooked
 * or in source.
 *
 * Developer builds might not have cooked content until it is requested,
 * so this convenience method answers the question:
 * "will this content exist if I ask for it?"
 */
Bool LoadManager::IsContentAvailable(FilePath filePath) const
{
	// Check cooked content first
	if (FileManager::Get()->Exists(filePath))
	{
		return true;
	}

	// Check in source
	return FileManager::Get()->ExistsInSource(filePath);
}

/** Allow hot loading to be momentarily suppressed for a specific file. Intended use case is saving. */
void LoadManager::TempSuppressSpecificHotLoad(FilePath filePath)
{
#if SEOUL_HOT_LOADING
	(void)m_tHotLoadingSuppressSpecific.Overwrite(filePath, SeoulTime::GetGameTimeInTicks());
#endif // /#if SEOUL_HOT_LOADING
}

#if SEOUL_HOT_LOADING
/**
 * Call to associate the set of files vDependencies with filePath.
 *
 * This method assumes that vDependencies is the complete set of dependencies.
 * It clears any previous dependency association with filePath.
 */
void LoadManager::SetDependencies(FilePath filePath, const DepVector& vDependencies)
{
	Lock lock(m_DependencyMutex);

	// Erase any existing association.
	InsideLockClearDependencies(filePath);

	// Update the dependents table.
	UInt32 const nDependencies = vDependencies.GetSize();
	for (UInt32 i = 0u; i < nDependencies; ++i)
	{
		FilePath const dependencyFilePath = vDependencies[i];

		DepTable::ValueType* pDependentsVector = m_tDependentsTable.Find(dependencyFilePath);
		if (nullptr == pDependentsVector)
		{
			pDependentsVector = &(m_tDependentsTable.Insert(dependencyFilePath, DepVector()).First->Second);
		}

		if (pDependentsVector->End() == pDependentsVector->FindFromBack(dependencyFilePath))
		{
			pDependentsVector->PushBack(filePath);
		}
	}

	// Set the new dependencies
	SEOUL_VERIFY(m_tDependencyTable.Overwrite(filePath, vDependencies).Second);
}
#endif // #if SEOUL_HOT_LOADING

/**
 * Version of WaitUntilLoadIsFinished with no templating,
 * so we can define it in the .cpp file.
 */
void LoadManager::InteranlWaitUntilLoadIsFinished(
	Bool(*isLoading)(void*),
	void* pContent,
	const FilePath& filePath)
{
	// Main thread blocking wait warning.
#if SEOUL_LOGGING_ENABLED
	auto const iStartTime = SeoulTime::GetGameTimeInTicks();
#endif // /#if SEOUL_LOGGING_ENABLED

	// Perform the actual wait - reduce job quantum if
	// the current thread has an active job to reduce.
	auto pJob = Jobs::Manager::Get()->GetCurrentThreadJob();
	if (pJob)
	{
		Jobs::ScopedQuantum scoped(*pJob, Jobs::Quantum::kWaitingForDependency);
		while (isLoading(pContent))
		{
			// Give the Jobs::Manager some thread time.
			Jobs::Manager::Get()->YieldThreadTime();
		}
	}
	else
	{
		while (isLoading(pContent))
		{
			// Give the Jobs::Manager some thread time.
			Jobs::Manager::Get()->YieldThreadTime();
		}
	}

	// If enabled, warn about blocking loads on the main
	// or render threads.
#if SEOUL_LOGGING_ENABLED
	auto const iEndTime = SeoulTime::GetGameTimeInTicks();
	if (m_bBlockingLoadCheck)
	{
		auto const f = SeoulTime::ConvertTicksToMilliseconds(
			iEndTime - iStartTime);
		if (IsMainThread())
		{
			SEOUL_WARN("%s: blocked main thread for %.2f ms waiting for load.",
				filePath.GetRelativeFilenameInSource().CStr(),
				f);
		}
		else if (IsRenderThread())
		{
			SEOUL_WARN("%s: blocked render thread for %.2f ms waiting for load.",
				filePath.GetRelativeFilenameInSource().CStr(),
				f);
		}
	}
#endif // /#if SEOUL_LOGGING_ENABLED
}

#if SEOUL_HOT_LOADING
/**
 * Call to clear all currently registered dependencies of filePath. Typically,
 * this should be called immediately before dependencies will be recomputed.
 *
 * \pre The m_DependenciesMutex must be locked before calling this method.
 */
void LoadManager::InsideLockClearDependencies(FilePath filePath)
{
	auto pDependencyVector = m_tDependencyTable.Find(filePath);
	if (nullptr != pDependencyVector)
	{
		UInt32 const nDependencies = pDependencyVector->GetSize();
		for (UInt32 i = 0u; i < nDependencies; ++i)
		{
			auto pDependentsVector = m_tDependentsTable.Find((*pDependencyVector)[i]);
			if (nullptr != pDependentsVector)
			{
				auto pDependent = pDependentsVector->Find(filePath);
				if (nullptr != pDependent)
				{
					Swap(*pDependent, pDependentsVector->Back());
					pDependentsVector->PopBack();
				}
			}
		}

		pDependencyVector = nullptr;
		SEOUL_VERIFY(m_tDependencyTable.Erase(filePath));
	}
}

/**
 * Processes pending content change events - a nop
 * unless hot loading is enabled.
 */
void LoadManager::InternalPollContentChanges()
{
	if (m_pContentChangeNotifier.IsValid())
	{
		// Pop every event off the change notifier queue.
		ChangeEvent* pEvent = m_pContentChangeNotifier->Pop();
		while (nullptr != pEvent)
		{
			// Insert the event and any dependent events that it generates
			// into the m_tContentChanges table.
			InternalInsertEventAndDependents(pEvent);

			// We're either disposing this event, or it's been inserted into the
			// table, so we dismiss the local reference.
			SeoulGlobalDecrementReferenceCount(pEvent);
			pEvent = m_pContentChangeNotifier->Pop();
		}
	}

	// Potentially dispatch the content change table.
	InternalDispatchContentChanges();

	// Prune specifics if at our time.
	{
		static const Double kfTempSuppressTimeInSeconds = 2.0;

		Vector<FilePath, MemoryBudgets::Content> vToRemove;
		{
			auto const iBegin = m_tHotLoadingSuppressSpecific.Begin();
			auto const iEnd = m_tHotLoadingSuppressSpecific.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - i->Second) > kfTempSuppressTimeInSeconds)
				{
					vToRemove.PushBack(i->First);
				}
			}
		}
		for (auto const& filePath : vToRemove)
		{
			m_tHotLoadingSuppressSpecific.Erase(filePath);
		}
	}
}

void LoadManager::InternalInsertEventAndDependents(ChangeEvent* pEvent)
{
	// If the file is in the suppresion set, ignore the event.
	if (m_tHotLoadingSuppressSpecific.HasValue(pEvent->m_Old) ||
		m_tHotLoadingSuppressSpecific.HasValue(pEvent->m_New))
	{
		return;
	}

	// We only keep this entry if the file is currently loaded, otherwise
	// we just dispose of it.
	if (Events::Manager::Get()->TriggerEvent(FileIsLoadedEventId, pEvent->m_New))
	{
		// Always use the latest event, so just overwrite an existing entry
		// with a new one.
		m_tContentChanges.Overwrite(pEvent->m_New, SharedPtr<ChangeEvent>(pEvent));
	}

	// Now get any dependents, generate new events, and insert those.
	DepVector vDependents;
	CookManager::Get()->GetDependents(pEvent->m_New, vDependents);

	// Also add in dynamic dependents.
	{
		DepVector v;
		{
			Lock lock(m_DependencyMutex);
			m_tDependentsTable.GetValue(pEvent->m_New, v);
		}
		vDependents.Append(v);
	}

	UInt32 const nDependents = vDependents.GetSize();
	for (UInt32 i = 0u; i < nDependents; ++i)
	{
		SharedPtr<ChangeEvent> pDependentEvent(SEOUL_NEW(MemoryBudgets::Content) ChangeEvent(
			vDependents[i],
			vDependents[i],
			pEvent->m_eEvent));
		InternalInsertEventAndDependents(pDependentEvent.GetPtr());
	}
}

/**
 * Dispatch file change events if the user has asked to do so, or clear
 * the pending change event table.
 */
void LoadManager::InternalDispatchContentChanges()
{
	// Track and (possibly) update the action.
	auto const eHotLoadMode = m_eHotLoadMode;

	// Unless permanent accept, reset mode to no action.
	if (LoadManagerHotLoadMode::kPermanentAccept != m_eHotLoadMode)
	{
		m_eHotLoadMode = LoadManagerHotLoadMode::kNoAction;
	}

	// Early out if no changes to process.
	if (m_tContentChanges.IsEmpty())
	{
		return;
	}

	// If told to hot load, trigger that now.
	if (LoadManagerHotLoadMode::kAccept == eHotLoadMode || LoadManagerHotLoadMode::kPermanentAccept == eHotLoadMode)
	{
		// TODO: Want to introduce a HotLoadingManager that formalizes
		// dependencies like this.
		Bool bSuppressUIMovieEvents = false;
		{
			auto const iBegin = m_tContentChanges.Begin();
			auto const iEnd = m_tContentChanges.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				// TODO: This is not a valid assumption in general.
				// It is specifically true for how we're currently using scripts,
				// but that could break.

				// Don't trigger hot loads of movies if a script change
				// has been triggered, since it will trigger a reload of the
				// UI.
				if (i->First.GetType() == FileType::kScript)
				{
					bSuppressUIMovieEvents = true;
					break;
				}
			}
		}

		// Trigger an event for each entry in the table, then clear the table.
		{
			auto const iBegin = m_tContentChanges.Begin();
			auto const iEnd = m_tContentChanges.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				if (!bSuppressUIMovieEvents || i->First.GetType() != FileType::kUIMovie)
				{
					Events::Manager::Get()->TriggerEvent(
						FileChangeEventId,
						i->Second.GetPtr());
				}
			}
		}
		m_tContentChanges.Clear();
	}
	// Otherwise, if instructed to cancel pending hot loads, do so now.
	else if (LoadManagerHotLoadMode::kReject == eHotLoadMode)
	{
		// Clear pending content changes.
		m_tContentChanges.Clear();
	}
}

#endif // /#if SEOUL_HOT_LOADING

} // namespace Content

} // namespace Seoul
