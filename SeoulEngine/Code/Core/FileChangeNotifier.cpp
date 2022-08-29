/**
 * \file FileChangeNotifier.cpp
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

#include "DiskFileSystem.h"
#include "FileChangeNotifier.h"
#include "Path.h"
#include "Platform.h"
#include "Prereqs.h"
#include "ScopedAction.h"
#include "Thread.h"

namespace Seoul
{

static const UInt32 kBufferSize = 64 * 1024; // TODO: Guess.

FileChangeNotifier::FileChangeNotifier(
	const String& sPath,
	const Callback& callback,
	UInt32 uFlags /* = kAll */,
	Bool bMonitorRecursive /* = true*/)
	: m_hDirectoryHandle()
	, m_vBuffer(kBufferSize, 0)
	, m_pThread()
	, m_sPath(sPath)
	, m_Callback(callback)
#if SEOUL_PLATFORM_WINDOWS
	, m_uFlags(uFlags)
	, m_bMonitorRecursive(bMonitorRecursive)
#endif // /#if SEOUL_PLATFORM_WINDOWS
	, m_bShuttingDown(false)
{
#if SEOUL_PLATFORM_WINDOWS
	auto hDirectoryHandle = ::CreateFileW(
		m_sPath.WStr(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr);

	// If the handle is valid, start monitoring the folder.
	if (nullptr != hDirectoryHandle && INVALID_HANDLE_VALUE != hDirectoryHandle)
	{
		m_hDirectoryHandle = hDirectoryHandle;
		m_pThread.Reset(SEOUL_NEW(MemoryBudgets::Developer) Thread(SEOUL_BIND_DELEGATE(&FileChangeNotifier::InternalWorkerBody, this)));
		m_pThread->Start("Seoul FileChangeNotifier Thread: " + sPath);  // FIXME: For some reason, this always gets truncated at 31 characters, even though some other threads (e.g. FMOD's) have names longer than that
	}
#endif // /SEOUL_PLATFORM_WINDOWS
}

FileChangeNotifier::~FileChangeNotifier()
{
	// Tell the worker thread it's time to shutdown.
	m_bShuttingDown = true;

	// Cancel the IO and wait for the thread to terminate.
#if SEOUL_PLATFORM_WINDOWS
	auto hDirectoryHandle = StaticCast<HANDLE>(m_hDirectoryHandle);
	m_hDirectoryHandle.Reset();

	if (nullptr != hDirectoryHandle && INVALID_HANDLE_VALUE != hDirectoryHandle)
	{
		// Kill IO on the thread.
		if (m_pThread.IsValid())
		{
			// If the current platform doesn't support IO cancellation,
			// then do the roundabout thing and "poke" the directory
			// to wake up the reader thread.
			if (!m_pThread->CancelSynchronousIO())
			{
				auto sDummy(Path::Combine(m_sPath, "wakeup.txt"));
				UInt32 uDummy = 0u;
				while (DiskSyncFile::FileExists(sDummy))
				{
					sDummy = Path::Combine(m_sPath, String::Printf("wakeup%u.txt", uDummy));
					++uDummy;
				}
				(void)DiskSyncFile::WriteAll(sDummy, "", 0u);
				(void)DiskSyncFile::DeleteFile(sDummy);
			}
		}

		// Wait for the worker thread to finish up.
		m_pThread.Reset();

		// Close the directory handle - this will also "wak
		SEOUL_VERIFY(FALSE != ::CloseHandle(hDirectoryHandle));
	}
#endif
}

#if SEOUL_PLATFORM_WINDOWS
/**
 * @return Win32 FILE_NOTIFY_* flags from Seoul::FileChangeNotifier::Flags.
 */
inline DWORD ToWin32Filters(UInt32 uFlags)
{
	DWORD uReturn = 0u;
	uReturn |= (0u != (FileChangeNotifier::kChangeFileName & uFlags)) ? FILE_NOTIFY_CHANGE_FILE_NAME : 0u;
	uReturn |= (0u != (FileChangeNotifier::kChangeDirectoryName & uFlags)) ? FILE_NOTIFY_CHANGE_DIR_NAME : 0u;
	uReturn |= (0u != (FileChangeNotifier::kChangeAttributes & uFlags)) ? FILE_NOTIFY_CHANGE_ATTRIBUTES : 0u;
	uReturn |= (0u != (FileChangeNotifier::kChangeSize & uFlags)) ? FILE_NOTIFY_CHANGE_SIZE : 0u;
	uReturn |= (0u != (FileChangeNotifier::kChangeLastWrite & uFlags)) ? FILE_NOTIFY_CHANGE_LAST_WRITE : 0u;
	uReturn |= (0u != (FileChangeNotifier::kChangeLastAccess & uFlags)) ? FILE_NOTIFY_CHANGE_LAST_ACCESS : 0u;
	uReturn |= (0u != (FileChangeNotifier::kChangeCreation & uFlags)) ? FILE_NOTIFY_CHANGE_CREATION : 0u;
	return uReturn;
}

/**
 * @return A Seoul::FileChangeNotifier::FileEvent enum value from a Win32
 * FILE_ACTION_* enum value.
 */
inline FileChangeNotifier::FileEvent FromWin32Event(DWORD uEvent)
{
	switch (uEvent)
	{
	case FILE_ACTION_ADDED: return FileChangeNotifier::kAdded;
	case FILE_ACTION_REMOVED: return FileChangeNotifier::kRemoved;
	case FILE_ACTION_MODIFIED: return FileChangeNotifier::kModified;
	case FILE_ACTION_RENAMED_NEW_NAME: return FileChangeNotifier::kRenamed;
	default:
		return FileChangeNotifier::kUnknown;
	};
}
#endif

/**
 * Workhorse of FileChangeNotifier, runs on a worker thread, monitoring the target
 * folder fo file change events.
 */
Int FileChangeNotifier::InternalWorkerBody(const Thread& thread)
{
	// Windows implementation.
#if SEOUL_PLATFORM_WINDOWS
	// Keep running until the destructor indicates it's time to shutdown.
	while (!m_bShuttingDown)
	{
		// If we got a valid read directory results, process it. Valid results happen
		// if we get a non-FALSE value back from the function, or we get a FALSE
		// value but the last error is ERROR_IO_PENDING (which means the results
		// are being processed asynchronously.
		DWORD zBytesRead = 0u;
		if (FALSE != ::ReadDirectoryChangesW(
			StaticCast<HANDLE>(m_hDirectoryHandle),
			m_vBuffer.Data(),
			m_vBuffer.GetSizeInBytes(),
			(m_bMonitorRecursive ? TRUE : FALSE),
			ToWin32Filters(m_uFlags),
			&zBytesRead,
			nullptr,
			nullptr))
		{
			// Don't process if shutting down.
			if (!m_bShuttingDown)
			{
				String sPrevious;

				// Loop until we hit the end of the buffer - the structure
				// is a FILE_NOTIFY_INFORMATION struct followed by an arbitrary
				// number of bytes for filenames.
				FILE_NOTIFY_INFORMATION* p = (FILE_NOTIFY_INFORMATION*)m_vBuffer.Data();
				while (true)
				{
					// Convert the action into a FileEvent enum value.
					FileEvent const eEvent = FromWin32Event(p->Action);

					// Convert the UTF16 string to Seoul::String - it is not nullptr terminated,
					// so we need to do it character by character.
					String s;
					s.Reserve(p->FileNameLength * 4u);
					for (UInt32 i = 0u; i < (p->FileNameLength / sizeof(WCHAR)); ++i)
					{
						s.Append(p->FileName[i]);
					}

					s = Path::Combine(m_sPath, s);

					// If we have a valid event, dispatch it now. Otherwise, cache
					// the string in sPrevious - likely, this is a rename "old name" event,
					// and we're waiting until the next event to dispatch it (which will be
					// a rename "new name" event.
					if (kUnknown != eEvent)
					{
						// If this is a rename event, pass the previous name as the old name
						// parameter.
						if (kRenamed == eEvent)
						{
							m_Callback(sPrevious, s, eEvent);
						}
						// Otherwise, pass the same name for both parameters.
						else
						{
							m_Callback(s, s, eEvent);
						}
					}

					// Cache the current name in the previous entry, may be used for
					// rename events.
					sPrevious = s;

					// If NextEntryOffset is 0, the list is done - this is equivalent
					// to a nullptr terminator.
					if (0 == p->NextEntryOffset)
					{
						break;
					}
					// Non-zero values for NextEntryOffset specify the offset, in bytes,
					// from the current entry to the next.
					else
					{
						p = (FILE_NOTIFY_INFORMATION*)(((Byte*)p) + p->NextEntryOffset);
					}
				}
			}
		}
	}
#endif

	return 0;
}

} // namespace Seoul
