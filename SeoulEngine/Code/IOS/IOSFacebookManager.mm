/**
 * \file IOSFacebookManager.mm
 * \brief iOS-specific Facebook SDK implementation. Wraps the
 * iOS Objective-C Facebook SDK.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "IOSFacebookManager.h"

#if SEOUL_WITH_FACEBOOK

#import <FBSDKCoreKit/FBSDKCoreKit.h>
#import <FBSDKLoginKit/FBSDKLoginKit.h>
#import <FBSDKShareKit/FBSDKShareKit.h>
#import <Accounts/ACAccountStore.h>

// Internal class, FBSDK does not expose this
// to disable crash reporting and this is
// less risky than recompiling the SDK to do
// the equivalent.
@interface FBSDKFeatureManager : NSObject
+ (void)disableFeature:(NSString *)featureName;
@end

#include "InputManager.h"
#include "JobsFunction.h"
#include "Logger.h"

// Facebook SDK gift request and callbacks:
@interface IOSFacebookGiftRequest : NSObject<FBSDKGameRequestDialogDelegate>
{
	// Keep the Seoul IOS Facebook Manager as a protected member.
	Seoul::IOSFacebookManager* m_FacebookManager;
}
- (id)initWithParent:(Seoul::IOSFacebookManager *)facebook;
- (void)gameRequestDialog:(FBSDKGameRequestDialog *)gameRequestDialog didCompleteWithResults:(NSDictionary *)results;
- (void)gameRequestDialog:(FBSDKGameRequestDialog *)gameRequestDialog didFailWithError:(NSError *)error;
- (void)gameRequestDialogDidCancel:(FBSDKGameRequestDialog *)gameRequestDialog;
@end

@implementation IOSFacebookGiftRequest
- (id)initWithParent:(Seoul::IOSFacebookManager *)facebook
{
	m_FacebookManager = facebook;
	return self;
}

- (void)gameRequestDialog:(FBSDKGameRequestDialog *)gameRequestDialog didCompleteWithResults:(NSDictionary *)results
{
	// The result URL is something like "fbconnect://success?request=<ID>&to%5B0%5D=<user0>&to%5B1%5D=<user1>&updated_frictionless=1&frictionless_recipients%5B%5D"
	NSString *rid = results[@"request"];
	NSString *data = [gameRequestDialog.content valueForKey:@"data"];
	NSString *recipients = [[gameRequestDialog.content valueForKey:@"recipients"] componentsJoinedByString:@","];

	// User completed the send, so send gift
	m_FacebookManager->ApplyGiftRequest(rid, recipients, data);
	[self autorelease];
}
- (void)gameRequestDialog:(FBSDKGameRequestDialog *)gameRequestDialog didFailWithError:(NSError *)error
{
	SEOUL_LOG("Facebook SendRequest error: %s\n", [[error localizedDescription] UTF8String]);
	[self autorelease];
}
- (void)gameRequestDialogDidCancel:(FBSDKGameRequestDialog *)gameRequestDialog
{
	// Ignore cancellations.
	[self autorelease];
}
@end

namespace Seoul
{

IOSFacebookManager::IOSFacebookManager()
{
	m_FacebookLogin = [[FBSDKLoginManager alloc] init];
}
IOSFacebookManager::~IOSFacebookManager()
{
	[m_FacebookLogin release];
}

/**
 * Helper function to parse a URL's query string ("?foo=bar&baz=quux") into an
 * NSDictionary of key=value pairs
 */
NSDictionary *ParseURLQueryParameters(NSURL *pURL)
{
	NSMutableDictionary *pParamDict = [[[NSMutableDictionary alloc] init] autorelease];

	for (NSString *pKVPair in [[pURL query] componentsSeparatedByString:@"&"])
	{
		NSArray *pComponents = [pKVPair componentsSeparatedByString:@"="];
		if ([pComponents count] >= 2)
		{
			pParamDict[pComponents[0]] = pComponents[1];
		}
	}

	return pParamDict;
}

/**
 * Begins the Facebook login flow.  The user is asked if he wants to allow the
 * app to have the given permissions.
 */
