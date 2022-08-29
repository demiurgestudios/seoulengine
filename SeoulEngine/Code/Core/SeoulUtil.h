/**
 * \file SeoulUtil.h
 * \brief Miscellaneous core utilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_UTIL_H
#define SEOUL_UTIL_H

#include "CoreVirtuals.h"
#include "Delegate.h"
#include "SeoulString.h"

namespace Seoul
{

/**
 * Globally enable/disable all message boxes - useful for command-line utilities
 * using SeoulEngine functionality. Message boxes are enabled by default.
 */
extern Bool g_bEnableMessageBoxes;

/**
 * Platform-dependent implementation for displaying a 1-button message
 * dialog box; may be a nop on some platforms.
 */
void ShowMessageBox(
	const String& sMessage,
	const String& sTitle = String(),
	MessageBoxCallback onCompleteCallback = MessageBoxCallback());

/**
 * Platform-dependent implementation for displaying a 2-button message box.
 * onCompleteCallback must be valid and will be invoked with the button which
 * was pressed to dismiss it.  If the message box could not be displayed,
 * onCompleteCallback will be invoked with eDefaultButton.
 *
 * @param[in] sYesLabel If not the empty string, the text that will label
 * the 'Yes' button - not supported on all platforms.
 * @param[in] sNoLabel If not the empty string, the text that will label
 * the 'No' button - not supported on all platforms.
 * @param[in] onCompleteCallback If defined, called when the message box is
 * dismissed.
 */
void ShowMessageBoxYesNo(
	const String& sMessage,
	const String& sTitle,
	MessageBoxCallback onCompleteCallback,
	EMessageBoxButton eDefaultButton = kMessageBoxButtonNo,
	const String& sYesLabel = String(),
	const String& sNoLabel = String());

/**
 * Platform-dependent implementation for displaying a 3-button message box.
 * onCompleteCallback must be valid and will be invoked with the button which
 * was pressed to dismiss it.  If the message box could not be displayed,
 * onCompleteCallback will be invoked with eDefaultButton.
 *
 * @param[in] sMessage Message to display
 * @param[in] sTitle Title of the message box to display
 * @param[in] onCompleteCallback Callback to call when the message box is
 *              dismissed
 * @param[in] eDefaultButton Default button to select if an error occurred
 * @param[in] sButtonLabel1 Label of the first button
 * @param[in] sButtonLabel2 Label of the second button
 * @param[in] sButtonLabel3 Label of the third button
 */
void ShowMessageBox3Button(
	const String& sMessage,
	const String& sTitle,
	MessageBoxCallback onCompleteCallback,
	EMessageBoxButton eDefaultButton,
	const String& sButtonLabel1,
	const String& sButtonLabel2,
	const String& sButtonLabel3);

// Helper function to compare two version strings
// Returns <0, 0, or >0 if sVersion1 is <, ==, or > then sVersion2 respectively
Int CompareVersionStrings(const String& sVersion1, const String& sVersion2);

} // namespace Seoul

#endif // include guard
