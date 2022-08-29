/**
* \file IOSKeychain.mm
* \brief IOS-specific functions for help managing interactions with the IOS keychain,
* an OS providede secure storage option.
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#pragma once
#ifndef IOS_KEYCHAIN_H
#define IOS_KEYCHAIN_H

#import <Foundation/NSString.h>

/**
 * Functions for managing the IOS keychain
 */
@interface IOSKeychain : NSObject
{
}

/**
 * Attempt to retrieve a value from the keychain for the given key, return nullptr
 * if the key is not in the keychain.
 */
+ (NSString*)getValueForKey:(NSString*)pKey
				searchCloud:(bool)bSearchAppleCloud;

/**
 * Attempt to put the given value into the keychain for the given key. Returns whether it was successful.
 */
+ (bool)setKey:(NSString*)pKey
	   toValue:(NSString*)pValue
   isInvisible:(bool)bIsInvisible
   saveToCloud:(bool)bSaveToAppleCloud;

/**
 * Attempt to remove the entry for the given key from the keychain.
 */
+ (bool)removeKey:(NSString*)pKey isCloud:(bool)bIsCloud;

@end

#endif /* IOS_KEYCHAIN_H */
