/**
 * \file IOSEngine.mm
 * \brief Objective-C support for specialization of Engine for the IOS platform.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "JobsFunction.h"
#include "IOSAppDelegate.h"
#include "IOSEngine.h"
#include "IOSFacebookManager.h"
#include "IOSUtil.h"
#include "ITextEditable.h"
#include "LocManager.h"
#include "Logger.h"
#include "PlatformData.h"
#include "SeoulProfiler.h"
#include "ThreadId.h"

#include <mach/mach.h>

#import "IOSKeychain.h"

#import <AdSupport/AdSupport.h>
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <StoreKit/StoreKit.h>
#import <UIKit/UIKit.h>

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
#import <UserNotifications/UserNotifications.h>
#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000

namespace Seoul
{

static NSString* const ksGDPRDefaultsKey = @"GDPR_Compliance_Version";
// GDPR Version for iOS. Note that each platform maintains its own version.
static NSNumber* const ksGDPRVersion = [NSNumber numberWithInt:1];

static NSString* const ksSawRemoteNotificationPrompt = @"RemoteNotifications_Saw_Prompt";

/**
 * Helper function to find a blob of memory within another blob of memory
 */
static const void *memmem(const void *pHaystack, size_t uHaystackLength, const void *pNeedle, size_t uNeedleLength)
{
	// TODO: Move this into Core project, it could be useful for other things

	// Check for trivial edge cases
	if (uNeedleLength == 0)
	{
		return pHaystack;
	}
	else if (uNeedleLength > uHaystackLength)
	{
		return nullptr;
	}

	const char *pCharHaystack = (const char *)pHaystack;
	const char *pCharNeedle = (const char *)pNeedle;
	const char *pSearchEnd = pCharHaystack + (uHaystackLength - uNeedleLength + 1);
	char cFirstChar = pCharNeedle[0];
	pCharNeedle++;
	uNeedleLength--;

	while (pCharHaystack < pSearchEnd)
	{
		// Find a possible starting point for a match
		const char *pPotentialMatch = (const char *)memchr(pCharHaystack, cFirstChar, pSearchEnd - pCharHaystack);
		if (pPotentialMatch == nullptr)
		{
			// No potential matches, we're done
			return nullptr;
		}

		// Check if it's a full match
		if (memcmp(pPotentialMatch + 1, pCharNeedle, uNeedleLength) == 0)
		{
			return pPotentialMatch;
		}

		// Try again for a new match
		pCharHaystack = pPotentialMatch + 1;
	}

	// No matches
	return nullptr;
}

Bool IOSIsSandboxEnvironment()
{
	String const sBundleIdentifier([[NSBundle mainBundle] bundleIdentifier]);
	String const sPlatformAppVersion([[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]);

	// Read in the entitlements file to determine if we're in the sandbox
	// environment or not.  If any of this fails, assume we're in the
	// production environment
	NSString *pEntitlementsPath = [[NSBundle mainBundle] pathForResource:@"embedded.mobileprovision" ofType:nil];
	if (pEntitlementsPath == nil)
	{
		return false;
	}

	NSData *pEntitlementsData = [NSData dataWithContentsOfFile:pEntitlementsPath];
	if (pEntitlementsData == nil)
	{
		return false;
	}

	// Search for the aps-environment key in the data.  We can't use
	// standard string search functions since the entitlements data
	// contains binary at the front and may contain embedded nulls.
	const char *pRawData = (const char *)[pEntitlementsData bytes];
	NSUInteger uLength = [pEntitlementsData length];

	static const char sEnvironmentKeyToken[] = "<key>aps-environment</key>";
	const char *pEnvironmentKey = (const char *)memmem(pRawData, uLength, sEnvironmentKeyToken, sizeof(sEnvironmentKeyToken) - 1);

	if (pEnvironmentKey == nullptr)
	{
		return false;
	}

	// Now find the corresponding value
	static const char sStringValueToken[] = "<string>";

	const char *pSearchStart = pEnvironmentKey + sizeof(sEnvironmentKeyToken) - 1;
	const char *pEnvironmentValue = (const char *)memmem(pSearchStart, uLength - (pSearchStart - pRawData), sStringValueToken, sizeof(sStringValueToken) - 1);
	if (pEnvironmentValue == nullptr)
	{
		return false;
	}

	// Check if we're in the development environment
	static const char sDevelopmentStr[] = "development";

	pEnvironmentValue += sizeof(sStringValueToken) - 1;
	if ((pEnvironmentValue - pRawData) + sizeof(sDevelopmentStr) - 1 <= uLength &&
		memcmp(pEnvironmentValue, sDevelopmentStr, sizeof(sDevelopmentStr) - 1) == 0)
	{
		SEOUL_LOG("Push notification environment is sandbox\n");
		return true;
	}

	return false;
}

/** Lookup install attribution via Facebook, if available. */
static inline String InternalStaticGetFacebookInstallAttribution()
{
	UIPasteboard *pPasteboard = [UIPasteboard pasteboardWithName:@"fb_app_attribution" create:NO];
	if (pPasteboard != nil)
	{
		return String(pPasteboard.string);
	}
	else
	{
		return String();
	}
}

/** Return the system's current default/preferred country code (ISO-3166). */
static inline String InternalStaticGetDefaultCountryCode()
{
	NSLocale* pLocale = [NSLocale currentLocale];
	if (nil != pLocale)
	{
		NSString* pCountryCode = [pLocale objectForKey: NSLocaleCountryCode];
		if (nil != pCountryCode)
		{
			return String(pCountryCode);
		}
	}

	return String();
}

/** Return the system's current default/preferred 2 letter language code. */
static inline String InternalStaticGetDefaultLanguageCodeIso2()
{
	NSArray* pLanguages = [NSLocale preferredLanguages];
	if ([pLanguages count] > 0)
	{
		// This tag is an IETF BCP 47 language tag, such as "en", "pt-BR", or
		// "nan-Hant-TW".  We get the ISO 639-1 language code by taking the
		// first two characters.
		NSString* pLanguageTag = [pLanguages objectAtIndex:0];
		NSString* pLanguageCode = [pLanguageTag substringToIndex:2];
		return String([pLanguageCode UTF8String]);
	}
	else
	{
		return String("en");
	}
}

/**
 * Rudimentary check for root access/jailbroken phone. Not intended
 * to be exhaustive, just barely sufficient for analytics. Can return
 * both false positives and false negatives since it's just searching
 * for the existence of specific files, not actually checking access
 * or permissions.
 */
static inline Bool IsDeviceRooted()
{
	static Byte const* s_kaPathsToCheck[] =
	{
		"/Applications/Cydia.app",
		"/bin/bash",
		"/etc/apt",
		"/private/var/lib/apt/",
	};

	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaPathsToCheck); ++i)
	{
		if (DiskSyncFile::FileExists(s_kaPathsToCheck[i]))
		{
			return true;
		}
	}

	return false;
}

