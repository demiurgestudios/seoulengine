/**
 * \file Path.mm
 * \brief Functions for manipulating file path strings in platform
 * independent ways.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Prereqs.h"
#if SEOUL_PLATFORM_IOS

#include "Path.h"
#import <Foundation/NSBundle.h>
#import <Foundation/NSFileManager.h>

namespace Seoul
{

namespace Path
{

String IOSGetProcessPath()
{
	return String([[NSBundle mainBundle] bundlePath]);
}

String IOSGetTempPath()
{
	return String(NSTemporaryDirectory());
}

} // namespace Path

} // namespace Seoul

#endif // /SEOUL_PLATFORM_IOS
