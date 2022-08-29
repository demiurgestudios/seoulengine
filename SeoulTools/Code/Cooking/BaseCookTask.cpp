/**
 * \file BaseCookTask.cpp
 * \brief Parent interface of any classes that handle cooker work.
 * All concrete instances that inherit from this interface will
 * be added to a Cooker instance and processed to handle cooking work.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "BaseCookTask.h"
#include "CookDatabase.h"
#include "Directory.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "JobsFunction.h"
#include "ICookContext.h"
#include "JobsManager.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "SccIClient.h"
#include "ScopedAction.h"
#include "SeoulProcess.h"

namespace Seoul
{

SEOUL_TYPE(Cooking::BaseCookTask);

namespace Cooking
{

/** Files in generated folders other than the current platform and local are excluded. */
static inline Bool IsExcluded(FilePath filePath, Platform ePlatform)
{
	// Check if the relative filename starts with Generated
	auto s = filePath.GetRelativeFilenameWithoutExtension().CStr();
	if (s == strstr(s, "Generated"))
	{
		// If the relative path starts with "Generated", the next chunk
		// must be the current platform string, or it is excluded.
		s += 9;

		if (0 != STRNCMP_CASE_INSENSITIVE(s, kaPlatformNames[(UInt32)ePlatform], strlen(kaPlatformNames[(UInt32)ePlatform])) &&
			0 != STRNCMP_CASE_INSENSITIVE(s, "Local", 5))
		{
			return true;
		}
	}

	return false;
}

BaseCookTask::BaseCookTask()
{
}

BaseCookTask::~BaseCookTask()
{
}

/**
 * Handles single file cooks. Caller is expected to have
 * used CanCook() to determine if this is the right task to
 * cook filePath or not.
 */
Bool BaseCookTask::CookSingle(ICookContext& rContext, FilePath filePath)
{
	// Single file cooks are meant for local on-demand cooking,
	// so they do not interact with source control nor do
	// they update the cook database.
	if (!InternalCook(rContext, filePath))
	{
		return false;
	}

	// Refresh cook database after cook.
	if (!PostCookUpdateMetadata(rContext, &filePath, &filePath + 1))
	{
		return false;
	}

	return true;
}

HString BaseCookTask::GetProgressType(const ICookContext& context) const
{
	// TODO: Cache.

	// Strip namespace, if present.
	auto const baseName = GetReflectionThis().GetType().GetName().CStr();
	auto name = strstr(baseName, "::");
	if (name)
	{
		name += 2;
	}
	else
	{
		name = baseName;
	}

	return HString(String::Printf("%s-%s",
		EnumToString<Platform>(context.GetPlatform()),
		name));
}

Bool BaseCookTask::GetSources(ICookContext& rContext, FilePath filePath, Sources& rv) const
{
	// Default is just the input itself.
	rv.Clear();
	rv.PushBack(CookSource{ filePath, false });
	return true;
}

Bool BaseCookTask::AtomicWriteFinalOutput(
	ICookContext& rContext,
	void const* p,
	UInt32 u,
	FilePath filePath) const
{
	auto const sOutputFilename(filePath.GetAbsoluteFilename());
	return AtomicWriteFinalOutput(rContext, p, u, sOutputFilename);
}

