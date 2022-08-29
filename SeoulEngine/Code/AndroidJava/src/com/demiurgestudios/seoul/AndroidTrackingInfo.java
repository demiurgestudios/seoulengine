/**
 * \file AndroidTrackingInfo.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

import java.lang.reflect.Method;

/** Captures and contains advertiser oriented user tracking and install attribution. */
public class AndroidTrackingInfo
{
	/** com.google.android.gms.common.ConnectionResult.SUCCESS */
	public static final int kiConnectionResultSuccess = 0;

	// Private constructors
	AndroidTrackingInfo()
	{
	}

	/** Utility, get a method from a class. Handles the "not found" case and returns null. */
	static Method getMethod(Class<?> classObject, String sMethodName, Class<?>... parameterTypes)
	{
		try
		{
			return classObject.getMethod(sMethodName, parameterTypes);
		}
		catch (NoSuchMethodException e)
		{
			return null;
		}
	}

	/** Utility, get a method from a named class. Handles the "not found" case and returns null. */
	static Method getMethod(String sClassName, String sMethodName, Class<?>... parameterTypes)
	{
		try
		{
			Class<?> classObject = Class.forName(sClassName);
			return getMethod(classObject, sMethodName, parameterTypes);
		}
		catch (ClassNotFoundException e)
		{
			return null;
		}
	}

	/** Utility, account for recent gms changes - return true if AdvertisingIdClient is available. */
	static boolean HasAdvertisingIdClient()
	{
		try
		{
			// Don't care about return, only about exception.
			Class.forName("com.google.android.gms.ads.identifier.AdvertisingIdClient");
			return true;
		}
		catch (Exception e)
		{
			// Nop - exception expected.
			return false;
		}
	}

	/** Construct AndroidTrackingInfo - explicitly an expensive operation that should be done asynchronously. */
	public static AndroidTrackingInfo CreateTrackingInfoSlow(AndroidNativeActivity activity)
	{
		AndroidTrackingInfo ret = new AndroidTrackingInfo();

		// Safe invocation of GooglePlayServices API to get
		// IDFA info. Use reflection and wrap in Exception handling.
		try
		{
			// Check for availability of advertising - return
			// a default class if not available.
			if (!HasAdvertisingIdClient())
			{
				return ret;
			}

			// Get and check the getAdvertisingIdInfo method.
			Method getAdvertisingIdInfo = getMethod(
				"com.google.android.gms.ads.identifier.AdvertisingIdClient",
				"getAdvertisingIdInfo",
				Context.class);
			if (null == getAdvertisingIdInfo)
			{
				return ret;
			}

			// Get and check advertising info.
			Object advertisingIdInfo = getAdvertisingIdInfo.invoke(null, activity);
			if (null == advertisingIdInfo)
			{
				return ret;
			}

			// Cache the class for further queries.
			Class advertisingIdInfoClass = advertisingIdInfo.getClass();

			// Get the advertising info we care about.

			// ID
			Method getId = getMethod(advertisingIdInfoClass, "getId");
			if (null == getId)
			{
				return ret;
			}
			String sAdvertisingId = (String)getId.invoke(advertisingIdInfo);

			// bLimitTracking.
			Method isLimitAdTrackingEnabled = getMethod(advertisingIdInfoClass, "isLimitAdTrackingEnabled");
			if (null == isLimitAdTrackingEnabled)
			{
				return ret;
			}
			boolean bLimitTracking = (Boolean)isLimitAdTrackingEnabled.invoke(advertisingIdInfo);

			// Set values.
			ret.m_sAdvertisingId = sAdvertisingId;
			ret.m_bLimitTracking = bLimitTracking;
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}

		return ret;
	}

	/** The GooglePlayServices advertiser ID. */
	public String m_sAdvertisingId = "";

	/** If true, the user has requested that tracking using their ID be restricted. */
	public boolean m_bLimitTracking = true;
}
