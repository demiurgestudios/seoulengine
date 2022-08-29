/**
 * \file AndroidCrashManager.cpp
 * \brief Specialization of NativeCrashManager for the Android platform. Uses
 * our server backend for crash reporting.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidCrashManager.h"
#include "Directory.h"
#include "DiskFileSystem.h"
#include "Engine.h"
#include "Once.h"
#include "PlatformData.h"

// breakpad used for native crash capture and handling.
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
#	include "android/log.h"
#	include "client/linux/handler/exception_handler.h"
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING

namespace Seoul
{

#if SEOUL_WITH_NATIVE_CRASH_REPORTING
#	include "android/log.h"
#	include "client/linux/handler/exception_handler.h"

/** Maximum number of .dmp files we allow on disk - this number + 1 is the max that will ever be written. */
static const UInt32 kuMaxDmpFiles = 3u;

static Seoul::Once s_NativeCrashHandlerOnce;
static google_breakpad::ExceptionHandler* s_pExceptionHandler = nullptr;

bool GoogleBreakpadDumpCallback(
	const google_breakpad::MinidumpDescriptor& descriptor,
	void* /*pContext*/,
	bool bSucceeded)
{
	// Nop
	return bSucceeded;
}

static void InitializeNativeCrashHandler(const AndroidCrashManagerSettings& settings)
{
	s_NativeCrashHandlerOnce.Call([&]()
	{
		using namespace google_breakpad;

		MinidumpDescriptor descriptor(settings.m_sCrashReportDirectory.CStr());
		delete s_pExceptionHandler;

		s_pExceptionHandler = new ExceptionHandler(
			descriptor,
			nullptr,
			GoogleBreakpadDumpCallback,
			nullptr,
			true,
			-1);
	});
}
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING

AndroidCrashManager::AndroidCrashManager(const AndroidCrashManagerSettings& settings)
	: NativeCrashManager(settings.m_BaseSettings)
	, m_Settings(settings)
	, m_vsNativeCrashDumps()
{
	// No native crash reporting if disabled.
	if (!m_bEnabled)
	{
		return;
	}

#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// Initialize our native crash handling.
	InitializeNativeCrashHandler(settings);

	// Gather existing native crashes for later processing.
	(void)Directory::GetDirectoryListing(
		m_Settings.m_sCrashReportDirectory,
		m_vsNativeCrashDumps,
		false,
		false,
		".dmp");
	QuickSort(m_vsNativeCrashDumps.Begin(), m_vsNativeCrashDumps.End());

	// Limit to maximum.
	while (m_vsNativeCrashDumps.GetSize() > kuMaxDmpFiles)
	{
		(void)DiskSyncFile::DeleteFile(m_vsNativeCrashDumps.Back());
		m_vsNativeCrashDumps.PopBack();
	}
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING
}

AndroidCrashManager::~AndroidCrashManager()
{
}

#if SEOUL_WITH_NATIVE_CRASH_REPORTING
static String FormatTime(UInt64 uFileTime)
{
	// Cast.
	time_t const timeval = (time_t)uFileTime;

	// Convert to UTC time.
	tm gm;
	memset(&gm, 0, sizeof(gm));
	(void)gmtime_r(&timeval, &gm);

	// Sun Jan 20 21:39:03 WIB 2019
	Byte aBuffer[256u];
	auto const zSize = strftime(aBuffer, sizeof(aBuffer), "%a %b %d %H:%M:%S WIB %Y", &gm);

	// Done.
	return String(aBuffer, (UInt32)zSize);
}

static String GetNativeDumpHeader(const String& sFilename)
{
	// Cache engine.
	auto pEngine = Engine::Get();
	if (!pEngine)
	{
		return String();
	}

	// Get platform data.
	PlatformData data;
	pEngine->GetPlatformData(data);

	// Get the file timestamp for date time.
	auto const uModified = DiskSyncFile::GetModifiedTime(sFilename);
	auto const sDate(FormatTime(uModified));

	String s;
	s += String::Printf("Package: %s\n", data.m_sPackageName.CStr());
	s += String::Printf("Version: %d\n", data.m_iAppVersionCode);
	s += String::Printf("Android: %s\n", data.m_sOsVersion.CStr());
	s += String::Printf("Manufacturer: %s\n", data.m_sDeviceManufacturer.CStr());
	s += String::Printf("Model: %s\n", data.m_sDeviceModel.CStr());
	s += String::Printf("Date: %s", sDate.CStr());
	return s;
}

static void PostProcessDumpFile(const String& sFilename, void*& rp, UInt32& ru)
{
	// Needs to be at least 4 bytes.
	if (ru < 4u) { return; }

	// Needs to start with "MDMP".
	if (0 != strncmp((const char*)rp, "MDMP", 4)) { return; }

	// Now update - construct a header string, which will
	// be followed by 2 null characters, and prefix
	// that to the dump file memory.
	auto const sHeader(GetNativeDumpHeader(sFilename));
	if (sHeader.IsEmpty()) { return; }

	// Resize and prepend - header + <null><null> + minidump.
	rp = MemoryManager::Reallocate(rp, ru + sHeader.GetSize() + 2u, MemoryBudgets::Strings);
	memmove((Byte*)rp + sHeader.GetSize() + 2u, rp, ru);
	memcpy(rp, sHeader.CStr(), sHeader.GetSize());
	*((Byte*)rp + sHeader.GetSize()) = '\0';
	*((Byte*)rp + sHeader.GetSize() + 1) = '\0';
	ru += sHeader.GetSize() + 2u;
}
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING

Bool AndroidCrashManager::InsideNativeLockGetNextNativeCrash(void*& rp, UInt32& ru)
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// Early out if no pending dumps.
	if (m_vsNativeCrashDumps.IsEmpty()) { return false; }

	// Get the filename.
	auto const sFilename(m_vsNativeCrashDumps.Back());

	// Read the file.
	if (!DiskSyncFile::ReadAll(
		sFilename,
		rp,
		ru,
		0u,
		MemoryBudgets::TBD))
	{
		return false;
	}

	// Additional processing - if the file is a native dump
	// (starts with 'MDMP'), then we prefix a textual header
	// that contains additional context data (e.g. timestamp,
	// package name, etc.)
	PostProcessDumpFile(sFilename, rp, ru);

	return true;
#else // #if !SEOUL_WITH_NATIVE_CRASH_REPORTING
	return false;
#endif // /#if !SEOUL_WITH_NATIVE_CRASH_REPORTING
}

Bool AndroidCrashManager::InsideNativeLockHasNativeCrash()
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// If we have at least one unprocessed dump, we
	// have a crash to process.
	return !m_vsNativeCrashDumps.IsEmpty();
#else
	return false;
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING
}

void AndroidCrashManager::InsideNativeLockPurgeNativeCrash()
{
#if SEOUL_WITH_NATIVE_CRASH_REPORTING
	// Early out if no pending dumps.
	if (m_vsNativeCrashDumps.IsEmpty()) { return; }

	// Delete the file.
	(void)DiskSyncFile::DeleteFile(m_vsNativeCrashDumps.Back());

	// Pop-back.
	m_vsNativeCrashDumps.PopBack();
#endif // /#if SEOUL_WITH_NATIVE_CRASH_REPORTING
}

} // namespace Seoul
