/**
 * \file AndroidAmazonDeviceMessaging.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.content.Context;
import android.util.Log;

import com.amazon.device.messaging.ADM;
import java.lang.Class;
import java.lang.ClassNotFoundException;
import java.lang.RuntimeException;

public class AndroidAmazonDeviceMessaging
{
	private static boolean s_bInProgress = false;

	public static void RegisterForRemoteNotifications(final Context context)
	{
		if (!s_bInProgress)
		{
			// From Amazon developer website, process to check for the existence of ADM.
			boolean bADMIsAvailable = false;
			try
			{
				Class.forName("com.amazon.device.messaging.ADM");
				bADMIsAvailable = true;
			}
			catch (ClassNotFoundException e)
			{
				e.printStackTrace();
				bADMIsAvailable = false;
			}

			// If we don't have ADM on the current Amazon device, return without further processing.
			if (!bADMIsAvailable)
			{
				return;
			}

			s_bInProgress = true;

			try
			{
				final ADM adm = new ADM(context);
				if (adm.isSupported())
				{
					String sRegistrationID = adm.getRegistrationId();
					if (null == sRegistrationID)
					{
						adm.startRegister();
					}
					else
					{
						OnRegisteredForRemoteNotifications(sRegistrationID);
					}
				}
			}
			catch (RuntimeException e)
			{
				s_bInProgress = false;
				e.printStackTrace();
			}
		}
	}

	public static void OnRegisteredForRemoteNotifications(final String sRegistrationId)
	{
		AndroidNativeActivity.NativeOnRegisteredForRemoteNotifications(sRegistrationId);
		s_bInProgress = false;
	}
}