/** Return a value in seconds indicating the system's time zone offset from GMT. */
static inline Int32 GetTimeZoneOffsetInSeconds()
{
	NSTimeZone* pTimeZone = [NSTimeZone defaultTimeZone];
	return (Int32)pTimeZone.secondsFromGMT;
}

/**
 * Fill out our platform data structure with
 * the bulk of data. Some members (PlatformUUID
 * and first run) will be filled in aftewards.
 */
void IOSEngine::InternalPopulatePlatformData()
{
	PlatformData data;
	data.m_sAdvertisingId = String([[ASIdentifierManager sharedManager].advertisingIdentifier UUIDString]);
	// TODO: data.m_iAppVersionCode = Java::Invoke<Int>(pEnv, pClass, "GetAppVersionCode", "()I");
	data.m_sAppVersionName = String([[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]);
	data.m_sCountryCode = InternalStaticGetDefaultCountryCode();
	data.m_sDeviceManufacturer = "apple";
	data.m_sDeviceModel = IOSEngine::GetHardwareName();
	// TODO: data.m_sDeviceNetworkCountryCode = Java::Invoke<String>(pEnv, pClass, "GetDeviceNetworkCountryCode", "()Ljava/lang/String;");
	// TODO: data.m_sDeviceNetworkOperatorName = Java::Invoke<String>(pEnv, pClass, "GetDeviceNetworkOperatorName", "()Ljava/lang/String;");
	data.m_sDevicePlatformName = String([[UIDevice currentDevice] model]);
	// TODO: data.m_sDeviceSimCountryCode = Java::Invoke<String>(pEnv, pClass, "GetDeviceSimCountryCode", "()Ljava/lang/String;");
	data.m_sFacebookInstallAttribution = InternalStaticGetFacebookInstallAttribution();
	data.m_sLanguageCodeIso2 = InternalStaticGetDefaultLanguageCodeIso2();
	// TODO: data.m_sLanguageCodeIso3 = Java::Invoke<String>(pEnv, pClass, "GetLanguageIso3Code", "()Ljava/lang/String;");
	data.m_sOsName = String([UIDevice currentDevice].systemName);
	data.m_sOsVersion = String([UIDevice currentDevice].systemVersion);
	data.m_sPackageName = String([[NSBundle mainBundle] bundleIdentifier]);
	data.m_bRooted = IsDeviceRooted();
	(void)InternalGetScreenPPI(data.m_vScreenPPI);
	data.m_iTargetApiOrSdkVersion = (Int32)__IPHONE_OS_VERSION_MIN_REQUIRED;
	data.m_iTimeZoneOffsetInSeconds = GetTimeZoneOffsetInSeconds();
	data.m_bEnableAdTracking = [ASIdentifierManager sharedManager].advertisingTrackingEnabled;

	// All done, fill in the platform data.
	{
		Lock lock(*m_pPlatformDataMutex);
		*m_pPlatformData = data;
	}
}

/**
 * Separately register user notification
 * settings, so we can use local notifications, even if we have
 * not registered for remote notifications.
 */
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000 // Support for projects targeting less than iOS 10.0.
static void NotificationAuthorizationCallback()
{
	Bool bAllowNotifications = [[UIApplication sharedApplication] isRegisteredForRemoteNotifications];

	// Inform the game that the user has changed remote notification settings.
	IOSEngine::Get()->OnRegisteredUserNotificationSettings(bAllowNotifications);
}
#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
void IOSEngine::InternalRegisterUserNotificationSettings()
{
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
	[[UNUserNotificationCenter currentNotificationCenter]
	 requestAuthorizationWithOptions:(UNAuthorizationOptionAlert |
									  UNAuthorizationOptionBadge |
									  UNAuthorizationOptionSound)
	 completionHandler:
	 ^(BOOL granted, NSError* _Nullable error)
	 {
		// NOTE:: Ignoring the error and granted here is a bit strange, but supposedly this
		// delegate calls a system function (isRegisteredForRemoteNotifications) which checks whether the
		// user has given permissions to use notifications.
		// I'm pulling this over from a different callback location while updating a deprecated
		// API, and do not currently have the bandwidth to investigate further, so I will leave as
		// is. Push notification code becomes a bit messy further up the stack. Someone else
		// working on push notifications functionality in the future should determine whether
		// this is the right call to make here.
		Jobs::AsyncFunction(GetMainThreadId(), &NotificationAuthorizationCallback);
	 }
	];
#else
	// Setup the notification types to register for.
	UIUserNotificationType eTypes = UIUserNotificationTypeAlert;
	eTypes |= UIUserNotificationTypeBadge;
	eTypes |= UIUserNotificationTypeSound;

	UIUserNotificationSettings* pNotificationSettings = [UIUserNotificationSettings settingsForTypes:eTypes categories:nil];
	[[UIApplication sharedApplication] registerUserNotificationSettings:pNotificationSettings];
#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
}

void IOSEngine::InternalEnableBatteryMonitoring()
{
	[[UIDevice currentDevice] setBatteryMonitoringEnabled:TRUE];
}

void IOSEngine::InternalDisableBatteryMonitoring()
{
	[[UIDevice currentDevice] setBatteryMonitoringEnabled:FALSE];
}

Bool IOSEngine::QueryBatteryLevel(Float& rfBatteryLevel)
{
	// Don't poll for battery level too frequently.
	static const Double kfBatteryLevelPollIntervalSeconds = 15.0;

	WorldTime const currentWorldTime = WorldTime::GetUTCTime();
	if (m_LastBatteryLevelCheckWorldTime == WorldTime() ||
		(currentWorldTime - m_LastBatteryLevelCheckWorldTime).GetSeconds() > kfBatteryLevelPollIntervalSeconds)
	{
		m_LastBatteryLevelCheckWorldTime = currentWorldTime;
		m_fBatteryLevel = Clamp([[UIDevice currentDevice] batteryLevel], 0.0f, 1.0f);
	}

	if (m_fBatteryLevel >= 0.0f)
	{
		rfBatteryLevel = m_fBatteryLevel;
		return true;
	}

	return false;
}

Bool IOSEngine::QueryProcessMemoryUsage(size_t& rzWorkingSetBytes, size_t& rzPrivateBytes) const
{
	struct task_basic_info info;
	memset(&info, 0, sizeof(info));

	mach_msg_type_number_t size = sizeof(info);
	kern_return_t const result = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);

	if (KERN_SUCCESS == result)
	{
		rzWorkingSetBytes = (size_t)info.resident_size;
		rzPrivateBytes = (size_t)info.virtual_size;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Shows the iOS App Store to allow the user to rate this app
 */
void IOSEngine::ShowAppStoreToRateThisApp()
{
	// If available, use the in-app rate me dialogue.
	if ([SKStoreReviewController class] != nil)
	{
		[SKStoreReviewController requestReview];
	}
	// Otherwise, fallback to launching a URL.
	else
	{
		static NSString* const sUrlFormat = @"itms-apps://itunes.apple.com/app/id%@?action=write-review";
		NSString* pSite = [NSString stringWithFormat:sUrlFormat, m_Settings.m_TrackingSettings.m_sAppleID.ToNSString()];

		// Create the URL and then submit.
#if __IPHONE_OS_VERSION_MIN_REQUIRED < 100000 // Support for projects targeting less than iOS 10.0.
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString:pSite]];
#else
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString:pSite] options:@{} completionHandler:nil];
#endif // /#if __IPHONE_OS_VERSION_MIN_REQUIRED < 100000
	}
}

