/**
 * \file ContentChangeNotifierLocal.cpp
 * \brief Specialization of Content::ChangeNotifier for monitoring local files. Uses
 * FileChangeNotifier to detect file changes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "HashTable.h"
#include "Path.h"
#include "ContentChangeNotifierLocal.h"
#include "Thread.h"

namespace Seoul::Content
{

ChangeNotifierLocal::ChangeNotifierLocal()
	: m_pThread(nullptr)
	, m_WorkerThreadSignal()
	, m_pConfigFileChangeNotifier(nullptr)
	, m_pSourceFileChangeNotifier(nullptr)
	, m_IncomingContentChanges()
	, m_bShuttingDown(false)
{
	m_pThread.Reset(SEOUL_NEW(MemoryBudgets::Content) Thread(
		SEOUL_BIND_DELEGATE(&ChangeNotifierLocal::ProcessIncomingChanges, this),
		true));

	m_pConfigFileChangeNotifier.Reset(SEOUL_NEW(MemoryBudgets::Content) FileChangeNotifier(
		GamePaths::Get()->GetConfigDir(),
		SEOUL_BIND_DELEGATE(&ChangeNotifierLocal::OnConfigFileChanged, this),
		FileChangeNotifier::kChangeFileName |
		FileChangeNotifier::kChangeSize |
		FileChangeNotifier::kChangeLastWrite |
		FileChangeNotifier::kChangeCreation));

	// NOTE: We only monitor Authored\\ because all the generated images by design,
	// never change (either new files are added, or existing files are reused). If that changes,
	// this should also change.
	m_pSourceFileChangeNotifier.Reset(SEOUL_NEW(MemoryBudgets::Content) FileChangeNotifier(
		Path::Combine(GamePaths::Get()->GetSourceDir(), "Authored\\"),
		SEOUL_BIND_DELEGATE(&ChangeNotifierLocal::OnSourceFileChanged, this),
		FileChangeNotifier::kChangeFileName |
		FileChangeNotifier::kChangeSize |
		FileChangeNotifier::kChangeLastWrite |
		FileChangeNotifier::kChangeCreation));
}

ChangeNotifierLocal::~ChangeNotifierLocal()
{
	// Indicate that we're shutting down and wake up the worker thread.
	m_bShuttingDown = true;
	m_WorkerThreadSignal.Activate();

	// Destroy the file change notifier and the worker thread.
	m_pSourceFileChangeNotifier.Reset();
	m_pConfigFileChangeNotifier.Reset();
	m_pThread.Reset();

	// Destroy any remaining entries in the incoming changes queue.
	ChangeEvent* pEvent = m_IncomingContentChanges.Pop();
	while (nullptr != pEvent)
	{
		SeoulGlobalDecrementReferenceCount(pEvent);

		pEvent = m_IncomingContentChanges.Pop();
	}
}

void ChangeNotifierLocal::HandleOnFileChanged(
	FilePath filePathOld,
	FilePath filePathNew,
	FileChangeNotifier::FileEvent eEvent)
{
	// If both paths are valid, instantiate a new content change event and insert it
	// into the incoming queue, to be processed by our worker thread.
	if (filePathNew.IsValid())
	{
		ChangeEvent* pContentChangeEvent = SEOUL_NEW(MemoryBudgets::Content) ChangeEvent(
			filePathOld,
			filePathNew,
			eEvent);
		SeoulGlobalIncrementReferenceCount(pContentChangeEvent);

		// If we've run out of queue, give the worker thread some time to process events.
		m_IncomingContentChanges.Push(pContentChangeEvent);

		// Tell the worker thread it has work to do.
		m_WorkerThreadSignal.Activate();
	}
}

/**
 * Callback hook for FileChangeNotifier, called
 * by FileChangeNotifier on a file change event.
 */
void ChangeNotifierLocal::OnFileChanged(
	Bool bConfigFile,
	const String& sOldPath,
	const String& sNewPath,
	FileChangeNotifier::FileEvent eEvent)
{
	// Convert the old and new paths to paths, based on the game directory.
	FilePath filePathOld = (bConfigFile ? FilePath::CreateConfigFilePath(sOldPath) : FilePath::CreateContentFilePath(sOldPath));
	FilePath filePathNew = (bConfigFile ? FilePath::CreateConfigFilePath(sNewPath) : FilePath::CreateContentFilePath(sNewPath));

	// Special handling for some one-to-many types.
	if (IsTextureFileType(filePathNew.GetType()))
	{
		filePathNew.SetType(FileType::kTexture0);
		HandleOnFileChanged(filePathOld, filePathNew, eEvent);
		filePathNew.SetType(FileType::kTexture1);
		HandleOnFileChanged(filePathOld, filePathNew, eEvent);
		filePathNew.SetType(FileType::kTexture2);
		HandleOnFileChanged(filePathOld, filePathNew, eEvent);
		filePathNew.SetType(FileType::kTexture3);
		HandleOnFileChanged(filePathOld, filePathNew, eEvent);
		filePathNew.SetType(FileType::kTexture4);
		HandleOnFileChanged(filePathOld, filePathNew, eEvent);
	}
	// Otherwise, dispatch normally.
	else
	{
		HandleOnFileChanged(filePathOld, filePathNew, eEvent);
	}
}