void IOSFacebookManager::Login(const Vector<String>& vRequestedPermissions)
{
	FBSDKAccessToken *token = [FBSDKAccessToken currentAccessToken];
	if ((token == nil) || ([[NSDate date] compare:[token expirationDate]] == NSOrderedDescending))
	{
		NSMutableArray *aRequestedPermissions = [NSMutableArray arrayWithCapacity:vRequestedPermissions.GetSize()];

		// Convert Vector<String> to NSArray of NSString
		for (UInt i = 0; i < vRequestedPermissions.GetSize(); i++)
		{
			[aRequestedPermissions addObject:vRequestedPermissions[i].ToNSString()];
		}

		[m_FacebookLogin logInWithPermissions:aRequestedPermissions
			fromViewController:[[[[UIApplication sharedApplication] delegate] window] rootViewController]
			handler:^(FBSDKLoginManagerLoginResult *result, NSError *error)
		{
			if (result.isCancelled)
			{
				// Nothing happens.
				return;
			}

			if (error)
			{
				SEOUL_LOG("Facebook session start error: %s\n", [[error localizedDescription] UTF8String]);
			}

			Jobs::AsyncFunction(
				GetMainThreadId(),
				SEOUL_BIND_DELEGATE(&IOSFacebookManager::OnFacebookLoginStatusChanged, (FacebookManager *)this));

			if (result.token != nil)
			{
				Jobs::AsyncFunction(
					GetMainThreadId(),
					SEOUL_BIND_DELEGATE(&IOSFacebookManager::SetFacebookID, (FacebookManager *)this),
					String([result.token userID]));
			}
		}];
	}
}

void IOSFacebookManager::RefreshAccessToken()
{
	FBSDKAccessToken* token = [FBSDKAccessToken currentAccessToken];
	if ((token != nil) && ([[NSDate date] compare:[token expirationDate]] == NSOrderedAscending))
	{
		// Report current user ID.
		SetFacebookID(String([token userID]));
	}
	else
	{
		// Report user not logged in.
		SetFacebookID(String());
	}
}

/**
 * Closes the current session but does not delete the user's current
 * authentication token.
 */
void IOSFacebookManager::CloseSession()
{
	[m_FacebookLogin logOut];
}

/**
 * Closes the current session and deletes the user's current authentication
 * token.
 */
void IOSFacebookManager::LogOff()
{
	[m_FacebookLogin logOut];
}

/**
 * Tests if the user is currently logged into Facebook
 */
Bool IOSFacebookManager::IsLoggedIn()
{
	FBSDKAccessToken* token = [FBSDKAccessToken currentAccessToken];
	return ((token != nil) && ([[NSDate date] compare:[token expirationDate]] == NSOrderedAscending));
}

/**
 * Gets the current Facebook authentication token, or the empty string if we
 * don't have a token.
 */
String IOSFacebookManager::GetAuthToken()
{
	FBSDKAccessToken* token = [FBSDKAccessToken currentAccessToken];
	if ((token != nil) && ([[NSDate date] compare:[token expirationDate]] == NSOrderedAscending))
		return String([token tokenString]);
	else
		return String();
}

void IOSFacebookManager::ApplyGiftRequest(NSString* rid, NSString* recipients, NSString* data)
{
	// User completed the send, so send gift
	Jobs::AsyncFunction(
		GetMainThreadId(),
		SEOUL_BIND_DELEGATE(&IOSFacebookManager::OnSentRequest, (FacebookManager *)this),
		String(rid),
		String(recipients),
		String(data));
}

void IOSFacebookManager::GetFriendsList(const String& sFields, Int iUserLimit)
{
	if (IsLoggedIn())
	{
		NSMutableDictionary* params = [NSMutableDictionary dictionaryWithObjectsAndKeys: sFields.ToNSString(), @"fields", nil];
		FBSDKGraphRequest* request = [[FBSDKGraphRequest alloc] initWithGraphPath:@"me/friends"
			parameters:params
			HTTPMethod:@"GET"];
		[request startWithCompletionHandler:^(FBSDKGraphRequestConnection *connection, id result, NSError *error)
		{
			String sResultStr;
			NSError* jsonError;
			if (error)
			{
				SEOUL_LOG("Facebook GetFriendsList error: %s\n", [[error localizedDescription] UTF8String]);
				return;
			}

			NSDictionary* fb = (NSDictionary*) result;
			if (fb)
			{
				int total = 0;
				int index;

				NSArray* jsonFriends = [fb objectForKey:@"data"];
				NSMutableArray* jsonFiltered = [[NSMutableArray alloc] init];
				NSMutableArray* jsonBystanders = [[NSMutableArray alloc] init];
				if (jsonFriends)
				{
					for (index = 0; index < [jsonFriends count]; index++)
					{
						NSDictionary* jsonFriend = [jsonFriends objectAtIndex:index];
						NSNumber* state = (NSNumber*) [jsonFriend objectForKey:@"installed"];
						if ((state != nil) && [state boolValue])
						{
							if ((iUserLimit == 0) || (total < iUserLimit))
							{
								total++;
								[jsonFiltered addObject:jsonFriend];
							}
							else
							{
								// If we have a limit, stop
								// when we reach it.
								break;
							}
						}
						else
						{
							[jsonBystanders addObject:jsonFriend];
						}
					}
				}

				// If space permits, add non-players.
				for (index = 0; index < [jsonBystanders count]; index++)
				{
					NSDictionary* jsonFriend = [jsonBystanders objectAtIndex:index];
					if ((iUserLimit == 0) || (total < iUserLimit))
					{
						[jsonFiltered addObject:jsonFriend];
					}
					else
					{
						break;
					}
					total++;
				}

				NSMutableDictionary* output = [[[NSMutableDictionary alloc] initWithDictionary:fb] autorelease];
				[output setObject:jsonFiltered forKey:@"data"];
				NSData *jsonResult = [NSJSONSerialization dataWithJSONObject:output
					options:0
					error:&jsonError];

				if (!jsonResult)
				{
					SEOUL_LOG("Facebook GetFriendsList JSON error: %s\n", [[jsonError localizedDescription] UTF8String]);
				}
				else
				{
					NSString *JSONString = [[[NSString alloc] initWithBytes:[jsonResult bytes] length:[jsonResult length] encoding:NSUTF8StringEncoding] autorelease];
					sResultStr = String([JSONString UTF8String]);
				}

				[jsonBystanders release];
				[jsonFiltered release];
			}

			Jobs::AsyncFunction(
				GetMainThreadId(),
				SEOUL_BIND_DELEGATE(&IOSFacebookManager::OnReturnFriendsList, (FacebookManager *)this),
				sResultStr);
		}];
    }
	else
	{
		Jobs::AsyncFunction(
			GetMainThreadId(),
			SEOUL_BIND_DELEGATE(&IOSFacebookManager::OnReturnFriendsList, (FacebookManager *)this),
			String());
	}

}

