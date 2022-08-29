/**
 * \file AndroidAppsFlyerImpl.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

// System
import android.content.SharedPreferences;

import java.util.Map;

// Null variation - used on projects that don't include AppsFlyer.
public final class AndroidAppsFlyerImpl
{
	public AndroidAppsFlyerImpl(final AndroidNativeActivity activity)
	{
	}

	public void AppsFlyerTrackEvent(
		final String sEventID,
		final boolean bLiveBuild)
	{
	}

	public void AppsFlyerSessionChange(
		final boolean bIsSessionStart,
		final String sSessionUUID,
		final long iTimeStampMicrosecondsUnixUTC,
		final long iDurationMicroseconds,
		final boolean bLiveBuild)
	{
	}

	public void handleDeepLinking(Map<String, String> tData)
	{
	}

	public void AppsFlyerInitialize(
		final SharedPreferences prefs,
		final String sUniqueUserID,
		final String sID,
		final String sDeepLinkCampaignScheme,
		final String sChannel,
		final boolean bIsUpdate,
		final boolean bEnableDebugLogging,
		final boolean bLiveBuild)
	{
	}

	public void AppsFlyerShutdown()
	{
	}
}
