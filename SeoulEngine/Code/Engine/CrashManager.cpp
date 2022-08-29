/**
 * \file CrashManager.cpp
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

#include "BuildChangelistPublic.h"
#include "CrashManager.h"
#include "Engine.h"
#include "FileManager.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "PlatformData.h"
#include "ReflectionDefine.h"
#include "SeoulTime.h"
#include "ThreadId.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(CrashContext)
	SEOUL_ENUM_N("Startup", CrashContext::kStartup)
	SEOUL_ENUM_N("Run", CrashContext::kRun)
	SEOUL_ENUM_N("Shutdown", CrashContext::kShutdown)
SEOUL_END_ENUM()

/** Maximum number of entries we'll place in the custom crash queue before popping existing entries. */
static const UInt32 kuMaxQueueSize = 32u;

/**
 * Maximum number of entries we'll place in the redundant filter table - much bigger than the queue,
 * we only cap this to prevent craziness in the event of a crash that is very frequent and always different
 * (possibly due to a change in reason string).
 */
static const UInt32 kuMaxRedundantFilterTableSize = 1024u;

/** Time between submission of redundant crashes. */
static const Double kfRedundantSubmissionIntervalInSeconds = 600.0; // 10 minutes.

/** Builtin default handler - just WARNs about the custom crash. */
void CustomCrashErrorState::ReportHandler(CrashContext eContext, const CustomCrashErrorState& state)
{
	String sOutput;
	state.ToString(eContext, sOutput);

#if SEOUL_LOGGING_ENABLED
	SEOUL_WARN("%s", sOutput.CStr());
#else
	PlatformPrint::PrintStringMultiline(PlatformPrint::Type::kWarning, "Warning: ", sOutput);
#endif // /#if SEOUL_LOGGING_ENABLED
}

void CustomCrashErrorState::ToString(CrashContext eContext, String& rs) const
{
	rs.Clear();
	rs.Assign("Reason: " + m_sReason);
	rs.Append(String::Printf(" (%s)", EnumToString<CrashContext>(eContext)));
	rs.Append("\nStack:");
	for (auto i = m_vStack.Begin(); m_vStack.End() != i; ++i)
	{
		if (i->m_sFilename.IsEmpty())
		{
			rs.Append(String::Printf("\n- %s", i->m_sFunction.CStr()));
		}
		else if (i->m_iLine < 0)
		{
			rs.Append(String::Printf("\n- %s: %s", i->m_sFilename.CStr(), i->m_sFunction.CStr()));
		}
		else
		{
			rs.Append(String::Printf("\n- %s(%d): %s", i->m_sFilename.CStr(), i->m_iLine, i->m_sFunction.CStr()));
		}
	}
}

CrashManager::CrashManager()
{
}

CrashManager::~CrashManager()
{
}

CrashServiceCrashManager::CrashServiceCrashManager(const CrashServiceCrashManagerSettings& settings)
	: m_BaseSettings(settings)
	, m_Mutex()
	, m_CustomCrashQueue()
	, m_tCustomCrashRedundantFilter()
	, m_SendCrashDelegate()
	, m_bCustomCrashPending(false)
{
}

CrashServiceCrashManager::~CrashServiceCrashManager()
{
}

/**
 * Submit custom crash information. Intended for use with script language
 * integrations or other error states which are not program crashes but
 * should be reported and tracked as a runtime failure/crash equivalent.
 */
void CrashServiceCrashManager::SendCustomCrash(const CustomCrashErrorState& errorState)
{
	Lock lock(m_Mutex);

	// Cache the current context.
	auto const eContext(m_eContext);

	// Push the entry onto the queue (possibly) and process the queue.
	InsideLockConditionalPush(eContext, errorState);
	InsideLockProcessQueue();

	// Also log when logging is enabled, for developers and automated testing.
#if SEOUL_LOGGING_ENABLED
	CustomCrashErrorState::ReportHandler(eContext, errorState);
#endif // /#if SEOUL_LOGGING_ENABLED
}

/** Callback received after delivering custom crash logs to the server backend. */
void CrashServiceCrashManager::OnCrashSendComplete(SendCrashType eType, Bool /*bSuccess*/)
{
	// If not a custom crash, early out, not handling the result.
	if (SendCrashType::Custom != eType)
	{
		return;
	}

	Lock lock(m_Mutex);

	// No longer a pending crash report.
	m_bCustomCrashPending = false;

	// Immediately send the next entry if still entries on the queue.
	InsideLockProcessQueue();
}

/**
 * Update the hook used to send crash bodies to a server backend.
 */
