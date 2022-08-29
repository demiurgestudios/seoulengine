/**
 * \file IOSAppDelegate.mm
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

#include "IOSAppDelegate.h"
#include "InputManager.h"
#include "IOSEngine.h"
#include "ITextEditable.h"
#include "LocManager.h"
#include "IOSTrackingManager.h"

#import <UIKit/UIAlertView.h>
#import <UIKit/UIKit.h>
#import <UIKit/UITextField.h>

#if SEOUL_WITH_APPS_FLYER
@implementation PendingOpenURLParams
@synthesize pURL = _pURL;
@synthesize pOptions = _pOptions;
@synthesize pSourceApplication = _pSourceApplication;
@synthesize annotation = _annotation;
- (id)initWithURL:(NSURL*)pURL
		andOptions:(NSDictionary<UIApplicationOpenURLOptionsKey,id>*)pOptions
		andSourceApp:(NSString*)pSourceApplication
		andAnnotation:(id)annotation
{
	if(self = [super init])
	{
		_pURL = [pURL retain];
		_pOptions = [pOptions retain];
		_pSourceApplication = [pSourceApplication retain];
		_annotation = [annotation retain];
		return self;
	}
	return nil;
}
- (void)dealloc
{
	if (_pURL != nil) { [_pURL release]; }
	if (_pOptions != nil) { [_pOptions release]; }
	if (_pSourceApplication != nil) { [_pSourceApplication release]; }
	if (_annotation != nil) { [_annotation release]; }
	[super dealloc];
}
@end
#endif // SEOUL_WITH_APPS_FLYER

using namespace Seoul;

/** Global array of touch instances for tracking. */
static FixedArray<UITouch*, (TouchButtonLast - TouchButtonFirst + 1)> s_apTouches;

@implementation IOSAppDelegate

#if SEOUL_WITH_APPS_FLYER
@synthesize m_aContinuationRestorationHandlers = _m_aContinuationRestorationHandlers;
@synthesize m_aContinuationActivities = _m_aContinuationActivities;
@synthesize m_aAppsFlyerURLParams = _m_aAppsFlyerURLParams;
@synthesize m_pLaunchURL = _m_pLaunchURL;
#endif // SEOUL_WITH_APPS_FLYER

- (void)dealloc
{
	self.view = nil;
	self.window = nil;
	
	// Release touches we were holding on to
	for (auto i = 0u; i < s_apTouches.GetSize(); ++i)
	{
		if (s_apTouches[i] != nullptr)
		{
			[s_apTouches[i] release];
			s_apTouches[i] = nullptr;
		}
	}

#if SEOUL_WITH_APPS_FLYER
	if (_m_aContinuationRestorationHandlers != nil)
	{
		[_m_aContinuationRestorationHandlers release];
		_m_aContinuationRestorationHandlers = nil;
	};
	if (_m_aContinuationActivities != nil)
	{
		[_m_aContinuationActivities release];
		_m_aContinuationActivities = nil;
	};
	if (_m_aAppsFlyerURLParams != nil)
	{
		[_m_aAppsFlyerURLParams release];
		_m_aAppsFlyerURLParams = nil;
	};
	if (_m_pLaunchURL != nil)
	{
		[_m_pLaunchURL release];
		_m_pLaunchURL = nil;
	}
#endif // SEOUL_WITH_APPS_FLYER

    [super dealloc];
}

/**
 * Initializes the application's window
 */
- (void)initWindow:(UIColor *)backgroundColor
{
	self.window = [[[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]] autorelease];
	self.window.backgroundColor = [UIColor blackColor];
}

#if SEOUL_WITH_APPS_FLYER
/**
 * Initializes the arrays used for holding pending AppsFlyer data.
 */
- (void)initAppsFlyerData
{
	_m_aContinuationRestorationHandlers = [[NSMutableArray alloc] init];
	_m_aContinuationActivities = [[NSMutableArray alloc] init];
	_m_aAppsFlyerURLParams = [[NSMutableArray alloc] init];
}
#endif // SEOUL_WITH_APPS_FLYER

/**
 * Gets the current EAGL layer of the root view.  Must be overridden by a subclass.
 */
- (CAEAGLLayer *)eaglLayer
{
	[self doesNotRecognizeSelector:_cmd];
	return nil;
}

