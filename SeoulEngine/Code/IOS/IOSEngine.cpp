/**
 * \file IOSEngine.cpp
 * \brief Specialization of Engine for iOS. Implements platform
 * specific methods and exposes a few system functions that
 * are iOS specific and contained.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "FMODSoundManager.h"
#include "GenericAnalyticsManager.h"
#include "JobsFunction.h"
#include "GenericSaveApi.h"
#include "IOSCommerceManager.h"
#include "IOSEngine.h"
#include "IOSInput.h"
#include "IOSApplePlatformSignInManager.h"
#include "IOSGameCenterPlatformSignInManager.h"
#include "IOSSettings.h"
#include "Logger.h"
#include "OGLES2RenderDevice.h"
#include "PlatformData.h"
#include "Thread.h"

#include <sys/sysctl.h>

int g_ArgcIOS = 0;
char** g_ppArgvIOS = nullptr;

namespace Seoul
{

Bool IOSIsSandboxEnvironment();

struct HardwareStringToHardwareId
{
	Byte const* m_sName;
	IOSHardwareId m_eId;
};

/**
 * iOS implementation of a monotonically increasing,
 * sleep independent, clock time independent tick
 * function.
 */
static inline TimeInterval IOSGetSystemUptime()
{
	// Query and cache the current wall clock time.
	WorldTime const now = WorldTime::GetUTCTime();

	// Now query the kernel boot time.
	struct timeval kernelBootTime;
	memset(&kernelBootTime, 0, sizeof(kernelBootTime));

	Int aMIB[2] = { CTL_KERN, KERN_BOOTTIME };
	size_t zSize = sizeof(kernelBootTime);

	// Must succeed or we don't have a value to report.
	SEOUL_VERIFY(-1 != sysctl(&aMIB[0], 2, &kernelBootTime, &zSize, nullptr, 0));
	SEOUL_ASSERT(0 != kernelBootTime.tv_sec);

	// Convert kernel boot time into a WorldTime structure.
	TimeValue timeValue;
	timeValue.tv_sec = (Int64)kernelBootTime.tv_sec;
	timeValue.tv_usec = (Int32)kernelBootTime.tv_usec;
	WorldTime const kernelBootWorldTime(WorldTime::FromTimeValue(timeValue));

	// Compute the delta - because kernel boot time will be adjusted
	// by the same amount as current wall clock time if the user
	// adjusts their clock, the delta between the two is stable
	// (and increases monotonically).
	return (now - kernelBootWorldTime);
}

/**
 * Query and return system uptime in milliseconds.
 */
static inline Int64 IOSGetUptimeInMilliseconds()
{
	TimeInterval const interval = IOSGetSystemUptime();
	return interval.GetMicroseconds() / (Int64)1000;
}

/**
 * @return The hardware name of the current iOS device.
 */
String IOSEngine::GetHardwareName()
{
	// Special handling in the simulator.
#if TARGET_IPHONE_SIMULATOR // TARGET_IPHONE_SIMULATOR is always defined in XCode, will be 0 or 1.  See TargetConditionals.h
	return String(getenv("SIMULATOR_MODEL_IDENTIFIER"));
#else // !TARGET_IPHONE_SIMULATOR
	size_t zHardwareNameSizeInBytes = 0u;
	if (0 != sysctlbyname("hw.machine", nullptr, &zHardwareNameSizeInBytes, nullptr, 0) ||
		0u == zHardwareNameSizeInBytes)
	{
		// Any error, return the empty String.
		return String();
	}

	Vector<Byte> vHardwareName((UInt32)zHardwareNameSizeInBytes);
	if (0 != sysctlbyname("hw.machine", vHardwareName.Get(0u), &zHardwareNameSizeInBytes, nullptr, 0))
	{
		// Any error, return the empty String.
		return String();
	}
	vHardwareName[vHardwareName.GetSize() - 1u] = '\0';

	return String(vHardwareName.Data());
#endif // /!TARGET_IPHONE_SIMULATOR
}

