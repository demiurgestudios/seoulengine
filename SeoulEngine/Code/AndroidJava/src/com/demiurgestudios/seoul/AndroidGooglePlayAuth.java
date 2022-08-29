/**
 * \file AndroidGooglePlayAuth.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.app.Activity;

import android.content.Context;
import android.content.Intent;
import android.content.IntentSender;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;

import com.google.android.gms.auth.api.Auth;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInOptions;
import com.google.android.gms.auth.api.signin.GoogleSignInResult;
import com.google.android.gms.auth.api.signin.GoogleSignInStatusCodes;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.common.api.CommonStatusCodes;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.OptionalPendingResult;
import com.google.android.gms.common.api.PendingResult;
import com.google.android.gms.common.api.ResultCallback;
import com.google.android.gms.common.api.Status;
import com.google.android.gms.games.Games;

/**
 * Utility that wraps Google SignIn and provides basic auth
 * capabilities with Google Play.
 */
public class AndroidGooglePlayAuth implements
	GoogleApiClient.OnConnectionFailedListener,
	GoogleApiClient.ConnectionCallbacks
{
	// Constant key name used to track whether we should automatically sign-in
	// after connecting.
	static final String kKeyAutoSignIn = "AndroidGooglePlayAuthAutoSignIn";
	// Intent id for sign-in - same as the GameHelper sample.
	static final int kIntentIdSignIn = 9876;
	// Intent id for "platform only" sign-in - same as regular sign-in flow
	// but we only care about establishing an account connection, not connecting
	// the current player's game to Google Play.
	static final int kIntentIdPlatformOnlySignIn = 7986;

	// Cache a reference to the native activity to dispatch native callbacks.
	AndroidNativeActivity m_Activity;
	// (Optional) If defined, the client id with which we request an id token
	// for the server to request user profile data.
	String m_sOauth2ClientId;
	// Storage - for platform only sign in, the callback to report results to.
	long m_pPlatformOnlyCallback;
	// Track whether logging is enabled or not.
	boolean m_bDebugging;
	// Track whether we should automatically sign-in or not.
	boolean m_bAutoSignIn;
	// Our client instance.
	GoogleApiClient m_ApiClient;
	// Tracking to filter redundant onStart()/onStop() calls.
	boolean m_bStarted;
	// Tracking for desired sign-in (if sign-in failed because connection was still in progress).
	boolean m_bWantSignIn;

	// Wrapper for connecting the api (must always be optional with games support).
	void Connect()
	{
		m_ApiClient.connect(GoogleApiClient.SIGN_IN_MODE_OPTIONAL);
	}

	// Begin GoogleApiClient.ConnectionCallbacks
	@Override
	public void onConnected(Bundle hint)
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("onConnected: " + (null != hint ? hint.toString() : "<null>")); }

		// Trigger a sign-in if requested.
		if (m_bWantSignIn)
		{
			SignIn();
		}
	}

	@Override
	public void onConnectionSuspended(int cause)
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("onConnectionSuspended: " + cause); }
	}
	// End GoogleApiClient.ConnectionCallbacks

	// Begin GoogleApiClient.OnConnectionFailedListener
	@Override
	public void onConnectionFailed(ConnectionResult result)
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("onConnectionFailed: " + result.toString()); }

		// Can perform resolution handling for this result.
		if (result.hasResolution())
		{
			// Based on recent Google examples of how to implement
			// this flow - apparently resolution intents are no longer
			// "the right way": https://github.com/playgameservices/play-games-plugin-for-unity/blob/master/source/SupportLib/PlayGamesPluginSupport/src/main/java/com/google/games/bridge/TokenFragment.java
			if (m_bAutoSignIn)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("onConnectionFailed: resolution possible and auto-sign in enabled, performing explicit sign-in flow."); }

				try
				{
					// Request an intent for the sign-in process.
					Intent intent = Auth.GoogleSignInApi.getSignInIntent(m_ApiClient);
					if (null == intent)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("onConnectionFailed: failed, getSignInIntent() returned a <null> intent."); }
						return;
					}

					// Kick off sign-in.
					if (AndroidLogConfig.ENABLED) { LogMessage("onConnectionFailed: kicking sign-in intent to activity."); }
					m_Activity.startActivityForResult(intent, kIntentIdSignIn);
				}
				catch (Exception e)
				{
					e.printStackTrace();
					if (AndroidLogConfig.ENABLED) { LogMessage("onConnectionFailed: exception on attempt to trigger explicit sign-in, reporting sign-in failure."); }
					AndroidNativeActivity.NativeOnSignInChange(false);
				}
			}
			else
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("onConnectionFailed: resolution possible but auto-sign in disabled, reporting sign-in failure."); }
				AndroidNativeActivity.NativeOnSignInChange(false);
			}
		}
		else
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("onConnectionFailed: failed result has no resolution, reporting sign-in failure."); }
			AndroidNativeActivity.NativeOnSignInChange(false);
		}
	}
	// End GoogleApiClient.OnConnectionFailedListener

	void HandleAutoSignInResult(GoogleSignInResult result)
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("HandleAutoSignInResult: " + (null != result ? result.toString() : "<null>")); }

		// Pre-filtering - if result is defined and failed, we handle several cases directly
		// without yet reporting to the user.
		if (null != result && !result.isSuccess())
		{
			final int iCode = result.getStatus().getStatusCode();

			// SIGN_IN_REQUIRED means we need to trigger standard sign-in flow.
			if (ConnectionResult.SIGN_IN_REQUIRED == iCode)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("HandleAutoSignInResult: triggering full sign-in flow due to SIGN_IN_REQUIRED result."); }
				SignIn();
				return;
			}
		}

		// Fallback to always handling.
		HandleSignInResult(result);
	}

	void HandleRequestIdTokenResult(GoogleSignInResult result, final long pUserData)
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("HandleRequestIdTokenResult: " + (null != result ? result.toString() : "<null>")); }

		// Success, dispatch with token.
		if (null != result && result.isSuccess())
		{
			GoogleSignInAccount account = result.getSignInAccount();
			if (null != account)
			{
				final String sIdToken = account.getIdToken();
				if (null != sIdToken && !sIdToken.isEmpty())
				{
					AndroidNativeActivity.NativeOnRequestIdToken(pUserData, true, sIdToken);
					return;
				}
			}
		}

		// If we get here, failure case.
		AndroidNativeActivity.NativeOnRequestIdToken(pUserData, false, "");
	}

	void HandleSignInResult(GoogleSignInResult result)
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("HandleSignInResult: " + (null != result ? result.toString() : "<null>")); }

		// A null result is considered a failure.
		if (null == result)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("HandleSignInResult: <null> result, classifying as failure."); }
			AndroidNativeActivity.NativeOnSignInChange(false);
		}
		// Success or failure based on contents.
		else if (result.isSuccess())
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("HandleSignInResult: successful result."); }
			AndroidNativeActivity.NativeOnSignInChange(true);
		}
		// Sign in failure.
		else
		{
			if (AndroidLogConfig.ENABLED)
			{
				LogMessage("HandleSignInResult: result failure (" +
					GoogleSignInStatusCodes.getStatusCodeString(result.getStatus().getStatusCode()) + ", " +
					result.getStatus().getStatusMessage() + ")");
			}

			AndroidNativeActivity.NativeOnSignInChange(false);
		}
	}

	SharedPreferences GetSharedPreferences()
	{
		try
		{
			return m_Activity.getSharedPreferences(AndroidGooglePlayAuth.class.getSimpleName(), Context.MODE_PRIVATE);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return null;
		}
	}

	void ReadAutoSignIn()
	{
		SharedPreferences prefs = GetSharedPreferences();
		if (null == prefs)
		{
			m_bAutoSignIn = true;
			return;
		}

		// Update local value.
		m_bAutoSignIn = prefs.getBoolean(kKeyAutoSignIn, true);
	}

	void WriteAutoSignIn(boolean bAutoSignIn)
	{
		// Update local value.
		m_bAutoSignIn = bAutoSignIn;

		SharedPreferences prefs = GetSharedPreferences();
		if (null == prefs)
		{
			return;
		}

		SharedPreferences.Editor editor = prefs.edit();
		editor.putBoolean(kKeyAutoSignIn, bAutoSignIn);
		editor.apply();
	}

	void LogMessage(String sMessage)
	{
		// Early out if not logging.
		if (!m_bDebugging)
		{
			return;
		}

		if (AndroidLogConfig.ENABLED)
		{
			Log.d(AndroidLogConfig.TAG, "[AndroidGooglePlayAuth]: " + sMessage);
		}
	}

	// Public utility for checking for the availability of Google Play
	// services. Caller is expected to check this for true before
	// constructing an AndroidGooglePlayAuth instance.
	public static boolean IsAvailable(final AndroidNativeActivity activity)
	{
		try
		{
			final int iStatus = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(activity);
			return (ConnectionResult.SUCCESS == iStatus);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return false;
		}
	}

	public AndroidGooglePlayAuth(
		final AndroidNativeActivity activity,
		String sOauth2ClientId,
		final boolean bDebugging)
	{
		m_Activity = activity;
		m_sOauth2ClientId = sOauth2ClientId;
		m_bDebugging = bDebugging;

		// Refresh internal state.
		ReadAutoSignIn();

		// Create our auth client.
		GoogleSignInOptions.Builder optionsBuilder = new GoogleSignInOptions.Builder(GoogleSignInOptions.DEFAULT_GAMES_SIGN_IN);

		// Request of id token.
		optionsBuilder.requestIdToken(m_sOauth2ClientId);

		// Resolve.
		GoogleSignInOptions options = optionsBuilder.build();

		// Instantiate our client.
		m_ApiClient = new GoogleApiClient.Builder(m_Activity)
			.addConnectionCallbacks(this)
			.addOnConnectionFailedListener(this)
			.addApi(Auth.GOOGLE_SIGN_IN_API, options)
			.addApi(Games.API)
			.build();
	}

	// Handlers - expected to be called as appropriate by
	// the activity.
	public void OnActivityResult(
		int iRequestCode,
		int iResultCode,
		Intent intent)
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("OnActivityResult: (" + iRequestCode + ", " + iResultCode + "): " + (null != intent ? intent.toString() : "<null>")); }

		// Handle various codes that we care about.
		switch (iRequestCode)
		{
		case kIntentIdPlatformOnlySignIn:
			{
				GoogleSignInResult result = Auth.GoogleSignInApi.getSignInResultFromIntent(intent);

				// Cache callback locally.
				final long pCallback = m_pPlatformOnlyCallback;
				m_pPlatformOnlyCallback = 0;

				// Handl result based on state.
				if (null != result && result.isSuccess())
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("OnActivityResult(kIntentIdPlatformOnlySignIn): result success, dispatching success result to callback."); }
					AndroidNativeActivity.NativeOnPlatformOnlySignInResult(pCallback, true);
				}
				else
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("OnActivityResult(kIntentIdPlatformOnlySignIn): result failure, dispatching failure result to callback."); }
					AndroidNativeActivity.NativeOnPlatformOnlySignInResult(pCallback, false);
				}
			}
			break;
		case kIntentIdSignIn:
			{
				GoogleSignInResult result = Auth.GoogleSignInApi.getSignInResultFromIntent(intent);

				// Handl result based on state.
				if (null != result && result.isSuccess())
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("OnActivityResult(kIntentIdSignIn): result success, handling result."); }
				}
				else
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("OnActivityResult(kIntentIdSignIn): result failure, handling result."); }

					// Handle cancellation explicitly, sign out so we don't prompt the user again.
					if (Activity.RESULT_CANCELED == iResultCode)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("OnActivityResult(kIntentIdSignIn): user cancellation, result code, signing out."); }
						AndroidNativeActivity.NativeOnSignInCancelled(); // Signal to native.
						SignOut();
						return;
					}
					else if (null != result && result.getStatus().getStatusCode() == CommonStatusCodes.CANCELED)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("OnActivityResult(kIntentIdSignIn): user cancellation, result status code, signing out."); }
						AndroidNativeActivity.NativeOnSignInCancelled(); // Signal to native.
						SignOut();
						return;
					}
				}

				HandleSignInResult(result);
			}
			break;
		}
	}

	public void OnResume()
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("OnResume"); }

		// If auto sign in is turned off, report this to native immediately and return.
		if (!m_bAutoSignIn)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnResume: auto sign-in disabled, reporting not signed in to native immediately."); }
			AndroidNativeActivity.NativeOnSignInChange(false);
			return;
		}

		try
		{
			OptionalPendingResult<GoogleSignInResult> pendingResult = Auth.GoogleSignInApi.silentSignIn(m_ApiClient);

			// Early out if no result.
			if (null == pendingResult)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("OnResume: <null> pending result."); }
				return;
			}

			// Already signed in, handle immediately.
			if (pendingResult.isDone())
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("OnResume: connected immediately, handling result."); }
				HandleAutoSignInResult(pendingResult.get());
			}
			// Wait for result callback.
			else
			{
				pendingResult.setResultCallback(new ResultCallback<GoogleSignInResult>()
				{
					@Override
					public void onResult(GoogleSignInResult result)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("OnResume: async result callback, handling result."); }
						HandleAutoSignInResult(result);
					}
				});
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
			if (AndroidLogConfig.ENABLED) { LogMessage("OnResume: exception caught, continuing."); }
			return;
		}
	}

	public void OnStart()
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("OnStart"); }

		// Early out if started.
		if (m_bStarted)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnStart: skipping connect, already started."); }
			return;
		}

		// Started.
		m_bStarted = true;

		try
		{
			Connect();
		}
		catch (Exception e)
		{
			e.printStackTrace();
			if (AndroidLogConfig.ENABLED) { LogMessage("OnStart: call of m_ApiClient.connect() failed."); }
			return;
		}
	}

	public void OnStop()
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("OnStop"); }

		// Early out if not started.
		if (!m_bStarted)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("OnStop: skipping disconnect, not started."); }
			return;
		}

		// Not started.
		m_bStarted = false;

		try
		{
			if (m_ApiClient.isConnected())
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("OnStop: calling m_ApiClient.disconnect()"); }
				m_ApiClient.disconnect();
			}
			else
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("OnStop: skipping m_ApiClient.disconnect(), client is not connected."); }
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
			if (AndroidLogConfig.ENABLED) { LogMessage("OnStop: call of m_ApiClient.disconnect() failed."); }
			return;
		}
	}

	// Call to explicitly request a new id token. Should be called
	// with each request to ensure the token is fresh.
	public void RequestIdToken(final long pUserData)
	{
		OptionalPendingResult<GoogleSignInResult> pendingResult = Auth.GoogleSignInApi.silentSignIn(m_ApiClient);

		// Early out if no result.
		if (null == pendingResult)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("RequestIdToken: <null> pending result."); }
			AndroidNativeActivity.NativeOnRequestIdToken(pUserData, false, "");
			return;
		}

		// Already signed in, handle immediately.
		if (pendingResult.isDone())
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("RequestIdToken: connected immediately, handling result."); }
			HandleRequestIdTokenResult(pendingResult.get(), pUserData);
		}
		// Wait for result callback.
		else
		{
			pendingResult.setResultCallback(new ResultCallback<GoogleSignInResult>()
			{
				@Override
				public void onResult(GoogleSignInResult result)
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("RequestIdToken: async result callback, handling result."); }
					HandleRequestIdTokenResult(result, pUserData);
				}
			});
		}
	}

	// Entry points.

	// Special case version of SignIn - kicks the API enough
	// to setup an account association but does not otherwise
	// change state. Used by the commerce system on IAP when
	// the user has not yet registered an account.
	public void PlatformSignInOnly(final long pCallback)
	{
		// Fail if already in progress.
		if (0 != m_pPlatformOnlyCallback)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("PlatformSignInOnly: fail, platform sign in already in progress."); }
			AndroidNativeActivity.NativeOnPlatformOnlySignInResult(pCallback, false);
			return;
		}

		// Fail if still connecting.
		if (m_ApiClient.isConnecting())
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("PlatformSignInOnly: fail, ApiClient still connecting, reporting failure to callback."); }
			AndroidNativeActivity.NativeOnPlatformOnlySignInResult(pCallback, false);
			return;
		}

		// Always sign in - either we're connected and this will succeed,
		// or we're not connected and (we assume) this will being
		// resolution handling.
		try
		{
			// Request an intent for the sign-in process.
			Intent intent = Auth.GoogleSignInApi.getSignInIntent(m_ApiClient);
			if (null == intent)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("PlatformSignInOnly: failed, getSignInIntent() returned a <null> intent, reporting failure to callback."); }
				AndroidNativeActivity.NativeOnPlatformOnlySignInResult(pCallback, false);
				return;
			}

			// Cache callback data.
			m_pPlatformOnlyCallback = pCallback;

			// Kick off sign-in.
			if (AndroidLogConfig.ENABLED) { LogMessage("PlatformSignInOnly: kicking sign-in intent to activity."); }
			m_Activity.startActivityForResult(intent, kIntentIdPlatformOnlySignIn);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			if (AndroidLogConfig.ENABLED) { LogMessage("PlatformSignInOnly: create of sign-in intent raised exception, reporting failure to callback."); }
			AndroidNativeActivity.NativeOnPlatformOnlySignInResult(pCallback, false);
			return;
		}
	}

	// Standard sign in entry point.
	public void SignIn()
	{
		// No longer desired sign-in this session.
		m_bWantSignIn = false;

		// Always, trigger auto-sign in on next startup.
		WriteAutoSignIn(true);

		// Nop if still connecting.
		if (m_ApiClient.isConnecting())
		{
			m_bWantSignIn = true;
			if (AndroidLogConfig.ENABLED) { LogMessage("SignIn: ignored, ApiClient still connecting, marking sign-in desired."); }
			return;
		}

		// Always sign in - either we're connected and this will succeed,
		// or we're not connected and (we assume) this will being
		// resolution handling.
		try
		{
			// Request an intent for the sign-in process.
			Intent intent = Auth.GoogleSignInApi.getSignInIntent(m_ApiClient);
			if (null == intent)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("SignIn: failed, getSignInIntent() returned a <null> intent."); }
				return;
			}

			// Kick off sign-in.
			if (AndroidLogConfig.ENABLED) { LogMessage("SignIn: kicking sign-in intent to activity."); }
			m_Activity.startActivityForResult(intent, kIntentIdSignIn);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			if (AndroidLogConfig.ENABLED) { LogMessage("SignIn: create of sign-in intent raised exception, ignoring, not signing in."); }
			return;
		}
	}

	public void SignOut()
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: called, disabling auto sign-in and reporting signed out to native immediately."); }

		// In all cases, set auto-sign in to false when an explicit
		// sign-out is requested. Effectivelly, use m_bAutoSignIn == false
		// to mean "signed out".
		WriteAutoSignIn(false);

		// Likewise, immediately report the sign out to native.
		AndroidNativeActivity.NativeOnSignInChange(false);

		// Nop if still connecting.
		if (m_ApiClient.isConnecting())
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: ignored, ApiClient still connecting."); }
			return;
		}
		// Nop if not connected.
		else if (!m_ApiClient.isConnected())
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: ignored, ApiClient is not signed in."); }
			return;
		}

		// Log out of Games API first, if it is connected.
		if (m_ApiClient.hasConnectedApi(Games.API))
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: calling Games.signOut()"); }
			try
			{
				Games.signOut(m_ApiClient);
			}
			catch (Exception e)
			{
				e.printStackTrace();
				if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: Games.signOut() failed: ignoring."); }
			}
		}
		else
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: skipping Games.signOut(), Games API is not connected."); }
		}

		if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: calling Auth.GoogleSignInApi.signOut()"); }
		try
		{
			PendingResult<Status> result = Auth.GoogleSignInApi.signOut(m_ApiClient);

			// Filter.
			if (null == result)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: result ignored, signOut() returned null result."); }
				return;
			}

			// Setup completion callbacks.
			result.setResultCallback(new ResultCallback<Status>()
			{
				@Override
				public void onResult(Status status)
				{
					// Currently, we just log for debugging purposes. Game assumes sign-out
					// succeeded as soon as it was called.
					if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: " + status.getStatusMessage()); }
				}
			});
		}
		catch (Exception e)
		{
			e.printStackTrace();
			if (AndroidLogConfig.ENABLED) { LogMessage("SignOut: Auth.GoogleSignInApi.signOut() failed: ignoring."); }
		}
	}
}
