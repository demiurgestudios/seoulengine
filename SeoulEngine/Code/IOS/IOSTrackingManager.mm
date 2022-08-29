/**
 * \file IOSTrackingManager.cpp
 * \brief Specialization of TrackingManager for IOS. Currently, integrates
 * the AppsFlyer SDK.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AnalyticsManager.h"
#include "JobsFunction.h"
#include "IOSAppDelegate.h"
#include "IOSEngine.h"
#include "IOSTrackingManager.h"
#include "IOSUtil.h"
#include "PlatformData.h"
#include "ToString.h"

namespace Seoul
{

void IOSTrackingManagerUpdatePlatformData(const String& sCampaign, const String& sMediaSource)
{
	if (IOSEngine::Get())
	{
		IOSEngine::Get()->SetAttributionData(sCampaign, sMediaSource);
	}
}

} // namespace Seoul

#if SEOUL_WITH_APPS_FLYER
#import "AppsFlyerTracker.h"

@interface IOSAppsFlyerDelegate : NSObject<AppsFlyerTrackerDelegate>;
@end

@implementation IOSAppsFlyerDelegate
/**
 * Parse an NSURL as an NSMutableDictionary of the scheme, path, and query items
 */
+ (NSMutableDictionary*) urlAsDict:(NSURL*) url
{
	NSMutableDictionary* pDictionary = [[NSMutableDictionary alloc] init];

	@try
	{
		NSURLComponents *urlComponents = [NSURLComponents componentsWithURL:url
			resolvingAgainstBaseURL:NO];
		[pDictionary setValue:urlComponents.scheme forKey:@"scheme"];
		[pDictionary setValue:urlComponents.path forKey:@"path"];

		if([urlComponents respondsToSelector:@selector(queryItems)])
		{
			NSArray* queryItems = urlComponents.queryItems;
			for(NSURLQueryItem* item in queryItems)
			{
				[pDictionary setValue:item.value forKey:item.name];
			}
		}
		else
		{
			// On iOS 7 and below we need to parse the URL manually
			NSString* query = urlComponents.query;
			NSArray* components = [query componentsSeparatedByString:@"&"];
			for (NSString* component in components)
			{
				NSArray* subComponents = [component componentsSeparatedByString:@"="];
				if(subComponents != nil && [subComponents count] == 2)
				{
					NSString* value = [[[subComponents objectAtIndex:1]
						stringByReplacingOccurrencesOfString:@"+" withString:@" "]
						stringByRemovingPercentEncoding];
					NSString* key = [[[subComponents objectAtIndex:0]
						stringByReplacingOccurrencesOfString:@"+" withString:@" "]
						stringByRemovingPercentEncoding];

					[pDictionary setValue:value forKey:key];
				}
			}
		}
	}
	@catch(NSException *e)
	{
		[pDictionary removeAllObjects];
	}

	return [pDictionary autorelease];
}

/**
 * Parse out the media source and campaign name from the install for attribution.
 */
+ (void) handleInstallAttributionData:(NSDictionary*) tData
{
	using namespace Seoul;

	// If a campaign was provided, use it, otherwise
	// default to "organic".
	String sCampaign;
	NSObject* pCampaign = [tData valueForKey:@"campaign"];
	if (pCampaign && [pCampaign isKindOfClass:[NSString class]])
	{
		sCampaign = String((NSString*)pCampaign);
	}
	else
	{
		sCampaign = "organic";
	}

	// If a media source was provided, use it, otherwise
	// default to "organic".
	String sMediaSource;
	NSObject* pMediaSource = [tData valueForKey:@"media_source"];
	if (pMediaSource && [pMediaSource isKindOfClass:[NSString class]])
	{
		sMediaSource = String((NSString*)pMediaSource);
	}
	else
	{
		sMediaSource = "organic";
	}

	Jobs::AsyncFunction(
		GetMainThreadId(),
		&IOSTrackingManagerUpdatePlatformData,
		sCampaign,
		sMediaSource);
}

/**
 * Parse out the name of the campaign for deep linking rewards.
 */
