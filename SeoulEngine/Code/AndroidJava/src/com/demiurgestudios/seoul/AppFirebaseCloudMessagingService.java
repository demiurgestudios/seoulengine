/**
 * \file AppFirebaseCloudMessagingService.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;
import com.demiurgestudios.seoul.LocalNotification;
import java.lang.NumberFormatException;
import java.util.Map;

public class AppFirebaseCloudMessagingService extends FirebaseMessagingService
{
	public static final int kDefaultNotificationID = 1024;
	public static final String ksAFSender = BuildConfig.LOCAL_NOTIFICATION_AFSENDER;

	private static boolean parseBoolean(Map<String, String> data, String sKey)
	{
		String sValue = data.get(sKey);
		return Boolean.parseBoolean(sValue);
	}

	private static int parseInt(Map<String, String> data, String sKey, int iDefaultValue)
	{
		String sValue = data.get(sKey);
		if (sValue == null)
		{
			return iDefaultValue;
		}

		try
		{
			return Integer.parseInt(sValue);
		}
		catch (NumberFormatException e)
		{
			return iDefaultValue;
		}
	}

	private void issueNotification(RemoteMessage message)
	{
		NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		if (null == notificationManager)
		{
			if (AndroidLogConfig.ENABLED)
			{
				Log.e(AndroidLogConfig.TAG, "Not issuing notification: no notification manager");
			}
			return;
		}

		if (ksAFSender != null && !ksAFSender.isEmpty())
		{
			String from = message.getFrom();

			// Do not rebroadcast messages that go to AppsFlyer.
			// TODO: Configure AppsFlyer Uninstall tracking. This was never actually set up, even
			// when we were using GCM. For FCM it requires registering a second sender with a new API call
			// See: https://firebase.google.com/docs/cloud-messaging/concept-options#receiving-messages-from-multiple-senders
			// This also requires making an AppsFlyer API call for the uninstall.
			// See: https://support.appsflyer.com/hc/en-us/articles/210289286-Uninstall-Measurement#Android-Uninstall
			if ((from != null) && (from.compareTo(ksAFSender) == 0))
			{
				return;
			}
		}

		Map<String, String> data = message.getData();

		String sLocalizedMessage = data.get("LocalizedMessage");
		// Let MixPanel push notifications override the message with the mp_message field.
		if (data.containsKey("mp_message")) { sLocalizedMessage = data.get("mp_message"); }
		if (null == sLocalizedMessage)
		{
			if (AndroidLogConfig.ENABLED)
			{
				Log.e(AndroidLogConfig.TAG, "Not issuing notification: no LocalizedMessage");
			}
			return;
		}

		String sLocalizedActionButtonText = data.get("LocalizedActionButtonText");
		// Let MixPanel push notifications override the title with the mp_title field.
		if (data.containsKey("mp_title")) { sLocalizedActionButtonText = data.get("mp_title"); }

		String sOpenUrl = data.get("OpenUrl");
		String sUserData = data.get("UserData");
		int iNotificationID = parseInt(data, "ID", kDefaultNotificationID);
		boolean bPlaySound = parseBoolean(data, "PlaySound");
		boolean bHasActionButton = parseBoolean(data, "HasActionButton");

		if (AndroidLogConfig.ENABLED)
		{
			Log.e(AndroidLogConfig.TAG, "Issuing local notification: " + sLocalizedMessage);
		}
		LocalNotification.notify(
			sLocalizedMessage,
			sLocalizedActionButtonText,
			sOpenUrl,
			iNotificationID,
			bPlaySound,
			bHasActionButton,
			sUserData,
			this);
	}

	@Override
	public void onMessageReceived(RemoteMessage message)
	{
		issueNotification(message);
	}

	@Override
	public void onNewToken(String token)
	{
		if (AndroidLogConfig.ENABLED)
		{
			Log.d(AndroidLogConfig.TAG, "Firebase new token: " + token);
		}
		AndroidNativeActivity.NativeOnRegisteredForRemoteNotifications(token);
	}
}