/**
 * Implementation of PPI for iPhone. Unfortunately, we
 * need to implement this as a table query, there is no
 * system API.
 *
 * From: http://stackoverflow.com/a/6505830
 */
Bool IOSEngine::InternalGetScreenPPI(Vector2D& rvPPI) const
{
	using namespace IOSHardwareId;

	switch (GetHardwareId())
	{
			/////////////////////////////
			// iPhones and iPods
			/////////////////////////////
		case kIPhone1G: // fall-through
		case kIPhone3G: // fall-through
		case kIPhone3GS: // fall-through
		case kIPod1G: // fall-through
		case kIPod2G: // fall-through
		case kIPod3G: // fall-through
			rvPPI = Vector2D(163.0f);
			return true;

		case kIPhone4: // fall-through
		case kIPhone4S: // fall-through
		case kIPhone5: // fall-through
		case kIPhone5s: // fall-through
		case kIPhone6: // fall-through
		case kIPhone6s: // fall-through
		case kIPhone7: // fall-through
		case kIPhoneSE: // fall-through
		case kIPhone8: // fall-through
		case kIPhoneXR: // fall-through
		case kIPhone11: // fall-through
		case kIPod4G: // fall-through
		case kIPod5G: // fall-through
		case kIPod6G: // fall-through
		case kIPod7G:
			rvPPI = Vector2D(326.0f);
			return true;

		case kIPhone6Plus: // fall-through
		case kIPhone6sPlus: // fall-through
		case kIPhone7Plus: // fall-through
		case kIPhone8Plus: // fall-through
			rvPPI = Vector2D(401.0f);
			return true;

		case kIPhoneX: // fall-through
		case kIPhoneXS: // fall-through
		case kIPhoneXSMax: // fall-through
		case kIPhone11Pro: // fall-through
		case kIPhone11ProMax:
			rvPPI = Vector2D(458.0f);
			return true;

			/////////////////////////////
			// iPads
			/////////////////////////////
		case kIPad1G: // fall-through
		case kIPad2G:
			rvPPI = Vector2D(132.0f);
			return true;

		case kIPad3G: // fall-through
		case kIPad4G: // fall-through
		case kIPadAir: // fall-through
		case kIPadAir2: // fall-through
		case kIPadPro9point7inch: // fall-through
		case kIPadPro12point9inch: // fall-through
		case kIPad2017: // fall-through
		case kIPadPro2G: // fall-through
		case kIPadPro10point5inch: // fall-through
		case kIPad6G: // fall-through
		case kIPad7G: // fall-through
		case kIPadPro11inch3G: // fall-through
		case kIPadPro12point9inch3G: // fall-through
		case kIPadAir3:
			rvPPI = Vector2D(264.0f);
			return true;

		case kIPadMini:
			rvPPI = Vector2D(163.0f);
			return true;

		case kIPadMini2: // fall-through
		case kIPadMini3: // fall-through
		case kIPadMini4: // fall-through
		case kIPadMini5:
			rvPPI = Vector2D(326.0f);
			return true;

			/////////////////////////////
			// Apple Watches
			/////////////////////////////
		case kAppleWatch38mm: // fall-through
		case kAppleWatch42mm: // fall-through
		case kAppleWatch38mmSeries1: // fall-through
		case kAppleWatch42mmSeries1: // fall-through
		case kAppleWatch38mmSeries2: // fall-through
		case kAppleWatch42mmSeries2: // fall-through
		case kAppleWatch38mmSeries3: // fall-through
		case kAppleWatch42mmSeries3: // fall-through
		case kAppleWatch40mmSeries4: // fall-through
		case kAppleWatch44mmSeries4: // fall-through
		case kAppleWatch40mmSeries5: // fall-through
		case kAppleWatch44mmSeries5:
			rvPPI = Vector2D(326.0f);
			return true;

		case kUnknown:
			return false;
	};
}

/**
 * @return The hardware id of the current iOS device.
 */