- (void) showInputEditTextView
{
	// Nothing more to do if IOS engine does not exist.
	if (!IOSEngine::Get())
	{
		return;
	}
	
	[self hideInputEditTextView];
	
	self.inputEditTextController = [[InputEditTextViewController alloc] init];
	[(UINavigationController*)self.window.rootViewController presentViewController:self.inputEditTextController animated:NO completion:nil];
	
	if (!self.m_bReleaseSystemBindingLock && InputManager::Get())
	{
		InputManager::Get()->IncrementSystemBindingLock();
	}
	self.m_bReleaseSystemBindingLock = true;
}

- (void) hideInputEditTextView
{
	if (self.inputEditTextController)
	{
		[[self inputEditTextController] dismissViewControllerAnimated:NO completion:nil];
		
		[self.inputEditTextController release];
		self.inputEditTextController = nil;
	}
	
	if (self.m_bReleaseSystemBindingLock && InputManager::Get())
	{
		InputManager::Get()->DecrementSystemBindingLock();
	}
	self.m_bReleaseSystemBindingLock = false;
}

/**
 * Dispatches a move event for the touch at the given index in the global touch array.
 */
- (void)handleTouchMove:(NSUInteger)uTouchIndex;
{
	if (uTouchIndex >= s_apTouches.GetSize())
	{
		return;
	}
	
	auto pTouch = s_apTouches[(UInt)uTouchIndex];
	auto pInput = InputManager::Get();
	if (pTouch && pInput)
	{
		// Need to rescale the point by any contentsScale that is being applied
		// to the OGLES2 layer.
		float fScale = 1.0f;
		CAEAGLLayer* pEAGLLayer = self.eaglLayer;
		if (pEAGLLayer)
		{
			fScale = pEAGLLayer.contentsScale;
		}

		// Dispatch a touch move event.
		CGPoint pt = [pTouch locationInView:self.view];
		pt.x *= fScale;
		pt.y *= fScale;

		pInput->QueueTouchMoveEvent(
			(InputButton)(TouchButtonFirst + uTouchIndex),
			Point2DInt(pt.x, pt.y));
	}
}

/**
 * Dispatches a button release event for the touch at the given index in the global touch array.
 */
- (void)handleTouchEnd:(NSUInteger)uTouchIndex
{
	if (uTouchIndex >= s_apTouches.GetSize())
	{
		return;
	}
	
	if (s_apTouches[(UInt)uTouchIndex] != nullptr)
	{
		// Dispatch a move event right before the release event
		[self handleTouchMove:uTouchIndex];
		
		// Dispatch a button release event
		auto pInput = InputManager::Get();
		if (pInput)
		{
			pInput->QueueTouchButtonEvent((InputButton)(TouchButtonFirst + uTouchIndex), false);
		}
		
		// Release the touch data and remove it from the array
		[s_apTouches[(UInt)uTouchIndex] release];
		s_apTouches[(UInt)uTouchIndex] = nullptr;
	}
}

/**
 * Checks the touch at the given index in the global touch array to see if it should
 * be "released". Releasing a touch dispatches a button release event and removes
 * it from the touch array. The two states that indicate a released touch on iOS are
 * "Ended" and "Cancelled".
 */
- (void)checkAndHandleReleasedTouch:(NSUInteger)uTouchIndex;
{
	if (uTouchIndex >= s_apTouches.GetSize())
	{
		return;
	}
	
	if (s_apTouches[(UInt)uTouchIndex] != nullptr)
	{
		if (s_apTouches[(UInt)uTouchIndex].phase == UITouchPhaseEnded)
		{
			[self handleTouchEnd:uTouchIndex];
		}
		else if (s_apTouches[(UInt)uTouchIndex].phase == UITouchPhaseCancelled)
		{
			// NOTE:: Wire up a proper "Cancelled" input event,
			// instead of pretending the cancelled touch released.
			[self handleTouchEnd:uTouchIndex];
		}
	}
}

/** Convenience function to iterate all our touches and check if they should be released. */
- (void)cleanTouches
{
	for (auto i = 0u; i < s_apTouches.GetSize(); ++i)
	{
		[self checkAndHandleReleasedTouch:i];
	}
}

