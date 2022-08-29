/**
 * \file AndroidNativeCrashManagerExceptionHandler.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.os.Process;
import java.io.File;
import java.io.IOException;
import java.lang.System;
import java.lang.Thread;
import java.lang.Thread.UncaughtExceptionHandler;
import java.lang.Throwable;
import java.util.Date;
import java.util.UUID;

public final class AndroidNativeCrashManagerExceptionHandler implements UncaughtExceptionHandler
{
	final String m_sCrashDumpDirectory;
	final AndroidNativeCrashLogConstants m_Constants;
	final UncaughtExceptionHandler m_Prev;

	void SaveCrashDump(Thread thread, Throwable exception) throws IOException
	{
		// Configure.
		final Date now = new Date();
		final String sThreadName = (null != thread ? (thread.getName() + "-" + thread.getId()) : "");
		final AndroidNativeCrashLog crashLog = new AndroidNativeCrashLog(now, m_Constants, sThreadName, exception);

		crashLog.Write(new File(m_sCrashDumpDirectory, UUID.randomUUID().toString() + ".dmp"));
	}

	public AndroidNativeCrashManagerExceptionHandler(final String sCrashDumpDirectory, final AndroidNativeCrashLogConstants constants, final UncaughtExceptionHandler prev)
	{
		m_sCrashDumpDirectory = sCrashDumpDirectory;
		m_Constants = constants;
		m_Prev = prev;
	}

	public void uncaughtException(Thread thread, Throwable exception)
	{
		// Commit.
		try
		{
			SaveCrashDump(thread, exception);
		}
		catch (Exception e)
		{
			// Nop
		}

		// Propagate to the previous uncaught exception handler, if
		// one was defined.
		if (null != m_Prev)
		{
			m_Prev.uncaughtException(thread, exception);
		}
	}
}
