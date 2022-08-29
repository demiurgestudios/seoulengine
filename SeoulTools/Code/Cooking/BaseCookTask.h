/**
 * \file BaseCookTask.h
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

#pragma once
#ifndef BASE_COOK_TASK_H
#define BASE_COOK_TASK_H

#include "Delegate.h"
#include "FilePath.h"
#include "Prereqs.h"
#include "ReflectionDeclare.h"
#include "Vector.h"
namespace Seoul { namespace Cooking { class ICookContext; } }
namespace Seoul { struct CookSource; }

namespace Seoul::Cooking
{

class BaseCookTask SEOUL_ABSTRACT
{
public:
	SEOUL_REFLECTION_POLYMORPHIC_BASE(BaseCookTask);

	typedef Vector<FilePath, MemoryBudgets::Cooking> ContentFiles;
	typedef FilePath const* FilePathIterator;

	virtual ~BaseCookTask();

	// Implemented per task, returns true if a single file cook
	// can be handled by the given task.
	//
	// Default implementation as some tasks cannot do single cooks.
	virtual Bool CanCook(FilePath filePath) const
	{
		return false;
	}

	// Called by a cooker to run this task's job when
	// cooking all out of date files (vs. a single
	// known out-of-date target file).
	virtual Bool CookAllOutOfDateContent(ICookContext& rContext) = 0;

	// Handles single file cooks. Caller is expected to have
	// used CanCook() to determine if this is the right task to
	// cook filePath or not.
	Bool CookSingle(ICookContext& rContext, FilePath filePath);

	// Priority number. Lower numbers are executed first.
	virtual Int32 GetPriority() const = 0;

	// Textual description to describe this form of progress,
	// for progress bar on the command-line.
	virtual HString GetProgressType(const ICookContext& context) const;

	// Potentially not defined for a task. When defined,
	// return trus if some task specific conditions for
	// cooking are met. This is a validation check against
	// the environment that will be run once per cooking session.
	// If it fails for any task, no cooking is performed.
	virtual Bool ValidateContentEnvironment(ICookContext& rContext)
	{
		return true;
	}

protected:
	BaseCookTask();

	typedef Vector<FilePath, MemoryBudgets::Cooking> FilePaths;
	typedef Vector<String, MemoryBudgets::TBD> ProcessArguments;
	typedef Vector<CookSource, MemoryBudgets::Cooking> Sources;

	virtual Bool GetSources(ICookContext& rContext, FilePath filePath, Sources& rv) const;

	// Called to handle the actual cooking operation,
	// either by CookSingle() or CookAllOutOfDateContent().
	//
	// Default implementation as some tasks cannot do single cooks.
	virtual Bool InternalCook(ICookContext& rContext, FilePath filePath)
	{
		return false;
	}

	// Called to handle multi-cooks from within DefaultOutOfDateCookMulti().
	//
	// By default, just iterates and performs InternalCook() on each.
	virtual Bool InternalCookMulti(ICookContext& rContext, FilePath const* p, FilePath const* const pEnd)
	{
		for (; pEnd != p; ++p)
		{
			if (!InternalCook(rContext, *p))
			{
				return false;
			}
		}

		return true;
	}

	Bool AtomicWriteFinalOutput(
		ICookContext& rContext,
		void const* p,
		UInt32 u,
		FilePath filePath) const;
	Bool AtomicWriteFinalOutput(
		ICookContext& rContext,
		void const* p,
		UInt32 u,
		const String& sOutputFilename) const;
	Bool AtomicWriteFinalOutput(
		ICookContext& rContext,
		const String& sTempFilename,
		const String& sOutputFilename) const;
	Bool GatherOutOfDateOfSourceType(
		ICookContext& rContext,
		FileType eType,
		ContentFiles& rv) const;
	Bool PostCookUpdateMetadata(
		ICookContext& rContext,
		FilePathIterator iBegin,
		FilePathIterator iEnd) const;
	Bool RunCommandLineProcess(
		const String& sCommand,
		const ProcessArguments& vs) const
	{
		return RunCommandLineProcess(String(), sCommand, vs);
	}

	Bool RunCommandLineProcess(
		const String& sStartingDirectory,
		const String& sCommand,
		const ProcessArguments& vs,
		Bool bTreatAnyErrorOutputAsFailure = false,
		Bool bTreatStdOutAsErrors = false,
		Delegate<void(Byte const* s)> customStdOut = Delegate<void(Byte const* s)>()) const;

	// The standard out-of-date cook flow does the following:
	// - calls GatherOutOfDateOfSourceType() for the given type. Return false on failure.
	// - calls PreCook() on the resulting list. Return false on failure.
	// - calls Cook() on each element of the list. Return false on failure.
	// - calls PostCook() on the resulting list. Return false on failure.
	// - return true, success.
	Bool DefaultOutOfDateCook(
		ICookContext& rContext,
		FileType eType,
		ContentFiles& rv,
		Bool bCanRunInParallel = false);

	// The standard out-of-date cook range flow does the following:
	// - calls GatherOutOfDateOfSourceType() for the given types. Return false on failure.
	// - calls PreCook() on the resulting list. Return false on failure.
	// - calls CookSeveral() on each element of the list. Files with the same path but different types are passed together.
	// - calls PostCook() on the resulting list. Return false on failure.
	// - return true, success.
	Bool DefaultOutOfDateCookMulti(
		ICookContext& rContext,
		FileType eFirstType,
		FileType eLastType,
		ContentFiles& rv,
		Bool bCanRunInParallel = false);

	class ParallelUtil SEOUL_SEALED
	{
	public:
		SEOUL_DELEGATE_TARGET(ParallelUtil);

		ParallelUtil(BaseCookTask& r, ICookContext& rContext, ContentFiles& rv)
			: m_r(r)
			, m_rContext(rContext)
			, m_rv(rv)
			, m_vbResults()
			, m_Finished()
		{
		}

		~ParallelUtil()
		{
		}

		Bool Run(Bool bMulti);

	private:
		SEOUL_DISABLE_COPY(ParallelUtil);

		Bool RunTasks();
		void DoWork(Bool bDispatcherThread);

		BaseCookTask& m_r;
		ICookContext& m_rContext;
		ContentFiles& m_rv;
		
		typedef Vector<Bool, MemoryBudgets::Cooking> Results;
		Results m_vbResults;

		struct TaskEntry SEOUL_SEALED
		{
			Atomic32Type m_Index{};
			Atomic32Type m_Count{};
		};
		typedef Vector<TaskEntry, MemoryBudgets::Cooking> Tasks;
		Tasks m_vTasks;

		Int64 m_iStartTimeInTicks = 0;
		Atomic32 m_Started;
		Atomic32 m_Finished;
	}; // class ParallelUtil

private:
	SEOUL_DISABLE_COPY(BaseCookTask);
}; // class BaseCookTask

} // namespace Seoul::Cooking

#endif // include guard
