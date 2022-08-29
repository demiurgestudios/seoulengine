/**
 * \file ContentStore.cpp
 * \brief Content::Store<> is a generic class that can be used to
 * manage content which is loaded from persistent media. It handles
 * unloading the content when all references to the content have been
 * released, as well as providing Getters() to acquire content by
 * key, and trigger loads when content has been requested for the
 * first time.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentLoadManager.h"
#include "ContentStore.h"
#include "EventsManager.h"
#include "JobsFunction.h"

namespace Seoul::Content
{

/** Delegate entry. */
static void StoreSendFileLoadCompleteEvent(FilePath filePath)
{
	// If Content::LoadManager still exists, dispatch the file load complete
	// callback for the loaded file.
	if (LoadManager::Get())
	{
		if (filePath.IsValid())
		{
			Events::Manager::Get()->TriggerEvent(FileLoadCompleteEventId, filePath);
		}
	}
}

/** Shared common. */
void StoreCompleteSyncLoad(const FilePath& filePath)
{
	// Need to deliver the loaded event in place of the Content::LoaderBase system.
	Jobs::AsyncFunction(
		GetMainThreadId(),
		&StoreSendFileLoadCompleteEvent,
		filePath);
}

BaseStore::BaseStore(Bool bUnloadAll)
	: m_bUnloadAll(bUnloadAll)
{
	SEOUL_ASSERT(IsMainThread());

	// Register for appropriate callbacks with ContentLoadManager.
	Events::Manager::Get()->RegisterCallback(
		UnloadEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallUnload, this));

	Events::Manager::Get()->RegisterCallback(
		ReloadEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallReload, this));

	Events::Manager::Get()->RegisterCallback(
		PollEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallPoll, this));

	Events::Manager::Get()->RegisterCallback(
		FileChangeEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallFileChange, this));

	Events::Manager::Get()->RegisterCallback(
		FileIsLoadedEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallIsFileLoaded, this));
}

BaseStore::~BaseStore()
{
	SEOUL_ASSERT(IsMainThread());

	Events::Manager::Get()->UnregisterCallback(
		FileIsLoadedEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallIsFileLoaded, this));

	Events::Manager::Get()->UnregisterCallback(
		FileChangeEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallFileChange, this));

	Events::Manager::Get()->UnregisterCallback(
		PollEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallPoll, this));

	Events::Manager::Get()->UnregisterCallback(
		ReloadEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallReload, this));

	Events::Manager::Get()->UnregisterCallback(
		UnloadEventId,
		SEOUL_BIND_DELEGATE(&BaseStore::CallUnload, this));
}

/**
 * Invokes FileChange() - this function exists to wrap FileChange() in a signature compatible
 * with Events::Manager(), and so that we can hook up the callback in Content::LoaderBase,
 * even though FileChange() is virtual.
 */
Bool BaseStore::CallFileChange(ChangeEvent* pFileChangeEvent)
{
	return FileChange(pFileChangeEvent);
}

/**
 * Invokes IsFileLoaded() - this function exists to wrap IsFileLoaded() in a signature compatible
 * with Events::Manager(), and so that we can hook up the callback in Content::LoaderBase,
 * even though IsFileLoaded() is virtual.
 */
Bool BaseStore::CallIsFileLoaded(FilePath filePath)
{
	return IsFileLoaded(filePath);
}

/**
 * Invokes Unload or UnloadLRU() depending on configuration.
 */
void BaseStore::CallPoll()
{
	if (m_bUnloadAll)
	{
		(void)Unload();
	}
	else
	{
		(void)UnloadLRU();
	}
}

/**
 * Invokes Reload() - this function exists to wrap Reload() in a signature compatible
 * with Events::Manager(), and so that we can hook up the callback in Content::LoaderBase,
 * even though Reload() is virtual.
 */
void BaseStore::CallReload(Content::Reload* pContentReload)
{
	Reload(*pContentReload);
}

/**
 * Invokes Unload() - this function exists to wrap Unload() in a signature compatible
 * with Events::Manager(), and so that we can hook up the callback in Content::LoaderBase,
 * even though Unload() is virtual.
 */
void BaseStore::CallUnload(UInt32* puTotalRemaining)
{
	auto const uRemaining = Unload();
	if (nullptr != puTotalRemaining)
	{
		*puTotalRemaining += uRemaining;
	}
}

} // namespace Seoul::Content