Bool IOSEngine::DoesSupportInAppRateMe() const
{
	return [SKStoreReviewController class] != nil;
}

/**
 * iOS implementation of OpenURL to open a URL in the user's web browser
 * *TODO: change this to have no return value
 */
Bool IOSEngine::InternalOpenURL(const String& sURL)
{
	NSURL *pURL = [NSURL URLWithString:sURL.ToNSString()];
#if __IPHONE_OS_VERSION_MIN_REQUIRED < 100000 // Support for projects targeting less than iOS 10.0.
	[[UIApplication sharedApplication] openURL:pURL];
#else
	[[UIApplication sharedApplication] openURL:pURL options:@{} completionHandler:nil];
#endif // /#if __IPHONE_OS_VERSION_MIN_REQUIRED < 100000
	return false;
}

/**
 * Gets the user's preferred language from the default locale
 */
String IOSEngine::GetSystemLanguage() const
{
	return LocManager::GetLanguageNameFromCode(InternalStaticGetDefaultLanguageCodeIso2());
}

/**
 * Attempt to retrieve the unique user ID from the keychain, return nullptr
 * if the key is not in the keychain.
 */
static NSString* GetUniqueUserIDFromKeychain(Bool bSaveDeviceIdToAppleCloud)
{
	return [IOSKeychain
			getValueForKey:@"UniqueUserID"
			searchCloud:bSaveDeviceIdToAppleCloud];
}

