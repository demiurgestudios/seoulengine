/**
 * \file IOSSettings.mm
 * \brief Defines functions for interacting with the iOS Settings bundle, which
 * exposes user configurable per-app settings in the iOS Settings application.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#import <Foundation/Foundation.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSString.h>
#include "Path.h"
#include "SeoulString.h"

namespace Seoul
{

/**
 * Gets the extra command line arguments defined in the app's Settings
 */
String IOSGetCommandLineArgumentsFromIOSSettings()
{
	String sArguments;

	NSUserDefaults* pDefaults = [NSUserDefaults standardUserDefaults];

	NSString* pMoriartyServer = [pDefaults stringForKey:@"moriarty_server"];
	if (pMoriartyServer)
	{
		sArguments.Append("-moriarty_server=");
		sArguments.Append([pMoriartyServer cStringUsingEncoding:NSUTF8StringEncoding]);
		sArguments.Append(' ');
	}

	NSString* pCommandLineArguments = [pDefaults stringForKey:@"commandline_arguments"];
	if (pCommandLineArguments)
	{
		sArguments.Append([pCommandLineArguments cStringUsingEncoding:NSUTF8StringEncoding]);
	}

	return sArguments;
}

} // namespace Seoul
