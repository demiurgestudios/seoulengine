/**
 * \file AndroidNativeCrashManager.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import java.lang.Thread;
import java.lang.Thread.UncaughtExceptionHandler;

public final class AndroidNativeCrashManager
{
	static native boolean NativeCrashManagerIsEnabled();
	final boolean m_bEnabled;

	static void RegisterUnhandledExceptionHook(final String sCrashDumpDirectory, final AndroidNativeCrashLogConstants constants)
	{
		// Get the current handler - either wrap or reuse.
		UncaughtExceptionHandler prev = Thread.getDefaultUncaughtExceptionHandler();
		if (!(prev instanceof AndroidNativeCrashManagerExceptionHandler))
		{
			Thread.setDefaultUncaughtExceptionHandler(new AndroidNativeCrashManagerExceptionHandler(sCrashDumpDirectory, constants, prev));
		}
	}

	public AndroidNativeCrashManager(final String sCrashDumpDirectory, final AndroidNativeCrashLogConstants constants)
	{
		m_bEnabled = NativeCrashManagerIsEnabled();

		// Nothing more to do if not enabled.
		if (!m_bEnabled)
		{
			return;
		}

		// Register exception hooks.
		RegisterUnhandledExceptionHook(sCrashDumpDirectory, constants);
	}
}