/**
 * Attempt to put the unique user ID into the keychain.
 */
static void SetUniqueUserIDToKeychain(NSString* pUUID, Bool bSaveDeviceIdToAppleCloud)
{
	(void)[IOSKeychain setKey:@"UniqueUserID"
					  toValue:pUUID
				  isInvisible:false
				  saveToCloud:bSaveDeviceIdToAppleCloud];
}

/** Attempt to commit an updated UUID as the platform UUID. */
Bool IOSEngine::UpdatePlatformUUID(const String& sPlatformUUID)
{
	static NSString *const ksUUIDKey = @"UUID";

	// Don't allow an empty UUID
	if (sPlatformUUID.IsEmpty())
	{
		return false;
	}

	if (sPlatformUUID != GetPlatformUUID())
	{
		// Commit the new UUID to m_Settings.
		{
			Lock lock(*m_pPlatformDataMutex);
			m_pPlatformData->m_sPlatformUUID = sPlatformUUID;
		}
		NSString *pUUID = sPlatformUUID.ToNSString();

		// Commit the UUID to the keychain.
		SetUniqueUserIDToKeychain(pUUID, m_Settings.m_bSaveDeviceIdToAppleCloud);

		// Save the UUID to default storage.
		NSUserDefaults *pDefaults = [NSUserDefaults standardUserDefaults];
		[pDefaults setObject:pUUID forKey:ksUUIDKey];
		[pDefaults synchronize];
	}

	return true;
}

void IOSEngine::InternalGenerateOrRestoreUniqueUserID()
{
	static NSString *const ksUUIDKey = @"UUID";

	// Try to load the UUID from the user defaults settings
	NSUserDefaults *pDefaults = [NSUserDefaults standardUserDefaults];
	NSString *pUUID = [pDefaults objectForKey:ksUUIDKey];
	if (pUUID == nil)
	{
		// Try to get the UUID from the keychain.
		pUUID = GetUniqueUserIDFromKeychain(m_Settings.m_bSaveDeviceIdToAppleCloud);

		// If it still doesn't exist, generate a new UUID.
		if (pUUID == nil)
		{
			// If the setting doesn't exist, get a vendor-specific ID if we're
			// running iOS 6.0 or higher.  This will persist across app reinstalls
			// as long as the user has another app still installed from the same
			// vendor.
			if ([[UIDevice currentDevice] respondsToSelector:@selector(identifierForVendor)])
			{
				pUUID = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
			}

			// Otherwise, for pre-6.0 devices, generate a UUID and store it back
			// into the settings.  This will persist after an iOS 6.0 upgrade,
			// since we check the settings before checking identifierForVendor.
			// Sometimes, even on iOS 6.0+, identifierForVendor can return a string
			// of all 0's due to a bug in iOS, see
			// http://stackoverflow.com/q/12605257 .
			if (pUUID == nil || [pUUID isEqualToString:@"00000000-0000-0000-0000-000000000000"])
			{
				CFUUIDRef uuid = CFUUIDCreate(nullptr);
				SEOUL_ASSERT(uuid != nil);

				pUUID = (NSString *)CFUUIDCreateString(nullptr, uuid);
				[pUUID autorelease];

				CFRelease(uuid);
			}

			// Commit the UUID to the keychain.
			SetUniqueUserIDToKeychain(pUUID, m_Settings.m_bSaveDeviceIdToAppleCloud);
		}

		// Save the UUID to default storage.
		[pDefaults setObject:pUUID forKey:ksUUIDKey];
		[pDefaults synchronize];
	}
	// Always commit the UUID to the keychain in this case - this makes sure that the UUID is "backed up" to the
	// keychain, even for users who have an entry in their user defaults prior to our usage of the keychain.
	else
	{
		SetUniqueUserIDToKeychain(pUUID, m_Settings.m_bSaveDeviceIdToAppleCloud);
	}

	// Cache the UUID
	{
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_sPlatformUUID = String([pUUID UTF8String]);
	}
}

