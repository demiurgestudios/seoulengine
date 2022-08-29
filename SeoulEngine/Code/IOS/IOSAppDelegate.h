/**
 * \file IOSAppDelegate.h
 * \brief Base class implementation for an iOS application delegate.
 * Contains app agnostic functionality to be specialized further
 * in an app specific subclass.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_APP_DELEGATE_H
#define IOS_APP_DELEGATE_H

#include "InputEditTextViewController.h"
#include "Prereqs.h"                    // for SEOUL_WITH_APPS_FLYER

#import <UIKit/UIAlertView.h>
#import <UIKit/UIKit.h>

@class CAEAGLLayer;
@class DummyKeyboardView;

#if SEOUL_WITH_APPS_FLYER
// Parameters to store from the following URL Open calls so they can send to AppsFlyer after it has
// completed initialization:
// application:handleOpenURL:
// application:openURL:options:
// application:openURL:sourceApplication:annotation:
@interface PendingOpenURLParams : NSObject
@property(retain, readonly) NSURL* pURL;
@property(retain, readonly) NSDictionary<UIApplicationOpenURLOptionsKey,id>* pOptions;
@property(retain, readonly) NSString* pSourceApplication;
@property(retain, readonly) id annotation;

- (id)initWithURL:(NSURL*)pURL
		andOptions:(NSDictionary<UIApplicationOpenURLOptionsKey,id>*)pOptions
		andSourceApp:(NSString*)pSourceApplication
		andAnnotation:(id)annotation;
@end
#endif // SEOUL_WITH_APPS_FLYER

@interface IOSAppDelegate : UIResponder

- (void)initWindow:(UIColor *)backgroundColor;
- (CAEAGLLayer *)eaglLayer;

- (void)showInputEditTextView;
- (void)hideInputEditTextView;

- (void)handleTouchMove:(NSUInteger)uTouchIndex;
- (void)handleTouchEnd:(NSUInteger)uTouchIndex;
- (void)checkAndHandleReleasedTouch:(NSUInteger)uTouchIndex;
- (void)cleanTouches;

@property(retain, nonatomic) UIWindow *window;
@property(retain, nonatomic) UIView *view;
@property(retain, nonatomic) InputEditTextViewController *inputEditTextController;
@property bool m_bReleaseSystemBindingLock;

#if SEOUL_WITH_APPS_FLYER
- (void)initAppsFlyerData;
- (void)delayedAppsFlyerSetDelegateAndGetConversionData;

// Array of Restoration Handlers: (void(^)(NSArray * __nullable restorableObjects))
// Stored from calls to application:continueUserActivity:restorationHandler:
// Forwarded to AppsFlyer after deferred initialization.
@property(retain, readonly) NSMutableArray *m_aContinuationRestorationHandlers;
// Array of User Acitivities: NSUserActivity*
// Stored from calls to application:continueUserActivity:restorationHandler:
// Forwarded to AppsFlyer after deferred initialization.
@property(retain, readonly) NSMutableArray *m_aContinuationActivities;
// Parameters from URL Open calls to store and relay to AppsFlyer after deferred initialization.
@property(retain, readonly) NSMutableArray *m_aAppsFlyerURLParams;
// The URL the game was launched with, reserved so that we can hand it to AppsFlyer once
// it initializes
@property(retain, readwrite) NSURL *m_pLaunchURL;
#endif // SEOUL_WITH_APPS_FLYER

@end

#endif  // Include guard
