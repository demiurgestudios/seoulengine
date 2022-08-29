/**
 * \file LocalNotificationService.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.app.IntentService;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

public class LocalNotificationService extends IntentService
{
	public static final String ACTION_BROADCAST_LOCAL = BuildConfig.LOCAL_NOTIFICATION_ACTION_BROADCAST_LOCAL;
	public static final String ACTION_POST_NOTIFICATION = BuildConfig.LOCAL_NOTIFICATION_ACTION_POST_NOTIFICATION;
	public static final String ACTION_CONSUME_NOTIFICATIONS = BuildConfig.LOCAL_NOTIFICATION_ACTION_CONSUME_NOTIFICATION;

	/**
	 * Create and send a broadcast intent with the given bundle of arguments.
	 * We create an ordered broadcast, so you can pre-empt the handler if
	 * necessary.
	 *
	 * @param context the sending context
	 * @param arguments a bundle of parameters to go with the broadcast
	 */
	static void BroadcastIntent(Context context, Bundle arguments)
	{
		Intent i = new Intent(ACTION_BROADCAST_LOCAL).setPackage(context.getApplicationContext().getPackageName()).putExtras(arguments);
		// User Data is just the message data. It should carry over in
		// the pending intent on the notification bar.
		String message = arguments.getString("LocalizedMessage");
		if (message != null)
		{
			i.putExtra("UserData", message.trim());
		}

		String url = arguments.getString("OpenUrl");
		if (url != null)
		{
			i.putExtra("OpenUrl", url.trim());
		}

		context.sendOrderedBroadcast(i, null);
	}

	/**
	 * Initiate a request to post a notification to the status bar.
	 *
	 * @param context context that requests the consume
	 * @param arguments a bundle of optional arguments
	 */
	public static void PostNotification(Context context, Bundle arguments)
	{
		// Consume all pending intents.
		ComponentName target = new ComponentName(context.getApplicationContext(), LocalNotificationService.class);
		context.startService(new Intent(LocalNotificationService.ACTION_POST_NOTIFICATION).putExtras(arguments).setComponent(target));
	}
	/**
	 * Initiate a request to consume notifications (clear out notifications
	 * from the status bar).
	 *
	 * @param context context that requests the consume
	 */
	public static void ConsumeNotifications(Context context)
	{
		// Consume all pending intents.
		ComponentName target = new ComponentName(context.getApplicationContext(), LocalNotificationService.class);
		context.startService(new Intent(LocalNotificationService.ACTION_CONSUME_NOTIFICATIONS).setComponent(target));
	}

	public LocalNotificationService()
	{
		super("LocalNotificationService");
	}

	@Override
	protected void onHandleIntent(Intent intent)
	{
		if (intent == null)
		{
			return;
		}

		String action = intent.getAction();
		if (action != null)
		{
			if (action.compareTo(ACTION_POST_NOTIFICATION) == 0)
			{
				String message = intent.getStringExtra("LocalizedMessage");
				// Let MixPanel push notifications override the message with the mp_message field.
				if(intent.hasExtra("mp_message")) { message = intent.getStringExtra("mp_message"); }
				String actionButton = intent.getStringExtra("LocalizedActionButtonText");
				// Let MixPanel push notifications override the title with the mp_title field.
				if (intent.hasExtra("mp_title")) { actionButton = intent.getStringExtra("mp_title"); }

				LocalNotification.notify(
						message,
						actionButton,
						intent.getStringExtra("OpenUrl"),
						LocalNotification.getIntentIntExtraDefault(intent, "ID", LocalNotification.NOTIFICATION_ID),
						LocalNotification.getIntentBooleanExtraDefault(intent, "bPlaySound", false),
						LocalNotification.getIntentBooleanExtraDefault(intent, "bHasActionButton", false),
						intent.getStringExtra("UserData"),
						this);
				return;
			}
			if (action.compareTo(ACTION_CONSUME_NOTIFICATIONS) == 0)
			{
				LocalNotification.clearNotifications(this);
				return;
			}
		}

		// Relay the intent; normally, the LocalNotificationReceiver will
		// handle the broadcast unless the main activity intercepts it (when
		// it's in the foreground).
		BroadcastIntent(this,  intent.getExtras());
	}
}