/**
 * Schedules a local notification to be delivered to us by the OS at a later
 * time.  Not supported on all platforms.
 *
 * @param[in] iNotificationID
 *   ID of the notification.  If a pending local notification with the same ID
 *   is alreadys scheduled, that noficiation is canceled and replaced by the
 *   new one.
 *
 * @param[in] fireDate
 *   Date and time at which the local notification should fire
 *
 * @param[in] bIsWallClockTime
 *   If true, the fireDate is interpreted as a time relative to the current
 *   time in wall clock time and is adjusted appropriately if the user's time
 *   zone changes.  If false, the fireDate is interpeted as an absolute time in
 *   UTC and is not affected by time zone changes.
 *
 * @param[in] sLocalizedMessage
 *   Localized message to display when the notification fires
 *
 * @param[in] bHasActionButton
 *   If true, the notification will have an additional action button
 *
 * @param[in] sLocalizedActionButtonText
 *   If bHasActionButton is true, contains the localized text to display on
 *   the additional action button
 *
 * @param[in] sLaunchImageFilePath
 *   If non-empty, contains the path to the launch image to be used instead of
 *   the normal launch image (e.g. Default.png on iOS) when launching the game
 *   from the notification
 *
 * @param[in] sSoundFilePath
 *   If non-empty, contains the path to the sound file to play when the
 *   notification fires
 *
 * @param[in] nIconBadgeNumber
 *   If non-zero, contains the number to display as the application's icon
 *   badge
 *
 * @param[in] userInfo
 *   Arbitrary dictionary of data to pass back to the application when the
 *   notification fires
 *
 * @return
 *   True if the notification was scheduled, or false if local notifications
 *   are not supported on this platform.
 */
void IOSEngine::ScheduleLocalNotification(
	Int iNotificationID,
	const WorldTime& fireDate,
	Bool bIsWallClockTime,
	const String& sLocalizedMessage,
	Bool bHasActionButton,
	const String& sLocalizedActionButtonText,
	const String& sLaunchImageFilePath,
	const String& sSoundFilePath,
	Int nIconBadgeNumber,
	const DataStore& userInfo)
{
	SEOUL_PROF("IOSEngine.ScheduleLocalNotification");

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
	// These options are no longer supported on iOS 10+. Notifications cannot present
	// as system pop-ups with actions, and now, by default, tapping a notification
	// opens the corresponding application.
	(void)bHasActionButton;
	(void)sLocalizedActionButtonText;

	// Set up the content of our notification
	UNMutableNotificationContent* pNotificationContent = [[[UNMutableNotificationContent alloc] init] autorelease];
	pNotificationContent.body = sLocalizedMessage.ToNSString();
	pNotificationContent.badge = [NSNumber numberWithInt:nIconBadgeNumber];
	if (!sLaunchImageFilePath.IsEmpty())
	{
		pNotificationContent.launchImageName = sLaunchImageFilePath.ToNSString();
	}
	if (!sSoundFilePath.IsEmpty())
	{
		pNotificationContent.sound = [UNNotificationSound soundNamed:sSoundFilePath.ToNSString()];
	}
	// Set user data, if any
	DataNode rootNode = userInfo.GetRootNode();
	if (userInfo.TableBegin(rootNode) != userInfo.TableEnd(rootNode)) // Data is a non-empty table.
	{
		NSDictionary* pUserInfoDict = ConvertToNSDictionary(userInfo);
		if (nil != pUserInfoDict)
		{
			pNotificationContent.userInfo = pUserInfoDict;
		}
	}

	// Set up the trigger for our notification
	UNNotificationTrigger* pNotificationTrigger;
	if (bIsWallClockTime)
	{
		// Create our date components using UTC time zone
		NSCalendar *pGregorianCalendar = [[[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian] autorelease];
		NSDateComponents* pDateComponents =
			[pGregorianCalendar componentsInTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0]
											fromDate:[NSDate
													  dateWithTimeIntervalSince1970:
													  (NSTimeInterval)fireDate.GetSeconds()]];

		// Assign our calendar based notification trigger
		pNotificationTrigger = [UNCalendarNotificationTrigger
								triggerWithDateMatchingComponents:pDateComponents repeats:NO];
	}
	else
	{
		// In order to treat the given time as an absolute time (not a "wall clock" time),
		// find the interval between the scheduled notification time and now, and use
		// that as the interval for a countdown trigger.
		NSTimeInterval interval = [[NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)fireDate.GetSeconds()]
								   timeIntervalSinceDate:
								   [NSDate date]];
		pNotificationTrigger = [UNTimeIntervalNotificationTrigger
								triggerWithTimeInterval:interval repeats:NO];
	}

	// Create the notification request
	UNNotificationRequest* pNotificationRequest =
		[UNNotificationRequest requestWithIdentifier:[NSNumber numberWithInt:iNotificationID].stringValue
											 content:pNotificationContent
											 trigger:pNotificationTrigger];