void IOSFacebookManager::SendRequest(const String& sMessage, const String& sTitle, const String& sRecipients, const String& sSuggestedFriends, const String& sData)
{
	if (IsLoggedIn())
	{
		IOSFacebookGiftRequest* handler = [[IOSFacebookGiftRequest alloc] initWithParent:this];
		FBSDKGameRequestContent* content = [[[FBSDKGameRequestContent alloc] init] autorelease];
		FBSDKGameRequestDialog* dialog = [[[FBSDKGameRequestDialog alloc] init] autorelease];

		content.message = sMessage.ToNSString();
		content.title = sTitle.ToNSString();
		if(!sRecipients.IsEmpty())
		{
			content.recipients = [sRecipients.ToNSString() componentsSeparatedByString:@","];
		}
		else if(!sSuggestedFriends.IsEmpty())
		{
			content.recipientSuggestions = [sSuggestedFriends.ToNSString() componentsSeparatedByString:@","];
		}
		if(!sData.IsEmpty())
		{
			content.data = sData.ToNSString();
		}

		// Dialog content makes a copy of the content data, but only a weak
		// reference to the handler. Make sure the handler will self destruct
		// when complete.
		dialog.content = content;
		dialog.delegate = handler;
		dialog.frictionlessRequestsEnabled = TRUE;

		[dialog show];
	}
}

void IOSFacebookManager::ShareLink(const String& sContentURL)
{
	FBSDKShareLinkContent *pContent = [[[FBSDKShareLinkContent alloc] init] autorelease];
	pContent.contentURL = [NSURL URLWithString:sContentURL.ToNSString()];
	FBSDKShareDialog *pShareDialog = [[[FBSDKShareDialog alloc] init] autorelease];
	pShareDialog.shareContent = pContent;
	[pShareDialog show];
}

// Called once the user has a player guid reported.
void IOSFacebookManager::SetUserID(const String& sUserID)
{
	if (m_bInitialized)
	{
		return;
	}

	if (nullptr == m_pApplication)
	{
		return;
	}

	// Tracking.
	m_bInitialized = true;

	// Need to disable these features prior to init, the FB SDK defaults
	// to registering an unhandled exception handler and other glue that
	// interferes with our crash reporting.
	[FBSDKFeatureManager disableFeature:@"CrashReport"];
	[FBSDKFeatureManager disableFeature:@"CrashShield"];
	[FBSDKFeatureManager disableFeature:@"ErrorReport"];

	// NOTE: We activate each of these settings here because at this point we know
	// the user has accepted GDPR. Each setting must be activated individually.
	// See https://developers.facebook.com/docs/app-events/getting-started-app-events-ios
	// for more info.
	[FBSDKSettings setAutoLogAppEventsEnabled:YES];
	[FBSDKSettings setAutoInitEnabled:YES];
	[FBSDKSettings setAdvertiserIDCollectionEnabled:YES];

	// Setup.
	[[FBSDKApplicationDelegate sharedInstance] application:(UIApplication*)m_pApplication didFinishLaunchingWithOptions:(NSDictionary*)m_pLaunchOptions];
	RefreshAccessToken();
}

