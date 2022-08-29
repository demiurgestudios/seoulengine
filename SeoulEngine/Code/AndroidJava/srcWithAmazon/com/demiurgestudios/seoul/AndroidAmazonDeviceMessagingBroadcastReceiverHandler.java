/**
 * \file AndroidAmazonDeviceMessagingBroadcastReceiverHandler.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.amazon.device.messaging.ADMMessageHandlerBase;
import com.amazon.device.messaging.ADMMessageReceiver;

public class AndroidAmazonDeviceMessagingBroadcastReceiverHandler extends ADMMessageHandlerBase
{
	public static class AndroidAmazonDeviceMessagingBroadcastReceiver extends ADMMessageReceiver
	{
		public AndroidAmazonDeviceMessagingBroadcastReceiver()
		{
			super(AndroidAmazonDeviceMessagingBroadcastReceiverHandler.class);
		}
	}

	@Override
	protected void onRegistered(final String sNewRegistrationId)
	{
		if (AndroidLogConfig.ENABLED)
		{
			Log.v(AndroidLogConfig.TAG, "ADM registered: " + sNewRegistrationId);
		}
		AndroidAmazonDeviceMessaging.OnRegisteredForRemoteNotifications(sNewRegistrationId);
	}

	@Override
	protected void onUnregistered(final String sRegistrationId)
	{
		if (AndroidLogConfig.ENABLED)
		{
			Log.v(AndroidLogConfig.TAG, "ADM unregistered: " + sRegistrationId);
		}
	}

	@Override
	protected void onRegistrationError(final String sErrorId)
	{
		if (AndroidLogConfig.ENABLED)
		{
			Log.v(AndroidLogConfig.TAG, "ADM registration Error: " + sErrorId);
		}
	}

	@Override
	protected void onMessage(final Intent intent)
	{
		final Context context = getApplicationContext();

		// Rebroadcast the ADM message as a local broadcast.
		LocalNotificationReceiver.BroadcastIntent(context, intent.getExtras());
	}

	public AndroidAmazonDeviceMessagingBroadcastReceiverHandler()
	{
		super(AndroidAmazonDeviceMessagingBroadcastReceiverHandler.class.getName());
	}
}