IOSHardwareId IOSEngine::GetHardwareId()
{
	using namespace IOSHardwareId;

	// list of known device IDs and their corresponding enum value
	//
	// See also: https://gist.github.com/adamawolf/3048717
	static HardwareStringToHardwareId kaKnownHardwareIds[] =
	{
		// iPhone
		{ "iPhone1,1", kIPhone1G },
		{ "iPhone1,2", kIPhone3G },
		{ "iPhone2,1", kIPhone3GS },
		{ "iPhone3,1", kIPhone4 },
		{ "iPhone3,2", kIPhone4 },
		{ "iPhone3,3", kIPhone4 },
		{ "iPhone4,1", kIPhone4S },
		{ "iPhone4,2", kIPhone4S },
		{ "iPhone4,3", kIPhone4S },
		{ "iPhone5,1", kIPhone5 },
		{ "iPhone5,2", kIPhone5 },
		{ "iPhone5,3", kIPhone5 },
		{ "iPhone5,4", kIPhone5 },
		{ "iPhone6,1", kIPhone5s },
		{ "iPhone6,2", kIPhone5s },
		{ "iPhone7,1", kIPhone6Plus },
		{ "iPhone7,2", kIPhone6 },
		{ "iPhone8,1", kIPhone6s },
		{ "iPhone8,2", kIPhone6sPlus },
		{ "iPhone8,4", kIPhoneSE },
		{ "iPhone9,1", kIPhone7 },
		{ "iPhone9,2", kIPhone7Plus },
		{ "iPhone9,3", kIPhone7 },
		{ "iPhone9,4", kIPhone7Plus },
		{ "iPhone10,1", kIPhone8 },
		{ "iPhone10,2", kIPhone8Plus },
		{ "iPhone10,3", kIPhoneX },
		{ "iPhone10,4", kIPhone8 },
		{ "iPhone10,5", kIPhone8Plus },
		{ "iPhone10,6", kIPhoneX },
		{ "iPhone11,2", kIPhoneXS },
		{ "iPhone11,4", kIPhoneXSMax },
		{ "iPhone11,6", kIPhoneXSMax },
		{ "iPhone11,8", kIPhoneXR },
		{ "iPhone12,1", kIPhone11 },
		{ "iPhone12,3", kIPhone11Pro },
		{ "iPhone12,5", kIPhone11ProMax },

		// iPod
		{ "iPod1,1", kIPod1G },
		{ "iPod2,1", kIPod2G },
		{ "iPod2,2", kIPod2G },
		{ "iPod3,1", kIPod3G },
		{ "iPod4,1", kIPod4G },
		{ "iPod5,1", kIPod5G },
		{ "iPod7,1", kIPod6G },
		{ "iPod9,1", kIPod7G },

		// iPad
		{ "iPad1,1", kIPad1G },
		{ "iPad1,2", kIPad3G },
		{ "iPad2,1", kIPad2G },
		{ "iPad2,2", kIPad2G },
		{ "iPad2,3", kIPad2G },
		{ "iPad2,4", kIPad2G },
		{ "iPad2,5", kIPadMini },
		{ "iPad2,6", kIPadMini },
		{ "iPad2,7", kIPadMini },
		{ "iPad3,1", kIPad3G },
		{ "iPad3,2", kIPad3G },
		{ "iPad3,3", kIPad3G },
		{ "iPad3,4", kIPad4G },
		{ "iPad3,5", kIPad4G },
		{ "iPad3,6", kIPad4G },
		{ "iPad4,1", kIPadAir },
		{ "iPad4,2", kIPadAir },
		{ "iPad4,3", kIPadAir },
		{ "iPad4,4", kIPadMini2 },
		{ "iPad4,5", kIPadMini2 },
		{ "iPad4,6", kIPadMini2 },
		{ "iPad4,7", kIPadMini3 },
		{ "iPad4,8", kIPadMini3 },
		{ "iPad4,9", kIPadMini3 },
		{ "iPad5,1", kIPadMini4 },
		{ "iPad5,2", kIPadMini4 },
		{ "iPad5,3", kIPadAir2 },
		{ "iPad5,4", kIPadAir2 },
		{ "iPad5,5", kIPadAir2 },
		{ "iPad6,3", kIPadPro9point7inch },
		{ "iPad6,4", kIPadPro9point7inch },
		{ "iPad6,7", kIPadPro12point9inch },
		{ "iPad6,8", kIPadPro12point9inch },
		{ "iPad6,11", kIPad2017 },
		{ "iPad6,12", kIPad2017 },
		{ "iPad7,1", kIPadPro2G },
		{ "iPad7,2", kIPadPro2G },
		{ "iPad7,3", kIPadPro10point5inch },
		{ "iPad7,4", kIPadPro10point5inch },
		{ "iPad7,5", kIPad6G },
		{ "iPad7,6", kIPad6G },
		{ "iPad7,11", kIPad7G },
		{ "iPad7,12", kIPad7G },
		{ "iPad8,1", kIPadPro11inch3G },
		{ "iPad8,2", kIPadPro11inch3G },
		{ "iPad8,3", kIPadPro11inch3G },
		{ "iPad8,4", kIPadPro11inch3G },
		{ "iPad8,5", kIPadPro12point9inch3G },
		{ "iPad8,6", kIPadPro12point9inch3G },
		{ "iPad8,7", kIPadPro12point9inch3G },
		{ "iPad8,8", kIPadPro12point9inch3G },
		{ "iPad11,1", kIPadMini5 },
		{ "iPad11,2", kIPadMini5 },
		{ "iPad11,3", kIPadAir3 },
		{ "iPad11,4", kIPadAir3 },
		{ "Watch1,1", kAppleWatch38mm },
		{ "Watch1,2", kAppleWatch42mm },
		{ "Watch2,6", kAppleWatch38mmSeries1 },
		{ "Watch2,7", kAppleWatch42mmSeries1 },
		{ "Watch2,3", kAppleWatch38mmSeries2 },
		{ "Watch2,4", kAppleWatch42mmSeries2 },
		{ "Watch3,1", kAppleWatch38mmSeries3 },
		{ "Watch3,2", kAppleWatch42mmSeries3 },
		{ "Watch3,3", kAppleWatch38mmSeries3 },
		{ "Watch3,4", kAppleWatch42mmSeries3 },
		{ "Watch4,1", kAppleWatch40mmSeries4 },
		{ "Watch4,2", kAppleWatch44mmSeries4 },
		{ "Watch4,3", kAppleWatch40mmSeries4 },
		{ "Watch4,4", kAppleWatch44mmSeries4 },
		{ "Watch5,1", kAppleWatch40mmSeries5 },
		{ "Watch5,2", kAppleWatch44mmSeries5 },
		{ "Watch5,3", kAppleWatch40mmSeries5 },
		{ "Watch5,4", kAppleWatch44mmSeries5 },
	};

	String const sHardwareName = GetHardwareName();
	if (sHardwareName.IsEmpty())
	{
		return kUnknown;
	}

	IOSHardwareId eCurrentHardware = kUnknown;
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(kaKnownHardwareIds); ++i)
	{
		if (0 == strcmp(kaKnownHardwareIds[i].m_sName, sHardwareName.CStr()))
		{
			eCurrentHardware = kaKnownHardwareIds[i].m_eId;
			break;
		}
	}

	return eCurrentHardware;
}

