/**
 * \file CrashManager.h
 * \brief Abstract base class of SeoulEngine CrashManager. CrashManager
 * provides exception/trap handling and reporting per platform. It is intended
 * as a shipping only (not development) system for tracking and reporting
 * crash information.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CRASH_MANAGER_H
#define CRASH_MANAGER_H

#include "Atomic32.h"
#include "Delegate.h"
#include "HashTable.h"
#include "Mutex.h"
#include "Queue.h"
#include "ScopedMemoryBuffer.h"
#include "SeoulString.h"
#include "Singleton.h"
#include "Vector.h"

namespace Seoul
{

/**
 * Global application context appended to custom crashes,
 * provides additional data about the overall state
 * of the application when a crash was generated.
 */
enum class CrashContext
{
	kStartup,
	kRun,
	kShutdown,
};

/** Utility structure, encapsulates context info for a custom crash error handler. */
struct CustomCrashErrorState SEOUL_SEALED
{
	// Builtin default handler - just WARNs about the custom crash.
	static void ReportHandler(CrashContext eContext, const CustomCrashErrorState& state);

	void ToString(CrashContext eContext, String& rs) const;

	struct Frame
	{
		Frame()
			: m_sFilename()
			, m_iLine(-1)
			, m_sFunction()
		{
		}

		/** Filename or other identifier of the file context of an error. */
		String m_sFilename;

		/** Line number of the stack frame, or -1 if undefined/unknown. */
		Int m_iLine;

		/** Function name of the stack frame or empty if undefined/unknown. */
		String m_sFunction;
	}; // struct Frame

	/** Human readable message string describing the error. */
	String m_sReason;

	/** Stack frame of the error. */
	typedef Vector<Frame, MemoryBudgets::TBD> Stack;
	Stack m_vStack;
}; // struct CustomCrashErrorState

class CrashManager SEOUL_ABSTRACT : public Singleton<CrashManager>
{
public:
	/** Convenience error handler. */
	static void DefaultErrorHandler(const CustomCrashErrorState& state)
	{
		auto const eContext = (CrashManager::Get() ? CrashManager::Get()->GetCrashContext() : CrashContext::kRun);
		CustomCrashErrorState::ReportHandler(eContext, state);
	}

	CrashManager();
	virtual ~CrashManager();

	/**
	 * @return True if custom crashes will successfully send (in general, may
	 * fail in specific cases even when this value is true).
	 */
	virtual Bool CanSendCustomCrashes() const = 0;

	/**
	 * Submit custom crash information. Intended for use with script language
	 * integrations or other error states which are not program crashes but
	 * should be reported and tracked as a runtime failure/crash equivalent.
	 */
	virtual void SendCustomCrash(const CustomCrashErrorState& errorState) = 0;

	/** Retrieve current crash context. */
	CrashContext GetCrashContext() const
	{
		return m_eContext;
	}

	/** Update the current crash context. */
	void SetCrashContext(CrashContext eContext)
	{
		m_eContext = eContext;
	}

	enum class SendCrashType
	{
		/** Crash is a script/custom crash filled in by client code. */
		Custom,
		
		/** Native crash (includes iOS native, Android Native, and Android Java). */
		Native,
	};
	typedef Delegate<void(SendCrashType eType, ScopedMemoryBuffer& r)> SendCrashDelegate;

	/**
	 * Invoked by the environment when a crash has been delivered, success
	 * or failure. Expected to be called after the crash delegate has
	 * been invoked.
	 */
	virtual void OnCrashSendComplete(SendCrashType eType, Bool bSuccess) = 0;

	/**
	 * Update the delegate used to submit crash bodies to
	 * a server backend.
	 */
	virtual void SetSendCrashDelegate(const SendCrashDelegate& del) = 0;

protected:
	Atomic32Value<CrashContext> m_eContext{ CrashContext::kStartup };

private:
	SEOUL_DISABLE_COPY(CrashManager);
}; // class CrashManager

/** Utility used to configure CrashServiceCrashManager. */
struct CrashServiceCrashManagerSettings SEOUL_SEALED
{
}; // struct CrashServiceCrashManagerSettings

/**
 * Intermediate specialization of CrashManager that uses the Demiurge
 * crash service. Must be again specialized to implement
 * platform dependent behavior.
 */
class CrashServiceCrashManager SEOUL_ABSTRACT : public CrashManager
{
public:
	SEOUL_DELEGATE_TARGET(CrashServiceCrashManager);

	CrashServiceCrashManager(const CrashServiceCrashManagerSettings& settings);
	virtual ~CrashServiceCrashManager();

	/**
	 * Submit custom crash information. Intended for use with script language
	 * integrations or other error states which are not program crashes but
	 * should be reported and tracked as a runtime failure/crash equivalent.
	 *
	 * CrashServiceCrashManager overrides and seals the base implementation - subclasses
	 * of CrashServiceCrashManager are expected instead to only implement DoPrepareCustomCrashHeader()
	 * to complete implementation of this functionality.
	 */
	virtual void SendCustomCrash(const CustomCrashErrorState& errorState) SEOUL_OVERRIDE SEOUL_SEALED;

