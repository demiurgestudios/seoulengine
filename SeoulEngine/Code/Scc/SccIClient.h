/**
 * \file SccIClient.h
 * \brief Abstract interface to a source control client provider.
 * Provides a generalized interface to various source control backends.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ISOURCE_CONTROL_CLIENT_H
#define ISOURCE_CONTROL_CLIENT_H

#include "Delegate.h"
#include "Prereqs.h"
#include "SccFileTypeOptions.h"
namespace Seoul { class String; }

namespace Seoul::Scc
{

class IClient SEOUL_ABSTRACT
{
public:
	typedef Delegate<void(Byte const* s)> ErrorOutDelegate;
	typedef String const* FileIterator;

	virtual ~IClient()
	{
	}

	/**
	 * @return Whether the defined source control client
	 * is the null client or not. Typically, you don't
	 * want to override this (only NullClient should)
	 */
	virtual Bool IsNull() const
	{
		return false;
	}

	virtual Bool OpenForAdd(
		FileIterator iBegin,
		FileIterator iEnd,
		const FileTypeOptions& fileTypeOptions = FileTypeOptions(),
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) = 0;
	virtual Bool OpenForDelete(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate(),
		Bool bSyncFirst = true) = 0;
	virtual Bool OpenForEdit(
		FileIterator iBegin,
		FileIterator iEnd,
		const FileTypeOptions& fileTypeOptions = FileTypeOptions(),
		const ErrorOutDelegate& errorOut = ErrorOutDelegate(),
		Bool bSyncFirst = true) = 0;
	virtual Bool ResolveAcceptYours(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) = 0;
	virtual Bool Revert(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) = 0;
	virtual Bool RevertUnchanged(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) = 0;
	virtual Bool Submit(
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) = 0;
	virtual Bool Sync(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) = 0;

protected:
	IClient()
	{
	}

private:
	SEOUL_DISABLE_COPY(IClient);
}; // class IClient

class NullClient SEOUL_SEALED : public IClient
{
public:
	NullClient()
	{
	}

	~NullClient()
	{
	}

	virtual Bool IsNull() const SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool OpenForAdd(
		FileIterator iBegin,
		FileIterator iEnd,
		const FileTypeOptions& fileTypeOptions = FileTypeOptions(),
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool OpenForDelete(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate(),
		Bool bSyncFirst = true) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool OpenForEdit(
		FileIterator iBegin,
		FileIterator iEnd,
		const FileTypeOptions& fileTypeOptions = FileTypeOptions(),
		const ErrorOutDelegate& errorOut = ErrorOutDelegate(),
		Bool bSyncFirst = true) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool ResolveAcceptYours(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool Revert(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool RevertUnchanged(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool Submit(
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return true;
	}

	virtual Bool Sync(
		FileIterator iBegin,
		FileIterator iEnd,
		const ErrorOutDelegate& errorOut = ErrorOutDelegate()) SEOUL_OVERRIDE
	{
		return true;
	}

private:
	SEOUL_DISABLE_COPY(NullClient);
}; // class NullClient

} // namespace Seoul::Scc

#endif // include guard