#if SEOUL_LOGGING_ENABLED
	// Copied onto the stack so the block can access it.
	Int64 uFireDateSecondsCopy = fireDate.GetSeconds();
#endif // /SEOUL_LOGGING_ENABLED

	// Run on the main thread
	dispatch_async(dispatch_get_main_queue(), ^{
		// Cancel existing notification with the given ID, if any
		InternalCancelLocalNotification(iNotificationID);

		SEOUL_LOG("Scheduling local notification for %s: ID=%d\n",
				  [[[NSDate dateWithTimeIntervalSince1970: (NSTimeInterval)uFireDateSecondsCopy] description]
				   UTF8String], iNotificationID);

		// Schedule the local notification.
		[[UNUserNotificationCenter currentNotificationCenter]
		 addNotificationRequest:pNotificationRequest
		 withCompletionHandler:^(NSError* error)
		{
			if (error != nil)
			{
				SEOUL_WARN("ScheduleLocalNotification: IOS failed to schedule a local notification because of error %s",
						   [[error localizedDescription] UTF8String]);
			}
		}];
	});
#else
	UILocalNotification *pNotification = [[[UILocalNotification alloc] init] autorelease];
	pNotification.fireDate = [NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)fireDate.GetSeconds()];

	// For absolute time, keep the time zone as nil.  For wall clock time, set
	// the time zone to UTC, since interally our WorldTime object is already in
	// UTC.
	if (bIsWallClockTime)
	{
		pNotification.timeZone = [NSTimeZone timeZoneForSecondsFromGMT:0];
	}

	// Don't repeat
	pNotification.repeatInterval = (NSCalendarUnit)0;
	pNotification.repeatCalendar = nil;

	// Set alert body and action
	pNotification.alertBody = sLocalizedMessage.ToNSString();

	pNotification.hasAction = bHasActionButton ? YES : NO;
	if (bHasActionButton)
	{
		pNotification.alertAction = sLocalizedActionButtonText.ToNSString();
	}

	// Set launch image file path, sound file path, and badge number
	if (!sLaunchImageFilePath.IsEmpty())
	{
		pNotification.alertLaunchImage = sLaunchImageFilePath.ToNSString();
	}

	if (!sSoundFilePath.IsEmpty())
	{
		pNotification.soundName = sSoundFilePath.ToNSString();
	}

	pNotification.applicationIconBadgeNumber = nIconBadgeNumber;

	// Set user data, if any
	Bool bHasUserData = false;
	DataNode rootNode = userInfo.GetRootNode();
	if (userInfo.TableBegin(rootNode) != userInfo.TableEnd(rootNode)) // Data is a non-empty table.
	{
		NSDictionary* pUserInfoDict = ConvertToNSDictionary(userInfo);
		if (nil != pUserInfoDict)
		{
			pNotification.userInfo =
				[NSDictionary dictionaryWithObjectsAndKeys:
							  [NSNumber numberWithInt:iNotificationID],
							  @"id",
							  pUserInfoDict,
							  @"user_info",
							  (id)nil];
			bHasUserData = true;
		}
	}

	if (!bHasUserData)
	{
		pNotification.userInfo =
			[NSDictionary dictionaryWithObjectsAndKeys:
						  [NSNumber numberWithInt:iNotificationID],
						  @"id",
						  (id)nil];
	}

	// Run on the main thread
	dispatch_async(dispatch_get_main_queue(), ^{
		// Cancel existing notification with the given ID, if any
		InternalCancelLocalNotification(iNotificationID);

		SEOUL_LOG("Scheduling local notification for %s: ID=%d\n",
			[[pNotification.fireDate description] UTF8String], iNotificationID);

		// Schedule the notification
		[[UIApplication sharedApplication] scheduleLocalNotification:pNotification];
	});
#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
}

/**
 * Cancels all currently scheduled local notifications.  Not supported on all
 * platforms.
 */
void IOSEngine::CancelAllLocalNotifications()
{
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
	// Run on the main thread
	dispatch_async(dispatch_get_main_queue(), ^{
		SEOUL_LOG("Canceling all scheduled local notifications\n");
		[[UNUserNotificationCenter currentNotificationCenter] removeAllPendingNotificationRequests];
	});
#else
	// Run on the main thread
	dispatch_async(dispatch_get_main_queue(), ^{
		SEOUL_LOG("Canceling all scheduled local notifications\n");
		[[UIApplication sharedApplication] cancelAllLocalNotifications];
	});
#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
}

