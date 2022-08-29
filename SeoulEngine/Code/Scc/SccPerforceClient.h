/**
 * \file SccPerforceClient.h
 * \brief Specialization of the abstract source control client interface
 * for Perforce source control.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCC_PERFORCE_CLIENT_H
#define SCC_PERFORCE_CLIENT_H

#include "SccIClient.h"
#include "SccPerforceClientParameters.h"
#include "SeoulString.h"
#include "Vector.h"
#include <initializer_list>

namespace Seoul::Scc
{

class PerforceClient SEOUL_SEALED : public IClient
{
public:
	PerforceClient(const PerforceClientParameters& parameters);
	~PerforceClient();

	/**
	 * Get the active changelist - if defined, all appropriate commands
	 * (add, edit, delete, etc.) will be directed to this changelist.
	 */
	Int32 GetActiveChangelist() const { return m_iActiveChangelist; }

	/**
	 * Set the active changelist - if defined, all appropriate commands
	 * (add, edit, delete, etc.) will be directed to this changelist.
	 */
	void SetActiveChangelist(Int32 iActiveChangelist) { m_iActiveChangelist = iActiveChangelist; }

	/**
	 * Open the files listed in asFiles for add, results
	 * written to messages.
	 *
	 * @return true if the command was reported as successful, false otherwise.
	 */
	virtual Bool OpenForAdd(
		FileIterator iBegin,
		FileIterator iEnd,
		const FileTypeOptions& fileTypeOptions = FileTypeOptions(),
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return RunCommand({ "add" }, true, iBegin, iEnd, fileTypeOptions, errorOut);
	}

	/**
	 * Open the files listed in asFiles for delete, results
	 * written to messages.
	 *
	 * @return true if the command was reported as successful, false otherwise.
	 */
	virtual Bool OpenForDelete(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate(),
		Bool bSyncFirst = true) SEOUL_OVERRIDE
	{
		if (bSyncFirst)
		{
			(void)RunCommand({ "sync" }, false, iBegin, iEnd, FileTypeOptions(), errorOut);
		}

		return RunCommand({ "delete" }, true, iBegin, iEnd, FileTypeOptions(), errorOut);
	}

	/**
	 * Open the files listed in asFiles for edit, results
	 * written to messages.
	 *
	 * @return true if the command was reported as successful, false otherwise.
	 */
	virtual Bool OpenForEdit(
		FileIterator iBegin,
		FileIterator iEnd,
		const FileTypeOptions& fileTypeOptions = FileTypeOptions(),
		const ErrorOutDelegate& errorOut = ErrorOutDelegate(),
		Bool bSyncFirst = true) SEOUL_OVERRIDE
	{
		if (bSyncFirst)
		{
			(void)RunCommand({ "sync" }, false, iBegin, iEnd, FileTypeOptions(), errorOut);
		}

		return RunCommand({ "edit" }, true, iBegin, iEnd, fileTypeOptions, errorOut);
	}

	/**
	 * Resolve the files listed in asFiles with Accept Yours, results written to messages.
	 *
	 * @return true if the command was reported as successful, false otherwise.
	 */
	virtual Bool ResolveAcceptYours(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return RunCommand({ "resolve", "-ay", "-f" }, true, iBegin, iEnd, FileTypeOptions(), errorOut);
	}

	/**
	 * Revert the files listed in asFiles, results written to messages.
	 *
	 * @return true if the command was reported as successful, false otherwise.
	 */
	virtual Bool Revert(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return RunCommand({ "revert" }, true, iBegin, iEnd, FileTypeOptions(), errorOut);
	}

	/**
	 * Revert the files listed in asFiles, only if they are unchanged from the current
	 * revision, results written to messages.
	 *
	 * @return true if the command was reported as successful, false otherwise.
	 */
	virtual Bool RevertUnchanged(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return RunCommand({ "revert", "-a" }, true, iBegin, iEnd, FileTypeOptions(), errorOut);
	}

	/**
	 * Submit the current changelist - either the default changelist or
	 * a number changelist (if ActiveChangelist) has been set
	 *
	 * @return true if the command was reported as successful, false otherwise.
	 * If this method returns true, ActiveChangelist will be reset to the default
	 * changlist, otherwise it will be left unchanged.
	 */
	virtual Bool Submit(
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		if (RunCommand({ "submit", "-f", "submitunchanged" }, true, nullptr, nullptr, FileTypeOptions(), errorOut))
		{
			SetActiveChangelist(-1);
			return true;
		}

		return false;
	}

	/**
	 * Sync the files listed in asFiles to head revision.
	 *
	 * @return true if the command was reported as successful, false otherwise.
	 */
	virtual Bool Sync(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return RunCommand({ "sync" }, false, iBegin, iEnd, FileTypeOptions(), errorOut);
	}

private:
	SEOUL_DISABLE_COPY(PerforceClient);

	typedef Vector<String, MemoryBudgets::TBD> ProcessArguments;

	void GetChangelistArgument(ProcessArguments& rv) const;
	void GetFileTypeArgument(const FileTypeOptions& fileTypeOptions, ProcessArguments& rv) const;
	ProcessArguments GetStandardArguments(Bool bNeedsStandardInput) const;
	Bool RunCommand(
		std::initializer_list<Byte const*> asCommands,
		Bool bNeedsChangelistArgument,
		FileIterator iBegin,
		FileIterator iEnd,
		const FileTypeOptions& fileTypeOptions,
		const ErrorOutDelegate& errorOut);

	String const m_sP4;
	PerforceClientParameters const m_Parameters;
	Int32 m_iActiveChangelist;
}; // class PerforceClient

} // namespace Seoul::Scc

#endif // include guard
