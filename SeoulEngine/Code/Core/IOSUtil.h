/**
 * \file IOSUtil.h
 * \brief Objective-C support function implementations for iOS
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_UTIL_H
#define IOS_UTIL_H

#include "Prereqs.h"
namespace Seoul { class DataStore; }

#ifdef __OBJC__
@class NSDictionary;
#else
typedef void NSDictionary;
#endif

namespace Seoul
{

// Ensures that Cocoa is aware that we are multithreaded
void IOSEnsureCocoaIsMultithreaded();

// Creates an NSAutoreleasePool for C++ code
void *IOSInitAutoreleasePool();

// Releases an NSAutoreleasePool for C++ code
void IOSReleaseAutoreleasePool(void *pPool);

void IOSPrintDebugString(const Byte *sMessage);

// Utility to scope a block in an autorelease pool, that is independent of
// the current platform or objective-C.
class ScopedAutoRelease SEOUL_SEALED
{
public:
	ScopedAutoRelease()
#if SEOUL_PLATFORM_IOS
		: m_pPool(IOSInitAutoreleasePool())
#endif // /#if SEOUL_PLATFORM_IOS
	{
	}

	~ScopedAutoRelease()
	{
#if SEOUL_PLATFORM_IOS
		void* p = m_pPool;
		m_pPool = nullptr;
		IOSReleaseAutoreleasePool(p);
#endif // /#if SEOUL_PLATFORM_IOS
	}

private:
#if SEOUL_PLATFORM_IOS
	void* m_pPool;
#endif // /#if SEOUL_PLATFORM_IOS

	SEOUL_DISABLE_COPY(ScopedAutoRelease);
}; // class ScopedAutoRelease

// Convert an NSDictionary into a DataStore.
Bool ConvertToDataStore(NSDictionary const* pDict, DataStore& rDataStore);
// Convert a DataStore to an NSDictionary.
NSDictionary* ConvertToNSDictionary(const DataStore& dataStore) SEOUL_RETURNS_AUTORELEASED;

} // namespace Seoul

#endif // include guard
