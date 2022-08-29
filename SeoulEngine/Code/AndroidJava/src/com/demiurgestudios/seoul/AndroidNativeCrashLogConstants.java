/**
 * \file AndroidNativeCrashLog.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;

public class AndroidNativeCrashLogConstants
{
	public String m_sAppPackage;
	public String m_sAppVersionCode;
	public String m_sAppVersionName;
	public String m_sDeviceManufacturer;
	public String m_sDeviceModel;
	public String m_sOsBuild;
	public String m_sOsVersion;

	@SuppressLint("NewApi")
	static int GetAppVersionCode(final PackageInfo packageInfo)
	{
		try
		{
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
			{
				@SuppressWarnings("deprecation")
				int ret = packageInfo.versionCode;
				return ret;
			}
			else
			{
				// TODO: As is too often, kind of a mess on Android. If we ever
				// want to use the new higher bits available as of P, we have to *require*
				// P, so practically, we can only use the lower 32-bits.
				return (int)packageInfo.getLongVersionCode();
			}
		}
		catch (Exception e)
		{
			return 0;
		}
	}

	public AndroidNativeCrashLogConstants(final Context context)
	{
		// Defaults.
		m_sAppPackage = "<exception>";
		m_sAppVersionCode = "<exception>";
		m_sAppVersionName = "<exception>";

		try
		{
			final PackageManager packageManager = context.getPackageManager();
			final PackageInfo packageInfo = packageManager.getPackageInfo(context.getPackageName(), 0);
			m_sAppPackage = packageInfo.packageName;
			m_sAppVersionCode = String.valueOf(GetAppVersionCode(packageInfo));
			m_sAppVersionName = packageInfo.versionName;
		}
		catch (Exception e)
		{
			// Nop
		}

		m_sDeviceManufacturer = Build.MANUFACTURER;
		m_sDeviceModel = Build.MODEL;
		m_sOsBuild = Build.DISPLAY;
		m_sOsVersion = Build.VERSION.RELEASE;
	}
}