/**
 * @return True if the current hardware meets minimum hardware requirements as specifieid in settings.
 *
 * This function errs on the side of returning true if the current hardware is unknown.
 */
static Bool InternalMeetsMinimumHardwareRequirements(const IOSEngineSettings& settings)
{
	using namespace IOSHardwareId;

	IOSHardwareId eCurrentHardware = IOSEngine::GetHardwareId();

	// Unknown hardware, always return true.
	if (kUnknown == eCurrentHardware)
	{
		return true;
	}
	// Otherwise, determine if the current device is >= the minimum device for its category.
	else
	{
		// iPad.
		if (eCurrentHardware >= MIN_IPAD && eCurrentHardware <= MAX_IPAD)
		{
			// True if the current hardware is >= the minimum iPad hardware rev. specified.
			return (eCurrentHardware >= settings.m_eMinimumIPad);
		}
		// iPhone.
		else if (eCurrentHardware >= MIN_IPHONE && eCurrentHardware <= MAX_IPHONE)
		{
			// True if the current hardware is >= the minimum iPhone hardware rev. specified.
			return (eCurrentHardware >= settings.m_eMinimumIPhone);
		}
		// iPod.
		else if (eCurrentHardware >= MIN_IPOD && eCurrentHardware <= MAX_IPOD)
		{
			// True if the current hardware is >= the minimum iPod hardware rev. specified.
			return (eCurrentHardware >= settings.m_eMinimumIPod);
		}
		// else case should never happen, indicates a mismatch of this logic with the enum.
		else
		{
			SEOUL_FAIL("Mismatched iOS hardware enum.");
			return true;
		}
	}
}

