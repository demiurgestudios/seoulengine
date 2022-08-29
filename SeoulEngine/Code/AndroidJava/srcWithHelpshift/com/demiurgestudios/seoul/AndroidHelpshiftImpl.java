/**
 * \file AndroidHelpshiftImpl.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

// System.
import android.net.Uri;
import android.util.Log;
import java.util.Date;
import java.util.HashMap;

// HelpShift
import com.helpshift.Core;
import com.helpshift.HelpshiftUser;
import com.helpshift.InstallConfig;
import com.helpshift.exceptions.InstallException;
import com.helpshift.support.ApiConfig;
import com.helpshift.support.Metadata;
import com.helpshift.support.Support;

public final class AndroidHelpshiftImpl
{
	protected AndroidNativeActivity m_Activity;

	public AndroidHelpshiftImpl(final AndroidNativeActivity activity)
	{
		m_Activity = activity;
	}

	public void HelpShiftInitialize(
		final String sUserName,
		final String sHelpShiftUserID,
		final String sHelpShiftKey,
		final String sHelpShiftDomain,
		final String sHelpShiftID)
	{
		try
		{
			final InstallConfig installConfig = new InstallConfig.Builder().build();
			Core.init(Support.getInstance());
			Core.install(
				m_Activity.getApplication(),
				sHelpShiftKey,
				sHelpShiftDomain,
				sHelpShiftID,
				installConfig);

			final String sName = sUserName;

			final HelpshiftUser user = new HelpshiftUser.Builder(sHelpShiftUserID, null).setName(sName).build();
			com.helpshift.Core.login(user);
			com.helpshift.support.Support.setUserIdentifier(sHelpShiftUserID);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public void HelpShiftShutdown()
	{
	}

	static HashMap<String, String[]> HelpShiftConvertCustomIssueFields(final String[] as)
	{
		if (null == as) { return null; }

		final int len = as.length;
		if (0 == len) { return null; }

		final HashMap<String, String[]> tReturn = new HashMap<String, String[]>();
		for (int i = 2; i < len; i += 3)
		{
			final String sKey = as[i-2];
			final String sType = as[i-1];
			final String sValue = as[i-0];

			tReturn.put(sKey, new String[] { sType, sValue });
		}

		return tReturn;
	}

	static HashMap<String, Object> HelpShiftConvertMetadata(final String[] as)
	{
		if (null == as) { return null; }

		final int len = as.length;
		if (0 == len) { return null; }

		final HashMap<String, Object> tReturn = new HashMap<String, Object>();
		for (int i = 1; i < len; i += 2)
		{
			final String sKey = as[i-1];
			final String sValue = as[i-0];

			tReturn.put(sKey, sValue);
		}

		return tReturn;
	}

	public boolean HelpShiftOpenUrl(
		final String[] asCustomIssueFields,
		final String[] asCustomMetadata,
		final String sURL)
	{
		try
		{
			final HashMap<String, String[]> tCustomIssueFields = HelpShiftConvertCustomIssueFields(asCustomIssueFields);
			final HashMap<String, Object> tMetadata = HelpShiftConvertMetadata(asCustomMetadata);

			final Uri uri = Uri.parse(sURL);
			final ApiConfig config = new ApiConfig.Builder()
				.setCustomIssueFields(tCustomIssueFields)
				.setCustomMetadata(new Metadata(tMetadata))
				.build();

			// HelpShift URIs should go to the FAQ section.
			com.helpshift.support.Support.showSingleFAQ(m_Activity, uri.getHost(), config);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return false;
		}

		return true;
	}

	public boolean HelpShiftShowHelp(
		final String[] asCustomIssueFields,
		final String[] asCustomMetadata)
	{
		try
		{
			final HashMap<String, String[]> tCustomIssueFields = HelpShiftConvertCustomIssueFields(asCustomIssueFields);
			final HashMap<String, Object> tMetadata = HelpShiftConvertMetadata(asCustomMetadata);

			final ApiConfig config = new ApiConfig.Builder()
				.setCustomIssueFields(tCustomIssueFields)
				.setCustomMetadata(new Metadata(tMetadata))
				.build();

			com.helpshift.support.Support.showFAQs(m_Activity, config);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return false;
		}

		return true;
	}
}