/** iOS system callback for when a set of touches begin. */
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	// First clean out any "stale" touches
	[self cleanTouches];
	
	auto pInput = InputManager::Get();
	
	// Add new touches one at a time
	for (UITouch* pTouch in touches)
	{
		Int iPlacedIndex = -1;
		for (auto i = 0u; i < s_apTouches.GetSize(); ++i)
		{
			// If this touch already exists in our array, then we should
			// just use this one.
			if (s_apTouches[i] == pTouch)
			{
				iPlacedIndex = (Int) i;
				break;
			}
			// Place the touch at the first empty slot in the touch array.
			if (s_apTouches[i] == nullptr)
			{
				iPlacedIndex = (Int) i;
				[pTouch retain];
				s_apTouches[i] = pTouch;
				break;
			}
		}
		
		// Dispatch a move event, followed by a button press event.
		// (Move must be dispatched first to avoid creating phantom drags.)
		if (pInput && iPlacedIndex >= 0)
		{
			[self handleTouchMove:(NSUInteger)iPlacedIndex];
			pInput->QueueTouchButtonEvent((InputButton)(TouchButtonFirst + iPlacedIndex), true);
		}
	}
}

/** iOS system callback for when a set of touches end. */
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self cleanTouches];
}

/** iOS system callback for when a set of touches are cancelled. */
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self cleanTouches];
}

/** iOS system callback for when a set of touches move. */
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	for (auto i = 0u; i < s_apTouches.GetSize(); ++i)
	{
		// Clean out any "stale" touches. If one of these happens
		// to also be in the touches set that was passed in, it
		// is ok as move events are dispatched for touches that
		// end.
		[self checkAndHandleReleasedTouch:i];
		
		// Then check if this touch is in the moved set,
		// and dispatch a move event if so.
		if ([touches containsObject:s_apTouches[i]])
		{
			[self handleTouchMove:i];
		}
	}
}

#if SEOUL_WITH_APPS_FLYER
/**
 * Perform delayed initialization of certain portions of AppsFlyer.
 * These operations have side effects which necessitate their delay.
 * Specifically setting the delegate triggers incorrect organic attribution if set too soon.
 * Set the delegate, and perform any other operations that require the delegate to be set.
 * These operations should be performed once.
 */
- (void) delayedAppsFlyerSetDelegateAndGetConversionData
{
	// We use the delegate being set on AppsFlyer to check whether or not we should tell AppsFlyer
	// about openURL and continueUserActivity calls. Snapshot the current unprocessed calls for processing and
	// set the delegate synchronously.
	NSMutableArray* pURLParams = nil;
	NSMutableArray* pContinuationActivities = nil;
	NSMutableArray* pContinuationResorationHandlers = nil;
	@synchronized(self)
	{
		pURLParams = [[[NSMutableArray alloc] initWithArray:_m_aAppsFlyerURLParams] autorelease];
		pContinuationActivities = [[[NSMutableArray alloc] initWithArray:_m_aContinuationActivities] autorelease];
		pContinuationResorationHandlers = [[[NSMutableArray alloc] initWithArray:_m_aContinuationRestorationHandlers] autorelease];
		[_m_aAppsFlyerURLParams removeAllObjects];
		[_m_aContinuationActivities removeAllObjects];
		[_m_aContinuationRestorationHandlers removeAllObjects];
		
		IOSTrackingManager::Get()->DelayedAppsFlyerSetDelegate();
	}
	
	// Now that the delegate is set, let AppsFlyer handle any unprocessed openURL and continueUserActivity calls
	// that were made prior to this point.
	for(id params in pURLParams)
	{
		PendingOpenURLParams* pURLParams = (PendingOpenURLParams*)params;
		if (pURLParams != nil)
		{
			IOSTrackingManager::Get()->HandleOpenURL(pURLParams.pURL, pURLParams.pSourceApplication, pURLParams.pOptions, pURLParams.annotation);
		}
	}
	
	// TODO @appsflyerupgrade: clean this up into one structure instead of two arrays when upgrading the SDK
	unsigned long int iNumContinuations = [pContinuationActivities count];
	for(int iContinuation = 0; iContinuation < iNumContinuations; iContinuation++)
	{
		if([pContinuationActivities objectAtIndex:iContinuation] != nil
			&& [pContinuationResorationHandlers objectAtIndex:iContinuation] != nil)
		{
			IOSTrackingManager::Get()->HandleContinueUserActivity([pContinuationActivities objectAtIndex:iContinuation],
				[pContinuationResorationHandlers objectAtIndex:iContinuation]);
		}
	}
}
#endif // SEOUL_WITH_APPS_FLYER

@end
