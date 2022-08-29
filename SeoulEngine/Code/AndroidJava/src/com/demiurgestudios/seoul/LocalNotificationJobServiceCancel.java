/**
 * \file LocalNotificationJobServiceCancel.java
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
import android.util.Log;

@TargetApi(21)
public class LocalNotificationJobServiceCancel extends JobService
{
	@Override
	public boolean onStartJob(JobParameters params)
	{
		LocalNotification.clearNotifications(this);

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
