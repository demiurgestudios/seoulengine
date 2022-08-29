/**
 * \file LocalNotificationJobService.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.annotation.TargetApi;
import android.app.job.JobParameters;
import android.app.job.JobService;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.PersistableBundle;
import android.util.Log;

@TargetApi(21)
public class LocalNotificationJobService extends JobService
{
	@Override
	public boolean onStartJob(JobParameters params)
	{
		Boolean bBackgrounded = AndroidNativeActivity.InBackground();
		if(!bBackgrounded)
		{
			return false;
		}

		PersistableBundle extras = params.getExtras();

		String message = extras.getString("LocalizedMessage");
		// Let MixPanel push notifications override the message with the mp_message field.
		if(extras.containsKey("mp_message")) { message = extras.getString("mp_message"); }
		String actionButton = extras.getString("LocalizedActionButtonText");
		// Let MixPanel push notifications override the title with the mp_title field.
		if (extras.containsKey("mp_title")) { actionButton = extras.getString("mp_title"); }

		LocalNotification.notify(
				message,
				actionButton,
				extras.getString("OpenUrl"),
				LocalNotification.getPersistableExtraIntDefault(extras, "ID", LocalNotification.NOTIFICATION_ID),
				LocalNotification.getPersistableExtraBooleanDefault(extras, "bPlaySound", false),
				LocalNotification.getPersistableExtraBooleanDefault(extras, "bHasActionButton", false),
				extras.getString("UserData"),
				this);

		// There is new thread processing work for this job. All work is complete.
		return false;
	}

	@Override
	public boolean onStopJob(JobParameters params)
	{
		// False to drop the job, we do not need to retry.
		return false;
	}
}