Bool BaseCookTask::AtomicWriteFinalOutput(
	ICookContext& rContext,
	void const* p,
	UInt32 u,
	const String& sOutputFilename) const
{
	// Generate a temporary file to store the existing file, if it exists.
	auto const sTemporaryOld(Path::GetTempFileAbsoluteFilename());
	auto const scoped(MakeScopedAction([]() {}, [&]()
	{
		(void)FileManager::Get()->Delete(sTemporaryOld);
	}));

	// Move old file - can fail if doesn't exist, or only exists in a read-only file system.
	(void)FileManager::Get()->Rename(sOutputFilename, sTemporaryOld);

	// Make sure the output directory exists.
	if (!FileManager::Get()->CreateDirPath(Path::GetDirectoryName(sOutputFilename)))
	{
		SEOUL_LOG_COOKING("%s: atomic write of final cook data filed, could not create dependent directories.", sOutputFilename.CStr());
		return false;
	}

	// Commit the data to the output file. On failure, attempt to restore the old file.
	if (!FileManager::Get()->WriteAll(sOutputFilename, p, u))
	{
		// Restore old - will fail if it didn't exist and wasn't made. This is ok.
		(void)FileManager::Get()->Delete(sOutputFilename);
		(void)FileManager::Get()->Rename(sTemporaryOld, sOutputFilename);

		// Warn and fail.
		SEOUL_LOG_COOKING("%s: atomic write of final cook data filed, could not write to final file location.", sOutputFilename.CStr());
		return false;
	}

	// Success otherwise.
	return true;
}

Bool BaseCookTask::AtomicWriteFinalOutput(
	ICookContext& rContext,
	const String& sTempFilename,
	const String& sOutputFilename) const
{
	auto const sTemporaryOld(Path::GetTempFileAbsoluteFilename());

	/* Move old file - can fail if doesn't exist, or only exists in a read-only file system. */
	(void)FileManager::Get()->Rename(sOutputFilename, sTemporaryOld);

	if (!FileManager::Get()->CreateDirPath(Path::GetDirectoryName(sOutputFilename)))
	{
		SEOUL_LOG_COOKING("%s: atomic write of final cook data filed, could not create dependent directories.", sOutputFilename.CStr());
		return false;
	}

	if (!FileManager::Get()->Rename(sTempFilename, sOutputFilename))
	{
		/* Restore old - will fail if it didn't exist and wasn't made. This is ok. */
		(void)FileManager::Get()->Rename(sTemporaryOld, sOutputFilename);
		(void)FileManager::Get()->Delete(sTempFilename);

		SEOUL_LOG_COOKING("%s: atomic write of final cook data filed, could not move final file from temp location \"%s\".", sOutputFilename.CStr(), sTempFilename.CStr());
		return false;
	}

	(void)FileManager::Get()->Delete(sTemporaryOld);
	return true;
}

// TODO: Bubble this out so it can happen once per run. This will require
// that the UICook adds any additional files to this list that it has generated
// as part of its cooking operation. In short:
// - Cooker generates this list if doing an out-of-date cook.
// - List is rearranged by file type.
// - Jobs use the list or their own processing as needed.
// - UI job needs to add to the list if it adds any files (provide an API
//   for this and use it to also update source control).

Bool BaseCookTask::GatherOutOfDateOfSourceType(
	ICookContext& rContext,
	FileType eType,
	ContentFiles& rv) const
{
	// Get desired files in the source directory.
	auto const& vFilePaths = rContext.GetSourceFilesOfType(eType);

	// Now check and accumulate.
	auto const ePlatform = rContext.GetPlatform();
	auto& db = rContext.GetDatabase();
	ContentFiles v;
	for (auto filePath : vFilePaths)
	{
		// Skip if excluded.
		if (IsExcluded(filePath, ePlatform))
		{
			continue;
		}

		// Update type - may be different from derived
		// for certain types (e.g. textures).
		filePath.SetType(eType);

		// Check if the file is out of date or not.
		if (db.CheckUpToDate(filePath))
		{
			continue;
		}

		// Append.
		v.PushBack(filePath);
	}

	// Done.
	rv.Swap(v);
	return true;
}

namespace
{

void LogError(Byte const* s)
{
	SEOUL_LOG_COOKING("%s", s);
}

} // namespace anonymous

