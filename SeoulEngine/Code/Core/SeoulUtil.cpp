/**
 * \file SeoulUtil.cpp
 * \brief Miscellaneous core utilities.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ContentKey.h"
#include "MemoryManager.h"
#include "Platform.h"
#include "SeoulUtil.h"
#include "StringUtil.h"
#include "Vector.h"

#if SEOUL_PLATFORM_WINDOWS
#include <shellapi.h>
#endif

namespace Seoul
{

/**
 * Globally enable/disable all message boxes - useful for command-line utilities
 * using SeoulEngine functionality. Message boxes are enabled by default.
 */
Bool g_bEnableMessageBoxes = true;

/**
 * Platform-dependent implementation for displaying a message dialog box; may
 * be a nop on some platforms.
 *
 * Note that this can may called in ship builds.
 */
void ShowMessageBox(
	const String& sMessage,
	const String& sTitle /* = String() */,
	MessageBoxCallback onCompleteCallback /* = MessageBoxCallback() */)
{
	// If message boxes are disabled, return immediately.
	if (!g_bEnableMessageBoxes)
	{
		if (onCompleteCallback)
		{
			onCompleteCallback(kMessageBoxButtonOK);
		}
		return;
	}

	// TODO: Localize the default title
	String sActualTitle = !sTitle.IsEmpty() ? sTitle : "Warning";

	// Set localized button label
	static const HString kOkButtonLabel("message_box_ok_button_label");
	String sButtonLabel = g_pCoreVirtuals->Localize(kOkButtonLabel, "OK");

	g_pCoreVirtuals->ShowMessageBox(sMessage, sActualTitle, onCompleteCallback, kMessageBoxButtonOK, sButtonLabel, "", "");
}

/**
 * Display a message box on the current platform with title sTitle
 * and body sMessage, with a "Yes/No" choice.
 */
void ShowMessageBoxYesNo(
	const String& sMessage,
	const String& sTitle,
	MessageBoxCallback onCompleteCallback,
	EMessageBoxButton eDefaultButton /* = kMessageBoxButtonNo */,
	const String& sYesLabel /* = String() */,
	const String& sNoLabel /* = String() */)
{
	// If message boxes are disabled, return immediately.
	if (!g_bEnableMessageBoxes)
	{
		if (onCompleteCallback)
		{
			onCompleteCallback(eDefaultButton);
		}
		return;
	}

	String sButtonLabel1 = sYesLabel;
	String sButtonLabel2 = sNoLabel;

	// Set localized button labels if no labels were specified
	static const HString kYesButtonLabel("yes_no_message_box_yes_button_label");
	static const HString kNoButtonLabel("yes_no_message_box_no_button_label");

	// If a custom button was provided for 'Yes', use that instead of the
	// default localization.
	if (sButtonLabel1.IsEmpty())
	{
		sButtonLabel1 = g_pCoreVirtuals->Localize(kYesButtonLabel, "Yes");
	}

	// If a custom button was provided for 'No', use that instead of the
	// default localization.
	if (sButtonLabel2.IsEmpty())
	{
		sButtonLabel2 = g_pCoreVirtuals->Localize(kNoButtonLabel, "No");
	}

	g_pCoreVirtuals->ShowMessageBox(sMessage, sTitle, onCompleteCallback, eDefaultButton, sButtonLabel1, sButtonLabel2, "");
}

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
	const String& sButtonLabel3)
{
	// If message boxes are disabled, return immediately.
	if (!g_bEnableMessageBoxes)
	{
		if (onCompleteCallback)
		{
			onCompleteCallback(eDefaultButton);
		}
		return;
	}

	g_pCoreVirtuals->ShowMessageBox(sMessage, sTitle, onCompleteCallback, eDefaultButton, sButtonLabel1, sButtonLabel2, sButtonLabel3);
}

/**
 * Helper function to compare two version strings
 *
 * A version string is something like W.X.Y.Z.  Strings are compared piecewise
 * by breaking at the dot separators; if the two corresponding pieces do not
 * match, the numerically smaller one is considered smaller.  Non-numeric
 * pieces are ordered using the usual ASCIIbetical ordering.
 *
 * @return <0 if sVersion1 <  sVersion2
 *          0 if sVersion1 == sVersion2
 *         >0 if sVersion1 >  sVersion2
 */
Int CompareVersionStrings(const String& sVersion1, const String& sVersion2)
{
	// Split each version string into dot-separated pieces
	Vector<String> vPieces1, vPieces2;
	SplitString(sVersion1, '.', vPieces1);
	SplitString(sVersion2, '.', vPieces2);

	UInt nPiecesToCompare = Min(vPieces1.GetSize(), vPieces2.GetSize());
	for (UInt i = 0; i < nPiecesToCompare; i++)
	{
		String sPiece1 = vPieces1[i];
		String sPiece2 = vPieces2[i];

		// Convert each piece to an integer and save the pointer to the end of
		// the integer portion, if any.  Ignores overflow (error ERANGE).
		char *pEnd1, *pEnd2;
		long nValue1 = strtol(sPiece1.CStr(), &pEnd1, 10);
		long nValue2 = strtol(sPiece2.CStr(), &pEnd2, 10);

		// Since strtol returns 0 if no conversion could be performed, map
		// those failures to come after all numeric results
		if (pEnd1 == sPiece1.CStr())
		{
			nValue1 = LONG_MAX;
		}

		if (pEnd2 == sPiece2.CStr())
		{
			nValue2 = LONG_MAX;
		}

		// If the numbers are different, we're done
		if (nValue1 < nValue2)
		{
			return -1;
		}
		else if (nValue1 > nValue2)
		{
			return 1;
		}

		// If the numbers are the same or both are non-numeric, compare the
		// non-numeric portions
		Int nComparison = strcmp(pEnd1, pEnd2);
		if (nComparison != 0)
		{
			return nComparison;
		}
	}

	// If one string has more pieces than the other, consider it to be a higher
	// version
	if (vPieces1.GetSize() < vPieces2.GetSize())
	{
		return -1;
	}
	else if (vPieces1.GetSize() > vPieces2.GetSize())
	{
		return 1;
	}
	else
	{
		// All pieces are the same
		return 0;
	}
}

} // namespace Seoul
