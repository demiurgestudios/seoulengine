/**
 * \file AndroidNativeActivity.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.ActivityManager;
import android.app.AlarmManager;
import android.app.Dialog;
import android.app.PendingIntent;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PermissionInfo;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.PersistableBundle;
import android.provider.Settings;
import android.support.v4.content.LocalBroadcastManager;
import android.text.Editable;
import android.text.InputFilter;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.Toast;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.provider.Settings.Secure;
import android.support.v4.app.ActivityCompat;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.Choreographer;
import android.view.Display;
import android.view.WindowManager;
import android.widget.Toast;

import com.google.android.gms.games.Games;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.firebase.FirebaseApp;
import com.google.firebase.iid.FirebaseInstanceId;
import com.google.firebase.iid.InstanceIdResult;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.lang.IllegalStateException;
import java.lang.SuppressWarnings;
import java.lang.reflect.Field;
import java.lang.Runnable;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Arrays;
import java.util.Currency;
import java.util.HashMap;
import java.util.Locale;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.Calendar;
import java.util.Date;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.Locale;
import java.util.TimeZone;

public class AndroidNativeActivity extends android.app.NativeActivity implements ComponentCallbacks2
{
	/**
	 * Filenames used to identify a LIVE Google Play build vs. a LIVE Amazon build.
	 */
	public static final String ksAmazonLiveFilename = "a03e8f09bd0bf96e2255f82d794afd90b526c1781f74b41b03dcc7ebab199db1";
	public static final String ksGooglePlayLiveFilename = "a219f49e3c2cd2d29c21da4a93c4b587724ea7d209a51d946019de699841d141";
	public static final String ksSamsungLiveFilename = "9e4955f29d704dcf891e9464180557569ab4acb067524e9387531ef534681a96";

	// DO NOT rename these unless you need to - these symbols
	// are sent to the server as-is, and renaming
	// them will break purchase verification until someone notices.
	//
	// Keep in-sync with the equivalent enum in AppAndroid.cpp.
	public enum EBuildType
	{
		/** Build type has not yet been determined, check for specific files in the APK to determine the build type. */
		kUnknown,

		/** Build is a development build. It reports non-LIVE and talks to Amazon services. */
		kAmazonDevelopment,

		/** Build is a LIVE build that should talk to Amazon services. */
		kAmazonLive,

		/** Build is a development build. It reports non-LIVE and talks to Google Play services. */
		kGooglePlayDevelopment,

		/** Build is a LIVE build that should talk to Google Play services. */
		kGooglePlayLive,

		/** Build is a development build. It reports non-LIVE and talks to Samsung services. */
		kSamsungDevelopment,

		/** Build is a LIVE build that should talk to Samsung services. */
		kSamsungLive,
	}

	/**
	 * Enumeration of different buttons which can be displayed by message boxes
	 *
	 * NOTE: Must be kept in sync with the EMessageBoxButton enum in native
	 * code in SeoulUtil.h
	 */
	public enum EMessageBoxButton
	{
		// 1-button message box buttons
		kMessageBoxButtonOK,

		// 2-button message box buttons
		kMessageBoxButtonYes,
		kMessageBoxButtonNo,

		// 3-button message box buttons
		kMessageBoxButton1,
		kMessageBoxButton2,
		kMessageBoxButton3
	}

	///////////////////////////////////////////////////////////////////////////
	// CrashManager for Java crashes.
	///////////////////////////////////////////////////////////////////////////
	AndroidNativeCrashManager m_CrashManager = null;

	///////////////////////////////////////////////////////////////////////////
	//
	// Cache of tracking info. Used to reduce the cost of these queries.
	//
	///////////////////////////////////////////////////////////////////////////
	/** 1 hour in milliseconds for easy comparison. */
	private static final long kiTrackingInfoUpdateFrequencyInMilliseconds = (3600 * 1000);
	ScheduledExecutorService m_TrackingInfoScheduledExecutorService = Executors.newScheduledThreadPool(1);
	AndroidTrackingInfo m_TrackingInfo = null;

	/**
	 * Timer, used to limit request frequency - we use the same interval as the
	 * Facebook SDK, which is 1 hour.
	 */
	long m_iTrackingInfoLastUpdateTimeInMilliseconds = 0;

	/** Only necessary to workaround limitations of Java closures. Sad. */
	Choreographer.FrameCallback m_ChoreographerCallback = null;

	/** Cache of preferences for later access. */
	SharedPreferences m_EnginePreferences;

	/** Build type caching and tracking. */
	EBuildType m_eBuildType = EBuildType.kUnknown;

	/** False by default - subclasses can selectively enable immersive mode with a call to Configure(). */
	boolean m_bImmersiveMode;

	/** Class name name by default - subclasses can override with a call to configure. */
	String m_sSharedPreferencesName;

	/** Guarantee the body of onCreate() is only ever called once per instance of this activity. */
	final AndroidOnce m_OnCreateOnce = new AndroidOnce();

	///////////////////////////////////////////////////////////////////////////
	//
	// Begin native method hooks - these are defined in AndroidNativeActivity.inl
	// in SeoulEngine native code.
	//
	///////////////////////////////////////////////////////////////////////////
	protected static native void NativeMessageBoxCallbackInvoke(long pUserData, int iButtonPressed);
	protected static native void NativeTrackingInfoCallbackInvoke(long pUserData, String sAdvertisingId, boolean bLimitTracking);

	protected static native void NativeOnVsync();
	protected static native void NativeSetCanPerformNativeStartup(boolean bCanPerformNativeStartup);
	protected static native void NativeOnWindowInsets(int iTop, int iBottom);

	////////////////////////////////////////////////////////////////////////////
	//
	// Commerce
	//
	////////////////////////////////////////////////////////////////////////////
	private AndroidCommerceManager m_CommerceManager = null;

	///////////////////////////////////////////////////////////////////////////
	//
	// Code shared across all SeoulEngine apps.
	//
	///////////////////////////////////////////////////////////////////////////
	public AndroidNativeActivity()
	{
		// Defaults.
		m_bImmersiveMode = false;
		m_sSharedPreferencesName = AndroidNativeActivity.class.getSimpleName();

		// Initialize.
		m_AppsFlyerImpl = new AndroidAppsFlyerImpl(this);
		m_HelpshiftImpl = new AndroidHelpshiftImpl(this);
		m_FacebookImpl = new AndroidFacebookImpl(this);
		m_GoogleAnalyticsImpl = new AndroidGoogleAnalyticsImpl(this);
	}

	// Expected to be called from within the constructor of a subclass to (re)configure
	// the default behavior of AndroidNativeActivity.
	protected void Configure(final boolean bImmersiveMode, final String sSharedPreferencesName)
	{
		m_bImmersiveMode = bImmersiveMode;
		m_sSharedPreferencesName = sSharedPreferencesName;
	}

	static String MemoryLevelToString(int level)
	{
		switch (level)
		{
			case ComponentCallbacks2.TRIM_MEMORY_BACKGROUND: return "TRIM_MEMORY_BACKGROUND";
			case ComponentCallbacks2.TRIM_MEMORY_COMPLETE: return "TRIM_MEMORY_COMPLETE";
			case ComponentCallbacks2.TRIM_MEMORY_MODERATE: return "TRIM_MEMORY_MODERATE";
			case ComponentCallbacks2.TRIM_MEMORY_RUNNING_CRITICAL: return "TRIM_MEMORY_RUNNING_CRITICAL";
			case ComponentCallbacks2.TRIM_MEMORY_RUNNING_LOW: return "TRIM_MEMORY_RUNNING_LOW";
			case ComponentCallbacks2.TRIM_MEMORY_RUNNING_MODERATE: return "TRIM_MEMORY_RUNNING_MODERATE";
			case ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN: return "TRIM_MEMORY_UI_HIDDEN";
			default:
				return "MemoryLevelToString<unknown>";
		}
	}

	@Override
	public void onLowMemory()
	{
		if (AndroidLogConfig.ENABLED)
		{
			Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: onLowMemory()");
		}
	}

	@Override
	public void onTrimMemory(int level)
	{
		if (AndroidLogConfig.ENABLED)
		{
			Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: onTrimMemory(): " + MemoryLevelToString(level));
		}
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		// Hook into the decor's window inset callback to report window inset to native. Check here
		// is necessary to satisfy the lint check, this entire workaround will only be applied
		// in configurations >= 29.
		if (Build.VERSION.SDK_INT >= MIN_WINDOW_INSET_API)
		{
			ApplyAndroid10NavBarWorkaround(); // Must be first, may need to reconfigure the window and we only get one shot at that.
		}
		EngineOnCreate(savedInstanceState);
		super.onCreate(savedInstanceState);
		AppOnCreate(savedInstanceState);
	}

	private void EngineOnCreate(Bundle savedInstanceState)
	{
		final AndroidNativeActivity self = this;
		m_OnCreateOnce.Call(new Runnable()
		{
			@Override
			public void run()
			{
				// First thing, register the crash manager.
				final AndroidNativeCrashLogConstants constants = new AndroidNativeCrashLogConstants(self);
				self.m_CrashManager = new AndroidNativeCrashManager(getFilesDir().getAbsolutePath(), constants);

				// Cache and setup.
				final Choreographer choreographer = Choreographer.getInstance();

				// Setup the callback.
				self.m_ChoreographerCallback = new Choreographer.FrameCallback()
				{
					public void doFrame(long iFrameTimeInNanos)
					{
						// Dispatch and reregister.
						NativeOnVsync();
						choreographer.postFrameCallback(m_ChoreographerCallback);
					}
				};
				choreographer.postFrameCallback(m_ChoreographerCallback);

				// Create Google Analytics
				m_GoogleAnalyticsImpl.OnCreate();

				// Cache preferences.
				self.m_EnginePreferences = getSharedPreferences(
					AndroidNativeActivity.class.getSimpleName(),
					Context.MODE_PRIVATE);
			}
		});
	}

	///////////////////////////////////////////////////////////////////////////
	//
	// Begin methods for invocation by native. All methods in this block
	// exist to be called via JNI invocation from various SeoulEngine
	// native code.
	//
	///////////////////////////////////////////////////////////////////////////
	/** Shows the Google Play Store or Kindle store to allow the user to rate this app */
	public void ShowAppStoreToRateThisApp(String sPlayStoreNotFoundMessage)
	{
		try
		{
			if (IsSamsungBuild())
			{
				Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("samsungapps://ProductDetail/" + getPackageName()));
				startActivity(intent);
			}
			else if (IsAmazonBuild())
			{
				Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("amzn://apps/android?p=" + getPackageName()));
				startActivity(intent);
			}
			else
			{
				Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("market://details?id=" + getPackageName()));
				startActivity(intent);
			}
		}
		catch (ActivityNotFoundException e)
		{
			Toast toast = Toast.makeText(this, sPlayStoreNotFoundMessage, Toast.LENGTH_LONG);
			toast.show();
		}
	}

	/** Manual exit trigger - used by unit testing and on final app exit. */
	public void Exit()
	{
		finish();
		System.exit(0);
	}

	/** Wrapper around ApplicationInfo.publicSourceDir */
	public String GetSourceDir()
	{
		try
		{
			final String sPackageName = getPackageName();
			final PackageManager pm = getPackageManager();
			final ApplicationInfo ai = pm.getApplicationInfo(sPackageName, 0);
			return ai.publicSourceDir;
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return "";
		}
	}

	int GetAppVersionCodeBelowP() throws NameNotFoundException
	{
		final PackageInfo packageInfo = getPackageManager().getPackageInfo(getPackageName(), 0);

		@SuppressWarnings("deprecation")
		int ret = packageInfo.versionCode;
		return ret;
	}

	@TargetApi(Build.VERSION_CODES.P)
	int GetAppVersionCodeP() throws NameNotFoundException
	{
		final PackageInfo packageInfo = getPackageManager().getPackageInfo(getPackageName(), 0);

		// TODO: As is too often, kind of a mess on Android. If we ever
		// want to use the new higher bits available as of P, we have to *require*
		// P, so practically, we can only use the lower 32-bits.
		return (int)packageInfo.getLongVersionCode();
	}

	/**
	 * Return the version code (number value) embedded in the current app.
	 */
	@SuppressLint("NewApi")
	public int GetAppVersionCode()
	{
		try
		{
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
			{
				return GetAppVersionCodeBelowP();
			}
			else
			{
				return GetAppVersionCodeP();
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return 0;
		}
	}

	/**
	 * Return the version name embedded in the current app.
	 */
	public String GetAppVersionName()
	{
		try
		{
			PackageInfo packageInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
			return packageInfo.versionName;
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return "";
		}
	}

	/** The target SDK of the App. */
	public int GetBuildSDKVersion()
	{
		return Build.VERSION.SDK_INT;
	}

	/**
	 * @return The current build type, used to determine commerce services and enable/disable of features
	 * such as crash reporting.
	 */
	public EBuildType GetBuildType()
	{
		synchronized(this)
		{
			// If we haven't determined the build type yet.
			if (m_eBuildType == EBuildType.kUnknown)
			{
				try
				{
					// Attempt to open the Amazon marker file.
					InputStream stream = getAssets().open("Data/" + ksAmazonLiveFilename);
					stream.close();

					// Success
					if (BuildConfig.DEBUG)
					{
						// this is an Amazon development build.
						m_eBuildType = EBuildType.kAmazonDevelopment;
					}
					else
					{
						// this is an Amazon live build.
						m_eBuildType = EBuildType.kAmazonLive;
					}

					// Return the type immediately.
					return m_eBuildType;
				}
				catch (IOException e)
				{
					// Nop
				}

				try
				{
					// Attempt to open the Samsung marker file.
					InputStream stream = getAssets().open("Data/" + ksSamsungLiveFilename);
					stream.close();

					// Success
					if (BuildConfig.DEBUG)
					{
						// this is a Samsung development build.
						m_eBuildType = EBuildType.kSamsungDevelopment;
					}
					else
					{
						// this is a Samsung live build.
						m_eBuildType = EBuildType.kSamsungLive;
					}

					// Return the type immediately.
					return m_eBuildType;
				}
				catch (IOException e)
				{
					// Nop
				}

				try
				{
					// Attempt to open the Google Play marker file.
					InputStream stream = getAssets().open("Data/" + ksGooglePlayLiveFilename);
					stream.close();

					// Success
					if (BuildConfig.DEBUG)
					{
						// this is a Google Play development build.
						m_eBuildType = EBuildType.kGooglePlayDevelopment;
					}
					else
					{
						// this is a Google Play live build.
						m_eBuildType = EBuildType.kGooglePlayLive;
					}

					// Return type immediately.
					return m_eBuildType;
				}
				catch (IOException e)
				{
					// Nop
				}

				// Fallback, this is a Google Play development build.
				m_eBuildType = EBuildType.kGooglePlayDevelopment;
			}

			return m_eBuildType;
		}
	}

	/**
	 * The current default system country (ISO 3166 2-letter -or- UN M.49 3-digit).
	 */
	public String GetCountryCode()
	{
		return Locale.getDefault().getCountry();
	}

	/**
	 * A queryable string that describes the manufacturer of the device.
	 */
	public String GetDeviceManufacturer()
	{
		return Build.MANUFACTURER;
	}

	/**
	 * A queryable string that describes the model of the device.
	 */
	public String GetDeviceModel()
	{
		return Build.MODEL;
	}

	/**
	 * A queryable string that describes the id of the device.
	 */
	@SuppressLint("HardwareIds")
	public String GetDeviceId()
	{
		try
		{
			return Secure.getString(getApplicationContext().getContentResolver(), Secure.ANDROID_ID);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return "";
		}
	}

	/** The country code of the user's phone network. */
	public String GetDeviceNetworkCountryCode()
	{
		try
		{
			TelephonyManager tm = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
			if (null != tm)
			{
				return tm.getNetworkCountryIso();
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}

		return "";
	}

	/** The name of the operator of the device's phone network. */
	public String GetDeviceNetworkOperatorName()
	{
		try
		{
			TelephonyManager tm = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
			if (null != tm)
			{
				return tm.getNetworkOperatorName();
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}

		return "";
	}

	/** The country code of the user's SIM card. */
	public String GetDeviceSimCountryCode()
	{
		try
		{
			TelephonyManager tm = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
			if (null != tm)
			{
				return tm.getSimCountryIso();
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}

		return "";
	}

	@TargetApi(17)
	private long GetElapsedRealtimeNanos17()
	{
		return SystemClock.elapsedRealtimeNanos();
	}

	private long GetElapsedRealtimeNanosBelow17()
	{
		return 1000000 * SystemClock.elapsedRealtime();
	}

	/**
	 * Wrapper around android.os.SystemClock.elapsedRealtimeNanos() or
	 * android.os.SystemClock.elapsedRealtime(), depending on API level.
	*/
	public long GetElapsedRealtimeNanos()
	{
		if (android.os.Build.VERSION.SDK_INT >= 17)
		{
			return GetElapsedRealtimeNanos17();
		}
		else
		{
			return GetElapsedRealtimeNanosBelow17();
		}
	}

	/**
	 * Called from C++ to get the name of the source to which we should
	 * attribute the install of the app by the current user, if
	 * the attribution was via Facebook. Returns the empty string
	 * on any failure.
	 */
	public String GetFacebookInstallAttribution()
	{
		try
		{
			// Resolve the launch URL.
			Cursor cursor = getApplicationContext().getContentResolver().query(
				Uri.parse("content://com.facebook.katana.provider.AttributionIdProvider"),  // uri
				new String[]{"aid"},  // projection
				null,  // selection
				null,  // selectionArgs
				null); // sortOrder

			// No instance, return empty string.
			if (cursor == null)
			{
				return "";
			}

			// Grab the attribution value.
			String sAttribution = "";
			if (cursor.moveToFirst())
			{
				sAttribution = cursor.getString(cursor.getColumnIndex("aid"));
			}
			cursor.close();

			// Done.
			return sAttribution;
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return "";
		}
	}

	/**
	 * The current default system language (ISO 639 2-letter).
	 */
	public String GetLanguageIso2Code()
	{
		return Locale.getDefault().getLanguage();
	}

	/**
	 * The current default system language (ISO 639-2/T or base 3-letter code).
	 */
	public String GetLanguageIso3Code()
	{
		return Locale.getDefault().getISO3Language();
	}

	/**
	 * Called from C++ to get the OS name
	 */
	public String GetOsName()
	{
		return "Android OS";
	}

	/**
	 * Called from C++ to get the OS version
	 */
	public String GetOsVersion()
	{
		return Build.VERSION.RELEASE;
	}

	/**
	 * Called from C++ to get the current package name.
	 */
	public String GetPackageName()
	{
		return getPackageName();
	}

	/** @return The screen's horizontal pixels-per-inch. */
	public float GetScreenPPIX()
	{
		return getResources().getDisplayMetrics().xdpi;
	}

	/** @return The screen's vertical pixels-per-inch. */
	public float GetScreenPPIY()
	{
		return getResources().getDisplayMetrics().ydpi;
	}

	/** @return The screen's refresh rate - defaults to 60 FPS. */
	public float GetScreenRefreshRateInHz()
	{
		try
		{
			Display display = ((WindowManager) getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
			return display.getRefreshRate();
		}
		catch (Exception e)
		{
			return 60.0f;
		}
	}

	/** Return a value in seconds representing the total offset from GMT. */
	public int GetTimeZoneOffsetInSeconds()
	{
		try
		{
			TimeZone def = TimeZone.getDefault();
			Date currentTime = Calendar.getInstance(def).getTime();

			int iReturn = (def.getRawOffset() + (def.inDaylightTime(currentTime) ? def.getDSTSavings() : 0)) / 1000;

			return iReturn;
		}
		catch (Exception e)
		{
			return 0;
		}
	}

	/**
	 * @return True if the application is running in immersive mode (full screen
	 * with invisible status+navigation bars), false otherwise.
	 */
	public boolean IsInImmersiveMode()
	{
		return m_bImmersiveMode;
	}

	/**
	 * @return True if the current build should use Amazon services, otherwise it should use Google Play services.
	 */
	public boolean IsAmazonBuild()
	{
		final EBuildType eBuildType = GetBuildType();
		return (EBuildType.kAmazonDevelopment == eBuildType || EBuildType.kAmazonLive == eBuildType);
	}

	/**
	 * @return True if the current build should use Samsung services, otherwise it should use Google Play services.
	 */
	public boolean IsSamsungBuild()
	{
		final EBuildType eBuildType = GetBuildType();
		return (EBuildType.kSamsungDevelopment == eBuildType || EBuildType.kSamsungLive == eBuildType);
	}

	/**
	 * Attempt to open sURL in the native web browser.
	 *
	 * @return True if the action was (apparently, can't be guaranteed)
	 * successful, false otherwise.
	 */
	public boolean OpenURL(String sURL)
	{
		try
		{
			Uri uri = Uri.parse(sURL);
			startActivity(new Intent(Intent.ACTION_VIEW, uri));
		}
		catch (Exception e)
		{
			return false;
		}

		return true;
	}

	/**
	 * @return The current battery level (on [0, 1]).
	 */
	public float QueryBatteryLevel()
	{
		try
		{
			IntentFilter filter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
			Intent batteryLevelIntent = registerReceiver(null, filter);

			int iLevel = batteryLevelIntent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
			int iScale = batteryLevelIntent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);

			return (float)iLevel / (float)iScale;
		}
		catch (Exception e)
		{
			return -1.0f;
		}
	}

	// Keep in sync with NetworkConnectionType.h in native code.
	final int kNetworkConnectionTypeUnknown = 0;
	final int kNetworkConnectionTypeWiFi = 1;
	final int kNetworkConnectionTypeMobile = 2;
	final int kNetworkConnectionTypeWired = 3;

	/**
	 * @return The type of the device's current network connection.
	 */
	public int QueryNetworkConnectionType()
	{
		try
		{
			final ConnectivityManager connManager = (ConnectivityManager) getSystemService(CONNECTIVITY_SERVICE);
			if (connManager == null)
			{
				return -1; // Indicate unsupported query.
			}

			final NetworkInfo netInfo = connManager.getActiveNetworkInfo();
			if (netInfo == null)
			{
				return -1; // Indicate unsupported query.
			}

			final int iType = netInfo.getType();
			if (ConnectivityManager.TYPE_WIFI == iType) { return kNetworkConnectionTypeWiFi; }
			else if (ConnectivityManager.TYPE_MOBILE == iType) { return kNetworkConnectionTypeMobile; }
			else if (ConnectivityManager.TYPE_ETHERNET == iType) { return kNetworkConnectionTypeWired; }
			else
			{
				return kNetworkConnectionTypeUnknown;
			}
		}
		catch (Exception e)
		{
			return -1; // Indicate unsupported query.
		}
	}

	/** @return The current memory usage of the process, in bytes. */
	public long QueryProcessMemoryUsage()
	{
		try
		{
			// getPss is in kilobytes, so multiply by 1024 to get bytes.
			return android.os.Debug.getPss() * 1024;
		}
		catch (Exception e)
		{
			return -1;
		}
	}

	/**
	 * Collect tracking info for the current user and return it. Asynchronous since
	 * certain parts of this query can take a very long time.
	 */
	public void AsyncGetTrackingInfo(final long pUserData)
	{
		final AndroidNativeActivity self = this;

		String sAdvertisingId = "";
		boolean bLimitTracking = true;

		synchronized(m_TrackingInfoScheduledExecutorService)
		{
			// If necessary, kick off async work to populate m_TrackingInfo, either due to no info,
			// or due to a timeout.
			long iDeltaTimeInMilliseconds = (System.currentTimeMillis() - m_iTrackingInfoLastUpdateTimeInMilliseconds);
			if (null == m_TrackingInfo || iDeltaTimeInMilliseconds > kiTrackingInfoUpdateFrequencyInMilliseconds)
			{
				long iTimeToWaitInSeconds = 0;
				try
				{
					// Schedule an async action to populate tracking info.
					m_TrackingInfoScheduledExecutorService.schedule(
						new Runnable()
						{
							public void run()
							{
								try
								{
									// Prepare to cache data we care about.
									String sAdvertisingId = "";
									boolean bLimitTracking = true;
									synchronized(self.m_TrackingInfoScheduledExecutorService)
									{
										// Populate the tracking info.
										self.m_TrackingInfo = AndroidTrackingInfo.CreateTrackingInfoSlow(self);
										self.m_iTrackingInfoLastUpdateTimeInMilliseconds = System.currentTimeMillis();

										// Update local data.
										sAdvertisingId = self.m_TrackingInfo.m_sAdvertisingId;
										bLimitTracking = self.m_TrackingInfo.m_bLimitTracking;
									}

									// Send data back to the callback.
									NativeTrackingInfoCallbackInvoke(
										pUserData,
										sAdvertisingId,
										bLimitTracking);
								}
								catch (Exception e)
								{
									e.printStackTrace();

									// Send initial/"no data" back to the callback.
									NativeTrackingInfoCallbackInvoke(pUserData, "", true);
								}
							}
						},
						iTimeToWaitInSeconds,
						TimeUnit.SECONDS);
				}
				catch (Exception e)
				{
					e.printStackTrace();

					// Send initial/"no data" back to the callback.
					NativeTrackingInfoCallbackInvoke(pUserData, "", true);
				}
				return;
			}
			// Otherwise, immediately report tracking info.
			else
			{
				sAdvertisingId = m_TrackingInfo.m_sAdvertisingId;
				bLimitTracking = m_TrackingInfo.m_bLimitTracking;
			}
		}

		// Send valid tracking info back to the callback.
		NativeTrackingInfoCallbackInvoke(
			pUserData,
			sAdvertisingId,
			bLimitTracking);
	}

	/**
	 * Open a system message box as configured by given arguments.
	 */
	public void ShowMessageBox(
		final String sMessage,
		final String sTitle,
		final long pUserData,
		final String sButtonLabel1,
		final String sButtonLabel2,
		final String sButtonLabel3)
	{
		// Dialogs must be displayed on the UI thread.
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				// Count the buttons
				int iButtonCount = 1;
				if (!sButtonLabel2.isEmpty())
				{
					iButtonCount++;
				}

				if (!sButtonLabel3.isEmpty())
				{
					iButtonCount++;
				}

				final int iFinalButtonCount = iButtonCount;

				// Construct an AlertDialog with the specified message and
				// title.
				AlertDialog.Builder alertDialog = new AlertDialog.Builder(AndroidNativeActivity.this);
				alertDialog.setMessage(sMessage);
				alertDialog.setTitle(sTitle);
				alertDialog.setCancelable(false);

				DialogInterface.OnClickListener onClickListener =
					new DialogInterface.OnClickListener()
					{
						public void onClick(DialogInterface dialog, int which)
						{
							// Figure out which button was pressed
							EMessageBoxButton eButtonPressed;
							if (iFinalButtonCount == 1)
							{
								eButtonPressed = EMessageBoxButton.kMessageBoxButtonOK;
							}
							else if (iFinalButtonCount == 2)
							{
								if (which == DialogInterface.BUTTON_POSITIVE)
								{
									eButtonPressed = EMessageBoxButton.kMessageBoxButtonYes;
								}
								else
								{
									eButtonPressed = EMessageBoxButton.kMessageBoxButtonNo;
								}
							}
							else
							{
								if (which == DialogInterface.BUTTON_POSITIVE)
								{
									eButtonPressed = EMessageBoxButton.kMessageBoxButton1;
								}
								else if (which == DialogInterface.BUTTON_NEUTRAL)
								{
									eButtonPressed = EMessageBoxButton.kMessageBoxButton2;
								}
								else
								{
									eButtonPressed = EMessageBoxButton.kMessageBoxButton3;
								}
							}

							// Call back native code
							if (pUserData != 0)
							{
								NativeMessageBoxCallbackInvoke(pUserData, eButtonPressed.ordinal());
							}

							dialog.dismiss();
						}
					};

				alertDialog.setPositiveButton(sButtonLabel1, onClickListener);

				if (!sButtonLabel2.isEmpty())
				{
					alertDialog.setNeutralButton(sButtonLabel2, onClickListener);
				}

				if (!sButtonLabel3.isEmpty())
				{
					alertDialog.setNegativeButton(sButtonLabel3, onClickListener);
				}

				// Show the dialog.
				alertDialog.create().show();
			}
		});
	}

	///////////////////////////////////////////////////////////////////////////
	// AppsFlyer Bindings
	///////////////////////////////////////////////////////////////////////////
	public static native void NativeSetAttributionData(String sCampaign, String sMediaSource);
	public static native void NativeDeepLinkCampaignDelegate(String sCampaign);
	public static native void NativeSetExternalTrackingUserID(String sJniExternalTrackingUserID);

	protected final AndroidAppsFlyerImpl m_AppsFlyerImpl;

	public void AppsFlyerTrackEvent(
		final String sEventID,
		final boolean bLiveBuild)
	{
		m_AppsFlyerImpl.AppsFlyerTrackEvent(sEventID, bLiveBuild);
	}

	public void AppsFlyerSessionChange(
		final boolean bIsSessionStart,
		final String sSessionUUID,
		final long iTimeStampMicrosecondsUnixUTC,
		final long iDurationMicroseconds,
		final boolean bLiveBuild)
	{
		m_AppsFlyerImpl.AppsFlyerSessionChange(
			bIsSessionStart,
			sSessionUUID,
			iTimeStampMicrosecondsUnixUTC,
			iDurationMicroseconds,
			bLiveBuild);
	}

	public void AppsFlyerInitialize(
		final String sUniqueUserID,
		final String sID,
		final String sDeepLinkCampaignScheme,
		final String sChannel,
		final boolean bIsUpdate,
		final boolean bEnableDebugLogging,
		final boolean bLiveBuild)
	{
		m_AppsFlyerImpl.AppsFlyerInitialize(
			m_EnginePreferences,
			sUniqueUserID,
			sID,
			sDeepLinkCampaignScheme,
			sChannel,
			bIsUpdate,
			bEnableDebugLogging,
			bLiveBuild);
	}

	public void AppsFlyerShutdown()
	{
		m_AppsFlyerImpl.AppsFlyerShutdown();
	}

	///////////////////////////////////////////////////////////////////////////
	// GooglePlayGames login hooks.
	///////////////////////////////////////////////////////////////////////////
	protected final int kiGooglePlayAuthStartDelayMs = 1000; // See comment in GooglePlayGamesSessionStart
	protected AndroidGooglePlayAuth m_GooglePlayAuth;
	protected final AtomicInteger m_iGooglePlayAuthStartStopPhase = new AtomicInteger(0);
	protected boolean m_bGooglePlayResumed = false;
	protected boolean m_bGooglePlayStarted = false;

	public static native void NativeOnPlatformOnlySignInResult(long pCallback, boolean bSuccess);
	public static native void NativeOnSignInCancelled();
	public static native void NativeOnSignInChange(boolean bSignedIn);
	public static native void NativeOnRequestIdToken(
		long pUserData,
		boolean bSuccess,
		String sIdToken);

	// Lifecycle hooks.
	@Override
	protected void onActivityResult(int iRequestCode, int iResultCode, Intent intent)
	{
		super.onActivityResult(iRequestCode, iResultCode, intent);

		// Give Facebook a chance to consume the result.
		if (m_FacebookImpl.onActivityResult(iRequestCode, iResultCode, intent))
		{
			return;
		}

		if (null != m_GooglePlayAuth)
		{
			m_GooglePlayAuth.OnActivityResult(iRequestCode, iResultCode, intent);
		}
	}

	@Override
	protected void onPause()
	{
		super.onPause();
		m_bGooglePlayResumed = false;

		try
		{
			// Unregister the shared notification receiver from ALL filters.
			LocalBroadcastManager.getInstance(this).unregisterReceiver(m_OnNotification);
			unregisterReceiver(m_OnNotification);
		}
		catch (Exception e)
		{
			// Unregistering a receiver that is not registered causes an
			// exception. We should just eat the exception.
			e.printStackTrace();
		}

		// Try to disable AV.
		m_iFocusLevel |= LEVEL_PAUSED;
		applyFocusLevel();

		// Hide the virtual keyboard when this activity gets paused, since
		// otherwise it will stay up on top of the next activity
		if (null != m_TextInput)
		{
			HideVirtualKeyboard();
		}
	}

	@TargetApi(Build.VERSION_CODES.O)
	private void LocalNotificationCancelO()
	{
		android.app.job.JobScheduler jobScheduler = (android.app.job.JobScheduler)getSystemService(Context.JOB_SCHEDULER_SERVICE);
		ComponentName jobService = new ComponentName(getPackageName(), LocalNotificationJobServiceCancel.class.getName());
		android.app.job.JobInfo jobInfo = new android.app.job.JobInfo.Builder(JOB_ID_PUSH_NOTIFICATION_CANCEL, jobService)
				.setOverrideDeadline(0)
				.setTriggerContentMaxDelay(0)
				.setTriggerContentUpdateDelay(0)
				.build();
		jobScheduler.schedule(jobInfo);
	}

	private void LocalNotificationCancelBelowO()
	{
		LocalNotificationService.ConsumeNotifications(this);
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		m_bGooglePlayResumed = true;

		if (null != m_GooglePlayAuth)
		{
			m_GooglePlayAuth.OnResume();
		}

		// Inform Facebook about the app activation
		m_FacebookImpl.onResume();

		// Re-apply immersive mode. Otherwise, the app can get stuck in a state where the navigation bar
		// is visible on top of the app.
		ApplyImmersiveMode();

		// Register our broadcast receiver with higher priority than the
		// default LocalNotificationReceiver instance (which is registered via
		// AndroidManifest.xml) so that we can handle broadcast intents before
		// it
		LocalBroadcastManager broadcaster = LocalBroadcastManager.getInstance(this);
		IntentFilter filter = new IntentFilter(LocalNotificationService.ACTION_BROADCAST_LOCAL);
		filter.setPriority(2);
		registerReceiver(m_OnNotification, filter);

		// Consume all pending intents. Android 8.0 uses Jobs instead of starting Intents directly
		// due to more restrictions on background services.
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
		{
			LocalNotificationCancelO();
		}
		else
		{
			LocalNotificationCancelBelowO();
		}

		// Try to start AV.
		m_iFocusLevel &= ~LEVEL_PAUSED;
		applyFocusLevel();
	}

	@Override
	protected void onStart()
	{
		// Necessary - some devices (e.g. Sony Xperia AX SO-01E), despite the documented control flow
		// at https://developer.android.com/guide/components/activities/state-changes,
		// do not necessarily call onRestart(). Native handling will filter the redundncy
		// when it happens (which is most of the time, since we expect onRestart() followed
		// by onStart().
		NativeOnSessionStart();

		super.onStart();
		m_bGooglePlayStarted = true;

		if (null != m_GooglePlayAuth)
		{
			m_GooglePlayAuth.OnStart();
		}
	}

	@Override
	protected void onStop()
	{
		super.onStop();
		m_bGooglePlayStarted = false;

		if (null != m_GooglePlayAuth)
		{
			m_GooglePlayAuth.OnStop();
		}

		NativeOnSessionEnd();
	}

	public boolean GooglePlayResumed()
	{
		return m_bGooglePlayResumed;
	}

	public boolean GooglePlayGamesIsAvailable()
	{
		return AndroidGooglePlayAuth.IsAvailable(this);
	}

	public void GooglePlayGamesInitialize(final boolean bDebug, final String sOauthClientId)
	{
		final AndroidNativeActivity self = this;
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized(this)
				{
					if (null != m_GooglePlayAuth)
					{
						return;
					}

					m_GooglePlayAuth = new AndroidGooglePlayAuth(self, sOauthClientId, bDebug);

					// Handle explicit calls of handlers we may have missed.
					if (m_bGooglePlayStarted)
					{
						m_GooglePlayAuth.OnStart();
					}
					if (m_bGooglePlayResumed)
					{
						m_GooglePlayAuth.OnResume();
					}
				}
			}
		});
	}

	public void GooglePlayGamesRequestIdToken(final long pUserData)
	{
		final AndroidNativeActivity self = this;
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized(this)
				{
					m_GooglePlayAuth.RequestIdToken(pUserData);
				}
			}
		});
	}

	public void GooglePlayGamesShutdown()
	{
		final AndroidNativeActivity self = this;
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized(this)
				{
					if (null == m_GooglePlayAuth)
					{
						return;
					}

					m_GooglePlayAuth = null;
				}
			}
		});
	}

	public void GooglePlayGamesSignIn()
	{
		final AndroidNativeActivity self = this;
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized(this)
				{
					m_GooglePlayAuth.SignIn();
				}
			}
		});
	}

	public void GooglePlayGamesSignOut()
	{
		final AndroidNativeActivity self = this;
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized(this)
				{
					m_GooglePlayAuth.SignOut();
				}
			}
		});
	}

	public void GooglePlayGamesPlatformSignInOnly(final long pCallback)
	{
		final AndroidNativeActivity self = this;
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized(this)
				{
					m_GooglePlayAuth.PlatformSignInOnly(pCallback);
				}
			}
		});
	}

	/**
	 * Called from C++ to display the achievements UI
	 */
	public void DisplayAchievementUI()
	{
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized(this)
				{
					try
					{
						if (null == m_GooglePlayAuth)
						{
							if (AndroidLogConfig.ENABLED)
							{
								Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: DisplayAchievementUI GooglePlayAuth not created.");
							}
							return;
						}
						if (null == m_GooglePlayAuth.m_ApiClient)
						{
							if (AndroidLogConfig.ENABLED)
							{
								Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: DisplayAchievementUI GooglePlayAuth ApiClient not created.");
							}
							return;
						}
						if (!m_GooglePlayAuth.m_ApiClient.hasConnectedApi(Games.API))
						{
							if (AndroidLogConfig.ENABLED)
							{
								Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: DisplayAchievementUI GooglePlayAuth ApiClient not connected.");
							}
							return;
						}

						final int RC_ACHIEVEMENT_UI = 9003;	// Unused
						startActivityForResult(Games.Achievements.getAchievementsIntent(m_GooglePlayAuth.m_ApiClient), RC_ACHIEVEMENT_UI);
					}
					catch (Exception e)
					{
						if (AndroidLogConfig.ENABLED)
						{
							Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: DisplayAchievementUI Games.Achievements.unlock exception:");
						}
						e.printStackTrace();
					}
				}
			}
		});
	}

	// Note that unlocking achievements is allowed to fail.
	// We will re-unlock every completed achievement on
	// every startup.
	// TODO:Also unlock achievements upon successful authorization
	public void UnlockAchievement(final String sAchievementId)
	{
		final AndroidNativeActivity self = this;
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized(this)
				{
					try
					{
						if (null == m_GooglePlayAuth)
						{
							if (AndroidLogConfig.ENABLED)
							{
								Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: UnlockAchievement GooglePlayAuth not created.");
							}
							return;
						}
						if (null == m_GooglePlayAuth.m_ApiClient)
						{
							if (AndroidLogConfig.ENABLED)
							{
								Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: UnlockAchievement GooglePlayAuth ApiClient not created.");
							}
							return;
						}
						if (!m_GooglePlayAuth.m_ApiClient.hasConnectedApi(Games.API))
						{
							if (AndroidLogConfig.ENABLED)
							{
								Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: UnlockAchievement GooglePlayAuth ApiClient not connected.");
							}
							return;
						}
						Games.Achievements.unlock(m_GooglePlayAuth.m_ApiClient, sAchievementId);
					}
					catch (Exception e)
					{
						if (AndroidLogConfig.ENABLED)
						{
							Log.d(AndroidLogConfig.TAG, "[AndroidNativeActivity]: UnlockAchievement Games.Achievements.unlock exception:");
						}
						e.printStackTrace();
					}
				}
			}
		});
	}

	///////////////////////////////////////////////////////////////////////////
	// CommerceManager hooks.
	///////////////////////////////////////////////////////////////////////////

	// Called by AndroidCommerceManager from native constructor.
	public synchronized void AndroidCommerceManagerInitialize(final boolean bDebugEnabled)
	{
		if (null == m_CommerceManager)
		{
			try
			{
				final AndroidNativeActivity activity = this;
				m_CommerceManager = new AndroidCommerceManager(activity, new AndroidCommerceApi.Factory()
				{
					@Override
					public AndroidCommerceApi Create(AndroidCommerceApi.Listener listener)
					{
						if (IsSamsungBuild())
						{
							return AndroidSamsungImpl.CreateCommerceManager(activity, listener, bDebugEnabled);
						}
						else if (IsAmazonBuild())
						{
							return AndroidAmazonImpl.CreateCommerceManager(activity, listener, bDebugEnabled);
						}
						else
						{
							return new AndroidCommerceGooglePlayApi(activity, listener, bDebugEnabled);
						}
					}
				}, bDebugEnabled);
			}
			catch (Exception e)
			{
				e.printStackTrace();
				m_CommerceManager = null;
			}
		}
	}

	// Called by AndroidCommerceManager from native destructor.
	public synchronized void AndroidCommerceManagerShutdown()
	{
		if (null != m_CommerceManager)
		{
			m_CommerceManager.Dispose();
			m_CommerceManager = null;
		}
	}

	// This method is called by AndroidCommerceManager to consume a consumable purchase.
	public synchronized void AndroidCommerceManagerConsumeItem(final String sProductID, final String sTransactionID)
	{
		// Ignore if no commerce manager.
		if (null == m_CommerceManager) { return; }

		m_CommerceManager.ConsumeItem(sProductID, sTransactionID);
	}

	// This method is called by AndroidCommerceManager to acknowledge a subscription purchase.
	public synchronized void AndroidCommerceManagerAcknowledgeItem(final String sProductID, final String sTransactionID)
	{
		// Ignore if no commerce manager.
		if (null == m_CommerceManager) { return; }

		m_CommerceManager.AcknowledgeItem(sProductID, sTransactionID);
	}

	// This method is called by AndroidCommerceManager to initiate an
	// item purchase.
	public synchronized void AndroidCommerceManagerPurchaseItem(final String sProductID)
	{
		// Report immediately if no commerce manager.
		if (null == m_CommerceManager)
		{
			AndroidCommerceManager.NativeTransactionCompleted(sProductID, "", "", "", AndroidCommerceResult.kPlatformNotInitialized.ordinal());
			return;
		}

		m_CommerceManager.PurchaseItem(sProductID);
	}

	// This method is called by AndroidCommerceManager to refresh the list
	// of purchaseable items (and to initialize the Java side of the commerce system).
	public synchronized void AndroidCommerceManagerRefreshProductInfo(final String[] skus)
	{
		// Report immediately if no commerce manager.
		if (null == m_CommerceManager)
		{
			AndroidCommerceManager.NativeSetProductsInfo(false, new AndroidCommerceApi.ProductInfo[0]);
			return;
		}

		m_CommerceManager.RefreshProductInfo(skus);
	}

	///////////////////////////////////////////////////////////////////////////
	// HelpShift hooks.
	///////////////////////////////////////////////////////////////////////////
	private final AndroidHelpshiftImpl m_HelpshiftImpl;

	public void HelpShiftInitialize(
		final String sUserName,
		final String sUserID,
		final String sHelpShiftKey,
		final String sHelpShiftDomain,
		final String sHelpShiftID)
	{
		m_HelpshiftImpl.HelpShiftInitialize(
			sUserName,
			sUserID,
			sHelpShiftKey,
			sHelpShiftDomain,
			sHelpShiftID);
	}

	public void HelpShiftShutdown()
	{
		m_HelpshiftImpl.HelpShiftShutdown();
	}

	public boolean HelpShiftOpenUrl(
		final String[] asCustomIssueFields,
		final String[] asCustomMetadata,
		final String sURL)
	{
		return m_HelpshiftImpl.HelpShiftOpenUrl(
			asCustomIssueFields,
			asCustomMetadata,
			sURL);
	}

	public boolean HelpShiftShowHelp(
		final String[] asCustomIssueFields,
		final String[] asCustomMetadata)
	{
		return m_HelpshiftImpl.HelpShiftShowHelp(
			asCustomIssueFields,
			asCustomMetadata);
	}

	///////////////////////////////////////////////////////////////////////////
	// Facebook hooks.
	///////////////////////////////////////////////////////////////////////////
	private final AndroidFacebookImpl m_FacebookImpl;

	public static native void NativeOnFacebookLoginStatusChanged();
	public static native void NativeOnReturnFriendsList(String sMessage);
	public static native void NativeOnSentRequest(String sRequestID, String sRecipients, String sData);
	public static native void NativeOnSentShareLink(boolean bShared);
	public static native void NativeUpdateFacebookUserInfo(String sID);
	public static native void NativeOnGetBatchUserInfo(String sID, String sName);
	public static native void NativeOnGetBatchUserInfoFailed(String sID);

	public void FacebookInitialize(final boolean bEnableDebugLogging)
	{
		m_FacebookImpl.FacebookInitialize(bEnableDebugLogging);
	}

	public void FacebookLogin(String[] aRequestedPermissions)
	{
		m_FacebookImpl.FacebookLogin(aRequestedPermissions);
	}

	public void FacebookCloseSession()
	{
		m_FacebookImpl.FacebookCloseSession();
	}

	public boolean FacebookIsLoggedIn()
	{
		return m_FacebookImpl.FacebookIsLoggedIn();
	}

	public String FacebookGetAuthToken()
	{
		return m_FacebookImpl.FacebookGetAuthToken();
	}

	public String FacebookGetUserID()
	{
		return m_FacebookImpl.FacebookGetUserID();
	}

	public void FacebookGetFriendsList(final String sFields, final int iLimit)
	{
		m_FacebookImpl.FacebookGetFriendsList(sFields, iLimit);
	}

	public void FacebookSendRequest(final String sMessage, final String sTitle, final String sRecipients, final String sSuggestedFriends, final String sData)
	{
		m_FacebookImpl.FacebookSendRequest(sMessage, sTitle, sRecipients, sSuggestedFriends, sData);
	}

	public void FacebookDeleteRequest(final String sRequestID)
	{
		m_FacebookImpl.FacebookDeleteRequest(sRequestID);
	}

	public void FacebookSendPurchaseEvent(final double fLocalPrice, final String sCurrencyCode)
	{
		m_FacebookImpl.FacebookSendPurchaseEvent(fLocalPrice, sCurrencyCode);
	}

	public void FacebookTrackEvent(final String sEventID)
	{
		m_FacebookImpl.FacebookTrackEvent(sEventID);
	}

	public void FacebookShareLink(final String sContentURL)
	{
		m_FacebookImpl.FacebookShareLink(sContentURL);
	}

	public void FacebookRequestBatchUserInfo(final String[] aUserIDs)
	{
		m_FacebookImpl.FacebookRequestBatchUserInfo(aUserIDs);
	}


	///////////////////////////////////////////////////////////////////////////
	// Google Analytics hooks.
	///////////////////////////////////////////////////////////////////////////
	protected final AndroidGoogleAnalyticsImpl m_GoogleAnalyticsImpl;

	///////////////////////////////////////////////////////////////////////////
	// AppAndroid Refactor
	///////////////////////////////////////////////////////////////////////////
	public static final int LEVEL_UNFOCUSED = 0x01;
	public static final int LEVEL_PAUSED = 0x02;

	/**
	 * App-defined job Ids
	 * Leave IDs [0-1000] reserved for Local Notification Jobs
	 */
	public static final int JOB_ID_PUSH_NOTIFICATION = 1001;
	public static final int JOB_ID_PUSH_NOTIFICATION_CANCEL = 1002;

	// GDPR Settings
	private static final String GDPR_PREFERENCES_KEY = "GDPR_Compliance_Version";
	// GDPR Version for Android. Note that each platform maintains its own version.
	private int m_iGDPRVersion = 1;

	private static native boolean NativeShouldEnableDebugLog();
	private static native void NativeSetCacheDirectory(String sCacheDir);
	private static native void NativeSetInternalStorageDirectory(String sInternalStorageDirectoryString);
	private static native void NativeSetSourceDir(String sSourceDir);
	private static native void NativeSetSubPlatform(int iBuildType);
	private static native void NativeSetTouchSlop(int iSlop);

	private static native void NativeHandleOpenURL(String sURL);
	private static native void NativeSetCommandline(String sCommandline);

	private static native void NativeOnSessionStart();
	private static native void NativeOnSessionEnd();

	private static native void NativeOnBackPressed();
	public static native void NativeHandleLocalNotification(boolean bWasInForeground, String sUserData);

	private AndroidFmodImpl m_AndroidFmodImpl = new AndroidFmodImpl();

	private int m_iAppVersionCode = 0;

	/**
	 * Store persistent data for this installation.
	 */
	private SharedPreferences m_AppPreferences;

	/** Flag indicating if debug logging should be enabled */
	private static final boolean s_bEnableDebugLog;

	/** Base activity content view group */
	private ViewGroup m_ViewGroup;

	/** We relay a line of input from this dialog to the native side. */
	protected Dialog m_TextInput;

	/** the current input field that we want to focus on */
	protected View m_Focus;

	/** C++ user data for the current sign-in */
	private long m_lSignInUserData;

	/** Activity request ID used for displaying the achievements UI */
	private static final int kRequestIDAchievements = 0x100000;

	/**
	 * 0 means activity has focus, positive values mean something else has
	 * focus. Usually, the focus level increments on pause/losing focus and
	 * decrements on resume/gaining focus.
	 */
	private int m_iFocusLevel = 0;

	protected FirebaseApp m_FirebaseApp;

	/**
	 * Treat the initial state as if we're in the background so that the
	 * applyFocusLevel() can recognize the change in state during the first
	 * onResume() call.
	 */
	private static boolean s_bInBackground = true;
	public static boolean InBackground() { return s_bInBackground; }

	private static native void NativeOnSignInFinished(boolean bSignedIn, long pUserData);

	public static native void NativeOnRegisteredForRemoteNotifications(String sRegistrationId);

	public static native void NativeApplyText(String sMessage);
	public static native void NativeStopEditing();

	/**
	 * BroadcastReceiver for handling local notification intents.  When an
	 * alarm for a local notification is received and the app is in the
	 * foreground, this received will receive the intent and inform the game
	 * accordingly.  When the app is not in the foreground, the
	 * LocalNotificationReceiver will instead receive the intent and post the
	 * actual notification.
	 */
	private BroadcastReceiver m_OnNotification = new BroadcastReceiver()
	{
		public void onReceive(Context context, final Intent intent)
		{
			// Public Notifications
			if (intent.getAction().equals(LocalNotificationService.ACTION_BROADCAST_LOCAL))
			{
				String sOpenUrl = intent.getStringExtra("OpenUrl");
				if (sOpenUrl != null && !sOpenUrl.isEmpty())
				{
					// By not calling abortBroadcast, we fall through to the next handler,
					// which posts a local notification to open the URL
					return;
				}

				// Inform the game that a notification was received while running
				String sUserData = intent.getStringExtra("UserData");
				NativeHandleLocalNotification(true, sUserData);

				// Don't let the LocalNotificationReceiver also receive this intent
				abortBroadcast();
			}
		}
	};

	@Override
	public void onBackPressed()
	{
		HideVirtualKeyboard();
		NativeOnBackPressed();
	}

	/**
	 * Called by game code when the game UI does not handle the back button request,
	 * and the game desired default back button handling.
	 */
	public void OnInvokeDefaultBackButtonHandling()
	{
		final AndroidNativeActivity activity = this;
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				// On the UI thread, invoke the base class onBackPressed() method to perform
				// standard back button handling (which will trigger an exit).
				activity.UIThread_InvokeDefaultBackButtonHandling();
			}
		});
	}

	public void UIThread_InvokeDefaultBackButtonHandling()
	{
		super.onBackPressed();
	}

	public static final int MIN_IMMERSIVE_MODE_API = 19;

	@TargetApi(MIN_IMMERSIVE_MODE_API)
	public static final int GetImmersiveModeFlags()
	{
		return
			View.SYSTEM_UI_FLAG_FULLSCREEN |
			View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
			View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
			View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
			View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
			View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
	}

	// Android 10 (API 29) introduced a bug in DecorView and the underlying native
	// canvas implementation: https://android.googlesource.com/platform/frameworks/base.git/+/a6aabacbb5f7704e177e9c72dd038939a24ffedc
	//
	// The apparent intention of the above change was to extend functionality previously added to Android that moved the responsibility
	// of drawing the background for the status bar into the root DecorView of the app in all cases (whether or not the app requested
	// the ability to draw behind the status bar). The linked commit applied the same design to the navigation bar (starting in Android 10,
	// the navigation bar is now always drawn by the root DecorView of the app, whether or not the app has requested the ability to draw
	// behind the navigation bar, as long as the app requests at least a target SDK API level of 24 - otherwise, the DecorView and native
	// Canvas run in compatibility mode and the system/canvas drawn the navbar background as in previous versions of the Android OS).
	//
	// By default, if the app has not explicitly requested the ability to draw behind the system bars (e.g. with FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS),
	// the DecorView emulates the old behavior, by drawing the system bars itself and reporting a clipped window area to its children. For
	// implementations that use (e.g.) GLSurfaceView, this works as expected. Effectively, the DecorView lies about the setup to its children
	// to emulate old behavior. Unfortunately, this lie was never properly implemented for the mechanism used by NativeActivity for rendering
	// o a surface, since it directly ties into the underlying SurfaceHandle of the DecorView (https://android.googlesource.com/platform/frameworks/base.git/+/master/core/java/android/app/NativeActivity.java#133)
	//
	// As a result, a NativeActivity based Activity will draw to the entire window in all cases for apps that target SDK api level >= 24 when
	// running on Android OS 10. In other configurations, a NativeActivity will draw based on typical viewport configuration flags
	// (e.g. if FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS is clear, the navigation bar will be drawn with a black background and the NativeActivity will
	// draw to a subset of the window rectangle).
	//
	// After several passes at attempting to bypass this bug through configurations of the DecorView and Window, the safest approach to workaround this
	// issue appears to be:
	// 1. explicitly request to draw the system bar backgrounds with FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS in Android 10 and above - if we don't do this,
	//    the DecorView will lie about the navbar inset sizes as part of its "pretend I'm not drawing but I actually am" functionality and we won't
	//    know the correct area to reserve for the navbar. Also, if Google fixes this bug in later versions of the OS, we risk a new bug when that
	//    fix goes live unless we advertise that we want to draw the navbar background.
	// 2. pass the inset sizes down to native so it can letterbox the area used for game rendering.
	final static int MIN_WINDOW_INSET_API = 20;

	@TargetApi(MIN_WINDOW_INSET_API)
	void ApplyAndroid10NavBarWorkaround()
	{
		// Nothing to do if in immersive mode, since we hide the system bars entirely in this mode.
		if (m_bImmersiveMode)
		{
			return;
		}

		// Nothing to do if we're targeting less than 24 or if the os is less than Android 10 (API 29)
		// See comment above - the code requiring this workaround was added in Android 10,
		// but is enabled/keyed off of a target sdk version >= 24 (if the target SDK is <= 23, the DecorView
		// and Canvas operate in a compatibility mode and navigation bar rendering works as it did prior
		// to Android 10).
		final ApplicationInfo ai = getApplicationInfo();
		if (null == ai || ai.targetSdkVersion < 24 || Build.VERSION.SDK_INT < 29)
		{
			return;
		}

		// Explicitly request to draw system bar background to avoid surprises and
		// get valid WindowInset values.
		final Window window = getWindow();
		window.addFlags(android.view.WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);

		// Hook into the decor's window inset callback to report window inset to native. Check here
		// is necessary to satisfy the lint check, this entire workaround will only be applied
		// in configurations >= 29.
		if (Build.VERSION.SDK_INT >= MIN_WINDOW_INSET_API)
		{
			final View view = window.getDecorView();
			view.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener()
			{
				public final WindowInsets onApplyWindowInsets(View view, WindowInsets insets)
				{
					if (null != insets) // Sanity.
					{
						// Report system window insets (top and bottom) to native.
						NativeOnWindowInsets(insets.getSystemWindowInsetTop(), insets.getSystemWindowInsetBottom());
					}
					return view.onApplyWindowInsets(insets);
				}
			});
		}
	}

	void ApplyImmersiveMode()
	{
		// Nop if disabled.
		if (!m_bImmersiveMode)
		{
			return;
		}

		// Request fullscreen immersive mode, if this is a build for api-19 or later.
		if (Build.VERSION.SDK_INT >= MIN_IMMERSIVE_MODE_API)
		{
			final View view = this.findViewById(android.R.id.content);

			final int iFlags = GetImmersiveModeFlags();
			view.setSystemUiVisibility(view.getSystemUiVisibility() | iFlags);

			// Not sure of a better way - on some devices and in some situations, we can't
			// just immediately apply or the value will get overwriten (e.g. on keyboard changes).
			// So we apply again in 250 milliseconds.
			final Handler handler = view.getHandler();
			if (null != handler)
			{
				handler.postDelayed(new Runnable()
				{
					public void run()
					{
						view.setSystemUiVisibility(view.getSystemUiVisibility() | iFlags);
					}
				}, 250);
			}
		}
	}

	private void AppOnCreate(Bundle savedInstanceState)
	{
		// Set up preferences tied to this installation.
		m_AppPreferences = getSharedPreferences(m_sSharedPreferencesName, Context.MODE_PRIVATE);

		// Initial application of immersive mode. Needs to be re-applied in other contexts,
		// because almnost nothing on Android "just works".
		ApplyImmersiveMode();

		// Setup on onchange listener that re-applies immersive mode.
		if (m_bImmersiveMode)
		{
			final View view = this.findViewById(android.R.id.content);
			final AndroidNativeActivity activity = this;
			view.setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener()
			{
				@Override
				public void onSystemUiVisibilityChange(int iVisibility)
				{
					if ((iVisibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0)
					{
						activity.ApplyImmersiveMode();
					}
				}
			});
		}

		// Report the sub platform to native code.
		NativeSetSubPlatform(GetBuildType().ordinal());

		// Cache the app version code
		m_iAppVersionCode = GetAppVersionCode();

		// Pass any commandline arguments to the native layer.
		if (null != getIntent() && null != getIntent().getStringExtra("commandline"))
		{
			NativeSetCommandline(Uri.decode(getIntent().getStringExtra("commandline")));
		}

		NativeSetCacheDirectory(getCacheDir().getAbsolutePath());
		NativeSetInternalStorageDirectory(getFilesDir().getAbsolutePath());
		NativeSetSourceDir(GetSourceDir());
		NativeSetTouchSlop(ViewConfiguration.get(this).getScaledTouchSlop());

		// Set up a view group so we can add subviews for keyboard input if
		// necessary
		m_ViewGroup = new RelativeLayout(this);
		setContentView(m_ViewGroup);
		m_ViewGroup.requestFocus();

		// Resolve any additional setup when started by an Intent.
		ResolveIntent(getIntent());

		// Init FMOD.
		m_AndroidFmodImpl.OnCreate(this);

		// Tell native code it can now continue starting up.
		NativeSetCanPerformNativeStartup(true);
	}

	@Override
	public void onDestroy()
	{
		// Kill FMOD.
		m_AndroidFmodImpl.OnDestroy(this);

		super.onDestroy();
	}

	// Called when the game is starting back up from the background, only when it was not visible
	@Override
	protected void onRestart()
	{
		super.onRestart();

		NativeOnSessionStart();
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus)
	{
		super.onWindowFocusChanged(hasFocus);

		// Audio depends on activity focus. When a device goes to sleep and the
		// user wakes it up, it actually resumes in a locked screen; only when
		// the window regains focus should it actually resume interactivity.
		if (hasFocus)
		{
			// Try to start AV.
			m_iFocusLevel &= ~LEVEL_UNFOCUSED;
			applyFocusLevel();
		}
		else
		{
			// Try to disable AV.
			m_iFocusLevel |= LEVEL_UNFOCUSED;
			applyFocusLevel();
		}

		// Re-apply immersive mode. Otherwise, the app can get stuck in a state where the navigation bar
		// is visible on top of the app.
		ApplyImmersiveMode();
	}

	private void applyFocusLevel()
	{
		if (s_bInBackground)
		{
			if (m_iFocusLevel == 0)
			{
				// Resume if we have focus AND resumed.
				m_AndroidFmodImpl.Start(this);
				s_bInBackground = false;
			}
		}
		else
		{
			if (m_iFocusLevel != 0)
			{
				// Always pause.
				m_AndroidFmodImpl.Stop(this);
				s_bInBackground = true;
			}
		}
	}

	@Override
	public void onNewIntent(Intent intent)
	{
		super.onNewIntent(intent);
		ResolveIntent(intent);
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu)
	{
		// We don't want the options menu to display at all.
		return false;
	}

	/**
	 * Apply any additional processing for an Intent. Usually, you need to
	 * apply an Intent on startup if the activity started by an external
	 * process.
	 *
	 * @param intent any Intent object
	 */
	public void ResolveIntent(Intent intent)
	{
		if (null != intent)
		{
			// Pass URL handling to native code.
			Uri data = intent.getData();
			if (null != data)
			{
				NativeHandleOpenURL(data.toString());

				// TODO @appsflyerupgrade: Right now the Android AppsFlyer
				// SDK is inconsistent about when it acknowledges deep link
				// opening intents. Because of this, manually instigate
				// a deep link parse. Reading a deep link twice is fine
				// since we track duplicates internally. Once the
				// AppsFlyer SDK is upgraded, check if this behavior can
				// be removed.
				HashMap<String, String> tLinkMap = new HashMap<>();
				tLinkMap.put("link", data.toString());
				m_AppsFlyerImpl.handleDeepLinking(tLinkMap);
			}

			// If the intent was issued as a result of the user tapping a
			// local notification, inform the game code
			String sUserData = intent.getStringExtra("UserData");
			if (intent.getBooleanExtra("IsNotification", false) || (sUserData != null))
			{
				NativeHandleLocalNotification(false, sUserData);
			}
		}
	}

	@TargetApi(Build.VERSION_CODES.O)
	private void ScheduleLocalNotificationO(
		int iNotificationID,
		long iFireTimeAbsoluteSeconds,
		boolean bIsWallClockTime,
		long iFireTimeRelativeSeconds,
		String sLocalizedMessage,
		boolean bHasActionButton,
		String sLocalizedActionButtonText,
		boolean bPlaySound,
		String sUserData)
	{
		PersistableBundle persistableExtras = new PersistableBundle();
		persistableExtras.putString("LocalizedMessage", sLocalizedMessage);
		persistableExtras.putBoolean("bHasActionButton", bHasActionButton);
		persistableExtras.putString("LocalizedActionButtonText", sLocalizedActionButtonText);
		persistableExtras.putBoolean("bPlaySound", bPlaySound);
		persistableExtras.putString("UserData", sUserData);
		persistableExtras.putInt("ID", iNotificationID);

		android.app.job.JobScheduler jobScheduler = (android.app.job.JobScheduler)getSystemService(Context.JOB_SCHEDULER_SERVICE);
		ComponentName jobService = new ComponentName(getPackageName(), LocalNotificationJobService.class.getName());
		android.app.job.JobInfo jobInfo = new android.app.job.JobInfo.Builder(iNotificationID, jobService)
				.setOverrideDeadline(iFireTimeRelativeSeconds * 1000)
				.setMinimumLatency(iFireTimeRelativeSeconds * 1000)
				.setExtras(persistableExtras)
				.build();
		int result = jobScheduler.schedule(jobInfo);
	}

	private void ScheduleLocalNotificationBelowO(
		int iNotificationID,
		long iFireTimeAbsoluteSeconds,
		boolean bIsWallClockTime,
		long iFireTimeRelativeSeconds,
		String sLocalizedMessage,
		boolean bHasActionButton,
		String sLocalizedActionButtonText,
		boolean bPlaySound,
		String sUserData)
	{
		AlarmManager alarmManager = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
		if (null != alarmManager)
		{
			// Setup the basic intent data.
			Intent intent = new Intent(this, LocalNotificationService.class);
			intent.putExtra("LocalizedMessage", sLocalizedMessage);
			intent.putExtra("bHasActionButton", bHasActionButton);
			intent.putExtra("LocalizedActionButtonText", sLocalizedActionButtonText);
			intent.putExtra("bPlaySound", bPlaySound);
			intent.putExtra("UserData", sUserData);
			intent.putExtra("ID", iNotificationID);

			// Create the pending intent.
			PendingIntent pendingIntent = PendingIntent.getService(this, iNotificationID, intent, PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_UPDATE_CURRENT);

			// Trigger time is the fire time in milliseconds.
			long iTriggerTime = (1000 * iFireTimeAbsoluteSeconds);

			// Set the alarm, replacing any existing alarm for the same pending
			// intent
			alarmManager.set(
				AlarmManager.RTC_WAKEUP,
				iTriggerTime,
				pendingIntent);
		}
	}

	public void ScheduleLocalNotification(
		int iNotificationID,
		long iFireTimeAbsoluteSeconds,
		boolean bIsWallClockTime,
		long iFireTimeRelativeSeconds,
		String sLocalizedMessage,
		boolean bHasActionButton,
		String sLocalizedActionButtonText,
		boolean bPlaySound,
		String sUserData)
	{
		// Android 8.0 + cannot use background services so local notifications are scheduled through
		// the JobScheduler
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
		{
			ScheduleLocalNotificationO(
				iNotificationID,
				iFireTimeAbsoluteSeconds,
				bIsWallClockTime,
				iFireTimeRelativeSeconds,
				sLocalizedMessage,
				bHasActionButton,
				sLocalizedActionButtonText,
				bPlaySound,
				sUserData);
		}
		// Old flow prior to 8.0. We still support old Android versions which do not have the
		// JobScheduler.
		else
		{
			ScheduleLocalNotificationBelowO(
				iNotificationID,
				iFireTimeAbsoluteSeconds,
				bIsWallClockTime,
				iFireTimeRelativeSeconds,
				sLocalizedMessage,
				bHasActionButton,
				sLocalizedActionButtonText,
				bPlaySound,
				sUserData);
		}
	}

	@TargetApi(Build.VERSION_CODES.O)
	private void CancelLocalNotificationO(int iNotificationID)
	{
		android.app.job.JobScheduler jobScheduler = (android.app.job.JobScheduler)getSystemService(Context.JOB_SCHEDULER_SERVICE);
		if(jobScheduler.getPendingJob(iNotificationID) != null)
		{
			jobScheduler.cancel(iNotificationID);
		}
	}

	private void CancelLocalNotificationBelowO(int iNotificationID)
	{
		AlarmManager alarmManager = (AlarmManager)getSystemService(Context.ALARM_SERVICE);
		if (alarmManager == null)
		{
			return;
		}

		// Setup the basic intent data
		Intent intent = new Intent(this, LocalNotificationService.class);

		// Create the pending intent
		PendingIntent pendingIntent = PendingIntent.getService(this, iNotificationID, intent, PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_UPDATE_CURRENT);

		// Cancel the alarm for the pending intent
		alarmManager.cancel(pendingIntent);
	}

	/**
	 * Cancels the local notification with the given ID
	 */
	public void CancelLocalNotification(int iNotificationID)
	{
		// Android 8.0 + cannot use background services so local notifications are scheduled through
		// the JobScheduler
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
		{
			CancelLocalNotificationO(iNotificationID);
		}
		// Old flow prior to 8.0. We still support old Android versions which do not have the
		// JobScheduler.
		else
		{
			CancelLocalNotificationBelowO(iNotificationID);
		}
	}

	public void SetGDPRAccepted(boolean bAccepted)
	{
		SharedPreferences.Editor editor = m_AppPreferences.edit();
		editor.putInt(GDPR_PREFERENCES_KEY, bAccepted ? m_iGDPRVersion : 0);
		editor.apply();
	}

	public boolean GetGDPRAccepted()
	{
		int version = m_AppPreferences.getInt(GDPR_PREFERENCES_KEY, 0);
		return version >= m_iGDPRVersion;
	}

	// Return the index of the next character - will be the end
	// of the string if no character is available.
	static int NextCharacterPosition(
		int iInput,
		String sRestrict)
	{
		int iReturn = iInput;
		if (iReturn < sRestrict.length())
		{
			if ('\\' == sRestrict.charAt(iReturn))
			{
				++iReturn;
			}
		}

		return iReturn;
	}

	/**
	 * @return True if c is valid as defined by sRestrict.
	 *
	 * This method should return identical results to
	 * Engine::TextEntrySettings::IsAllowedCharacter() in
	 * Engine.cpp.
	 */
	static boolean IsAllowedCharacter(String sRestrict, char c)
	{
		// Empty m_sRestriction string, return true immediately.
		if (null == sRestrict || sRestrict.isEmpty())
		{
			return true;
		}

		// Exact match to is_allowed() in the iggy codebase, implementation
		// of the Action Script text_field.restricted field.
		boolean bAllowed = false;
		boolean bInAllowList = true;

		// If the first character is '^', the default allowed state is set to true
		// (the string is a "disallowed" list.
		if (sRestrict.charAt(0) == '^')
		{
			bAllowed = true;
		}

		// '\\' is a special character which escapes the next character.
		// '^' is a special character which changes the meaning of bInAllowList.
		for (int i = 0; i < sRestrict.length(); )
		{
			// Special character '^' toggles the allow mode.
			if ('^' == sRestrict.charAt(i))
			{
				bInAllowList = !bInAllowList;
				++i;
			}
			else
			{
				// Get the next character and update the position.
				i = NextCharacterPosition(i, sRestrict);
				char chFirst = (i < sRestrict.length() ? sRestrict.charAt(i) : 0);
				i = (i < sRestrict.length() ? (i + 1) : i);

				// Range of characters.
				if (i < sRestrict.length() && '-' == sRestrict.charAt(i))
				{
					// Skip the hyphen.
					++i;

					// Get the next character and update the position.
					i = NextCharacterPosition(i, sRestrict);
					char chLast = (i < sRestrict.length() ? sRestrict.charAt(i) : 0);
					i = (i < sRestrict.length() ? (i + 1) : i);

					// In range or equal.
					if ((0 != chLast ? (c >= chFirst && c <= chLast) : (c == chFirst)))
					{
						bAllowed = bInAllowList;
					}
				}
				// Individual character check.
				else
				{
					if (c == chFirst)
					{
						bAllowed = bInAllowList;
					}
				}
			}
		}

		return bAllowed;
	}

	/**
	 * Specialization of InputFilter that handles restricting characters to a limited set.
	 * The set is defined by an ActionScript 3 TextInput.restrict style filter.
	 */
	static class AllowedCharacterInputFilter implements InputFilter
	{
		String m_sRestrict;

		public AllowedCharacterInputFilter(String sRestrict)
		{
			m_sRestrict = sRestrict;
		}

		// Adapted from: http://stackoverflow.com/a/12656159
		@Override
		public CharSequence filter(CharSequence source, int iStart, int iEnd, Spanned destination, int iDestinationStart, int iDestinationEnd)
		{
			// Filter each span individually.
			if (source instanceof SpannableStringBuilder)
			{
				boolean bChanged = false;
				SpannableStringBuilder spannableBuilder = (SpannableStringBuilder)source;
				for (int i = (iEnd - 1); i >= iStart; --i)
				{
					// If the character is not allowed, delete it from the span.
					if (!IsAllowedCharacter(m_sRestrict, source.charAt(i)))
					{
						spannableBuilder.delete(i, i+1);
						bChanged = true;
					}
				}

				// Any changes, return the modified source.
				if (bChanged)
				{
					return source;
				}
				// Otherwise, return no modifications.
				else
				{
					return null;
				}
			}
			// Single span buffer.
			else
			{
				boolean bChanged = false;
				StringBuilder builder = new StringBuilder();

				// For each character.
				for (int i = iStart; i < iEnd; ++i)
				{
					char c = source.charAt(i);

					// If allowed, add it to our accumulation buffer.
					if (IsAllowedCharacter(m_sRestrict, c))
					{
						builder.append(c);
					}
					// Otherwise, a character has been removed.
					else
					{
						bChanged = true;
					}
				}

				// Any changes, return the new buffer.
				if (bChanged)
				{
					return builder.toString();
				}
				// Otherwise, return no modifications.
				else
				{
					return null;
				}
			}
		}
	}

	/**
	 * Called from C++ to show the virtual keyboard.
	 */
	public void ShowVirtualKeyboard(final String sText, final String sDescription, final int iMaxCharacters, final String sRestrict)
	{
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				// Dismiss any pre-existing dialog.
				if (null != m_TextInput)
				{
					m_TextInput.dismiss();
					m_TextInput = null;
				}

				// Create a new dialog if one isn't up yet.
				m_TextInput = new Dialog(AndroidNativeActivity.this);
				m_TextInput.requestWindowFeature(Window.FEATURE_NO_TITLE);
				m_TextInput.setContentView(R.layout.textinput);
				m_TextInput.setCancelable(true);
				m_TextInput.setCanceledOnTouchOutside(true);
				m_TextInput.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);

				m_TextInput.getWindow().setBackgroundDrawableResource(android.R.color.transparent); // hide dialog bg.
				m_TextInput.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND); // don't dim bg

				// remove padding/margin from dialog
				WindowManager.LayoutParams wmlp = m_TextInput.getWindow().getAttributes();
				wmlp.width = android.view.WindowManager.LayoutParams.MATCH_PARENT;
				wmlp.height = android.view.WindowManager.LayoutParams.MATCH_PARENT;

				 // dismiss dialog when dialog bg when tapped.
				RelativeLayout outerLayout = (RelativeLayout) m_TextInput.findViewById(R.id.OuterRelativeLayout);
				outerLayout.setOnClickListener(new View.OnClickListener(){
					public void onClick(View v) {
						if (null != m_TextInput)
						{
							m_TextInput.dismiss();
							m_TextInput = null;
						}
					}
				});


				final Dialog self = m_TextInput;

				// do nothing when inner layout tapped (i.e. absorb input)
				final RelativeLayout innerLayout = (RelativeLayout) m_TextInput.findViewById(R.id.InnerRelativeLayout);
				innerLayout.setOnClickListener(new View.OnClickListener(){ public void onClick(View v) {} });

				// start dialog invisible then show in .33 seconds, AKA the "Bart Special": doesn't show the dialog at first so it appears to show once the keyboard pops in.
				final Handler handler = new Handler();
				innerLayout.setVisibility(View.INVISIBLE);
				handler.postDelayed(new Runnable() {
					public void run() {
						if(innerLayout != null && m_TextInput != null)
						{
							innerLayout.setVisibility(View.VISIBLE);
						}
					}
				}, 250);
				// End "Bart Special"
				final EditTextBackEvent field = (EditTextBackEvent)self.findViewById(R.id.textEntry);

				// hide our view when the back button (on the keyboard) simply dismisses the virtual keyboard.
				field.setOnEditTextImeBackListener(new EditTextImeBackListener(){
					@Override
					public void onImeBack(EditTextBackEvent ctrl, String text)
					{
						if (null != m_TextInput)
						{
							m_TextInput.dismiss();
							m_TextInput = null;
						}
					}
				});

				final Button okButton = (Button)self.findViewById(R.id.okButton);
				okButton.setOnClickListener(new View.OnClickListener()
				{
					@Override
					public void onClick(View view)
					{
						// Pass the string to the native engine.
						NativeApplyText(field.getText().toString());
						NativeStopEditing();
						self.dismiss();
					}
				});

				field.setText(sText);
				field.setOnEditorActionListener(new OnEditorActionListener()
				{
					@Override
					public boolean onEditorAction(TextView _target, int _action, KeyEvent _event)
					{
						if (_action == EditorInfo.IME_ACTION_SEND)
						{
							// Pass the string to the native engine.
							NativeApplyText(_target.getText().toString());
							NativeStopEditing();
							self.dismiss();
							return true;
						}

						return false;
					}
				});
				field.addTextChangedListener(new TextWatcher()
				{
					int m_iStart = 0;
					int m_iEnd = 0;
					AllowedCharacterInputFilter m_AllowedCharacterFilter = new AllowedCharacterInputFilter(sRestrict);
					InputFilter.LengthFilter m_LengthFilter = new InputFilter.LengthFilter(iMaxCharacters);

					void ApplyFilter(Editable content, InputFilter filter)
					{
						CharSequence input = content.subSequence(m_iStart, m_iEnd);
						CharSequence output = filter.filter(
							input,
							0,
							input.length(),
							content,
							m_iStart,
							m_iEnd);

						// null means keep the original unchanged.
						if (null != output)
						{
							int iStart = m_iStart;
							int iEnd = m_iEnd;
							m_iEnd += output.length() - input.length();

							content.replace(iStart, iEnd, output.toString());
						}
					}

					@Override
					public void beforeTextChanged(CharSequence content, int iStart, int iOld, int iNow)
					{
					}
					@Override
					public void onTextChanged(CharSequence content, int iStart, int iOld, int iNow)
					{
						m_iStart = iStart;
						m_iEnd = iStart + iNow;
					}
					@Override
					public void afterTextChanged(Editable content)
					{
						// Apply the restriction filter if specified.
						if (null != sRestrict && !sRestrict.isEmpty())
						{
							ApplyFilter(content, m_AllowedCharacterFilter);
						}

						// Apply max length restriction if specified.
						if (iMaxCharacters > 0)
						{
							ApplyFilter(content, m_LengthFilter);
						}
					}
				});

				// When the window closes, hide any visible soft keyboard.
				self.setOnDismissListener(new OnDismissListener()
				{
					@Override
					public void onDismiss(DialogInterface _dialog)
					{
						NativeStopEditing();
						HideIME();
						if (self == m_TextInput)
						{
							m_TextInput = null;
						}
					}
				});

				m_Focus = field;

				if (null != m_TextInput){
					m_TextInput.show();
				}

				if (m_Focus.requestFocus())
				{
					InputMethodManager imm = (InputMethodManager)getSystemService(INPUT_METHOD_SERVICE);

					if (null != imm)
					{
						// Just in case, try to force the soft input keyboard if the
						// IME is available.
						imm.showSoftInput(m_Focus, InputMethodManager.SHOW_IMPLICIT);
					}
				}
			}
		});
	}

	/**
	 * Called from C++ to hide the virtual keyboard
	 */
	public void HideVirtualKeyboard()
	{
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				HideIME();
				if (null != m_TextInput)
				{
					m_TextInput.dismiss();
					m_TextInput = null;
				}
			}
		});
	}

	protected void HideIME()
	{
		if (null != m_Focus)
		{
			InputMethodManager imm = (InputMethodManager)getSystemService(INPUT_METHOD_SERVICE);
			if (null != imm)
			{
				imm.hideSoftInputFromWindow(m_Focus.getWindowToken(), 0);
			}

			m_Focus = null;
		}
	}

	/**
	 * Called from C++ to converts a (keycode, metakey state) pair into a
	 * Unicode character, according to Android's key map
	 */
	public int GetUnicodeChar(int iKeyCode, int iMetaState)
	{
		KeyEvent event = new KeyEvent(KeyEvent.ACTION_UP, iKeyCode);
		return event.getUnicodeChar(iMetaState);
	}

	static
	{
		// Ensure that our native libraries have been loaded by now, in case
		// this activity gets launched directly (e.g. by Facebook)
		LoadNativeLibrariesActivity.forceStaticInit();

		s_bEnableDebugLog = NativeShouldEnableDebugLog();
	}

	/**
	 * Kick off remote notifications registration.
	 */
	public synchronized void RegisterForRemoteNotifications()
	{
		final AndroidNativeActivity self = this;
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized (self)
				{
					if (null == m_FirebaseApp)
					{
						// Initialize default FirebaseApp; uses string resources generated from
						// google-services.json by refresh_google_services.py
						m_FirebaseApp = FirebaseApp.initializeApp(self);
						if (null == m_FirebaseApp)
						{
							if (AndroidLogConfig.ENABLED)
							{
								Log.e(AndroidLogConfig.TAG, "FirebaseApp not initialized");
							}
							return;
						}
					}
					FirebaseInstanceId.getInstance().getInstanceId().addOnSuccessListener(self, new OnSuccessListener<InstanceIdResult>()
					{
						@Override
						public void onSuccess(InstanceIdResult res)
						{
							final String sToken = res.getToken();
							NativeOnRegisteredForRemoteNotifications(sToken);
						}
					});
				}
			}
		});
	}
}