IOSEngine::IOSEngine(const IOSEngineSettings& settings)
	: Engine()
	, m_Mutex()
	, m_pOGLES2RenderDevice()
	, m_LastBatteryLevelCheckWorldTime()
	, m_fBatteryLevel(-1.0f)
	, m_Settings(settings)
	, m_bHasFocus(true)
	, m_bMeetsMinimumHardwareRequirements(InternalMeetsMinimumHardwareRequirements(settings))
	, m_bIsSandboxEnvironment(IOSIsSandboxEnvironment())
#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	, m_bEnabledRemoteNotifications(false)
#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS
{
	// TODO: Max here is just a safety, given our method
	// of measuring a monotonic clock unaffected by sleep on iOS
	// is a bit complex. Should never get a negative value from
	// IOSGetUptimeInMilliseconds().
	m_iStartUptimeInMilliseconds = Max(IOSGetUptimeInMilliseconds(), (Int64)0);
	m_iUptimeInMilliseconds = m_iStartUptimeInMilliseconds;
}

IOSEngine::~IOSEngine()
{
}

void IOSEngine::RefreshUptime()
{
	auto const iNewUptimeInMilliseconds = IOSGetUptimeInMilliseconds();

	// TODO: Max here is just a safety, given our method
	// of measuring a monotonic clock unaffected by sleep on iOS
	// is a bit complex. It should never happen that the computed
	// uptime value goes backward.

	// Update the uptime.
	Lock lock(*m_pUptimeMutex);
	m_iUptimeInMilliseconds = Max(iNewUptimeInMilliseconds, (Int64)m_iUptimeInMilliseconds);
}

Bool IOSEngine::MeetsMinimumHardwareRequirements() const
{
	return m_bMeetsMinimumHardwareRequirements;
}

void IOSEngine::Initialize()
{
	InternalEnableBatteryMonitoring();

	// Set executable name from the command-line.
	if (g_ArgcIOS > 0)
	{
		SetExecutableName(g_ppArgvIOS[0]);
	}

	InternalPopulatePlatformData();

	InternalSetIsFirstRun();

	Engine::InternalPreRenderDeviceInitialization(
		m_Settings.m_CoreSettings,
		m_Settings.m_SaveLoadManagerSettings);

	InternalGenerateOrRestoreUniqueUserID();

	Jobs::AwaitFunction(
		GetRenderThreadId(),
		SEOUL_BIND_DELEGATE(&IOSEngine::RenderThreadInitializeOGLES2RenderDevice, this));

	Engine::InternalPostRenderDeviceInitialization();

	InternalInitializeIOSInput();

	Engine::InternalPostInitialization();

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	if (Engine::Get()->CanRequestRemoteNotificationsWithoutPrompt())
#endif
	{
		InternalRegisterUserNotificationSettings();
	}
}