/**
 * Cancels all local notifications with the given ID.  Not
 * supported on all platforms.
 */
void IOSEngine::CancelLocalNotification(Int iNotificationID)
{
	// Run on the main thread
	dispatch_async(dispatch_get_main_queue(), ^{
		InternalCancelLocalNotification(iNotificationID);
	});
}

/**
 * Cancels all local notifications with the given ID.  Must run on the
 * main thread.
 */
void IOSEngine::InternalCancelLocalNotification(Int iNotificationID)
{
	SEOUL_ASSERT(IsMainThread());

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
	SEOUL_LOG("Canceling local notification with ID %d\n", iNotificationID);
	[[UNUserNotificationCenter currentNotificationCenter]
	 removePendingNotificationRequestsWithIdentifiers:@[[NSNumber numberWithInt:iNotificationID].stringValue]];
#else
	UIApplication *pApplication = [UIApplication sharedApplication];
	// Find the currently scheduled local notification with the given ID
	for (UILocalNotification *pNotification in pApplication.scheduledLocalNotifications)
	{
		if (pNotification.userInfo != nil)
		{
			id pIDObject = [pNotification.userInfo objectForKey:@"id"];
			if ([pIDObject isKindOfClass:[NSNumber class]])
			{
				Int iID = [(NSNumber *)pIDObject intValue];
				if (iID == iNotificationID)
				{
					SEOUL_LOG("Canceling local notification with ID %d\n", iNotificationID);
					[pApplication cancelLocalNotification:pNotification];
				}
			}
		}
	}
#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
}

#if SEOUL_WITH_REMOTE_NOTIFICATIONS

/**
 * Asynchronously registers this device to receive remote notifications.  Not
 * supported on all platforms.  When registration completes, updates Engine::SetRemoteNotificationToken.
 */
void IOSEngine::RegisterForRemoteNotifications()
{
	// TODO: No way to dismiss system dialogs on iOS
	// unless we run inside XCUITest, which is "flakey", as
	// a surprising number of device testing documents put it.
#if SEOUL_AUTO_TESTS
	if (g_bRunningAutomatedTests)
	{
		return;
	}
#endif // /SEOUL_AUTO_TESTS

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000 // Support for projects targeting less than iOS 10.0.
	[[UNUserNotificationCenter currentNotificationCenter]
	 requestAuthorizationWithOptions:(UNAuthorizationOptionAlert)
	 completionHandler:
	 ^(BOOL granted, NSError* _Nullable error)
	 {
		// NOTE:: Ignoring the error and granted here is a bit strange, but supposedly this
		// delegate calls a system function (isRegisteredForRemoteNotifications) which checks whether the
		// user has given permissions to use notifications.
		// I'm pulling this over from a different callback location while updating a deprecated
		// API, and do not currently have the bandwidth to investigate further, so I will leave as
		// is. Push notification code becomes a bit messy further up the stack. Someone else
		// working on push notifications functionality in the future should determine whether
		// this is the right call to make here.
		Jobs::AsyncFunction(GetMainThreadId(), &NotificationAuthorizationCallback);
	 }
	];
#else
	// Setup the notification types to register for.
	UIUserNotificationType eTypes = UIUserNotificationTypeAlert;
	UIUserNotificationSettings* pNotificationSettings = [UIUserNotificationSettings settingsForTypes:eTypes categories:nil];
	[[UIApplication sharedApplication] registerUserNotificationSettings:pNotificationSettings];
#endif // __IPHONE_OS_VERSION_MIN_REQUIRED

	// Register for remote notifications
	[[UIApplication sharedApplication] registerForRemoteNotifications];
}

/**
 * Called from the app delegate after registering for remote notifications
 */
void IOSEngine::OnRegisteredForRemoteNotifications(Bool bSuccess, const String& sDeviceToken)
{
	m_bEnabledRemoteNotifications = bSuccess || m_bEnabledRemoteNotifications;

	if (bSuccess)
	{
		SetRemoteNotificationToken(sDeviceToken);
	}
}

/**
 * Called from the app delegate after getting (or not getting) permission from the user to display remote notifications
 */
void IOSEngine::OnRegisteredUserNotificationSettings(Bool bAllowNotifications)
{
	m_bEnabledRemoteNotifications = bAllowNotifications;

	OnDisplayRemoteNotificationToken(bAllowNotifications);
}

Bool IOSEngine::IsRemoteNotificationDevelopmentEnvironment() const
{
	return m_bIsSandboxEnvironment;
}

#endif // /SEOUL_WITH_REMOTE_NOTIFICATIONS

