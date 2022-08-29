/**
 * \file GamePaths.mm
 * \brief iOS specific Objective-C functions used by the GamePaths
 * singleton.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Prereqs.h"
#if SEOUL_PLATFORM_IOS

#import <Foundation/Foundation.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSString.h>
#include "Path.h"
#include "SeoulString.h"

namespace Seoul
{

/**
 * @return The Binaries/ folder on iOS. All GamePaths directories,
 * except for the user directory, are relative to this directory.
 */
String IOSGetBaseDirectory()
{
	NSString* pBasePath = [[NSBundle mainBundle] resourcePath];
	String sBaseDirectory = [pBasePath UTF8String];
	return Seoul::Path::Combine(sBaseDirectory, "Binaries");
}

/** @return The user's folder on iOS. */
String IOSGetUserDirectory()
{
	NSArray* pSearchPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString* pDocumentDirectoryPath = [pSearchPaths objectAtIndex:0];

	String sUserDirectory = [pDocumentDirectoryPath UTF8String];
	sUserDirectory.Append('/');

	return sUserDirectory;
}

} // namespace Seoul

#endif // SEOUL_PLATFORM_IOS
