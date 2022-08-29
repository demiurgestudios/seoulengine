/**
 * \file AndroidCrashManager.h
 * \brief Specialization of NativeCrashManager for the Android platform. Uses
 * our server backend for crash reporting.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_CRASH_MANAGER_H
#define ANDROID_CRASH_MANAGER_H

#include "CrashManager.h"
#include "Vector.h"

namespace Seoul
{

struct AndroidCrashManagerSettings
{
	AndroidCrashManagerSettings()
		: m_sCrashReportDirectory()
		, m_BaseSettings()
	{
	}

	/** Absolute path to the directory to use for storing crash reports. */
	String m_sCrashReportDirectory;

	/** Settings for the base CrashServiceCrashManager class. */
	CrashServiceCrashManagerSettings m_BaseSettings;
}; // struct AndroidCrashManagerSettings

/** Android crash management is based on our backend service. */
class AndroidCrashManager SEOUL_SEALED : public NativeCrashManager
{
public:
	AndroidCrashManager(const AndroidCrashManagerSettings& settings);
	~AndroidCrashManager();

private:
	AndroidCrashManagerSettings const m_Settings;
	Vector<String> m_vsNativeCrashDumps;

	virtual Bool InsideNativeLockGetNextNativeCrash(void*& rp, UInt32& ru) SEOUL_OVERRIDE;
	virtual Bool InsideNativeLockHasNativeCrash() SEOUL_OVERRIDE;
	virtual void InsideNativeLockPurgeNativeCrash() SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(AndroidCrashManager);
}; // class AndroidCrashManager

} // namespace Seoul

#endif // include guard
