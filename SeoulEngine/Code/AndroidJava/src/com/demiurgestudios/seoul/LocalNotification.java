/**
 * \file LocalNotification.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.annotation.TargetApi;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.media.AudioAttributes;
import android.net.Uri;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import java.lang.RuntimeException;

/**
 * Common logic for displaying an Android notification using the NotificationManager.
 */
public class LocalNotification
{
	private static final String NOTIFICATION_CHANNEL_ID = BuildConfig.LOCAL_NOTIFICATION_CHANNEL_ID;
	private static final String NOTIFICATION_CHANNEL_NAME = BuildConfig.LOCAL_NOTIFICATION_CHANNEL_NAME;

	/**
	 * All notifications will use this ID and replace each other.
	 */
	public static int NOTIFICATION_ID = 1024;

	/**
	 * Helper method for parsing extra parameters from an Intent bundle, used in pre-8.0 code paths
	 */
	public static int getIntentIntExtraDefault(Intent intent, String key, int defaultInt)
	{
		return getExtraIntDefault(intent.getExtras(), key, defaultInt);
	}

	/**
	 * Helper method for parsing extra parameters from an Intent bundle, used in pre-8.0 code paths
	 */
	public static int getExtraIntDefault(Bundle extras, String key, int defaultInt)
	{
		int result = defaultInt;
		if(extras.containsKey(key))
		{
			Object object = extras.get(key);
			if (object.getClass().equals(Integer.class))
			{
				result = (int) object;
			}
			else if (object.getClass().equals(String.class))
			{
				try
				{
					result = Integer.parseInt((String) object);
				}
				catch (NumberFormatException e)
				{
				}
			}
		}
		return result;
	}

	/**
	 * Helper method for parsing extra parameters from a job PersistableBundle, used in 8.0+ code paths
	 */
	@TargetApi(21)
	public static int getPersistableExtraIntDefault(PersistableBundle extras, String key, int defaultInt)
	{
		int result = defaultInt;
		if(extras.containsKey(key))
		{
			Object object = extras.get(key);
			if (object.getClass().equals(Integer.class))
			{
				result = (int) object;
			}
			else if (object.getClass().equals(String.class))
			{
				try
				{
					result = Integer.parseInt((String) object);
				}
				catch (NumberFormatException e)
				{
				}
			}
		}
		return result;
	}

	/**
	 * Helper method for parsing extra parameters from an Intent bundle, used in pre-8.0 code paths
	 */
	public static boolean getIntentBooleanExtraDefault(Intent intent, String key, boolean defaultBoolean)
	{
		return getExtraBooleanDefault(intent.getExtras(), key, defaultBoolean);
	}

	/**
	 * Helper method for parsing extra parameters from an Intent bundle, used in pre-8.0 code paths
	 */
	public static boolean getExtraBooleanDefault(Bundle extras, String key, boolean defaultBoolean)
	{
		boolean result = defaultBoolean;
		if(extras.containsKey(key))
		{
			Object object = extras.get(key);
			if(object.getClass().equals(Boolean.class))
			{
				result = (boolean)object;
			}
			else if(object.getClass().equals(String.class))
			{
				result = Boolean.parseBoolean((String) object);
			}
		}
		return result;
	}

	/**
	 * Helper method for parsing extra parameters from a job PersistableBundle, used in 8.0+ code paths
	 */
	@TargetApi(21)
	public static boolean getPersistableExtraBooleanDefault(PersistableBundle extras, String key, boolean defaultBoolean)
	{
		boolean result = defaultBoolean;
		if(extras.containsKey(key))
		{
			Object object = extras.get(key);
			if(object.getClass().equals(Boolean.class))
			{
				result = (boolean)object;
			}
			else if(object.getClass().equals(String.class))
			{
				result = Boolean.parseBoolean((String) object);
			}
		}
		return result;
	}

