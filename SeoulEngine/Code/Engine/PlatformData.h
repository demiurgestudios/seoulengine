/**
 * \file PlatformData.h
 * \brief Collection of platform specific data, acquire via Engine.
 * Generalized, although members that are tied to a specific platform
 * only will be noted as such.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PLATFORM_DATA_H
#define PLATFORM_DATA_H

#include "PlatformFlavor.h"
#include "Prereqs.h"
#include "SeoulString.h"
#include "Vector2D.h"

namespace Seoul
{

struct PlatformData SEOUL_SEALED
{
	PlatformData()
		: m_sAdvertisingId()
		, m_iAppVersionCode(0)
		, m_sAppVersionName()
		, m_sCountryCode()
		, m_sDeviceManufacturer()
		, m_sDeviceModel()
		, m_sDeviceId()
		, m_sDeviceNetworkCountryCode()
		, m_sDeviceNetworkOperatorName()
		, m_eDevicePlatformFlavor(PlatformFlavor::kUnknown)
		, m_sDevicePlatformName()
		, m_sDeviceSimCountryCode()
		, m_sFacebookInstallAttribution()
		, m_sLanguageCodeIso2()
		, m_sLanguageCodeIso3()
		, m_sOsName()
		, m_sOsVersion()
		, m_sPackageName()
		, m_sPlatformUUID()
		, m_bRooted(false)
		, m_vScreenPPI()
		, m_iTargetApiOrSdkVersion(0)
		, m_iTimeZoneOffsetInSeconds(0)
		, m_sUACampaign()
		, m_sUAMediaSource()
		, m_bEnableAdTracking(false)
		, m_bFirstRunAfterInstallation(false)
		, m_bImmersiveMode(false)
	{
	}

	/**
	 * On supported platforms, unique user identifier (typically a UUID) meant
	 * to be used for advertising attribution. Typically, users can reset
	 * the ID at any time, so it cannot be reliably used for permanent
	 * user identification, but is (also, typically) *required* by platforms
	 * to be used for advertising attribution.
	 */
	String m_sAdvertisingId;

	/**
	 * Platform version number - integer value uniquely
	 * identifying the app's version.
	 */
	Int m_iAppVersionCode;

	/**
	 * Platform version string - usually similar or the same
	 * as our BUILD_VERSION + BUILD_CHANGELIST_STR, but pulled
	 * from the platform's store.
	 */
	String m_sAppVersionName;

	/** The platform's country code (ISO 3166 2-letter -or- UN M.49 3-digit). */
	String m_sCountryCode;

	/** Manufacturer of the current device. */
	String m_sDeviceManufacturer;

	/** Model of the current device. */
	String m_sDeviceModel;

	/** Identifier of the current device. Not available for all platforms. */
	String m_sDeviceId;

	/** The country code of the device's registered phone network. */
	String m_sDeviceNetworkCountryCode;

	/** The name of the operator of the device's registered phone network. */
	String m_sDeviceNetworkOperatorName;

	/** Currently applies to Android only - Amazon vs. Google Play, live vs. dev. */
	PlatformFlavor m_eDevicePlatformFlavor;

	/** Platform dependent platform name (can be different from SeoulEngine's identifier). */
	String m_sDevicePlatformName;

	/** The country code of the device's SIM card. */
	String m_sDeviceSimCountryCode;

	/** Attribution string if an install was attributed to a Facebook ad. */
	String m_sFacebookInstallAttribution;

	/** The platform's language code (ISO 639 2-letter). */
	String m_sLanguageCodeIso2;

	/** The platform's language code (ISO 639-2/T or base 3-letter code). */
	String m_sLanguageCodeIso3;

	/** Name of the current operating system - typically, human readable, not for procedural use. */
	String m_sOsName;

	/** Version of the current operating system - typically, human readable, not for procedural use. */
	String m_sOsVersion;

	/**
	 * Global unqiue application identifier (typically a reverse domain name):
	 * - Android: package name.
	 * - iOS: bundle id
	 **/
	String m_sPackageName;

	/**
	 * A platform-specific UUID implemented to, as much as possible:
	 * - persist across application uninstalls.
	 */
	String m_sPlatformUUID;

	/**
	 * True if the application is running on a rooted/jailbroken device, false otherwise.
	 *
	 * NOTE: Our detection is simple and this sort of detection is easy to bypass in general,
	 * so this should be viewed as a rough heuristic, not an absolute. Not recommended
	 * to make programmatic decisions based on this value.
	 */
	Bool m_bRooted;

	/** Screen pixels-per-inch (x and y), if supported on the current platform. */
	Vector2D m_vScreenPPI;

	/** Target version or SDK level of the build (iOS verison on iOS, SDK version on Android). */
	Int32 m_iTargetApiOrSdkVersion;

	/** Value in seconds indicating the system's time zone offset from GMT. */
	Int32 m_iTimeZoneOffsetInSeconds;

	/** When attribution data of any sort is available, this will be populated with the media source of that campaign. */
	String m_sUACampaign;

	/** When attribution data of any sort is available, this will be populated with the media source of that campaign. */
	String m_sUAMediaSource;

	/**
	 * If true, the user has opted in (or, more often, *not* opted out) of advertising
	 * tracking and attribution. Typically, software is expected (or required) to
	 * limit certain tracking behaviors when this flag is false.
	 */
	Bool m_bEnableAdTracking;

	/** True if the current application is being run for the first time on the current device, false otherwise. */
	Bool m_bFirstRunAfterInstallation;

	/** True on Android if the application is using the entire screen (the navigation and status bars are not visible. */
	Bool m_bImmersiveMode;
}; // struct PlatformData

} // namespace Seoul

#endif // include guard
