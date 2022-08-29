/**
 * \file SeoulUtil.mm
 * \brief iOS specific implementation of Objective-C functionality
 * tied to the SeoulUtil miscellaneous bucket of engine utilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#import "UIKit/UIKit.h"
#include "Engine.h"
#include "JobsFunction.h"
#include "InputManager.h"
#include "JobsManager.h"
#include "LocManager.h"
#include "Logger.h"
#include "SeoulString.h"
#include "SeoulUtil.h"

namespace Seoul
{

// TODO: This can be eliminated now that Jobs::AsyncFunction and variations
// support more flexible binding and generic functions.
/**
 * Utility, wraps arguments to bound functions.
 */
struct IOSMessageBoxArguments
{
	IOSMessageBoxArguments()
		: m_sMessage()
		, m_sTitle()
		, m_OnCompleteCallback()
		, m_iButtonCount(1)
	{
	}

	String m_sMessage;
	String m_sTitle;
	MessageBoxCallback m_OnCompleteCallback;
	Int m_iButtonCount;
	String m_sButtonLabels[3];
};

using namespace Seoul;

namespace Platform
{

static void IOSMainThreadMessageBoxActionCallback(
	Bool bReleaseSystemBindingLock,
	MessageBoxCallback onCompleteCallback,
	Int iTotalButtonCount,
	Int iButtonIndex)
{
	if (onCompleteCallback)
	{
		// Determine which button was pressed after unshuffling the button order
		EMessageBoxButton eButtonPressed;
		switch (iTotalButtonCount)
		{
		default:
			eButtonPressed = kMessageBoxButtonOK;
			break;

		case 2:
			eButtonPressed = (iButtonIndex == 0 ? kMessageBoxButtonYes : kMessageBoxButtonNo);
			break;

		case 3:
			if (iButtonIndex == 2)
			{
				eButtonPressed = kMessageBoxButton3;
			}
			else if (iButtonIndex == 1)
			{
				eButtonPressed = kMessageBoxButton2;
			}
			else  // index == 0
			{
				eButtonPressed = kMessageBoxButton1;
			}

			break;
		}

		onCompleteCallback(eButtonPressed);
	}

	if (bReleaseSystemBindingLock && InputManager::Get())
	{
		InputManager::Get()->DecrementSystemBindingLock();
	}
}

static void IOSMainThreadShowMessageBox(const IOSMessageBoxArguments& arguments)
{
	UIViewController* pRootView = [[[[UIApplication sharedApplication] delegate] window] rootViewController];
	if (pRootView == nil)
	{
		// Don't warn, we might have been sent here by a warning
		SEOUL_LOG("IOSShowMessageBox: Cannot show message box, root view is nil.");
		return;
	}

	// TODO: Set a callback which will unblock the thread waiting for us, if a
	// thread other than the render thread is displaying a message box

	Bool bReleaseSystemBindingLock = false;
	if (InputManager::Get())
	{
		InputManager::Get()->IncrementSystemBindingLock();
		bReleaseSystemBindingLock = true;
	}

	// See https://developer.apple.com/documentation/uikit/uialertcontroller?language=objc
	UIAlertController* pAlertController =
		[UIAlertController alertControllerWithTitle:arguments.m_sTitle.ToNSString()
											message:arguments.m_sMessage.ToNSString()
									 preferredStyle:UIAlertControllerStyleAlert];

	// Add other buttons to the alert view
	for (Int i = 0; i < arguments.m_iButtonCount; i++)
	{
		// NOTE: Copy attributes of the arguments that we need onto the stack, so that they
		// will then be copied by the Objective-C block below. If we just referenced
		// the const reference arguments in the block, we would copy the reference
		// pointer into the block and hit some funky memory errors.
		MessageBoxCallback callbackCopy = arguments.m_OnCompleteCallback;
		Int iButtonCountCopy = arguments.m_iButtonCount;

		[pAlertController addAction:[UIAlertAction actionWithTitle:arguments.m_sButtonLabels[i].ToNSString()
															 style:UIAlertActionStyleDefault
														   handler:^(UIAlertAction* _Nonnull action)
		// Line alignment makes this hard to see, but this is the "block" for the handler
		{
			if (IsMainThread())
			{
				IOSMainThreadMessageBoxActionCallback(bReleaseSystemBindingLock,
													  callbackCopy,
													  iButtonCountCopy,
													  i);
			}
			else if (Jobs::Manager::Get())
			{
				Jobs::AsyncFunction(
					GetMainThreadId(),
					&IOSMainThreadMessageBoxActionCallback,
					bReleaseSystemBindingLock,
					callbackCopy,
					iButtonCountCopy,
					i);
			}
			else
			{
				SEOUL_LOG("IOSShowMessageBox: Cannot respond to a message box selection from off of the"
						  "main thread, there's no Jobs::Manager.");
			}
		}]];
	}

	// Display the message box
	[pRootView presentViewController:pAlertController animated:YES completion:nil];
}

static void IOSShowMessageBox(
	const String& sMessage,
	const String& sTitle,
	MessageBoxCallback onCompleteCallback,
	EMessageBoxButton eDefaultButton,
	const String& sButtonLabel1,
	const String& sButtonLabel2,
	const String& sButtonLabel3)
{
	// Not used by iOS. Only Windows build uses this field now.
	(void)eDefaultButton;

	IOSMessageBoxArguments arguments;
	arguments.m_sMessage = sMessage;
	arguments.m_sTitle = sTitle;
	arguments.m_OnCompleteCallback = onCompleteCallback;

	if (!sButtonLabel3.IsEmpty())
	{
		arguments.m_iButtonCount = 3;
		arguments.m_sButtonLabels[0] = sButtonLabel1;
		arguments.m_sButtonLabels[1] = sButtonLabel2;
		arguments.m_sButtonLabels[2] = sButtonLabel3;
	}
	else if (!sButtonLabel2.IsEmpty())
	{
		arguments.m_iButtonCount = 2;
		arguments.m_sButtonLabels[0] = sButtonLabel1;
		arguments.m_sButtonLabels[1] = sButtonLabel2;
	}
	else
	{
		arguments.m_iButtonCount = 1;
		arguments.m_sButtonLabels[0] = sButtonLabel1;
	}

	if (IsMainThread())
	{
		IOSMainThreadShowMessageBox(arguments);
	}
	else if (Jobs::Manager::Get())
	{
		Jobs::AsyncFunction(
			GetMainThreadId(),
			&IOSMainThreadShowMessageBox,
			arguments);
	}
	else
	{
		SEOUL_LOG("IOSShowMessageBox: Cannot show message box from off of the main thread, there's no Jobs::Manager\n");
	}
}

/**
 * iOS-specific core function table
 */
const CoreVirtuals g_IOSCoreVirtuals =
{
	&IOSShowMessageBox,
	&LocManager::CoreLocalize,
	&Engine::CoreGetPlatformUUID,
	&Engine::CoreGetUptime,
};

/**
 * iOS-specific core function table pointer
 */
const CoreVirtuals *g_pCoreVirtuals = &g_IOSCoreVirtuals;

}  // namespace Platform

} // namespace Seoul
