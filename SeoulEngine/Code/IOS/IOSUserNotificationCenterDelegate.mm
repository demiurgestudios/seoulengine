/**
 * \file IOSUserNotificationCenterDelegate.mm
 * \brief Implementation for an iOS UNUserNotificationCenterDelegate.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "IOSUserNotificationCenterDelegate.h"
#include "DataStore.h"
#include "EngineVirtuals.h"
#include "IOSUtil.h"
#include "SeoulAssert.h"
#include "ThreadId.h"

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000

@implementation IOSUserNotificationCenterDelegate

+ (void) handleNotification:(UNNotification *)notification
				 wasRunning:(bool)bWasRunning
			wasInForeground:(bool)bWasInForeground
{
	using namespace Seoul;
	
	SEOUL_ASSERT(IsMainThread());
	
	DataStore dataStore;
	NSDictionary *pUserInfo = notification.request.content.userInfo;
	if (pUserInfo != nil)
	{
		(void)ConvertToDataStore(pUserInfo, dataStore);
	}
	
	String sAlertBody;
	if (notification.request.content.body != nil)
	{
		sAlertBody = String(notification.request.content.body);
	}
	
	// Ensure that the boolean type is Seoul::Bool and not Objective-C BOOL
	Bool bIsRemote = [notification.request.trigger
					  isKindOfClass:[UNPushNotificationTrigger class]] ? true : false;

	g_pEngineVirtuals->OnReceivedOSNotification(bIsRemote, bWasRunning, bWasInForeground, dataStore, sAlertBody);
}

// Called when the app is opened from the BACKGROUND via a notification
// selection. (BACKGROUND includes when the app is not running.)
//
// NOTE: This may be called for remote notifications as well as local
// notifications. Background remote notifications are routed here or
// to didReceiveRemoteNotification depending on which type of remote
// notification they are.
- (void) userNotificationCenter:(UNUserNotificationCenter *)center
 didReceiveNotificationResponse:(UNNotificationResponse *)response
		  withCompletionHandler:(void (^)())completionHandler
{
	if (response.actionIdentifier == UNNotificationDismissActionIdentifier)
	{
		// Nop, the user dismissed our notification and did not attempt any actions
		// with the app.
		completionHandler();
		return;
	}
	
	[IOSUserNotificationCenterDelegate
	 handleNotification:response.notification wasRunning:false wasInForeground:false];
	
	// We must call the completion handler to indicate we
	// are done.
	completionHandler();
}

// Called when a notification will show while the app is in the foreground.
//
// NOTE: This may be called for remote notifications as well as local
// notifications. Foreground remote notifications are routed here or
// to didReceiveRemoteNotification depending on which type of remote
// notification they are.
- (void) userNotificationCenter:(UNUserNotificationCenter *)center
		willPresentNotification:(UNNotification *)notification
		  withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler
{
	[IOSUserNotificationCenterDelegate
	 handleNotification:notification wasRunning:true wasInForeground:true];
	
	// We must call the completion handler to indicate we
	// are done. Pass None to silence the notification.
	completionHandler(UNNotificationPresentationOptionNone);
}

@end

#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000
