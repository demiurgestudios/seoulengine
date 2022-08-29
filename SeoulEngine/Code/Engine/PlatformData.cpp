/**
 * \file PlatformData.cpp
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

#include "PlatformData.h"
#include "ReflectionDefine.h"

namespace Seoul
{

// TODO: These values are reported directly to the server. That is not ideal.

// WARNING: Do not rename - server code depends on these strings.
SEOUL_BEGIN_ENUM(PlatformFlavor)
	SEOUL_ENUM_N("kUnknown", PlatformFlavor::kUnknown)
	SEOUL_ENUM_N("kAmazonDevelopment", PlatformFlavor::kAmazonDevelopment)
	SEOUL_ENUM_N("kAmazonLive", PlatformFlavor::kAmazonLive)
	SEOUL_ENUM_N("kGooglePlayDevelopment", PlatformFlavor::kGooglePlayDevelopment)
	SEOUL_ENUM_N("kGooglePlayLive", PlatformFlavor::kGooglePlayLive)
	SEOUL_ENUM_N("kSamsungDevelopment", PlatformFlavor::kSamsungDevelopment)
	SEOUL_ENUM_N("kSamsungLive", PlatformFlavor::kSamsungLive)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(PlatformData)
	SEOUL_PROPERTY_N("AdvertisingId", m_sAdvertisingId)
	SEOUL_PROPERTY_N("AppVersionCode", m_iAppVersionCode)
	SEOUL_PROPERTY_N("AppVersionName", m_sAppVersionName)
	SEOUL_PROPERTY_N("CountryCode", m_sCountryCode)
	SEOUL_PROPERTY_N("DeviceManufacturer", m_sDeviceManufacturer)
	SEOUL_PROPERTY_N("DeviceModel", m_sDeviceModel)
	SEOUL_PROPERTY_N("DeviceId", m_sDeviceId)
	SEOUL_PROPERTY_N("DeviceNetworkCountryCode", m_sDeviceNetworkCountryCode)
	SEOUL_PROPERTY_N("DeviceNetworkOperatorName", m_sDeviceNetworkOperatorName)
	SEOUL_PROPERTY_N("DevicePlatformName", m_sDevicePlatformName)
	SEOUL_PROPERTY_N("DeviceSimCountryCode", m_sDeviceSimCountryCode)
	SEOUL_PROPERTY_N("FacebookInstallAttribution", m_sFacebookInstallAttribution)
	SEOUL_PROPERTY_N("LanguageCodeIso2", m_sLanguageCodeIso2)
	SEOUL_PROPERTY_N("LanguageCodeIso3", m_sLanguageCodeIso3)
	SEOUL_PROPERTY_N("OsName", m_sOsName)
	SEOUL_PROPERTY_N("OsVersion", m_sOsVersion)
	SEOUL_PROPERTY_N("PackageName", m_sPackageName)
	SEOUL_PROPERTY_N("PlatformUUID", m_sPlatformUUID)
	SEOUL_PROPERTY_N("Rooted", m_bRooted)
	SEOUL_PROPERTY_N("ScreenPPI", m_vScreenPPI)
	SEOUL_PROPERTY_N("TargetApiOrSdkVersion", m_iTargetApiOrSdkVersion)
	SEOUL_PROPERTY_N("TimeZoneOffsetInSeconds", m_iTimeZoneOffsetInSeconds)
	SEOUL_PROPERTY_N("UACampaign", m_sUACampaign)
	SEOUL_PROPERTY_N("UAMediaSource", m_sUAMediaSource)
	SEOUL_PROPERTY_N("EnableAdTracking", m_bEnableAdTracking)
	SEOUL_PROPERTY_N("FirstRunAfterInstallation", m_bFirstRunAfterInstallation)
SEOUL_END_TYPE()

} // namespace Seoul