Bool BaseCookTask::PostCookUpdateMetadata(
	ICookContext& rContext,
	FilePathIterator iBegin,
	FilePathIterator iEnd) const
{
	// Match the modification time, and update metadata.
	auto& db = rContext.GetDatabase();
	auto const ePlatform = rContext.GetPlatform();
	Sources vSources;
	for (auto i = iBegin; iEnd != i; ++i)
	{
		auto const& filePath = *i;
		// TODO: Don't special case like this.
		//
		// We don't want to match the modified time of the audio project - the project file in source
		// almost never changes (it is a placeholder file with essentially no data) but in cooked
		// output, it is essential and stores the event metadata for the entire project.
		UInt64 uModifiedTime = 0u;
		if (FileType::kSoundProject != filePath.GetType())
		{
			uModifiedTime = FileManager::Get()->GetModifiedTime(filePath.GetAbsoluteFilenameInSource());
			if (0u == uModifiedTime)
			{
				SEOUL_LOG_COOKING("Failed getting modification time for source of \"%s\"", filePath.CStr());
				return false;
			}

			if (!FileManager::Get()->SetModifiedTime(filePath, uModifiedTime))
			{
				SEOUL_LOG_COOKING("Failed updating modification time for cooked output of \"%s\"", filePath.CStr());
				return false;
			}
		}
		else
		{
			uModifiedTime = FileManager::Get()->GetModifiedTime(filePath);
			if (0u == uModifiedTime)
			{
				SEOUL_LOG_COOKING("Failed getting modification time for cooked version of \"%s\"", filePath.CStr());
				return false;
			}
		}

		// Make sure the cook database version of attributes is up-to-date
		// after the time stamp mutation.
		rContext.GetDatabase().ManualOnFileChange(filePath);

		// Acquire sources for metadata update.
		if (!GetSources(rContext, filePath, vSources))
		{
			SEOUL_LOG_COOKING("Failed acquiring sources for \"%s\"", filePath.CStr());
			return false;
		}

		// Sanity, verify that all sources/siblings exist/have a valid modified time stamp
		// prior to updating the metdata. If this fails, it is always a cooker bug.
		for (auto const& e : vSources)
		{
			if (e.m_bDirectory) { continue; }
			if (e.m_bSibling)
			{
				if (0 == FileManager::Get()->GetModifiedTimeForPlatform(ePlatform, e.m_FilePath))
				{
					SEOUL_LOG_COOKING("%s: cooker bug, sibling dependency \"%s\" was not generated.", filePath.CStr(), e.m_FilePath.CStr());
					return false;
				}
			}
			else
			{
				if (0 == FileManager::Get()->GetModifiedTimeInSource(e.m_FilePath))
				{
					SEOUL_LOG_COOKING("%s: cooker bug, source dependency \"%s\" does not exist.", filePath.CStr(), e.m_FilePath.CStr());
					return false;
				}
			}
		}

		db.UpdateMetadata(filePath, uModifiedTime, vSources.Begin(), vSources.End());
	}

	return true;
}

namespace
{

struct RunUtil SEOUL_SEALED
{
	SEOUL_DELEGATE_TARGET(RunUtil);

	void LogErrors()
	{
		Lock lock(m_Mutex);
		for (auto const& s : m_v)
		{
			LogError(s.CStr());
		}
	}

	void OnError(Byte const* s)
	{
		++m_ErrorLogCount;
		Lock lock(m_Mutex);
		m_v.PushBack(s);
	}

	Atomic32 m_ErrorLogCount;
	Mutex m_Mutex;
	Vector<String, MemoryBudgets::Cooking> m_v;
};

} // namespace anonymou