	/**
	 * @return The current hook used to deliver crash bodies (custom script and native).
	 */
	SendCrashDelegate GetSendCrashDelegate() const
	{
		SendCrashDelegate ret;
		{
			Lock lock(m_Mutex);
			ret = m_SendCrashDelegate;
		}
		return ret;
	}

	// Crash delivery callback.
	virtual void OnCrashSendComplete(SendCrashType eType, Bool bSuccess) SEOUL_OVERRIDE;

	// Update the hook used to deliver crash bodies to a server backend.
	virtual void SetSendCrashDelegate(const SendCrashDelegate& del) SEOUL_OVERRIDE;

protected:
	/**
	 * Implemented by subclasses, prepend header data for the custom
	 * crash report and provide an opportunity to refuse to send
	 * the custom crash.
	 */
	virtual Bool DoPrepareCustomCrashHeader(
		const CustomCrashErrorState& errorState,
		String& rsCrashBody) const = 0;

	CrashServiceCrashManagerSettings const m_BaseSettings;

private:
	Mutex m_Mutex;
	typedef Queue< Pair<CrashContext, CustomCrashErrorState>, MemoryBudgets::TBD> CustomCrashQueue;
	CustomCrashQueue m_CustomCrashQueue;
	typedef HashTable<UInt32, Int64, MemoryBudgets::TBD> CustomCrashRedundantFilterTable;
	CustomCrashRedundantFilterTable m_tCustomCrashRedundantFilter;
	SendCrashDelegate m_SendCrashDelegate;
	Bool m_bCustomCrashPending;

	// Add a CustomCrashErrorState object to the queue, or ignore it, using
	// heuristics to balance the use of data against the amount of data we're
	// uploading.
	void InsideLockConditionalPush(CrashContext eContext, const CustomCrashErrorState& errorState);

	// Process the next entry on the queue, if there is one.
	void InsideLockProcessQueue();

	// Pop the next entry from the queue and return true, or return
	// false if no entry is available.
	Bool InsideLockTryPop(CrashContext& reContext, CustomCrashErrorState& rErrorState);

	SEOUL_DISABLE_COPY(CrashServiceCrashManager);
}; // class CrashServiceCrashManager

/** Specialization of CrashServiceCrashManager that serves as a base class for native implementations. */
class NativeCrashManager SEOUL_ABSTRACT : public CrashServiceCrashManager
{
public:
	NativeCrashManager(const CrashServiceCrashManagerSettings& settings);
	virtual ~NativeCrashManager();

	/**
	* @return True if custom crashes will successfully send (in general, may
	* fail in specific cases even when this value is true).
	*/
	virtual Bool CanSendCustomCrashes() const SEOUL_OVERRIDE SEOUL_SEALED;

	/**
	* Invoked by the environment when a crash has been delivered, success
	* or failure. Expected to be called after the crash delegate has
	* been invoked.
	*/
	virtual void OnCrashSendComplete(SendCrashType eType, Bool bSuccess) SEOUL_OVERRIDE SEOUL_SEALED;

	/**
	* Update the current crash report user ID. Crashes will
	* be associated with this ID (if specified) for easier tracking.
	*/
	virtual void SetSendCrashDelegate(const SendCrashDelegate& del) SEOUL_OVERRIDE SEOUL_SEALED;

protected:
	virtual Bool InsideNativeLockGetNextNativeCrash(void*& rp, UInt32& ru) = 0;
	virtual Bool InsideNativeLockHasNativeCrash() = 0;
	virtual void InsideNativeLockPurgeNativeCrash() = 0;

	Bool const m_bEnabled;

private:
	/**
	* Implemented by subclasses, prepend header data for the custom
	* crash report and provide an opportunity to refuse to send
	* the custom crash.
	*/
	virtual Bool DoPrepareCustomCrashHeader(
		const CustomCrashErrorState& errorState,
		String& rsCrashBody) const SEOUL_OVERRIDE SEOUL_SEALED;

	Mutex m_NativeMutex;

	void ConsumeAndSendNativeCrash();
	void SendNativeCrash();
	void WorkerThreadConsumeNextNativeCrash();

	SEOUL_DISABLE_COPY(NativeCrashManager);
}; // class NativeCrashManager

/** "Nop" CrashManager - for platforms with no CrashManager support. */
class NullCrashManager SEOUL_SEALED : public CrashManager
{
public:
	NullCrashManager();
	~NullCrashManager();

	virtual Bool CanSendCustomCrashes() const SEOUL_OVERRIDE;

	virtual void SendCustomCrash(const CustomCrashErrorState& errorState) SEOUL_OVERRIDE;

	virtual void OnCrashSendComplete(SendCrashType eType, Bool bSuccess) SEOUL_OVERRIDE
	{
		// Nop
	}

	virtual void SetSendCrashDelegate(const SendCrashDelegate& del) SEOUL_OVERRIDE
	{
		// Nop
	}

private:
	SEOUL_DISABLE_COPY(NullCrashManager);
}; // class NullCrashManager

} // namespace Seoul

#endif // include guard