void CrashServiceCrashManager::SetSendCrashDelegate(const SendCrashDelegate& del)
{
	Lock lock(m_Mutex);

	if (del != m_SendCrashDelegate)
	{
		m_SendCrashDelegate = del;

		// Immediately send the next entry if still entries on the queue.
		InsideLockProcessQueue();
	}
}

/**
 * Add a CustomCrashErrorState object to the queue, or ignore it, using
 * heuristics to balance the use of data against the amount of data we're
 * uploading.
 */
void CrashServiceCrashManager::InsideLockConditionalPush(
	CrashContext eContext,
	const CustomCrashErrorState& errorState)
{
	// Get the current time. Note that we use SeoulTime here for a few
	// reasons that are not typical:
	// - don't want to rely on WorldTime since it's susceptible to
	//   local clock changes (don't want to flood crash reporting because
	//   someone happens to be messing with their clock).
	// - don't want to use server authoritative world time (e.g. Game::Client)
	//   because it's an up reference and would block crash reporting based on
	//   the presence of a server.
	// - don't want to use up time for similar reasons (Engine::GetUpTime() is
	//   an up reference since crash manager exists first).
	Int64 const iCurrentTimeInTicks = SeoulTime::GetGameTimeInTicks();

	// Hash the pertitent parts of the errorState (top level frame and reason string).
	UInt32 uHash = 0u;
	IncrementalHash(uHash, Seoul::GetHash(errorState.m_sReason));
	if (!errorState.m_vStack.IsEmpty())
	{
		const CustomCrashErrorState::Frame& frame = errorState.m_vStack.Front();
		IncrementalHash(uHash, Seoul::GetHash(frame.m_iLine));
		IncrementalHash(uHash, Seoul::GetHash(frame.m_sFilename));
		IncrementalHash(uHash, Seoul::GetHash(frame.m_sFunction));
	}

	// Don't insert the error state if it's in the reset crashes table and
	// we haven't reached the minimum interval.
	Int64 iLastTimeInTicks = -1;
	if (m_tCustomCrashRedundantFilter.GetValue(uHash, iLastTimeInTicks) &&
		SeoulTime::ConvertTicksToSeconds(iCurrentTimeInTicks - iLastTimeInTicks) < kfRedundantSubmissionIntervalInSeconds)
	{
		return;
	}

	// Overwrite the entry in the set with the current time.
	(void)m_tCustomCrashRedundantFilter.Overwrite(uHash, iCurrentTimeInTicks);

	// Place crashes in a queue to be sent on completion of a previous send.
	// Pop from the queue to keep under our maximum size
	m_CustomCrashQueue.Push(MakePair(eContext, errorState));

	// Cleanup the Queue - keep it under the maximum size.
	while (m_CustomCrashQueue.GetSize() > kuMaxQueueSize)
	{
		m_CustomCrashQueue.Pop();
	}

	// Cleanup the redundant filter table - keep it under the maximum size.
	if (m_tCustomCrashRedundantFilter.GetSize() > kuMaxRedundantFilterTableSize)
	{
		Vector<UInt32> vToErase;

		// Iterate over the table and erase any entries that are old enough.
		{
			auto const iBegin = m_tCustomCrashRedundantFilter.Begin();
			auto const iEnd = m_tCustomCrashRedundantFilter.End();
			for (auto i = iBegin; iEnd != i; ++i)
			{
				// Entries beyond the threshold are old enough, so erase them.
				if (SeoulTime::ConvertTicksToSeconds(iCurrentTimeInTicks - i->Second) >= kfRedundantSubmissionIntervalInSeconds)
				{
					vToErase.PushBack(i->First);
				}
			}
		}

		// Erase the entries.
		UInt32 const uEntries = vToErase.GetSize();
		for (UInt32 i = 0u; i < uEntries; ++i)
		{
			(void)m_tCustomCrashRedundantFilter.Erase(vToErase[i]);
		}
	}
}