Bool BaseCookTask::RunCommandLineProcess(
	const String& sStartingDirectory,
	const String& sCommand,
	const ProcessArguments& vs,
	Bool bTreatAnyErrorOutputAsFailure /*= false*/,
	Bool bTreatStdOutAsErrors /*= false*/,
	Delegate<void(Byte const* s)> customStdOut /*= Delegate<void(Byte const* s)>()*/) const
{
	RunUtil util;
	Int32 iResult = -1;
	{
		auto const onError = SEOUL_BIND_DELEGATE(&RunUtil::OnError, &util);
		Process process(
			sStartingDirectory,
			sCommand,
			vs,
			customStdOut.IsValid() ? customStdOut : (bTreatStdOutAsErrors ? onError : Process::OutputDelegate()),
			onError);
		if (!process.Start())
		{
			SEOUL_LOG_COOKING("Failed starting process \"%s\"", sCommand.CStr());
			return false;
		}

		iResult = process.WaitUntilProcessIsNotRunning();
	}

	// Result handling.
	if (0 != iResult)
	{
		util.LogErrors();
		return false;
	}

	// Extra handling if specified.
	if (bTreatAnyErrorOutputAsFailure && util.m_ErrorLogCount > 0)
	{
		util.LogErrors();
		return false;
	}

	return true;
}

Bool BaseCookTask::DefaultOutOfDateCook(
	ICookContext& rContext,
	FileType eType,
	ContentFiles& rv,
	Bool bCanRunInParallel /*= false*/)
{
	// Gather files of the requested source type.
	if (!GatherOutOfDateOfSourceType(rContext, eType, rv))
	{
		return false;
	}

	// Handle no files to cook case.
	if (rv.IsEmpty())
	{
		return true;
	}

	// Timing for reporting.
	auto const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();

	// If enabled, run cooks in parallel.
	if (bCanRunInParallel)
	{
		ParallelUtil util(*this, rContext, rv);
		if (!util.Run(false /* bMulti */))
		{
			goto fail;
		}
	}
	else
	{
		// Now enumerate and cook.
		Int32 iComplete = 0;
		for (auto const& filePath : rv)
		{
			rContext.AdvanceProgress(
				GetProgressType(rContext),
				(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks),
				(Float)iComplete / (Float)rv.GetSize(),
				1u,
				(UInt32)((Int32)rv.GetSize() - iComplete));

			if (!InternalCook(rContext, filePath))
			{
				goto fail;
			}

			++iComplete;
		}
	}

	// Refresh cook database after cook.
	if (!PostCookUpdateMetadata(rContext, rv.Begin(), rv.End()))
	{
		goto fail;
	}

	rContext.CompleteProgress(
		GetProgressType(rContext),
		(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks),
		true);
	return true;

fail:
	rContext.CompleteProgress(
		GetProgressType(rContext),
		(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks),
		false);
	return false;
}

namespace
{

/**
 * Used to sort multi-ops - multi-ops submit more than one FilePath to
 * be cooked. They are for ops that can shared work across ops. The drawback
 * of a multi-op is that the individual ops are not parallelized.
 */
struct NameThenTypeSorter SEOUL_SEALED
{
	Bool operator()(const FilePath& a, const FilePath& b) const
	{
		if (a.GetRelativeFilenameWithoutExtension() == b.GetRelativeFilenameWithoutExtension())
		{
			return (a.GetType() < b.GetType());
		}
		else
		{
			return (strcmp(a.GetRelativeFilenameWithoutExtension().CStr(), b.GetRelativeFilenameWithoutExtension().CStr()) < 0);
		}
	}
};

} // namespace anonymous

