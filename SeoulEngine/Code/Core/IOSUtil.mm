/**
 * \file IOSUtil.mm
 * \brief Objective-C support function implementations for iOS
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
#import <UIKit/UIKit.h>
#import <os/log.h>

#include "DataStore.h"
#include "Logger.h"
#include "SeoulAssert.h"
#include "SeoulString.h"

/** Dummy class for creating a dummy NSThread */
@interface DummyThreadClass : NSObject
{
}

+ (void)dummyThreadProc:(id)object;

@end

@implementation DummyThreadClass

/** Do-nothing thread procedure */
+ (void)dummyThreadProc:(id)object
{
	// Do nothing
}

@end

namespace Seoul
{

/**
 * Ensures that Cocoa is aware that we are multithreaded
 */
void IOSEnsureCocoaIsMultithreaded()
{
	if (![NSThread isMultiThreaded])
	{
		// If we're not yet multithreaded, create a dummy thread.  See
		// "Protecting the Cocoa Frameworks" under the "Thread Management"
		// page of the iOS Threading Programming Guide:
		// https://developer.apple.com/library/ios/documentation/Cocoa/Conceptual/Multithreading/CreatingThreads/CreatingThreads.html#//apple_ref/doc/uid/10000057i-CH15-125024-BAJGFJED
		[NSThread detachNewThreadSelector:@selector(dummyThreadProc:)
								 toTarget:[DummyThreadClass class]
							   withObject:nil];
		SEOUL_ASSERT([NSThread isMultiThreaded]);
	}
}

/**
 * Creates an NSAutoreleasePool for C++ code
 */
void *IOSInitAutoreleasePool()
{
	return [[NSAutoreleasePool alloc] init];
}

/**
 * Releases an NSAutoreleasePool for C++ code
 */
void IOSReleaseAutoreleasePool(void *pPool)
{
	[(NSAutoreleasePool *)pPool release];
}

Bool IOSSetDoNotBackupFlag(const String& sAbsoluteFilename)
{
	NSURL* pURL = [[NSURL alloc] initFileURLWithPath:sAbsoluteFilename.ToNSString()];

	Bool const bReturn = [pURL setResourceValue:[NSNumber numberWithBool:YES] forKey:NSURLIsExcludedFromBackupKey error:nil];

	[pURL release];

	return bReturn;
}

void IOSPrintDebugString(Byte const* sMessage)
{
	NSLog(@"%@", @(sMessage));
}

static Bool Convert(NSArray const* p, DataStore& ds, const DataNode& node);
static Bool Convert(NSDictionary const* p, DataStore& ds, const DataNode& node);

/** Generic NSObject to HString key conversion. */
static Bool ConvertToHString(NSObject* pObject, HString& r)
{
	// Numbers.
	if ([pObject isKindOfClass:[NSNumber class]])
	{
		NSNumber* pNumber = (NSNumber*)pObject;
		r = HString(String([pNumber stringValue]));
		return true;
	}
	// Strings.
	else if ([pObject isKindOfClass:[NSString class]])
	{
		NSString* psString = (NSString*)pObject;
		r = HString(String(psString));
		return true;
	}
	else
	{
		SEOUL_WARN("%s: Invalid object for string conversion: %s", __FUNCTION__, [[pObject debugDescription] UTF8String]);
		return false;
	}
}

/** Generic NSObject to Array element conversion. */
static Bool ConvertToArrayElement(NSObject* pObject, DataStore& ds, const DataNode& arr, UInt32 uIndex)
{
	if ([pObject isKindOfClass:[NSNumber class]])
	{
		// See https://developer.apple.com/library/ios/#documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtTypeEncodings.html
		// for a description of the type encodings
		NSNumber* pNumber = (NSNumber*)pObject;
		Byte const* sObjCType = [pNumber objCType];

		// Sanity.
		if (nullptr == sObjCType) { return false; }

		switch (sObjCType[0])
		{
		case 'B':  // bool
			return ds.SetBooleanValueToArray(arr, uIndex, (Bool)[pNumber boolValue]);

		case 'c':  // char
			// Can be a bool or an integer - note that the internal boolean
			// class is private so we have to resort to weirdness like this to
			// tell bool from integer.
			if ([pNumber isKindOfClass:[@YES class]])
			{
				return ds.SetBooleanValueToArray(arr, uIndex, (Bool)[pNumber boolValue]);
			}
			// fall-through
		case 'C':  // unsigned char
		case 's':  // short
		case 'S':  // unsigned short
		case 'i':  // int
#if SEOUL_PLATFORM_32
		case 'l':  // long
			SEOUL_STATIC_ASSERT(sizeof(long) == sizeof(Int32));
#endif // /#if SEOUL_PLATFORM_32
			SEOUL_STATIC_ASSERT(sizeof(int) == sizeof(Int32));
			return ds.SetInt32ValueToArray(arr, uIndex, (Int32)[pNumber intValue]);

		case 'I':  // unsigned int
#if SEOUL_PLATFORM_32
		case 'L':  // unsigned long
#endif // /#if SEOUL_PLATFORM_32
			return ds.SetUInt32ValueToArray(arr, uIndex, (UInt32)[pNumber unsignedIntValue]);

		case 'f':  // float
		case 'd':  // double
			return ds.SetFloat32ValueToArray(arr, uIndex, (Float32)[pNumber floatValue]);

#if SEOUL_PLATFORM_64
		case 'l':  // long
			SEOUL_STATIC_ASSERT(sizeof(long) == sizeof(Int64));
#endif // /#if SEOUL_PLATFORM_64
		case 'q':  // long long
			SEOUL_STATIC_ASSERT(sizeof(long long) == sizeof(Int64));
			return ds.SetInt64ValueToArray(arr, uIndex, (Int64)[pNumber longLongValue]);

#if SEOUL_PLATFORM_64
		case 'L':  // unsigned long
			SEOUL_STATIC_ASSERT(sizeof(unsigned long) == sizeof(UInt64));
#endif // /#if SEOUL_PLATFORM_64
		case 'Q':  // unsigned long long
			return ds.SetUInt64ValueToArray(arr, uIndex, (UInt64)[pNumber unsignedLongLongValue]);

			// Unexpected or unsupported, fail.
		default:
			SEOUL_WARN("%s: Invalid NSNumber type: %s [%s]", __FUNCTION__, sObjCType, [[pNumber debugDescription] UTF8String]);
			return false;
		}
	}
	else if ([pObject isKindOfClass:[NSString class]])
	{
		return ds.SetStringToArray(arr, uIndex, String((NSString*)pObject));
	}
	else if ([pObject isKindOfClass:[NSArray class]])
	{
		if (!ds.SetArrayToArray(arr, uIndex)) { return false; }
		DataNode inner;
		if (!ds.GetValueFromArray(arr, uIndex, inner)) { return false; }

		return Convert((NSArray*)pObject, ds, inner);
	}
	else if ([pObject isKindOfClass:[NSDictionary class]])
	{
		if (!ds.SetTableToArray(arr, uIndex)) { return false; }
		DataNode inner;
		if (!ds.GetValueFromArray(arr, uIndex, inner)) { return false; }

		return Convert((NSDictionary*)pObject, ds, inner);
	}
	else
	{
		SEOUL_WARN("%s: Invalid object: %s", __FUNCTION__, [[pObject debugDescription] UTF8String]);
		return false;
	}

	return true;
}

/** Generic NSObject to Table element conversion. */
static Bool ConvertToTableElement(NSObject* pObject, DataStore& ds, const DataNode& tbl, HString key)
{
	if ([pObject isKindOfClass:[NSNumber class]])
	{
		// See https://developer.apple.com/library/ios/#documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Articles/ocrtTypeEncodings.html
		// for a description of the type encodings
		NSNumber* pNumber = (NSNumber*)pObject;
		Byte const* sObjCType = [pNumber objCType];

		// Sanity.
		if (nullptr == sObjCType) { return false; }

		switch (sObjCType[0])
		{
		case 'B':  // bool
			return ds.SetBooleanValueToTable(tbl, key, (Bool)[pNumber boolValue]);

		case 'c':  // char
			// Can be a bool or an integer - note that the internal boolean
			// class is private so we have to resort to weirdness like this to
			// tell bool from integer.
			if ([pNumber isKindOfClass:[@YES class]])
			{
				return ds.SetBooleanValueToTable(tbl, key, (Bool)[pNumber boolValue]);
			}
			// fall-through
		case 'C':  // unsigned char
		case 's':  // short
		case 'S':  // unsigned short
		case 'i':  // int
#if SEOUL_PLATFORM_32
		case 'l':  // long
			SEOUL_STATIC_ASSERT(sizeof(long) == sizeof(Int32));
#endif // /#if SEOUL_PLATFORM_32
			SEOUL_STATIC_ASSERT(sizeof(int) == sizeof(Int32));
			return ds.SetInt32ValueToTable(tbl, key, (Int32)[pNumber intValue]);

		case 'I':  // unsigned int
#if SEOUL_PLATFORM_32
		case 'L':  // unsigned long
#endif // /#if SEOUL_PLATFORM_32
			return ds.SetUInt32ValueToTable(tbl, key, (UInt32)[pNumber unsignedIntValue]);

		case 'f':  // float
		case 'd':  // double
			return ds.SetFloat32ValueToTable(tbl, key, (Float32)[pNumber floatValue]);

#if SEOUL_PLATFORM_64
		case 'l':  // long
			SEOUL_STATIC_ASSERT(sizeof(long) == sizeof(Int64));
#endif // /#if SEOUL_PLATFORM_64
		case 'q':  // long long
			SEOUL_STATIC_ASSERT(sizeof(long long) == sizeof(Int64));
			return ds.SetInt64ValueToTable(tbl, key, (Int64)[pNumber longLongValue]);

#if SEOUL_PLATFORM_64
		case 'L':  // unsigned long
			SEOUL_STATIC_ASSERT(sizeof(unsigned long) == sizeof(UInt64));
#endif // /#if SEOUL_PLATFORM_64
		case 'Q':  // unsigned long long
			return ds.SetUInt64ValueToTable(tbl, key, (UInt64)[pNumber unsignedLongLongValue]);

			// Unexpected or unsupported, fail.
		default:
			SEOUL_WARN("%s: Invalid NSNumber type: %s [%s]", __FUNCTION__, sObjCType, [[pNumber debugDescription] UTF8String]);
			return false;
		}
	}
	else if ([pObject isKindOfClass:[NSString class]])
	{
		return ds.SetStringToTable(tbl, key, String((NSString*)pObject));
	}
	else if ([pObject isKindOfClass:[NSArray class]])
	{
		if (!ds.SetArrayToTable(tbl, key)) { return false; }
		DataNode inner;
		if (!ds.GetValueFromTable(tbl, key, inner)) { return false; }

		return Convert((NSArray*)pObject, ds, inner);
	}
	else if ([pObject isKindOfClass:[NSDictionary class]])
	{
		if (!ds.SetTableToTable(tbl, key)) { return false; }
		DataNode inner;
		if (!ds.GetValueFromTable(tbl, key, inner)) { return false; }

		return Convert((NSDictionary*)pObject, ds, inner);
	}
	else
	{
		SEOUL_WARN("%s: Invalid object: %s", __FUNCTION__, [[pObject debugDescription] UTF8String]);
		return false;
	}

	return true;
}

/** Array to DataStore array. */
static Bool Convert(NSArray const* p, DataStore& ds, const DataNode& node)
{
	UInt32 u = 0u;
	for (NSObject* pObject in p)
	{
		if (!ConvertToArrayElement(pObject, ds, node, u++)) { return false; }
	}

	return true;
}

/** Dict to DataStore table. */
static Bool Convert(NSDictionary const* p, DataStore& ds, const DataNode& node)
{
	for (id pKey in p)
	{
		NSObject* pObject = [p objectForKey:pKey];

		HString key;
		if (!ConvertToHString(pKey, key)) { return false; }
		if (!ConvertToTableElement(pObject, ds, node, key)) { return false; }
	};

	return true;
}

/** Convert an NSDictionary into a DataStore. */
Bool ConvertToDataStore(NSDictionary const* pDict, DataStore& rDataStore)
{
	if (nil == pDict) { return false; } // Sanity.

	DataStore dataStore;
	dataStore.MakeTable();
	if (!Convert(pDict, dataStore, dataStore.GetRootNode())) { return false; }

	// Output and return success.
	rDataStore.Swap(dataStore);
	return true;
}

static NSArray* ConvertToNSArray(const DataStore& dataStore, const DataNode& value);
static NSDictionary* ConvertToNSDictionary(const DataStore& dataStore, const DataNode& value);
static NSNumber* ConvertToNSNumber(const DataStore& dataStore, const DataNode& value);
static NSObject* ConvertToNSObject(const DataStore& dataStore, const DataNode& value);
static NSString* ConvertToNSString(const DataStore& dataStore, const DataNode& value);

/**
 * Converts the given DataNode to an NSArray instance.  Returns nil if value is
 * not an array.  The returned NSArray is autoreleased.
 */
static NSArray* ConvertToNSArray(const DataStore& dataStore, const DataNode& value)
{
	if (!value.IsArray()) { return nil; }

	UInt32 uLength = 0;
	if (!dataStore.GetArrayCount(value, uLength)) { return nil; }

	NSMutableArray* pArray = [NSMutableArray arrayWithCapacity:uLength];
	for (UInt32 i = 0u; i < uLength; i++)
	{
		DataNode element;
		if (!dataStore.GetValueFromArray(value, i, element)) { return nil; }

		NSObject* pElement = ConvertToNSObject(dataStore, element);
		if (nil == pElement) { return nil; }

		[pArray addObject:pElement];
	}

	return pArray;
}

/**
 * Converts the given DataNode to an NSDictionary instance.  Returns nil if
 * value is not a table.  The returned NSDictionary is autoreleased.
 */
static NSDictionary* ConvertToNSDictionary(const DataStore& dataStore, const DataNode& value)
{
	if (!value.IsTable()) { return nil; }

	UInt32 uCount = 0;
	if (!dataStore.GetTableCount(value, uCount)) { return nil; }

	NSMutableDictionary* pDictionary = [NSMutableDictionary dictionaryWithCapacity:uCount];
	auto const iBegin = dataStore.TableBegin(value);
	auto const iEnd = dataStore.TableEnd(value);
	for (auto i = iBegin; iEnd != i; ++i)
	{
		NSObject* pValue = ConvertToNSObject(dataStore, i->Second);
		if (nil == pValue) { return nil; }

		NSString* pKey = i->First.ToNSString();
		[pDictionary setValue:pValue forKey:pKey];
	}

	return pDictionary;
}

/**
 * Converts the given DataNode to an NSNumber instance.  Returns nil if value
 * is not numeric or boolean type.  The returned NSNumber is autoreleased.
 */
static NSNumber* ConvertToNSNumber(const DataStore& dataStore, const DataNode& value)
{
	switch (value.GetType())
	{
	case DataNode::kBoolean:
		return [NSNumber numberWithBool:dataStore.AssumeBoolean(value)];
	case DataNode::kFloat31:
		return [NSNumber numberWithFloat:dataStore.AssumeFloat31(value)];
	case DataNode::kFloat32:
		return [NSNumber numberWithFloat:dataStore.AssumeFloat32(value)];
	case DataNode::kInt32Big:
		return [NSNumber numberWithInt:dataStore.AssumeInt32Big(value)];
	case DataNode::kInt32Small:
		return [NSNumber numberWithInt:dataStore.AssumeInt32Small(value)];
	case DataNode::kInt64:
		return [NSNumber numberWithLongLong:dataStore.AssumeInt64(value)];
	case DataNode::kUInt32:
		return [NSNumber numberWithUnsignedInt:dataStore.AssumeUInt32(value)];
	case DataNode::kUInt64:
		return [NSNumber numberWithUnsignedLongLong:dataStore.AssumeUInt64(value)];
	default:
		return nil;
	};
}

/**
 * Converts the given DataNode to one of an NSNumber, NSString, NSArray, or
 * NSDictionary depending on its type.  Null values are converted into empty
 * strings.  Always returns non-nil.  The returned NSObject is autoreleased.
 */
static NSObject* ConvertToNSObject(const DataStore& dataStore, const DataNode& value)
{
	switch (value.GetType())
	{
	case DataNode::kBoolean:
	case DataNode::kFloat31:
	case DataNode::kFloat32:
	case DataNode::kInt32Big:
	case DataNode::kInt32Small:
	case DataNode::kInt64:
	case DataNode::kUInt32:
	case DataNode::kUInt64:
		return ConvertToNSNumber(dataStore, value);

	case DataNode::kNull:
	case DataNode::kFilePath:
	case DataNode::kSpecialErase:
	case DataNode::kString:
		return ConvertToNSString(dataStore, value);

	case DataNode::kArray:
		return ConvertToNSArray(dataStore, value);

	case DataNode::kTable:
		return ConvertToNSDictionary(dataStore, value);
	};

	SEOUL_FAIL("Out-of-sync enum.");
	return nil;
}

/**
 * Converts the given DataNode to an NSString instance.  Returns nil if value
 * is not a string, identifier, or file path type.  The returned NSString is
 * autoreleased.
 */
static NSString* ConvertToNSString(const DataStore& dataStore, const DataNode& value)
{
	switch (value.GetType())
	{
	case DataNode::kNull:
	case DataNode::kSpecialErase:
		return String().ToNSString();
	case DataNode::kFilePath:
	{
		FilePath filePath;
		SEOUL_VERIFY(dataStore.AsFilePath(value, filePath));
		return filePath.ToSerializedUrl().ToNSString();
	}
	case DataNode::kString:
	{
		String s;
		SEOUL_VERIFY(dataStore.AsString(value, s));
		return s.ToNSString();
	}
	default:
		return nil;
	};
}

/** Public API, convert a DataStore to a dictionary. */
NSDictionary* ConvertToNSDictionary(const DataStore& dataStore)
{
	return ConvertToNSDictionary(dataStore, dataStore.GetRootNode());
}

} // namespace Seoul

#endif // /SEOUL_PLATFORM_IOS
