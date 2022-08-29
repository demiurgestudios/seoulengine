/**
 * \file IOSUserNotificationCenterDelegate.h
 * \brief Implementation for an iOS UNUserNotificationCenterDelegate.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_USER_NOTIFICATION_CENTER_DELEGATE_H
#define IOS_USER_NOTIFICATION_CENTER_DELEGATE_H

#import <UserNotifications/UserNotifications.h>

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000

@interface IOSUserNotificationCenterDelegate : NSObject<UNUserNotificationCenterDelegate>

+ (void) handleNotification:(UNNotification *)notification
				 wasRunning:(bool)bWasRunning
			wasInForeground:(bool)bWasInForeground;

@end

#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 100000

#endif  // Include guard