Bool BaseCookTask::DefaultOutOfDateCookMulti(
	ICookContext& rContext,
	FileType eFirstType,
	FileType eLastType,
	ContentFiles& rv,
	Bool bCanRunInParallel /*= false*/)
{
	// Gather files of the requested source type.
	for (Int i = (Int)eFirstType; i <= (Int)eLastType; ++i)
	{
		ContentFiles v;
		if (!GatherOutOfDateOfSourceType(rContext, (FileType)i, v))
		{
			return false;
		}
		rv.Append(v);
	}
	// Sorted by name, followed by type.
	{
		NameThenTypeSorter sorter;
		QuickSort(rv.Begin(), rv.End(), sorter);
	}

	// Handle no files to cook case.
	if (rv.IsEmpty())
	{
		return true;
	}

	// Timing for reporting.
	auto const iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();

	// If enabled, run cooks in parallel.
	if (bCanRunInParallel)
	{
		ParallelUtil util(*this, rContext, rv);
		if (!util.Run(true /* bMulti */))
		{
			goto fail;
		}
	}
	else
	{
		// Progress.
		Int32 iComplete = 0;

		// Now enumerate and cook.
		auto const uSize = rv.GetSize();
		for (auto i = 0u; i < uSize; ++i)
		{
			auto const uStart = i;
			auto const filePath = rv[uStart];

			// Gather FilePaths with the same root name, different type, into
			// a single multi op.
			UInt32 uCount = 1u;
			while (i + 1 < uSize && rv[i + 1].GetRelativeFilenameWithoutExtension() == filePath.GetRelativeFilenameWithoutExtension())
			{
				++uCount;
				++i;
			}

			rContext.AdvanceProgress(
				GetProgressType(rContext),
				(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks),
				(Float)iComplete / (Float)uSize,
				uCount,
				(UInt32)((Int32)uSize - iComplete));

			// Single operation for this pass, perform normally.
			if (uCount <= 1u)
			{
				if (!InternalCook(rContext, filePath))
				{
					goto fail;
				}
			}
			// Otherwise, perform as a multi.
			else
			{
				if (!InternalCookMulti(rContext, rv.Data() + uStart, rv.Data() + uStart + uCount))
				{
					goto fail;
				}
			}

			iComplete += (Int32)uCount;
		}
	}

	// Refresh cook database after cook.
	if (!PostCookUpdateMetadata(rContext, rv.Begin(), rv.End()))
	{
		goto fail;
	}

	rContext.CompleteProgress(
		GetProgressType(rContext),
		(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks),
		true);
	return true;

fail:
	rContext.CompleteProgress(
		GetProgressType(rContext),
		(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - iStartTimeInTicks),
		false);
	return false;
}

Bool BaseCookTask::ParallelUtil::Run(Bool bMulti)
{
	auto const total = (Atomic32Type)m_rv.GetSize();

	// Tracking.
	m_Started.Reset();
	m_Finished.Reset();
	// Allocate for results.
	m_vbResults.Resize((UInt32)total);
	m_vbResults.Fill(false);

	// Make sure output directories exist - can fight otherwise if this happened
	// during threaded portion.
	auto const ePlatform = m_rContext.GetPlatform();
	for (auto const& e : m_rv)
	{
		if (!Directory::CreateDirPath(Path::GetDirectoryName(e.GetAbsoluteFilenameForPlatform(ePlatform))))
		{
			SEOUL_LOG_COOKING("%s: failed creating output directory.", e.CStr());
			return false;
		}
	}

	// Allocate tasks - 1:1 if not multi.
	m_vTasks.Reserve(total);
	m_vTasks.Clear();
	if (bMulti)
	{
		for (Atomic32Type i = 0; i < total; ++i)
		{
			// Multi, gather all ops with the same base relative filename.
			auto const iStart = i;
			Atomic32Type count = 1;
			while (i + 1 < total && m_rv[i + 1].GetRelativeFilenameWithoutExtension() == m_rv[i].GetRelativeFilenameWithoutExtension())
			{
				++count;
				++i;
			}

			TaskEntry task;
			task.m_Count = count;
			task.m_Index = iStart;
			m_vTasks.PushBack(task);
		}
	}
	else
	{
		for (Atomic32Type i = 0; i < total; ++i)
		{
			TaskEntry task;
			task.m_Count = 1;
			task.m_Index = i;
			m_vTasks.PushBack(task);
		}
	}

	// Common now that tasks are allocated.
	return RunTasks();
}

