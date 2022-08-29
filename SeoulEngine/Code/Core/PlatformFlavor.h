/**
 * \file PlatformFlavor.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PLATFORM_FLAVOR_H
#define PLATFORM_FLAVOR_H

#include "Prereqs.h"

namespace Seoul
{

// TODO: Don't send these unfiltered to remove
// the requirement that the enum can't be renamed.

// DO NOT rename these unless you need to - these symbols
// are sent to the server as-is, and renaming
// them will break purchase verification until someone notices.
//
// Keep in-sync with the equivalent enum in AndroidNativeActivity.java.
enum class PlatformFlavor
{
	/** Unknown or we're running on a platform that does not have a flavor. */
	kUnknown,

	/** Build is a development build. It reports non-LIVE and talks to Amazon services. */
	kAmazonDevelopment,

	/** Build is a LIVE build that should talk to Amazon services. */
	kAmazonLive,

	/** Build is a development build. It reports non-LIVE and talks to Google Play services. */
	kGooglePlayDevelopment,

	/** Build is a LIVE build that should talk to Google Play services. */
	kGooglePlayLive,

	/** Build is a development build. It reports non-LIVE and talks to Samsung services. */
	kSamsungDevelopment,

	/** Build is a LIVE build that should talk to Samsung services. */
	kSamsungLive,
};

inline Bool IsAmazonPlatformFlavor(PlatformFlavor eDevicePlatformFlavor)
{
	return (
		PlatformFlavor::kAmazonDevelopment == eDevicePlatformFlavor ||
		PlatformFlavor::kAmazonLive == eDevicePlatformFlavor);
}

inline Bool IsGooglePlayPlatformFlavor(PlatformFlavor eDevicePlatformFlavor)
{
	return (
		PlatformFlavor::kGooglePlayDevelopment == eDevicePlatformFlavor ||
		PlatformFlavor::kGooglePlayLive == eDevicePlatformFlavor);
}

inline Bool IsSamsungPlatformFlavor(PlatformFlavor eDevicePlatformFlavor)
{
	return (
		PlatformFlavor::kSamsungDevelopment == eDevicePlatformFlavor ||
		PlatformFlavor::kSamsungLive == eDevicePlatformFlavor);
}

} // namespace Seoul

#endif // include guard
