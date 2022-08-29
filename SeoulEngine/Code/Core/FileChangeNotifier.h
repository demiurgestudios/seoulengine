/**
 * \file FileChangeNotifier.h
 * \brief Instances of this class monitor a specified absolute directory path for
 * changes and then dispatch those changes to the registered delegate. Can be used
 * to implement hot loading or other functionality that depends on reacting
 * to file modification events.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FILE_CHANGE_NOTIFIER_H
#define FILE_CHANGE_NOTIFIER_H

#include "Atomic32.h"
#include "Delegate.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "SeoulString.h"
#include "UnsafeHandle.h"
#include "Vector.h"

namespace Seoul
{

// Forward declarations
class Thread;

/**
 * Triggers a callback on changes to files, within the specified set of events
 * to listen for, within the specified directory.
 *
 * This class makes no guarantees about the frequency of change events - it is
 * possible (and likely) that you will receive multiple "modify" events, for example,
 * for the same file.
 */
class FileChangeNotifier SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(FileChangeNotifier);

	/**
	 * Flags used to specify what events to listen for.
	 */
	enum Flags
	{
		/** No events. */
		kNone = 0u,

		/** File rename events will trigger the callback. */
		kChangeFileName = (1u << 0u),

		/** Directory rename events will trigger the callback. */
		kChangeDirectoryName = (1u << 1u),

		/** Attribute change events (i.e. read-only) will trigger the callback. */
		kChangeAttributes = (1u << 2u),

		/** Changes to a file's size will trigger the callback. */
		kChangeSize = (1u << 3u),

		/** Changes to the last write time of a file will trigger the callback. */
		kChangeLastWrite = (1u << 4u),

		/** Changes to the last access time of a file will trigger the callback. */
		kChangeLastAccess = (1u << 5u),

		/** Changes to the creation time of a file will trigger the callback. */
		kChangeCreation = (1u << 6u),

		/** Use this flag to specify that you want to listen for all events. */
		kAll = 0xFFFFFFFF,
	};

	/**
	 * Event types that will be passed to the callback.
	 */
	enum FileEvent
	{
		/** Invalid event, you will never receive this, it is only used internally. */
		kUnknown = 0,

		/** File was added, both sOldPath and sNewPath will refere to the same path. */
		kAdded,

		/** File was deleted, both sOldPath and sNewPath will refere to the same path. */
		kRemoved,

		/** File was changed, both sOldPath and sNewPath will refere to the same path. */
		kModified,

		/** File was renamed, sOldPath will refer to the previous filename, sNewPath will refer to the new filename. */
		kRenamed,
	};

	/** Signature of the delegate that will be invoked on events. */
	typedef Delegate<void(const String& sOldPath, const String& sNewPath, FileEvent eEvent)> Callback;

	FileChangeNotifier(
		const String& sPath,
		const Callback& callback,
		UInt32 uFlags = kAll,
		Bool bMonitorRecursive = true);
	~FileChangeNotifier();

private:
	UnsafeHandle m_hDirectoryHandle;
	Vector<Byte, MemoryBudgets::Developer> m_vBuffer;
	ScopedPtr<Thread> m_pThread;
	String m_sPath;
	Callback m_Callback;
#if SEOUL_PLATFORM_WINDOWS
	UInt32 const m_uFlags;
	Bool const m_bMonitorRecursive;
#endif // /#if SEOUL_PLATFORM_WINDOWS
	Atomic32Value<Bool> m_bShuttingDown;

	Int InternalWorkerBody(const Thread& thread);

	SEOUL_DISABLE_COPY(FileChangeNotifier);
}; // class FileChangeNotifier

} // namespace Seoul

#endif // include guard