// Process the next entry on the queue, if there is one.
void CrashServiceCrashManager::InsideLockProcessQueue()
{
	// Don't process while a request is pending.
	if (m_bCustomCrashPending)
	{
		return;
	}

	// Dont' process until we have an endpoint to send to.
	if (!m_SendCrashDelegate.IsValid())
	{
		return;
	}

	CrashContext eContext{};
	CustomCrashErrorState errorState;

	// Done if nothing on the queue.
	if (!InsideLockTryPop(eContext, errorState))
	{
		return;
	}

	String sCrashBody;
	if (DoPrepareCustomCrashHeader(errorState, sCrashBody))
	{
		// Now pending
		m_bCustomCrashPending = true;

		// Add date, crash reason, and the stack trace.
		sCrashBody += "Date: " + WorldTime::GetUTCTime().ToISO8601DateTimeUTCString() + "\n\n";

		// First is the message string describing the error.
		sCrashBody += errorState.m_sReason;
		if (!errorState.m_sReason.IsEmpty()) { sCrashBody += " "; }
		sCrashBody += String::Printf("(%s)", EnumToString<CrashContext>(eContext));

		// We group crashes based on the top stack entry on the backend.
		// So, we don't want the top entry to be underqualified (it needs
		// to have a line and file).
		//
		// To achieve this, we skip entries until we hit at least one fully
		// qualified entry.
		Bool bHasTopEntry = false;

		// Next is the stack trace.
		for (UInt32 i = 0u; i < errorState.m_vStack.GetSize(); ++i)
		{
			const CustomCrashErrorState::Frame& frame = errorState.m_vStack[i];

			// Skip the frame if we have no info.
			if (frame.m_sFunction.IsEmpty() &&
				frame.m_sFilename.IsEmpty() &&
				frame.m_iLine < 0)
			{
				continue;
			}

			// Skip the frame if we don't yet have a top entry and the
			// entry is not fully qualified (it doesn't have a line and file).
			if (!bHasTopEntry &&
				(frame.m_sFilename.IsEmpty() ||
				frame.m_iLine < 0))
			{
				continue;
			}

			// We now have a top entry one way or another.
			bHasTopEntry = true;

			// Format is "\n  at <function>(<file>:<line>)"
			sCrashBody += "\n  at ";
			if (frame.m_sFunction.IsEmpty())
			{
				sCrashBody += "<unknown>";
			}
			else
			{
				sCrashBody += frame.m_sFunction;
			}

			// Only add the file/line section if both are defined.
			if (!frame.m_sFilename.IsEmpty() && frame.m_iLine >= 0)
			{
				sCrashBody += "(";
				sCrashBody += frame.m_sFilename;
				sCrashBody += String::Printf(":%d", frame.m_iLine);
				sCrashBody += ")";
			}
			// Otherwise, add "(Native Method)" to indicate (what is likely
			// an unknown native function call or system function).
			else
			{
				sCrashBody += "(Native Method)";
			}
		}

		// Send the request, no resends, we don't want to back up on failures.
		ScopedMemoryBuffer buf;
		{
			void* p = nullptr;
			UInt32 u = 0u;
			sCrashBody.RelinquishBuffer(p, u);
			buf.Reset(p, u);
		}
		m_SendCrashDelegate(SendCrashType::Custom, buf);
	}
}

/**
 * Pop the next entry from the queue and return true, or return
 * false if no entry is available.
 */
Bool CrashServiceCrashManager::InsideLockTryPop(CrashContext& reContext, CustomCrashErrorState& rErrorState)
{
	// Pop the next entry from the queue and set it to rErrorState
	// if there is one.
	if (!m_CustomCrashQueue.IsEmpty())
	{
		auto const pair = m_CustomCrashQueue.Front();
		reContext = pair.First;
		rErrorState = pair.Second;
		m_CustomCrashQueue.Pop();

		return true;
	}

	return false;
}

NativeCrashManager::NativeCrashManager(const CrashServiceCrashManagerSettings& settings)
	: CrashServiceCrashManager(settings)
	// Disabled if a local build.
	, m_bEnabled(0 != g_kBuildChangelistFixed)
	, m_NativeMutex()
{
}

NativeCrashManager::~NativeCrashManager()
{
}

/**
* @return True if custom crashes will successfully send (in general, may
* fail in specific cases even when this value is true).
*/
Bool NativeCrashManager::CanSendCustomCrashes() const
{
	// Can send crashes if enabled.
	return m_bEnabled;
}

/**
* Implemented by subclasses, prepend header data for the custom
* crash report and provide an opportunity to refuse to send
* the custom crash.
*/
Bool NativeCrashManager::DoPrepareCustomCrashHeader(
	const CustomCrashErrorState& errorState,
	String& rsCrashBody) const
{
	// No crash reporting if disabled.
	if (!m_bEnabled)
	{
		return false;
	}

	// Cache engine.
	auto pEngine = Engine::Get();
	if (!pEngine)
	{
		return false;
	}

	// Get platform data.
	PlatformData data;
	pEngine->GetPlatformData(data);

	// Setup header and return true.
	rsCrashBody.Clear();
	rsCrashBody += String::Printf("Package: %s\n", data.m_sPackageName.CStr());

	// TODO: Bit of silly legacy, likely not needed with our own crashservice.
#if SEOUL_PLATFORM_IOS
	rsCrashBody += String::Printf("Version: %s\n", data.m_sAppVersionName.CStr());
#else
	rsCrashBody += String::Printf("Version: %d\n", data.m_iAppVersionCode);
#endif

	rsCrashBody += String::Printf("OS: %s\n", data.m_sOsVersion.CStr());
	rsCrashBody += String::Printf("Manufacturer: %s\n", data.m_sDeviceManufacturer.CStr());
	rsCrashBody += String::Printf("Model: %s\n", data.m_sDeviceModel.CStr());
	return true;
}

