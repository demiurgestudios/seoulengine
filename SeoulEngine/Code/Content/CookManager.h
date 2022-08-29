/**
 * \file CookManager.h
 * \brief CookManager handles converting raw asset files into cooked files that
 * SeoulEngine can load. Whether cooking is available or not is platform
 * dependent. The NullCookManager can be used to disable cooking, for example,
 * in ship builds or on platforms that do not support cooking.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOK_MANAGER_H
#define COOK_MANAGER_H

#include "Atomic32.h"
#include "FilePath.h"
#include "Prereqs.h"
#include "Singleton.h"
#include "Vector.h"

namespace Seoul
{

class CookManager SEOUL_ABSTRACT : public Singleton<CookManager>
{
public:
	virtual ~CookManager()
	{
	}

	/**
	 * Results of an attempted Cook operation.
	 */
	enum CookResult
	{
		/** Should be returned if cooking was requested, was executed, and was successful. */
		kSuccess = 1,

		/**
		 * Should be returned if cooking was requested with bCheckTimestamp = true and a
		 * cooked version of the file already exists which is up-to-date with the source file.
		 */
		kUpToDate,

		/**
		 * Should be returned if cooking is temporarily disabled with a call to
		 * CookManager::SetCookingEnabled().
		 */
		kErrorCookingDisabled,

		/**
		 * Should be returned if some underlying support necessary for the type
		 * of cook was not found - for example, on PC, this might mean an external cooker
		 * EXE is missing.
		 */
		kErrorMissingCookerSupport,

		/** Should be returned if cooking is attempted on an unsupported file type. */
		kErrorCannotCookFileType,

		/** Should be returned if the source of a requested cook does not exist. */
		kErrorSourceFileNotFound,

		/** Should be returned if cooking was attempted but failed for some reason. */
		kErrorCookingFailed
	};

	// When specialized by a CookManager, attempt a cook the file.
	// If bOnlyIfNeeded is true, the cook database will be checked
	// first and cooking will only occur if the file is marked
	// as not up-to-date.
	CookResult Cook(FilePath filePath, Bool bOnlyIfNeeded = true);

	/**
	 * Simple wrapper around Cook() that makes it more explicit that a conditional cook
	 * on the file's time stamp is being requested.
	 */
	CookResult CookIfOutOfDate(FilePath filePath)
	{
		return Cook(filePath, true);
	}

	/**
	 * @return True if cooking is currently enabled, false otherwise.
	 */
	Bool IsCookingEnabled() const
	{
		return m_bCookingEnabled;
	}

	/**
	 * Temporarily disable/enable cooking - setting this flag to
	 * false has no effect on cooking operations currently in flight.
	 */
	void SetCookingEnabled(Bool bCookingEnabled)
	{
		m_bCookingEnabled = bCookingEnabled;
	}

	// When specialized by a CookManager subclass, reports the FilePath of
	// a file that is currently being cooked.
	virtual FilePath GetCurrent() const
	{
		return FilePath();
	}

	// Can be specialized by a cook manager to provide cooking dependency information.
	typedef Vector<FilePath, MemoryBudgets::Cooking> Dependents;
	virtual void GetDependents(FilePath filePath, Dependents& rv) const
	{
		rv.Clear();
	}

	// Hook, expected to be called once per frame on the main thread.
	virtual void Tick(Float fDeltaTimeinSeconds)
	{
		// Nop
	}

	// When specialized by a CookManager subclass, should return true if
	// a file of type eType can be cooked. Note that this function
	// should return true if eType is the type of both the "cooked" and
	// "source" versions of a file.
	virtual Bool SupportsCooking(FileType eType) const;

protected:
	CookManager()
		: m_bCookingEnabled(true)
	{
	}

	virtual CookResult DoCook(FilePath filePath, Bool bOnlyIfNeeded) = 0;

private:
	Atomic32Value<Bool> m_bCookingEnabled;

	SEOUL_DISABLE_COPY(CookManager);
}; // class CookManager

/**
 * Specialization of CookManager that resolves all operations
 * to nops. Can be used to disable cooking or on platforms that do
 * not support cooking.
 */
class NullCookManager SEOUL_SEALED : public CookManager
{
public:
	NullCookManager()
	{
	}

	~NullCookManager()
	{
	}

	/**
	 * @return Always false - NullCookManager does not support cooking of files
	 * of any type.
	 */
	virtual Bool SupportsCooking(FileType /*eType*/) const SEOUL_OVERRIDE
	{
		return false;
	}

private:
	CookResult DoCook(FilePath filePath, Bool /*bOnlyIfNeeded*/) SEOUL_OVERRIDE
	{
		return kErrorCannotCookFileType;
	}

	SEOUL_DISABLE_COPY(NullCookManager);
}; // class NullCookManager

} // namespace Seoul

#endif // include guard
