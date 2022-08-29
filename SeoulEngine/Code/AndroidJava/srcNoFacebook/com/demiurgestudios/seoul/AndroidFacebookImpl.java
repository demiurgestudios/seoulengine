/**
 * \file AndroidFacebookImpl.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

// System
import android.content.Intent;

// Null variation - used on projects that don't include Facebook.
public class AndroidFacebookImpl
{
	private AndroidNativeActivity m_Activity;

	public AndroidFacebookImpl(final AndroidNativeActivity activity)
	{
	}

	public boolean onActivityResult(int iRequestCode, int iResultCode, Intent intent)
	{
		return false;
	}

	public void onResume()
	{
	}

	public void FacebookInitialize(final boolean bEnableDebugLogging)
	{
	}

	public void FacebookLogin(String[] aRequestedPermissions)
	{
	}

	public void FacebookCloseSession()
	{
	}

	public boolean FacebookIsLoggedIn()
	{
		return false;
	}

	public String FacebookGetAuthToken()
	{
		return "";
	}

	public String FacebookGetUserID()
	{
		return "";
	}

	public void FacebookGetFriendsList(final String sFields, final int iLimit)
	{
	}

	public void FacebookSendRequest(final String sMessage, final String sTitle, final String sRecipients, final String sSuggestedFriends, final String sData)
	{
	}

	public void FacebookDeleteRequest(final String sRequestID)
	{
	}

	public void FacebookSendPurchaseEvent(final double fLocalPrice, final String sCurrencyCode)
	{
	}

	public void FacebookTrackEvent(final String sEventID)
	{
	}

	public void FacebookShareLink(final String sContentURL)
	{
	}

	public void FacebookRequestBatchUserInfo(final String[] aUserIDs)
	{
	}
}