/**
* Invoked by the environment when a crash has been delivered, success
* or failure. Expected to be called after the crash delegate has
* been invoked.
*/
void NativeCrashManager::OnCrashSendComplete(SendCrashType eType, Bool bSuccess)
{
	// Call the base.
	CrashServiceCrashManager::OnCrashSendComplete(eType, bSuccess);

	// Now handle if a native crash.
	if (SendCrashType::Native == eType)
	{
		// Consume and send again, if available.
		ConsumeAndSendNativeCrash();
	}
}

/**
* Update the current crash report user ID. Crashes will
* be associated with this ID (if specified) for easier tracking.
*/
void NativeCrashManager::SetSendCrashDelegate(const SendCrashDelegate& del)
{
	// Cache for comparison.
	auto const prev = GetSendCrashDelegate();

	// Commit to the parent.
	CrashServiceCrashManager::SetSendCrashDelegate(del);

	// No crash reporting if disabled, if
	// there was no change to the delegate, or
	// if the delegate is still invalid.
	if (!m_bEnabled || prev == del || !del.IsValid())
	{
		return;
	}

	// Kick off the process of consuming native crashes.
	SendNativeCrash();
}

void NativeCrashManager::ConsumeAndSendNativeCrash()
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	{
		// We write a file to mark that we've started processing a crash. This is
		// done in case the crash data itself causes a (repeatable) crash, so
		// we never find ourselves stuck in an endless crash loop.
		auto const startProcessingFilePath(FilePath::CreateSaveFilePath("CrashManager/ProcessingLock.dat"));

		// Lock.
		Lock lock(m_NativeMutex);

		// Cleanup.
		(void)FileManager::Get()->Delete(startProcessingFilePath); // TODO: Perform off main thread.

		// Purge the report.
		InsideNativeLockPurgeNativeCrash();
	}

	// Continue.
	SendNativeCrash();
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING
}

void NativeCrashManager::SendNativeCrash()
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// Check delegate.
	auto del = GetSendCrashDelegate();
	if (!del) { return; }

	// Check before dispatch.
	{
		// Lock.
		Lock lock(m_NativeMutex);

		// Done forever if no more crashes.
		if (!InsideNativeLockHasNativeCrash()) { return; }
	}

	// Otherwise, dispatch.
	Jobs::AsyncFunction(SEOUL_BIND_DELEGATE(&NativeCrashManager::WorkerThreadConsumeNextNativeCrash, this));
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING
}

void NativeCrashManager::WorkerThreadConsumeNextNativeCrash()
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// Cache delegate for dispatch.
	auto const del = GetSendCrashDelegate();
	// Early out if we somehow are called with an invalid delegate.
	if (!del) { return; }

	// Lock.
	Lock lock(m_NativeMutex);

	// We write a file to mark that we've started processing a crash. This is
	// done in case the crash data itself causes a (repeatable) crash, so
	// we never find ourselves stuck in an endless crash loop.
	auto const startProcessingFilePath(FilePath::CreateSaveFilePath("CrashManager/ProcessingLock.dat"));

	// Doesn't exist, cleanup and skip processing.
	if (FileManager::Get()->Exists(startProcessingFilePath))
	{
		// Purge the report.
		InsideNativeLockPurgeNativeCrash();

		// Cleanup.
		(void)FileManager::Get()->Delete(startProcessingFilePath);

		// Done.
		return;
	}

	// Otherwise, start the sending processing.

	// Write the file.
	(void)FileManager::Get()->WriteAll(startProcessingFilePath, "<pid>", 5);

	// Retrieve the data.
	ScopedMemoryBuffer buf;
	{
		void* p = nullptr;
		UInt32 u = 0u;
		if (!InsideNativeLockGetNextNativeCrash(p, u))
		{
			// Delete the tracking file prior to return.
			(void)FileManager::Get()->Delete(startProcessingFilePath);
			return;
		}

		buf.Reset(p, u);
	}

	// Dispatch.
	del(CrashManager::SendCrashType::Native, buf);
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING
}

NullCrashManager::NullCrashManager()
	: CrashManager()
{
}

NullCrashManager::~NullCrashManager()
{
}

Bool NullCrashManager::CanSendCustomCrashes() const
{
	return true;
}

void NullCrashManager::SendCustomCrash(const CustomCrashErrorState& errorState)
{
	CustomCrashErrorState::ReportHandler(m_eContext, errorState);
}

} // namespace Seoul
