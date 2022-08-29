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

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.lang.RuntimeException;
import java.lang.Throwable;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

public final class AndroidNativeCrashLog
{
	static final int kiMaxStackTraceSize = 8 * 1024 * 1024;

	static final String kKeyAppPackage = "Package";
	static final String kKeyAppVersionCode = "Version Code";
	static final String kKeyAppVersionName = "Version Name";
	static final String kKeyCrashDate = "Date";
	static final String kKeyDeviceManufacturer = "Manufacturer";
	static final String kKeyDeviceModel = "Model";
	static final String kKeyOsBuild = "Android Build";
	static final String kKeyOsVersion = "Android";
	static final String kKeyThreadName = "Thread";

	final Date m_CrashDate;
	final AndroidNativeCrashLogConstants m_Constants;
	final String m_sThreadName;
	final String m_StackTrace;

	void WriteLine(final Writer writer, final String sKey, final String sValue) throws IOException
	{
		writer.write(sKey + ": " + sValue + "\n");
	}

	public AndroidNativeCrashLog(final Date date, final AndroidNativeCrashLogConstants constants, final String sThreadName, final Throwable exception)
	{
		m_CrashDate = date;
		m_Constants = constants;
		m_sThreadName = sThreadName;

		final Writer stackTraceWriter = new StringWriter();
		final Writer boundedStackTraceWriter = new AndroidLimitedWriter(stackTraceWriter, kiMaxStackTraceSize);
		exception.printStackTrace(new PrintWriter(boundedStackTraceWriter));

		m_StackTrace = stackTraceWriter.toString();
	}

	public void Write(final File file) throws IOException
	{
		BufferedWriter writer = null;

		try
		{
			final SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US);
			dateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));

			writer = new BufferedWriter(new FileWriter(file));

			WriteLine(writer, kKeyAppPackage, m_Constants.m_sAppPackage);
			WriteLine(writer, kKeyAppVersionCode, m_Constants.m_sAppVersionCode);
			WriteLine(writer, kKeyAppVersionName, m_Constants.m_sAppVersionName);
			WriteLine(writer, kKeyOsVersion, m_Constants.m_sOsVersion);
			WriteLine(writer, kKeyOsBuild, m_Constants.m_sOsBuild);
			WriteLine(writer, kKeyDeviceManufacturer, m_Constants.m_sDeviceManufacturer);
			WriteLine(writer, kKeyDeviceModel, m_Constants.m_sDeviceModel);
			WriteLine(writer, kKeyThreadName, m_sThreadName);
			WriteLine(writer, kKeyCrashDate, dateFormat.format(m_CrashDate));

			writer.write("\n");
			writer.write(m_StackTrace);
			writer.flush();
		}
		finally
		{
			if (writer != null)
			{
				writer.close();
			}
		}
	}
}
