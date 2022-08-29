/**
 * \file LocalNotificationReceiver.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.ComponentName;
import android.os.Bundle;
import android.os.Build;
import android.os.PersistableBundle;
import android.util.Log;

import java.util.Iterator;

/**
 * Handle local broadcasts, which are broadcasts that are internal to the
 * application. Only our application can send them.
 *
 * This receiver is the fall-back handler that will post/update the client's
 * entry in the Android notification toolbar. If the game is in the
 * foreground, you should intercept the broadcast with your own receiver at a
 * higher priority, then call abortBroadcast() to prevent the fallback
 * behavior.
 */
public class LocalNotificationReceiver extends BroadcastReceiver
{
	// Request to Open URL
	public static final String ACTION_BROADCAST_LOCAL = BuildConfig.LOCAL_NOTIFICATION_ACTION_BROADCAST_LOCAL;

	/**
	 * Create and send a broadcast intent with the given bundle of arguments.
	 * We create an ordered broadcast, so you can pre-empt the handler if
	 * necessary.
	 *
	 * @param context the sending context
	 * @param arguments a bundle of parameters to go with the broadcast
	 */
	public static void BroadcastIntent(Context context, Bundle arguments)
	{
		if (AndroidLogConfig.ENABLED)
		{
			Log.i(AndroidLogConfig.TAG, "LocalNotificationReceiver BroadcastIntent.");
		}

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

	@Override
	public void onReceive(Context context, Intent intent)
	{
		// If the main activity doesn't intercept the broadcast, then tell the
		// notification service to post the intent on the status bar.

		// In Android 8.0 and greater starting a service from a background service will fail.
		// The recommended approach is to instead use a Job to run the intent.
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
		{
			Bundle extras = intent.getExtras();
			PersistableBundle persistableExtras = new PersistableBundle();
			Iterator<String> itr = extras.keySet().iterator();
			while(itr.hasNext())
			{
				String key = itr.next();
				Object o = extras.get(key);
				if (o instanceof String)
					persistableExtras.putString(key, (String)o);
				else if (o instanceof Integer)
					persistableExtras.putInt(key, (Integer)o);
				else if (o instanceof Boolean)
					persistableExtras.putBoolean(key, (Boolean)o);
			}

			android.app.job.JobScheduler jobScheduler = (android.app.job.JobScheduler)context.getSystemService(Context.JOB_SCHEDULER_SERVICE);
			ComponentName jobService = new ComponentName(context.getPackageName(), LocalNotificationJobService.class.getName());
			android.app.job.JobInfo jobInfo = new android.app.job.JobInfo.Builder(AndroidNativeActivity.JOB_ID_PUSH_NOTIFICATION, jobService)
					.setOverrideDeadline(0)
					.setTriggerContentMaxDelay(0)
					.setTriggerContentUpdateDelay(0)
					.setExtras(persistableExtras)
					.build();
			jobScheduler.schedule(jobInfo);
		}

		// Pre 8.0 use the existing service behavior.
		else
		{
			LocalNotificationService.PostNotification(context, intent.getExtras());
		}
	}
}
