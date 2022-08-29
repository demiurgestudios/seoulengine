/**
 * \file IOSCrashManager.h
 * \brief Specialization of NativeCrashManager for the iOS platform. Uses
 * our server backend for crash reporting.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_CRASH_MANAGER_H
#define IOS_CRASH_MANAGER_H

#include "CrashManager.h"

namespace Seoul
{

/** iOS crash management is based on our backend service. */
class IOSCrashManager SEOUL_SEALED : public NativeCrashManager
{
public:
	IOSCrashManager(const CrashServiceCrashManagerSettings& settings);
	~IOSCrashManager();

private:
	virtual Bool InsideNativeLockGetNextNativeCrash(void*& rp, UInt32& ru) SEOUL_OVERRIDE;
	virtual Bool InsideNativeLockHasNativeCrash() SEOUL_OVERRIDE;
	virtual void InsideNativeLockPurgeNativeCrash() SEOUL_OVERRIDE;

	SEOUL_DISABLE_COPY(IOSCrashManager);
}; // class IOSCrashManager

} // namespace Seoul

#endif // include guard