	/**
	 * Create a notification
	 */
	public static void notify(String sLocalizedMessage,
							  String sLocalizedActionButtonText,
							  String sOpenUrl,
							  int iNotificationID,
							  boolean bPlaySound,
							  boolean bHasActionButton,
							  String sUserData,
							  Context context)
	{
		NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
		if (null != notificationManager)
		{
			// Extract settings from the Intent - note that, these all end up as string types, so we parse them into desired
			// types manually
			SharedPreferences preferences = context.getSharedPreferences(LocalNotificationService.class.getSimpleName(), Context.MODE_PRIVATE);
			int total = preferences.getInt(String.valueOf(iNotificationID), 0) + 1;

			// Setup notification channel
			if (android.os.Build.VERSION.SDK_INT >= 26)
			{
				NotificationChannel channel = new NotificationChannel(
						NOTIFICATION_CHANNEL_ID,
						NOTIFICATION_CHANNEL_NAME,
						NotificationManager.IMPORTANCE_DEFAULT);
				channel.enableVibration(true);
				notificationManager.createNotificationChannel(channel);
			}

			// Lookup the notification id.
			final int iNotificationId = context.getResources().getIdentifier(BuildConfig.LOCAL_NOTIFICATION_ICON_NAME, "drawable", context.getPackageName());

			// Setup the Notification.
			NotificationCompat.Builder builder = new NotificationCompat.Builder(context, NOTIFICATION_CHANNEL_ID);
			builder.setContentText(sLocalizedMessage);
			builder.setContentTitle(sLocalizedActionButtonText);
			builder.setSmallIcon(iNotificationId);
			builder.setChannelId(NOTIFICATION_CHANNEL_ID);

			// BigTextStyle only available in 22.1+
			// https://developer.android.com/reference/android/support/v4/app/NotificationCompat.BigTextStyle
			if (android.os.Build.VERSION.SDK_INT > 22)
			{
				builder.setStyle(new NotificationCompat.BigTextStyle().bigText(sLocalizedMessage));
			}

			// The first notification shows the content text. Subsequent
			// notifications should include the notification count.
			if (total > 1)
			{
				builder.setNumber(total);
			}

			// Update the number of notifications left.
			Editor e = preferences.edit();
			e.putInt(String.valueOf(iNotificationID), total);
			e.apply();

			Intent notificationIntent = null;
			if (sOpenUrl != null && !sOpenUrl.isEmpty())
			{
				Uri pageUri = Uri.parse(sOpenUrl);
				notificationIntent = new Intent(Intent.ACTION_VIEW, pageUri);
			}
			else
			{
				// Send the user to the game when he clicks the intent.
				notificationIntent = new Intent();
				notificationIntent.setComponent(new ComponentName(BuildConfig.ANDROID_LAUNCH_PACKAGE, BuildConfig.ANDROID_LAUNCH_NAME));
				notificationIntent.putExtra("UserData", sUserData);
			}

			if (bHasActionButton)
			{
				// If the notification is supposed to have an action button, set
				// custom attributes when launching the activity.
				notificationIntent.putExtra("IsNotification", true);
			}

			notificationIntent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
			PendingIntent pendingIntent = PendingIntent.getActivity(context, iNotificationID, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);
			builder.setContentIntent(pendingIntent);

			// Issue the notification.
			Notification notification = builder.build();
			notification.flags |= Notification.FLAG_AUTO_CANCEL;

			try
			{
				notificationManager.notify(iNotificationID, notification);
			}
			// Happens during OS shutdown, catching to reduce crashservice noise.
			//
			// Specifically, DeadSystemException, which will be wrapped in
			// a RuntimeException.
			catch (RuntimeException ex)
			{
				ex.printStackTrace();
			}
		}
	}

	/**
	 * Clear out all visible notifications that we track.
	 */
	public static void clearNotifications(Context context)
	{
		SharedPreferences preferences = context.getSharedPreferences(LocalNotificationService.class.getSimpleName(), Context.MODE_PRIVATE);
		NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
		if (null != notificationManager)
		{
			for (String key : preferences.getAll().keySet())
			{
				// Clear out all posted notifications.
				try
				{
					notificationManager.cancel(Integer.valueOf(key));
				}
				catch (NumberFormatException ex)
				{
					ex.printStackTrace();
				}
			}
		}

		Editor e = preferences.edit();
		e.clear();
		e.apply();
	}
}