+ (void) handleDeepLinking:(NSDictionary*) tData
{
	using namespace Seoul;

	NSObject* pAfdp = [tData valueForKey:@"af_dp"];
	NSObject* pCampaign = [tData valueForKey:@"campaign"];

	if(!pAfdp || ![pAfdp isKindOfClass:[NSString class]] || !pCampaign || ![pCampaign isKindOfClass:[NSString class]])
	{
		// For Universal Links we need to parse out the link ourselves
		NSObject* pLink = [tData valueForKey:@"link"];
		if (!pLink || ![pLink isKindOfClass:[NSString class]])
		{
			return;
		}

		NSURL* pURL = [NSURL URLWithString:(NSString*)pLink];
		NSDictionary* pDict = [IOSAppsFlyerDelegate urlAsDict:pURL];

		pAfdp = [pDict valueForKey:@"af_dp"];
		pCampaign = [pDict valueForKey:@"c"];
		if(!pAfdp || ![pAfdp isKindOfClass:[NSString class]] || !pCampaign || ![pCampaign isKindOfClass:[NSString class]])
		{
			return;
		}
	}

	NSURL* pURL = [NSURL URLWithString:(NSString*)pAfdp];
	if (![[pURL scheme] isEqualToString:IOSTrackingManager::Get()->GetDeepLinkScheme().ToNSString()])
	{
		return;
	}

	Jobs::AsyncFunction(
		GetMainThreadId(),
		IOSTrackingManager::Get()->GetDeepLinkDelegate(),
		String((NSString*)pCampaign));
}

- (void) onConversionDataReceived:(NSDictionary*) tInstallData
{
	// After first install, this data is cached and should always be the same.
	[IOSAppsFlyerDelegate handleInstallAttributionData:tInstallData];
	[IOSAppsFlyerDelegate handleDeepLinking:tInstallData];
}

- (void) onConversionDataRequestFailure:(NSError*) error
{
	// Nop
}

- (void) onAppOpenAttribution:(NSDictionary*) tAttributionData
{
	[IOSAppsFlyerDelegate handleDeepLinking:tAttributionData];
}

- (void) onAppOpenAttributionFailure:(NSError*) error
{
	// Nop
}
@end
#endif // /#if SEOUL_WITH_APPS_FLYER

#if SEOUL_WITH_HELPSHIFT
#import "HelpshiftCore.h"
#import "HelpshiftSupport.h"
#endif