FacebookManager *IOSEngine::InternalCreateFacebookManager()
{
	// TODO: I'd prefer to leave IOSFacebookManager enabled
	// so we can test the implementation and API, but the methods
	// available on iOS for interacting with system dialoguse require
	// using a separate process running XCUITest, which itself has
	// proven to be unstable.
#if SEOUL_AUTO_TESTS
	// TODO: I'd really rather show the helpshift popup,
	// but I also don't want to integrate all the iOS infrastructure
	// necessary to dismiss arbitrary system and third party popups.
	if (g_bRunningAutomatedTests)
	{
		return SEOUL_NEW(MemoryBudgets::TBD) NullFacebookManager();
	}
#endif // /SEOUL_AUTO_TESTS

#if SEOUL_WITH_FACEBOOK
	return SEOUL_NEW(MemoryBudgets::TBD) IOSFacebookManager();
#else
	return SEOUL_NEW(MemoryBudgets::TBD) NullFacebookManager();
#endif
}

/**
 * Initializes the value of our flag indicating if this is the first time the
 * user has launched the app since it was installed
 */
void IOSEngine::InternalSetIsFirstRun()
{
	static NSString *const ksInstalledKey = @"installed";

	// Try to load the flag from the user defaults database
	NSUserDefaults *pDefaults = [NSUserDefaults standardUserDefaults];

	BOOL bWasInstalled = [pDefaults boolForKey:ksInstalledKey];
	if (bWasInstalled)
	{
		// Key was present and set to YES, so this is not the first run
		Lock lock(*m_pPlatformDataMutex);
		m_pPlatformData->m_bFirstRunAfterInstallation = false;
	}
	else
	{
		{
			Lock lock(*m_pPlatformDataMutex);

			// If key was not present, insert it into the defaults database and
			// indicate that this is the first run
			m_pPlatformData->m_bFirstRunAfterInstallation = true;
		}

		[pDefaults setBool:YES forKey:ksInstalledKey];
		[pDefaults synchronize];
	}
}

/**
 * Shows the virtual keyboard
 */
void IOSEngine::InternalStartTextEditing(ITextEditable* pTextEditable, const String& sText, const String& sDescription, const StringConstraints& constraints, Bool allowNonLatinKeyboard)
{
	m_EditTextSettings.m_sText = sText;
	m_EditTextSettings.m_sDescription = sDescription;
	m_EditTextSettings.m_Constraints = constraints;
	m_EditTextSettings.m_bAllowNonLatinKeyboard = allowNonLatinKeyboard;

	// Run on the main thread
	dispatch_async(dispatch_get_main_queue(), ^{
		IOSAppDelegate *pAppDelegate = (IOSAppDelegate*)[UIApplication sharedApplication].delegate;
		[pAppDelegate showInputEditTextView];
	});
}

/**
 * Hides the virtual keyboard
 */
void IOSEngine::InternalStopTextEditing()
{
	// Run on main thread
	dispatch_async(dispatch_get_main_queue(), ^{
		IOSAppDelegate *pAppDelegate = (IOSAppDelegate*)[UIApplication sharedApplication].delegate;
		[pAppDelegate hideInputEditTextView];
	});
}

// GDPR Compliance
void IOSEngine::SetGDPRAccepted(Bool bAccepted)
{
    NSUserDefaults *pDefaults = [NSUserDefaults standardUserDefaults];
    [pDefaults setObject:(bAccepted ? ksGDPRVersion : 0) forKey:ksGDPRDefaultsKey];
    [pDefaults synchronize];
}

Bool IOSEngine::GetGDPRAccepted() const
{
    NSUserDefaults *pDefaults = [NSUserDefaults standardUserDefaults];
    NSNumber *pGDPRVersion = [pDefaults objectForKey:ksGDPRDefaultsKey];
    if (pGDPRVersion == nil)
    {
        return false;
    }
    return [pGDPRVersion intValue] >= [ksGDPRVersion intValue];
}

Bool IOSEngine::CanRequestRemoteNotificationsWithoutPrompt()
{
	NSUserDefaults *pDefaults = [NSUserDefaults standardUserDefaults];
	BOOL bSawPrompt = [pDefaults boolForKey:ksSawRemoteNotificationPrompt];
	return bSawPrompt;
}

void IOSEngine::SetCanRequestRemoteNotificationsWithoutPrompt(Bool bCanRequest)
{
	Bool bCouldRequest = CanRequestRemoteNotificationsWithoutPrompt();

	NSUserDefaults *pDefaults = [NSUserDefaults standardUserDefaults];
	[pDefaults setBool:bCanRequest forKey:ksSawRemoteNotificationPrompt];
	[pDefaults synchronize];

	if (bCanRequest && !bCouldRequest)
	{
		InternalRegisterUserNotificationSettings();
	}
}

} // namespace Seoul