/**
 * Worker thread, processes incoming events:
 * - filters redundant events (redundant define as events targeting the same file
 * - checks that a file can be read before inserting the event into the output queue.
 */
Int ChangeNotifierLocal::ProcessIncomingChanges(const Thread& thread)
{
	static const Double kfQuietPeriodInSeconds = 0.1;

	typedef HashTable<FilePath, ChangeEvent*, MemoryBudgets::Content> Changes;
	Changes tChanges;
	typedef Vector<FilePath, MemoryBudgets::Content> ChangesToRemove;
	ChangesToRemove vChangesToRemove;

	Bool bTouched = false;

	// Keep polling until the class is being destroyed.
	while (!m_bShuttingDown)
	{
		// If we didn't process anything the last loop around, wait until we're
		// signaled to do work.
		if (!bTouched)
		{
			m_WorkerThreadSignal.Wait();
		}

		bTouched = false;

		// For each event in the queue, process redundant events.
		ChangeEvent* p = m_IncomingContentChanges.Pop();
		while (nullptr != p)
		{
			bTouched = true;

			// If there already exists any entry for the target file,
			// merge this entry into it.
			ChangeEvent* pContentChangeEvent = nullptr;
			if (tChanges.GetValue(p->m_New, pContentChangeEvent))
			{
				// Use the most recent event time.
				pContentChangeEvent->m_iCurrentTimeInTicks = Max(pContentChangeEvent->m_iCurrentTimeInTicks, p->m_iCurrentTimeInTicks);

				// If the new entry is a rename entry, use the most recent
				// old name, and set the event to rename.
				if (p->m_eEvent == FileChangeNotifier::kRenamed)
				{
					pContentChangeEvent->m_eEvent = FileChangeNotifier::kRenamed;
					pContentChangeEvent->m_Old = p->m_Old;
				}

				// Destroy the new event, we've merged its contribution into the existing instance.
				SeoulGlobalDecrementReferenceCount(p);
			}
			// If there isn't an existing entry, just insert the incoming entry.
			else
			{
				tChanges.Insert(p->m_New, p);
			}

			// Get the next incoming content change.
			p = m_IncomingContentChanges.Pop();
		}

		// Now process the entries table - for each entry, check if
		// the file can be opened - if this succeeds, push the entry onto the outgoing entries
		// queue.
		{
			auto const iBegin = tChanges.Begin();
			auto const iEnd = tChanges.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				// We've done work for this loop, so immediately try to do work
				// the next time around.
				bTouched = true;

				// Avoid processing for a time - some of our content creation tools
				// appear to create a file, and then open it again to populate it,
				// and we can get in the way of that if we try to open it/check it too quickly.
				if (SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - i->Second->m_iCurrentTimeInTicks) < kfQuietPeriodInSeconds)
				{
					continue;
				}

				// For content, we're monitoring the source folder. Otherwise we're just
				// monitoring the game directory.
				String sAbsoluteFilename = (i->Second->m_New.GetDirectory() == GameDirectory::kContent
					? i->Second->m_New.GetAbsoluteFilenameInSource()
					: i->Second->m_New.GetAbsoluteFilename());

				// Try to move the file to itself - if this succeeds,
				// treat the file as readable. TODO: Verify that this
				// is ok on all platforms - so far, it has proven to be the
				// most reliable method to verify that an external tool is actually
				// done with a file before we try to reload it.
				if (DiskSyncFile::RenameFile(sAbsoluteFilename, sAbsoluteFilename))
				{
					ChangeEvent* p = i->Second;
					vChangesToRemove.PushBack(i->First);

					m_OutgoingContentChanges.Push(p);
				}
				else if (
					FileManager::Get()->IsDirectory(i->Second->m_New) ||
					!FileManager::Get()->Exists(i->Second->m_New))
				{
					// If we failed to open it because it's a directory or because the file was deleted, skip it
					// and don't try to keep re-opening it
					ChangeEvent* p = i->Second;
					vChangesToRemove.PushBack(i->First);
					SeoulGlobalDecrementReferenceCount(p);
				}
			}
		}

		UInt32 const uChangesToRemove = vChangesToRemove.GetSize();
		for (UInt32 i = 0u; i < uChangesToRemove; ++i)
		{
			(void)tChanges.Erase(vChangesToRemove[i]);
		}
		vChangesToRemove.Clear();
	}

	for (auto i = tChanges.Begin();
		tChanges.End() != i;
		++i)
	{
		SeoulGlobalDecrementReferenceCount(i->Second);
	}

	return 0;
}

} // namespace Seoul::Content
