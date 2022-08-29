/**
 * \file IOSKeychain.mm
 * \brief Implementation of functions for managing IOS keychain
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
*/

#import "IOSKeychain.h"

#import <Foundation/NSBundle.h>
#import <Foundation/NSDictionary.h>
#import <Security/Security.h>

/**
 * Return the keychain service key.
 */
static NSString* getServiceName()
{
	return [NSString stringWithFormat:@"%@.KeyChainService", [[NSBundle mainBundle] bundleIdentifier]];
}

/**
 * Return an NSMutableDictionary that can be used for the unique user ID keychain queries.
 */
static NSMutableDictionary* getQueryDictionaryForKeychain(NSString* sKeyName)
{
	NSMutableDictionary* pReturn = [[NSMutableDictionary alloc] init];

	// Treat the lookup as a generic password.
	[pReturn setObject:(id)kSecClassGenericPassword forKey:(id)kSecClass];

	// Get the NSString as NSData.
	NSData* pKeyNameData = [sKeyName dataUsingEncoding:NSUTF8StringEncoding];

	// Set remaining elements of the dictionary.
	[pReturn setObject:pKeyNameData forKey:(id)kSecAttrAccount];
	[pReturn setObject:pKeyNameData forKey:(id)kSecAttrGeneric];
	[pReturn setObject:getServiceName() forKey:(id)kSecAttrService];
	
	return pReturn;
}

@implementation IOSKeychain

+ (NSString*)getValueForKey:(NSString*)pKey
				searchCloud:(bool)bSearchAppleCloud
{
	NSMutableDictionary* pDictionary = getQueryDictionaryForKeychain(pKey);

	// Add search attribute, limit one result.
	[pDictionary setObject:(id)kSecMatchLimitOne forKey:(id)kSecMatchLimit];

	// Return an NSData blob.
	[pDictionary setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnData];

	if (bSearchAppleCloud)
	{
		// Accept synchronized (iCloud) and unsynchronized (local keychain) query results
		[pDictionary setObject:(id)kSecAttrSynchronizableAny forKey:(id)kSecAttrSynchronizable];
	}

	// Get the existing value.
	NSData* pOutData = nil;
	OSStatus eStatus = SecItemCopyMatching(
		(CFDictionaryRef)pDictionary,
		(CFTypeRef*)&pOutData);
	(void)eStatus;

	// Release the query dictionary.
	[pDictionary release];

	// If we have a result, convert it to an NSString.
	if (nil != pOutData)
	{
		NSString* sReturn = [[NSString alloc] initWithData:pOutData encoding:NSUTF8StringEncoding];

		// Release the data.
		[pOutData release];

		[sReturn autorelease];
		// return the NSString.
		return sReturn;
	}

	// No result.
	return nil;
}

+ (bool)setKey:(NSString*)pKey
	   toValue:(NSString*)pValue
   isInvisible:(bool)bIsInvisible
   saveToCloud:(bool)bSaveToAppleCloud
{
	NSMutableDictionary* pDictionary = getQueryDictionaryForKeychain(pKey);

	// Get the value as NSData.
	NSData* pData = [pValue dataUsingEncoding:NSUTF8StringEncoding];
	[pDictionary setObject:pData forKey:(id)kSecValueData];

	if (bSaveToAppleCloud)
	{
		// Mark the keychain item as synchronizable, and accessible regardless of screen lock state
		[pDictionary setObject:(__bridge id)kCFBooleanTrue forKey:(id)kSecAttrSynchronizable];
		[pDictionary setObject:(__bridge id)kSecAttrAccessibleAlways forKey:(id)kSecAttrAccessible];
	}
	
	// Controls whether this key is visible to the user in keychain management.
	//
	// NOTE:: This property has some extremely strange behavior. It is not
	// considered a "significant" (i.e. an existing value in the keychain only
	// matches the dictionary if this attribute matches) key for the SecItemAdd
	// call, but it is "significant" for SecItemUpdate, SecItemCopyMatching,
	// and SecItemDelete. That is why it is added here, but not used in any
	// query dictionaries for other calls, to maintain backwards compatibility
	// with keychain items added when this property was not used.
	//
	// See the third post at this link for more info:
	// https://stackoverflow.com/questions/52750477/relevance-of-ksecattrisinvisible-in-ios
	if (bIsInvisible)
	{
		[pDictionary setObject:(__bridge id)kCFBooleanTrue forKey:(id)kSecAttrIsInvisible];
	}
	else
	{
		[pDictionary setObject:(__bridge id)kCFBooleanFalse forKey:(id)kSecAttrIsInvisible];
	}

	// Add the item.
	OSStatus eStatus = SecItemAdd((CFDictionaryRef)pDictionary, nil);

	// Release the dictionary.
	[pDictionary release];

	// If the item is already in the keychain, update it.
	if (errSecDuplicateItem == eStatus)
	{
		NSMutableDictionary* pSearchDictionary = getQueryDictionaryForKeychain(pKey);

		if (bSaveToAppleCloud)
		{
			// Accept synchronized (iCloud) and unsynchronized (local keychain) query results
			[pSearchDictionary setObject:(id)kSecAttrSynchronizableAny forKey:(id)kSecAttrSynchronizable];
		}

		NSMutableDictionary* pUpdateDictionary = [[NSMutableDictionary alloc] init];
		[pUpdateDictionary setObject:pData forKey:(id)kSecValueData];

		if (bSaveToAppleCloud)
		{
			// Mark the keychain item as synchronizable, and accessible regardless of screen lock state
			[pUpdateDictionary setObject:(__bridge id)kCFBooleanTrue forKey:(id)kSecAttrSynchronizable];
			[pUpdateDictionary setObject:(__bridge id)kSecAttrAccessibleAlways forKey:(id)kSecAttrAccessible];
		}
		
		if (bIsInvisible)
		{
			[pUpdateDictionary setObject:(__bridge id)kCFBooleanTrue forKey:(id)kSecAttrIsInvisible];
		}
		else
		{
			[pUpdateDictionary setObject:(__bridge id)kCFBooleanFalse forKey:(id)kSecAttrIsInvisible];
		}

		eStatus = SecItemUpdate(
			(CFDictionaryRef)pSearchDictionary,
			(CFDictionaryRef)pUpdateDictionary);

		[pUpdateDictionary release];
		[pSearchDictionary release];
	}
	
	return eStatus == errSecSuccess;
}

+ (bool)removeKey:(NSString*)pKey isCloud:(bool)bIsCloud
{
	NSMutableDictionary* pSearchDictionary = getQueryDictionaryForKeychain(pKey);

	if (bIsCloud)
	{
		// Delete synchronized (iCloud) and unsynchronized (local keychain) query results
		[pSearchDictionary setObject:(id)kSecAttrSynchronizableAny forKey:(id)kSecAttrSynchronizable];
	}

	OSStatus eStatus = SecItemDelete((CFDictionaryRef)pSearchDictionary);

	[pSearchDictionary release];
	
	return eStatus == errSecSuccess;
}

@end
