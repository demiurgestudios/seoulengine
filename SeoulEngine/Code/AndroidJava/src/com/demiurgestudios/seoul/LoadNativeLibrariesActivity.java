/**
 * \file LoadNativeLibrariesActivity.java
 *
 * Helper activity to load the native libraries needed by SeoulEngine.  If
 * loading a native library fails, this shows an alert dialog informing the
 * user to try to reinstall the app.  This can't be done inside the main activity
 * since the main activity extends android.app.NativeActivity, and the latter
 * requires its native library to be loaded to initialize properly.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class LoadNativeLibrariesActivity extends Activity
{
	/**
	 * List of native shared libraries to load
	 */
	private static final String[] NATIVE_LIBRARIES =
	{
		AndroidFmodImpl.kLowLevelLibName,
		AndroidFmodImpl.kHighLevelLibName,
		BuildConfig.NATIVE_ANDROID_LIBNAME,
	};

	/**
	 * Index of the native shared library which failed to load, or -1 if they
	 * all loaded successfully.
	 */
	private static final int s_iFailedLibraryIndex;

	static
	{
		// Try to load each native shared library.  If this fails, we
		// have no fallback, so inform the user to reinstall the
		// application.
		int iFailedLibraryIndex = -1;
		for (int i = 0; i < NATIVE_LIBRARIES.length; i++)
		{
			// Handle libraries that may have been conditionally removed/not
			// available in this build.
			if (null == NATIVE_LIBRARIES[i])
			{
				continue;
			}

			try
			{
				System.loadLibrary(NATIVE_LIBRARIES[i]);
			}
			catch (UnsatisfiedLinkError e)
			{
				e.printStackTrace();
				iFailedLibraryIndex = i;
				break;
			}
		}

		s_iFailedLibraryIndex = iFailedLibraryIndex;
	}

	/**
	 * Helper function to ensure that this class's static initializer gets run
	 * to load the native libraries
	 */
	public static void forceStaticInit()
	{
		// No-op
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// If we successfully loaded all of our needed native shared libraries,
		// start up the real main activity.  If it's already running, just
		// bring it to the top of the activity stack instead of starting it
		// again.
		if (s_iFailedLibraryIndex == -1)
		{
			Intent intent = new Intent();
			intent.setComponent(new ComponentName(BuildConfig.ANDROID_LAUNCH_PACKAGE, BuildConfig.ANDROID_LAUNCH_NAME));
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_FORWARD_RESULT);

			if (getIntent() != null)
			{
				// Copy any data and extras sent to us to the main activity
				// activity
				intent.fillIn(getIntent(), 0);
			}

			startActivity(intent);

			// We're done with this helper activity
			finish();
		}
		else
		{
			// If we failed to load a native shared library, inform the user
			// that the installation may have failed.
			AlertDialog.Builder builder = new AlertDialog.Builder(this);
			builder.setTitle(R.string.initialization_failed_title);
			builder.setMessage(getResources().getString(R.string.initialization_failed_body, s_iFailedLibraryIndex));
			builder.setIcon(android.R.drawable.stat_notify_error);
			builder.setCancelable(false);
			builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener()
			{
				public void onClick(DialogInterface dialog, int which)
				{
					LoadNativeLibrariesActivity.this.finish();
				}
			});
			builder.create().show();
		}
	}
}