namespace Seoul
{

static const String ksEventLaunch("af_app_open");
static const String ksEventSessionStart("af_session_start");
static const String ksEventSessionEnd("af_session_end");

IOSTrackingManager::IOSTrackingManager(const IOSTrackingManagerSettings& settings)
	: m_Settings(settings)
	, m_bIsOnProd(settings.m_GetIsOnProd())
	, m_sExternalTrackingUserID()
#if SEOUL_WITH_APPS_FLYER
	, m_pAppsFlyerTracker(nullptr)
#endif // /#if SEOUL_WITH_APPS_FLYER
	, m_sUniqueUserId()
	, m_bHasUserID(false)
{
}

IOSTrackingManager::~IOSTrackingManager()
{
	if (!m_bHasUserID)
	{
		return;
	}

	// Shutdown AppsFlyer if enabled.
#if SEOUL_WITH_APPS_FLYER
	if (nullptr != m_pAppsFlyerTracker)
	{
		[m_pAppsFlyerTracker release];
		m_pAppsFlyerTracker = nullptr;
	}
#endif // /SEOUL_WITH_APPS_FLYER
}

Bool IOSTrackingManager::OpenThirdPartyURL(const String& sURL) const
{
	if (!m_bHasUserID)
	{
		return false;
	}

#if SEOUL_AUTO_TESTS
	// TODO: I'd really rather show the helpshift popup,
	// but I also don't want to integrate all the iOS infrastructure
	// necessary to dismiss arbitrary system and third party popups.
	if (g_bRunningAutomatedTests)
	{
		return false;
	}
#endif // /SEOUL_AUTO_TESTS

#if SEOUL_WITH_HELPSHIFT
	if (!m_Settings.m_sHelpShiftKey.IsEmpty())
	{
		static const String sHelpShiftDomain("helpshift://");
		if (sURL.StartsWith(sHelpShiftDomain))
		{
			auto const sHost(sURL.Substring(sHelpShiftDomain.GetSize()));
			return ShowHelpshiftFAQ(sHost);
		}
	}
#endif // /#if SEOUL_WITH_HELPSHIFT

	return false;
}

Bool IOSTrackingManager::ShowHelp() const
{
	if (!m_bHasUserID)
	{
		return false;
	}

#if SEOUL_AUTO_TESTS
	// TODO: I'd really rather show the helpshift popup,
	// but I also don't want to integrate all the iOS infrastructure
	// necessary to dismiss arbitrary system and third party popups.
	if (g_bRunningAutomatedTests)
	{
		return false;
	}
#endif // /SEOUL_AUTO_TESTS

#if SEOUL_WITH_HELPSHIFT
	return ShowHelpshiftFAQ();
#else
	return false;
#endif // /#if SEOUL_WITH_HELPSHIFT
}

#if SEOUL_WITH_HELPSHIFT
/**
 * Helper method to wrap the HelpshiftAPIConfig creation and issue HelpshiftSupport calls.
 * TODO: Wrap the HelpshiftAPIConfig creation logic in an Objective C method isntead of using this
 * method
 * @param[in] sFAQ Optional. If empty, FAQs landing page will be shown. If present, the single
 *	FAQ will be shown.
 * @return Success or failure.
 */
Bool IOSTrackingManager::ShowHelpshiftFAQ(String sFAQ /* = String() */) const
{
	if(nullptr == m_pRootViewController)
	{
		return false;
	}

#if SEOUL_AUTO_TESTS
	// TODO: I'd really rather show the helpshift popup,
	// but I also don't want to integrate all the iOS infrastructure
	// necessary to dismiss arbitrary system and third party popups.
	if (g_bRunningAutomatedTests)
	{
		return false;
	}
#endif // /SEOUL_AUTO_TESTS

	@try
	{
		NSDictionary* ptCustomIssueFields = nil;
		NSDictionary* ptMetaData = nil;
		{
			Lock lock(m_UserDataMutex);
			ptCustomIssueFields = ConvertToNSDictionary(m_UserCustomData);
			ptMetaData = ConvertToNSDictionary(m_UserMetaData);
		}

		HelpshiftAPIConfigBuilder* pApiBuilder = [[[HelpshiftAPIConfigBuilder alloc] init] autorelease];
		pApiBuilder.customIssueFields = ptCustomIssueFields;
		pApiBuilder.customMetaData = [[[HelpshiftSupportMetaData alloc] initWithMetaData:ptMetaData] autorelease];

		HelpshiftAPIConfig* pApiConfigObject = [pApiBuilder build];

		if (sFAQ.IsEmpty())
		{
			[HelpshiftSupport showFAQs:(UIViewController*)m_pRootViewController
							withConfig:pApiConfigObject];
		}
		else
		{
			[HelpshiftSupport showSingleFAQ:sFAQ.ToNSString()
							 withController:(UIViewController*)m_pRootViewController
								 withConfig:pApiConfigObject];
		}
	}
	@catch(NSException *e)
	{
		SEOUL_WARN("HelpShift exception: %s: %s",
				   String([e name]).CStr(),
				   String([e reason]).CStr());
		return false;
	}
	return true;
}
#endif

void IOSTrackingManager::SetTrackingUserID(const String& sUserName, const String& sUserID)
{
	// SetTrackingUserID is a bit unique - we don't perform further
	// processing if we already have a user ID, or if the user ID
	// is invalid.
	if (sUserID.IsEmpty() || !m_sUniqueUserId.IsEmpty())
	{
		return;
	}

	// We've now hit the point where we have a user ID.
	m_sUniqueUserId = sUserID;
	m_bHasUserID = true;

	// Initialize AppsFlyer if enabled and we now have a valid user ID.
#if SEOUL_WITH_APPS_FLYER
	// Absorb AppsFlyer exceptions, so it doesn't take down the rest of our app.
	@try
	{
		Bool const bEnableDebugLogging = !SEOUL_SHIP;

		NSString* sAppleID = m_Settings.m_sAppleID.ToNSString();
		NSString* sDevKey = m_Settings.m_sAppsFlyerID.ToNSString();

		// Configure AppsFlyer.
		m_pAppsFlyerTracker = [[AppsFlyerTracker alloc] init];
		m_pAppsFlyerTracker.isDebug = (bEnableDebugLogging ? YES : NO);
		m_pAppsFlyerTracker.appsFlyerDevKey = sDevKey;
		m_pAppsFlyerTracker.appleAppID = sAppleID;
		m_pAppsFlyerTracker.customerUserID = sUserID.ToNSString();

		// Send out the initial event and initialize the SDK.
		[m_pAppsFlyerTracker trackAppLaunch];

		// TODO: Do not register the delegate at this point. We need to wait at least 10 seconds.
		// This is because AppsFlyer internally waits 10 seconds before applying attribution data.
		// This is because Apple Search Ads can take some time to report proper attribution data. If
		// we register the delegate at this point then AppsFlyer automatically tries to obtain data
		// and it will incorrectly report organic attribution.
		// TODO @appsflyerupgrade: see if an SDK upgrade fixes this issue.
		[NSTimer scheduledTimerWithTimeInterval: 20.0
			target: [UIApplication sharedApplication].delegate
			selector: @selector(delayedAppsFlyerSetDelegateAndGetConversionData)
			userInfo: nil
			repeats: NO];

		// TODO: Originally I had the below logic in the
		// delayedAppsFlyerSetDelegateAndGetConversionData method, but this
		// creates an awkward pause for the user, making the deep link feedback feel
		// unresponsive / broken. Parsing of the URL does not actually require any AppsFlyer
		// functionality, and this launch URL case does is not handled by AppsFlyer anyway, so
		// just parse it now.

		// TODO: The app could have been opened with a URL that represents a deep link URL.
		// In this case for iOS 8.0 and below AppsFlyer is unaware of the link and will not
		// inform us. Also, the URL given to the app at launch is not the complete URL, it is
		// the scheme portion, we is custom set by us for the app. As such, forwarding it
		// to AppsFlyer's handleOpenUrl method has no effect as it expects a full deep link URL.
		// Parse the URL directly and give it to the handleDeepLinking method directly.
		// TODO @appsflyerupgrade: see if an SDK upgrade fixes this issue.
		IOSAppDelegate *pAppDelegate = (IOSAppDelegate*)[UIApplication sharedApplication].delegate;
		if (pAppDelegate != nil && pAppDelegate.m_pLaunchURL != nil)
		{
			NSDictionary* pDict = [IOSAppsFlyerDelegate urlAsDict:pAppDelegate.m_pLaunchURL];
			[IOSAppsFlyerDelegate handleDeepLinking:pDict];
		}

		// Cache the UDID.
		m_sExternalTrackingUserID = String([m_pAppsFlyerTracker getAppsFlyerUID]);

		// Launch event.
		TrackEvent(ksEventLaunch);
	}
	@catch (NSException* e)
	{
		SEOUL_WARN("AppsFlyer exception: %s: %s",
			String([e name]).CStr(),
			String([e reason]).CStr());
	}
#endif // /#if SEOUL_WITH_APPS_FLYER

	// Update platform data once we've retrieved tracking info to associate the iOS
	// advertiser ID, if we're not using AppsFlyer. Otherwise, wait for attribution
	// data from AppsFlyer.
#if !SEOUL_WITH_APPS_FLYER
	IOSTrackingManagerUpdatePlatformData(String(), String());
#endif // /#if !SEOUL_WITH_APPS_FLYER

	// Initialize HelpShift if enabled - deferred initialization
	// until we have a unique user ID.
#if SEOUL_WITH_HELPSHIFT
	if (!m_Settings.m_sHelpShiftKey.IsEmpty())
	{
		NSString* sHelpShiftID = (sUserID + m_Settings.m_sHelpShiftUserIDSuffix).ToNSString();
		NSString* sHelpShiftName = (sUserName.IsEmpty() ? nil : sUserName.ToNSString());

		@try
		{
			HelpshiftInstallConfigBuilder* pInstallConfigBuilder = [[[HelpshiftInstallConfigBuilder alloc] init] autorelease];
			pInstallConfigBuilder.addFaqsToDeviceSearch = HsAddFaqsToDeviceSearchAfterViewingFAQs;
			HelpshiftInstallConfig* pInstallConfig = [pInstallConfigBuilder build];

			[HelpshiftCore initializeWithProvider:[HelpshiftSupport sharedInstance]];
			[HelpshiftCore installForApiKey:m_Settings.m_sHelpShiftKey.ToNSString()
				domainName:m_Settings.m_sHelpShiftDomain.ToNSString()
				appID:m_Settings.m_sHelpShiftID.ToNSString()
				withConfig:pInstallConfig];

			// Login user to Helpshift
			HelpshiftUserBuilder* pUserBuilder = [[HelpshiftUserBuilder alloc]
												  initWithIdentifier:sHelpShiftID
												  andEmail:nil];
			pUserBuilder.name = sHelpShiftName;
			HelpshiftUser* pHelpshiftUser = [pUserBuilder build];
			[HelpshiftCore login:pHelpshiftUser];

			[pUserBuilder release];
		}
		@catch (NSException* e)
		{
			SEOUL_WARN("HelpShift exception: %s: %s",
				String([e name]).CStr(),
				String([e reason]).CStr());
		}
	}
#endif
}

void IOSTrackingManager::TrackEvent(const String& sEventName)
{
	if (!m_bHasUserID)
	{
		return;
	}

	// Send a launch event to AppsFlyer to reflect
	// the session start if enabled.
#if SEOUL_WITH_APPS_FLYER
	@try
	{
		[m_pAppsFlyerTracker trackEvent:sEventName.ToNSString() withValues:@{
			@"af_is_dev_build": @(!m_bIsOnProd),
		}];
	}
	@catch (NSException* e)
	{
		SEOUL_WARN("AppsFlyer exception: %s: %s",
			String([e name]).CStr(),
			String([e reason]).CStr());
	}
#endif // /SEOUL_WITH_APPS_FLYER
}

void IOSTrackingManager::OnSessionChange(const AnalyticsSessionChangeEvent& evt)
{
	if (!m_bHasUserID)
	{
		return;
	}

	// Report via AppsFlyer if enabled.
#if SEOUL_WITH_APPS_FLYER
	{
		SEOUL_LOG_TRACKING("AppsFlyerSessionChange(%s, %s, %s, %" PRIu64 ")",
			evt.m_bSessionStart ? "start" : "end",
			evt.m_SessionUUID.ToString().CStr(),
			evt.m_TimeStamp.ToISO8601DateTimeUTCString().CStr(),
			evt.m_Duration.GetMicroseconds());

		@try
		{
			if (evt.m_bSessionStart)
			{
				[m_pAppsFlyerTracker trackEvent:@"af_session_start" withValues:@{
					@"af_session_id": evt.m_SessionUUID.ToString().ToNSString(),
					@"af_is_dev_build": @(!m_bIsOnProd),
				}];
			}
			else
			{
				[m_pAppsFlyerTracker trackEvent:@"af_session_end" withValues:@{
					@"af_session_duration": [NSNumber numberWithLongLong:evt.m_Duration.GetSeconds()],
					@"af_session_id": evt.m_SessionUUID.ToString().ToNSString(),
					@"af_is_dev_build": @(!m_bIsOnProd),
				}];
			}
		}
		@catch (NSException* e)
		{
			SEOUL_WARN("AppsFlyer exception: %s: %s",
				String([e name]).CStr(),
				String([e reason]).CStr());
		}
	}
#endif // /SEOUL_WITH_APPS_FLYER
}

#if SEOUL_WITH_APPS_FLYER
Bool IOSTrackingManager::IsAppsFlyerInitialized() const
{
	return m_pAppsFlyerTracker != nullptr && m_pAppsFlyerTracker.delegate != nil;
}

void IOSTrackingManager::DelayedAppsFlyerSetDelegate()
{
	// Associate our delegate for receiving campaign tracking data.
	// This has the side effect of setting attribution data, which is
	// why it has to happen in a delayed call.
	IOSAppsFlyerDelegate* pIOSAppsFlyerDelegate = [[IOSAppsFlyerDelegate alloc] init];
	m_pAppsFlyerTracker.delegate = pIOSAppsFlyerDelegate;
	[IOSAppsFlyerDelegate release];

	// TODO: Setting the delegate should invoke getConversionData, but in practice I do not see the
	// callback invoked in all cases. Invoke it manually here
	// TODO @appsflyerupgrade: see if an SDK upgrade fixes this issue.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundeclared-selector"
	if([m_pAppsFlyerTracker respondsToSelector:@selector(getConversionData)])
	{
		[m_pAppsFlyerTracker performSelector:@selector(getConversionData)];
	}
#pragma GCC diagnostic pop
}

Bool IOSTrackingManager::HandleOpenURL(
	void* pUrl,
	void* pSourceApplication,
	void* pInOptions,
	void* pAnnotation) const
{
	NSURL* url = (NSURL*)pUrl;
	// TODO @appsflyerupgrade: this is commented out because we don't need it unless
	// we use the AppsFlyer SDK
	// NSString* sourceApplication = (NSString*)pSourceApplication;
	NSDictionary<UIApplicationOpenURLOptionsKey,id>* options =
	(NSDictionary<UIApplicationOpenURLOptionsKey,id>*)pInOptions;
	// TODO @appsflyerupgrade: this is commented out because we don't need it unless
	// we use the AppsFlyer SDK
	// id annotation = (id)pAnnotation;

	if (options != nil)
	{
		// TODO @appsflyerupgrade: ideally, we could make the call to handleOpenURL in the
		// AppsFlyer SDK at this point, which would then parse and pass the appropriate
		// data to the delegate. However, the AppsFlyer SDK appears to not work
		// at this point. Possibly fixed with an SDK update. When the AppsFlyer SDK is updated,
		// try uncommenting this call and remove the manual call to the delegate below.

		//[m_pAppsFlyerTracker handleOpenUrl:url options:options];

		NSMutableDictionary* pLink = [[[NSMutableDictionary alloc] init] autorelease];
		[pLink setObject:[url absoluteString] forKey:@"link"];
		[m_pAppsFlyerTracker.delegate onAppOpenAttribution:pLink];
	}
	else
	{
		// TODO @appsflyerupgrade: same thing applies here, see comment above, possibly fixed by an SDK
		// update:
		//[m_pAppsFlyerTracker handleOpenURL:url sourceApplication:sourceApplication withAnnotation:annotation];

		NSMutableDictionary* pLink = [[[NSMutableDictionary alloc] init] autorelease];
		[pLink setObject:[url absoluteString] forKey:@"link"];
		[m_pAppsFlyerTracker.delegate onAppOpenAttribution:pLink];
	}
	return true;
}

Bool IOSTrackingManager::HandleContinueUserActivity(void* pUserActivity, void* pRestorationHandler) const
{
	NSUserActivity* userActivity = (NSUserActivity*)pUserActivity;
	//void (^restorationHandler)(NSArray *) = (void (^)(NSArray *))pRestorationHandler;

	// TODO @appsflyerupgrade: same thing applies here, see comment above, possibly fixed by an SDK
	// update:
	//[m_pAppsFlyerTracker continueUserActivity:userActivity restorationHandler:restorationHandler];

	NSMutableDictionary* pLink = [[[NSMutableDictionary alloc] init] autorelease];
	[pLink setObject:[userActivity.webpageURL absoluteString] forKey:@"link"];
	[m_pAppsFlyerTracker.delegate onAppOpenAttribution:pLink];

	return true;
}
#endif // /SEOUL_WITH_APPS_FLYER

} // namespace Seoul
