/**
 * \file AndroidGoogleAnalyticsImpl.java
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
import com.google.firebase.analytics.FirebaseAnalytics;

public class AndroidGoogleAnalyticsImpl
{
	private FirebaseAnalytics mFirebaseAnalytics;
	private AndroidNativeActivity m_Activity;

	public AndroidGoogleAnalyticsImpl(final AndroidNativeActivity activity)
	{
		m_Activity = activity;
	}

	public void OnCreate()
	{
		mFirebaseAnalytics = FirebaseAnalytics.getInstance(m_Activity);
	}
}