Bool BaseCookTask::ParallelUtil::RunTasks()
{
	// Start time tracking.
	m_iStartTimeInTicks = SeoulTime::GetGameTimeInTicks();

	// Create work jobs for each Jobs::Manager thread.
	auto const jobs = (Atomic32Type)Jobs::Manager::Get()->GetThreadCount();
	Vector<SharedPtr<Jobs::Job>, MemoryBudgets::Cooking> vJobs;
	vJobs.Reserve(jobs);

	// Kick jobs.
	for (Atomic32Type i = 0; i < jobs; ++i)
	{
		vJobs.PushBack(Jobs::MakeFunction(
			SEOUL_BIND_DELEGATE(&BaseCookTask::ParallelUtil::DoWork, this),
			false));
	}
	for (auto const& pJob : vJobs)
	{
		pJob->StartJob();
	}
	// Do work on dispatch thread as well.
	DoWork(true);
	// Wait for jobs to complete.
	for (auto const& pJob : vJobs)
	{
		pJob->WaitUntilJobIsNotRunning();
	}
	// Done.
	vJobs.Clear();
	// Sanity.
	SEOUL_ASSERT(m_Started >= m_Finished);
	SEOUL_ASSERT(m_Finished == m_vTasks.GetSize());
	// Now check results.
	auto const uTotal = m_vbResults.GetSize();
	for (UInt32 i = 0u; i < uTotal; ++i)
	{
		// Failure, fail immediately.
		if (!m_vbResults[i])
		{
			return false;
		}
	}

	// Done, success.
	return true;
}

void BaseCookTask::ParallelUtil::DoWork(Bool bDispatcherThread)
{
	// Utility for updating displayed progress. Called only
	// if bDispatcherThread is true.
	Atomic32Type progress = 0;
	auto const updateProgress = [&]()
	{
		UInt32 const uActive = (UInt32)Max((Int32)0, Min((Int32)m_Started, (Int32)m_vTasks.GetSize()) - (Int32)m_Finished);
		m_rContext.AdvanceProgress(
			m_r.GetProgressType(m_rContext),
			(Float)SeoulTime::ConvertTicksToSeconds(SeoulTime::GetGameTimeInTicks() - m_iStartTimeInTicks),
			(Float)m_Finished / (Float)m_vTasks.GetSize(),
			uActive,
			(UInt32)((Int32)m_vTasks.GetSize() - (Int32)m_Finished));
	};

	// Init progress if we're the dispatcher thread.
	if (bDispatcherThread)
	{
		updateProgress();
	}

	Atomic32Type const total = (Atomic32Type)m_vTasks.GetSize();
	while (true)
	{
		// Acquire a task slot.
		auto const taskIndex = (++m_Started) - 1;
		// If we're out of range, done.
		if (taskIndex >= total)
		{
			return;
		}

		// Run the task.
		auto const& task = m_vTasks[taskIndex];
		Bool bSuccess = false;
		{
			// More than one, this is a multi-op cook.
			if (task.m_Count > 1)
			{
				bSuccess = m_r.InternalCookMulti(m_rContext, m_rv.Data() + task.m_Index, m_rv.Data() + task.m_Index + task.m_Count);
			}
			else
			{
				bSuccess = m_r.InternalCook(m_rContext, m_rv[task.m_Index]);
			}
		}

		// Apply results.
		for (auto i = task.m_Index; i < task.m_Index + task.m_Count; ++i)
		{
			m_vbResults[i] = bSuccess;
		}

		// Marks completion of the run - must always be incremented.
		++m_Finished;
		// Advance progress if we're the dispatcher thread.
		if (bDispatcherThread)
		{
			updateProgress();
		}
	}

	// Finalize.
	if (bDispatcherThread)
	{
		while (m_Finished < total)
		{
			updateProgress();
			Jobs::Manager::Get()->YieldThreadTime();
		}
	}
}

} // namespace Cooking

} // namespace Seoul
