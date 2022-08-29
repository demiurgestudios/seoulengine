/**
 * \file IOSCrashManager.mm
 * \brief Specialization of NativeCrashManager for the iOS platform. Uses
 * our server backend for crash reporting.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "IOSCrashManager.h"
#include "Once.h"
#include "ScopedAction.h"

// plcrashreporter used for native crash capture and handling.
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
#import <CrashReporter/CrashReporter.h>
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING

namespace Seoul
{

#if SEOUL_WITH_NATIVE_CRASH_REPORTING
static Seoul::Once s_NativeCrashHandlerOnce;
static PLCrashReporter* s_pPlCrashReporter{};

static void PlCrashReporterPostCrashCallback(
	siginfo_t* /* pInfo */,
	ucontext_t* /* pUcontact */,
	void* /* pUserData */)
{
	// Nop
}

static PLCrashReporterCallbacks s_PlCrashCallbacks =
{
	0, /* version */
	nullptr, /* context */
	PlCrashReporterPostCrashCallback, /* handleSignal */
};

static void InitializeNativeCrashHandler(const CrashServiceCrashManagerSettings& settings)
{
	s_NativeCrashHandlerOnce.Call([&]()
	{
		// Configuration.
		auto const eSignalHandlerType = PLCrashReporterSignalHandlerTypeBSD;
		auto const eSymbolicationStrategy = PLCrashReporterSymbolicationStrategyNone;

		// Construct the crash reporter.
		PLCrashReporterConfig* pConfig = [[PLCrashReporterConfig alloc]
			initWithSignalHandlerType: eSignalHandlerType
			symbolicationStrategy: eSymbolicationStrategy];
		s_pPlCrashReporter = [[PLCrashReporter alloc] initWithConfiguration: pConfig];
		[pConfig release];

		// Set callbacks and enable.
		[s_pPlCrashReporter setCrashCallbacks:&s_PlCrashCallbacks];
		[s_pPlCrashReporter enableCrashReporter];
	});
}
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING

IOSCrashManager::IOSCrashManager(const CrashServiceCrashManagerSettings& settings)
	: NativeCrashManager(settings)
{
	// No native crash reporting if disabled.
	if (!m_bEnabled)
	{
		return;
	}

#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// Initialize our native crash handling.
	InitializeNativeCrashHandler(settings);
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING
}

IOSCrashManager::~IOSCrashManager()
{
}

Bool IOSCrashManager::InsideNativeLockGetNextNativeCrash(void*& rp, UInt32& ru)
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	NSData* pCrashData = [[NSData alloc] initWithData: [s_pPlCrashReporter loadPendingCrashReportData]];
	if (nil == pCrashData)
	{
		// Empty, done.
		return false;
	}

	// Make sure we cleanup.
	auto const scope0(MakeScopedAction([](){}, [&]()
	{
		[pCrashData release];
		pCrashData = nil;
	}));

	// Convert the dump data into a report.
	NSError* pError = nil;
	PLCrashReport* pCrashLog = [[PLCrashReport alloc] initWithData:pCrashData error:&pError];
	if (nil == pCrashLog)
	{
		return false;
	}

	// Make sure we cleanup.
	auto const scope1(MakeScopedAction([](){}, [&]()
	{
		[pCrashLog release];
		pCrashLog = nil;
	}));

	// Convert to a text based log format ("iOS crash dump" compatible)
	NSString* pReport = [PLCrashReportTextFormatter stringValueForCrashReport:pCrashLog withTextFormat:PLCrashReportTextFormatiOS];
	if (nil == pReport)
	{
		return false;
	}

	// Acquire the data.
	String s(pReport);
	s.RelinquishBuffer(rp, ru);
	return true;
#else // #if !SEOUL_WITH_NATIVE_CRASH_REPORTING
	return false;
#endif // /#if !SEOUL_WITH_NATIVE_CRASH_REPORTING
}

Bool IOSCrashManager::InsideNativeLockHasNativeCrash()
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// Need a crash reporter.
	if (nil == s_pPlCrashReporter) { return false; }

	// Need a crash report.
	if (!([s_pPlCrashReporter hasPendingCrashReport])) { return false; }

	// Have a report.
	return true;
#else
	return false;
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING
}

void IOSCrashManager::InsideNativeLockPurgeNativeCrash()
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// Done forever if no reporter.
	if (nil == s_pPlCrashReporter) { return; }

	// Done forever if no pending crash.
	if (!([s_pPlCrashReporter hasPendingCrashReport])) { return; }

	// Purge the report.
	[s_pPlCrashReporter purgePendingCrashReport];
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING
}

} // namespace Seoul