void IOSEngine::Shutdown()
{
	Engine::InternalPreShutdown(); SEOUL_TEARDOWN_TRACE();

	InternalShutdownIOSInput(); SEOUL_TEARDOWN_TRACE();

	Engine::InternalPreRenderDeviceShutdown(); SEOUL_TEARDOWN_TRACE();

	Jobs::AwaitFunction(
		GetRenderThreadId(),
		SEOUL_BIND_DELEGATE(&IOSEngine::RenderThreadShutdownOGLES2RenderDevice, this));
	SEOUL_TEARDOWN_TRACE();

	Engine::InternalPostRenderDeviceShutdown(); SEOUL_TEARDOWN_TRACE();

	InternalIOSPostShutdown(); SEOUL_TEARDOWN_TRACE();

	InternalDisableBatteryMonitoring(); SEOUL_TEARDOWN_TRACE();
}

Bool IOSEngine::Tick()
{
	Engine::InternalBeginTick();
	Engine::InternalEndTick();

	return true;
}

void IOSEngine::InternalIOSPostShutdown()
{
}

void IOSEngine::InternalInitializeIOSInput()
{
	IOSInputDeviceEnumerator iosEnumerator;
	InputManager::Get()->EnumerateInputDevices(&iosEnumerator);
}

void IOSEngine::InternalShutdownIOSInput()
{
}

AnalyticsManager* IOSEngine::InternalCreateAnalyticsManager()
{
	return CreateGenericAnalyticsManager(m_Settings.m_AnalyticsSettings);
}

CommerceManager* IOSEngine::InternalCreateCommerceManager()
{
	return SEOUL_NEW(MemoryBudgets::TBD) IOSCommerceManager();
}

PlatformSignInManager* IOSEngine::InternalCreatePlatformSignInManager()
{
#if SEOUL_WITH_GAMECENTER
	// GameCenter only functions reliably in non-enterprise builds.
	if (!m_Settings.m_bEnterpriseBuild)
	{
		return SEOUL_NEW(MemoryBudgets::TBD) IOSGameCenterPlatformSignInManager();
	}
#endif

#if SEOUL_WITH_APPLESIGNIN
	return SEOUL_NEW(MemoryBudgets::TBD) IOSApplePlatformSignInManager();
#endif

	// Fallback.
	return Engine::InternalCreatePlatformSignInManager();
}

Sound::Manager* IOSEngine::InternalCreateSoundManager()
{
#if SEOUL_WITH_FMOD
	return SEOUL_NEW(MemoryBudgets::Audio) FMODSound::Manager;
#else
	return SEOUL_NEW(MemoryBudgets::Audio) Sound::NullManager;
#endif
}

TrackingManager* IOSEngine::InternalCreateTrackingManager()
{
	// Just use the base implementation if tracking is not enabled.
	if (!m_Settings.m_IsTrackingEnabled || !m_Settings.m_IsTrackingEnabled())
	{
		return Engine::InternalCreateTrackingManager();
	}

	return SEOUL_NEW(MemoryBudgets::Analytics) IOSTrackingManager(m_Settings.m_TrackingSettings);
}

void IOSEngine::RenderThreadInitializeOGLES2RenderDevice()
{
	SEOUL_ASSERT(IsRenderThread());

	m_pOGLES2RenderDevice.Reset(SEOUL_NEW(MemoryBudgets::Game) OGLES2RenderDevice(m_Settings.m_pLayer));
}

void IOSEngine::RenderThreadShutdownOGLES2RenderDevice()
{
	SEOUL_ASSERT(IsRenderThread());

	m_pOGLES2RenderDevice.Reset();
}

SaveApi* IOSEngine::CreateSaveApi()
{
	return SEOUL_NEW(MemoryBudgets::Saving) GenericSaveApi();
}

/** Async update of the media source and campaign data. */
void IOSEngine::SetAttributionData(const String& sCampaign, const String& sMediaSource)
{
	{
		// Commit the new data to platform data.
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_sUACampaign = sCampaign;
		m_pPlatformData->m_sUAMediaSource = sMediaSource;
	}
	AnalyticsManager::Get()->SetAttributionData(sCampaign, sMediaSource);
}

} // namespace Seoul