void IOSFacebookManager::DeleteRequest(const String& sRequestID)
{
	// If we're not logged in, queue up the request to be deleted later, after we
	// do log in
	if (!IsLoggedIn())
	{
		m_vRequestsToDelete.PushBack(sRequestID);
		return;
	}

	// Kick off the call to delete the request; ignore errors
	FBSDKGraphRequest *pRequest = [[[FBSDKGraphRequest alloc] initWithGraphPath:sRequestID.ToNSString()
																	 HTTPMethod:@"DELETE"] autorelease];
	[pRequest startWithCompletionHandler:nil];
}

#define SEOUL_ASCIISTR_FROM_NSOBJECTS(sFormat, ...) ([[NSString stringWithFormat:sFormat, ##__VA_ARGS__] cStringUsingEncoding:NSASCIIStringEncoding])

void IOSFacebookManager::SendPurchaseEvent(Double fLocalPrice, const String& sCurrencyCode)
{
	@try
	{
		[FBSDKAppEvents logPurchase:fLocalPrice currency:sCurrencyCode.ToNSString()];
	}
	@catch (NSException* e)
	{
		SEOUL_LOG("%s", SEOUL_ASCIISTR_FROM_NSOBJECTS(@"FacebookManager SendPurchaseEvent: failed to track event. error: %@", e));
	}
}

void IOSFacebookManager::TrackEvent(const String& sEventID)
{
	@try
	{
		[FBSDKAppEvents logEvent: sEventID.ToNSString()];
	}
	@catch (NSException* e)
	{
		SEOUL_LOG("FacebookManager TrackEvent: failed to track: %s with error: %s", sEventID.CStr(), SEOUL_ASCIISTR_FROM_NSOBJECTS(@"%@", e));
	}
}

void IOSFacebookManager::RequestBatchUserInfo(const Vector<String>& vUserIDs)
{
	FBSDKGraphRequestConnection *connection = [[[FBSDKGraphRequestConnection alloc] init] autorelease];

	for (UInt i = 0; i < vUserIDs.GetSize(); ++i)
	{
		NSString* nsFacebookIDStr = vUserIDs[i].ToNSString();
		FBSDKGraphRequest *request = [[[FBSDKGraphRequest alloc] initWithGraphPath:nsFacebookIDStr
																		HTTPMethod:@"GET"] autorelease];

		[connection addRequest:request
				batchEntryName:[NSString stringWithFormat:@"Batch_Request_%@", nsFacebookIDStr]
			 completionHandler:^(FBSDKGraphRequestConnection *connection, id result, NSError *error)
		{
			if(!error)
			{
				NSDictionary *obj = (NSDictionary *) result;
				Jobs::AsyncFunction(
					GetMainThreadId(),
					SEOUL_BIND_DELEGATE(&IOSFacebookManager::OnReceiveBatchUserInfo, (FacebookManager *)this),
					String([obj objectForKey:@"id"]),
					String([obj objectForKey:@"name"]));
			}
			else
			{
				Jobs::AsyncFunction(
					GetMainThreadId(),
					SEOUL_BIND_DELEGATE(&IOSFacebookManager::OnReceiveBatchUserInfoFailed, (FacebookManager *)this),
					String(nsFacebookIDStr));
			}
		}];
	}

	[connection start];
}

void IOSFacebookManager::SetLaunchOptions(void *pApplication, void *pLaunchOptions)
{
	if (nullptr != m_pApplication)
	{
		[((UIApplication*)m_pApplication) release];
		m_pApplication = nullptr;
	}
	if (nullptr != m_pLaunchOptions)
	{
		[((NSDictionary*)m_pLaunchOptions) release];
		m_pLaunchOptions = nullptr;
	}

	if (nullptr != pApplication)
	{
		[((UIApplication*)pApplication) retain];
		m_pApplication = pApplication;
	}

	if (nullptr != pLaunchOptions)
	{
		[((NSDictionary*)pLaunchOptions) retain];
		m_pLaunchOptions = pLaunchOptions;
	}
}

Bool IOSFacebookManager::HandleOpenURL(
	void* pInApplication,
	NSURL* url,
	NSString* sourceApplication,
	void* pInOptions,
	id annotation)
{
	UIApplication* application = (UIApplication*)pInApplication;
	NSDictionary<UIApplicationOpenURLOptionsKey,id>* options =
		(NSDictionary<UIApplicationOpenURLOptionsKey,id>*)pInOptions;

	if (options != nil
		&& [[FBSDKApplicationDelegate sharedInstance] application:application openURL:url options:options])
	{
		return true;
	}
	else if ([[FBSDKApplicationDelegate sharedInstance] application:application openURL:url sourceApplication:sourceApplication annotation:annotation])
	{
		return true;
	}

	return false;
}

} // namespace Seoul

#endif  // SEOUL_WITH_FACEBOOK
