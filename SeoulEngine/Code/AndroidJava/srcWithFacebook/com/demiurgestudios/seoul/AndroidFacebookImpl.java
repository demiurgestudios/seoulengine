/**
 * \file AndroidFacebookImpl.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

// System
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import java.math.BigDecimal;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Currency;
import java.util.Date;
import org.json.JSONArray;
import org.json.JSONObject;

// Facebook
import com.facebook.AccessToken;
import com.facebook.LoggingBehavior;
import com.facebook.appevents.AppEventsLogger;
import com.facebook.CallbackManager;
import com.facebook.FacebookCallback;
import com.facebook.FacebookException;
import com.facebook.FacebookSdk;
import com.facebook.GraphRequestBatch;
import com.facebook.HttpMethod;
import com.facebook.FacebookRequestError;
import com.facebook.GraphRequest;
import com.facebook.GraphResponse;
import com.facebook.login.LoginManager;
import com.facebook.login.LoginResult;
import com.facebook.share.model.GameRequestContent;
import com.facebook.share.model.ShareLinkContent;
import com.facebook.share.widget.GameRequestDialog;
import com.facebook.share.widget.ShareDialog;

public final class AndroidFacebookImpl
{
	protected AndroidNativeActivity m_Activity;

	public AndroidFacebookImpl(final AndroidNativeActivity activity)
	{
		m_Activity = activity;
	}

	/**
	 * Facebook event logger.
	 */
	protected AppEventsLogger m_FacebookEventLogger = null;

	/**
	 * Facebook's Callback Manager handles latent callbacks during application
	 * resumes. Create it during the onCreate() method.
	 */
	protected CallbackManager m_FacebookCallback = null;

	protected Boolean IsFacebookInitialized()
	{
		return m_FacebookCallback != null;
	}

	public boolean onActivityResult(int iRequestCode, int iResultCode, Intent intent)
	{
		// Give Facebook a chance to consume the result.
		if (m_FacebookCallback != null && m_FacebookCallback.onActivityResult(iRequestCode, iResultCode, intent))
		{
			return true;
		}

		return false;
	}

	public void onResume()
	{
		// After every resume, pull the user's facebook id from their auth token
		// and update FacebookManager
		if(IsFacebookInitialized())
		{
			m_Activity.NativeUpdateFacebookUserInfo(FacebookGetUserID());
		}
	}

	public void FacebookInitialize(final boolean bEnableDebugLogging)
	{
		if (bEnableDebugLogging)
		{
			if (AndroidLogConfig.ENABLED) { Log.d(AndroidLogConfig.TAG, "Initializing Facebook."); }
		}

		// NOTE: We activate each of these settings here because at this point we know
		// the user has accepted GDPR. Each setting must be activated individually.
		// See https://developers.facebook.com/docs/app-events/getting-started-app-events-android
		// for more info.
		FacebookSdk.setAutoLogAppEventsEnabled(true);
		FacebookSdk.setAutoInitEnabled(true);
		FacebookSdk.setAdvertiserIDCollectionEnabled(true);

		// Enable debug only local logging for the SDK
		if (bEnableDebugLogging)
		{
			FacebookSdk.setIsDebugEnabled(bEnableDebugLogging);
			FacebookSdk.addLoggingBehavior(LoggingBehavior.REQUESTS);
			FacebookSdk.addLoggingBehavior(LoggingBehavior.INCLUDE_ACCESS_TOKENS);
			FacebookSdk.addLoggingBehavior(LoggingBehavior.INCLUDE_RAW_RESPONSES);
			FacebookSdk.addLoggingBehavior(LoggingBehavior.CACHE);
			FacebookSdk.addLoggingBehavior(LoggingBehavior.APP_EVENTS);
			FacebookSdk.addLoggingBehavior(LoggingBehavior.DEVELOPER_ERRORS);
			FacebookSdk.addLoggingBehavior(LoggingBehavior.GRAPH_API_DEBUG_WARNING);
			FacebookSdk.addLoggingBehavior(LoggingBehavior.GRAPH_API_DEBUG_INFO);
		}
		m_FacebookCallback = CallbackManager.Factory.create();
		FacebookRegisterLoginCallback();
		if (bEnableDebugLogging)
		{
			if (AndroidLogConfig.ENABLED) { Log.i(AndroidLogConfig.TAG, "FacebookSdk initialized with access token: " + AccessToken.getCurrentAccessToken()); }
		}

		m_FacebookEventLogger = AppEventsLogger.newLogger(m_Activity);

		if (m_Activity.GooglePlayResumed())
		{
			m_Activity.NativeUpdateFacebookUserInfo(FacebookGetUserID());
		}
	}

	private void FacebookRegisterLoginCallback()
	{
		if (!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookRegisterLoginCallback called prior to initializing Facebook."); }
			return;
		}

		final LoginManager login = LoginManager.getInstance();
		login.registerCallback(m_FacebookCallback, new FacebookCallback<LoginResult>()
		{
			@Override
			public void onSuccess(LoginResult loginResult)
			{
				final Date now = new Date();
				final AccessToken token = loginResult.getAccessToken();
				m_Activity.runOnUiThread(new Runnable()
				{
					@Override
					public void run()
					{
						if ((token != null) && now.before(token.getExpires()))
						{
							// TODO: Pass the session state back to the game
							m_Activity.NativeUpdateFacebookUserInfo(token.getUserId());
							m_Activity.NativeOnFacebookLoginStatusChanged();
						}
					}
				});
			}

			@Override
			public void onCancel()
			{
				m_Activity.runOnUiThread(new Runnable()
				{
					@Override
					public void run()
					{
						m_Activity.NativeUpdateFacebookUserInfo("");
						m_Activity.NativeOnFacebookLoginStatusChanged();
					}
				});
			}

			@Override
			public void onError(FacebookException exception)
			{
				m_Activity.runOnUiThread(new Runnable()
				{
					@Override
					public void run()
					{
						m_Activity.NativeUpdateFacebookUserInfo("");
						m_Activity.NativeOnFacebookLoginStatusChanged();
					}
				});
			}
		});
	}

	/**
	 * Begins the Facebook login flow.  The user is asked if he wants to allow
	 * the app to have the given permissions.  Upon success or failure,
	 * GameApp::Get()->OnFacebookLoginStatusChanged() is called back.
	 */
	public void FacebookLogin(String[] aRequestedPermissions)
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookLogin called prior to initializing Facebook."); }
			return;
		}

		final LoginManager login = LoginManager.getInstance();
		login.logInWithReadPermissions(m_Activity, Arrays.asList(aRequestedPermissions));
	}

	/**
	 * Closes the current session but does not delete the user's current
	 * authentication token.
	 */
	public void FacebookCloseSession()
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookCloseSession called prior to initializing Facebook."); }
			return;
		}

		LoginManager.getInstance().logOut();
	}

	/**
	 * Tests if the user is currently logged into Facebook
	 */
	public boolean FacebookIsLoggedIn()
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookIsLoggedIn called prior to initializing Facebook."); }
			return false;
		}

		Date now = new Date();
		AccessToken token = AccessToken.getCurrentAccessToken();
		return (token != null) && now.before(token.getExpires());
	}

	/**
	 * Gets the current Facebook authentication token, or the empty string if
	 * we don't have a token.
	 */
	public String FacebookGetAuthToken()
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookGetAuthToken called prior to initializing Facebook."); }
			return "";
		}

		AccessToken token = AccessToken.getCurrentAccessToken();
		return (token != null) ? token.getToken() : "";
	}

	/**
	 * Gets the current Facebook user ID, or the empty string if
	 * we don't have a token.
	 */
	public String FacebookGetUserID()
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookGetUserID called prior to initializing Facebook."); }
			return "";
		}

		AccessToken token = AccessToken.getCurrentAccessToken();
		return (token != null) ? token.getUserId() : "";
	}

	/**
	 * Kicks off an asynchronous friend list request that will return data for
	 * the given fields for a max iLimit number of friends.
	 *
	 * This function gathers a list of ALL friends, then searches for friends
	 * who have actually installed the game. It passes the filtered list of
	 * friends into the native handler.
	 */
	public void FacebookGetFriendsList(final String sFields, final int iLimit)
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookGetFriendsList called prior to initializing Facebook."); }
			return;
		}

		final Date now = new Date();
		final AccessToken token = AccessToken.getCurrentAccessToken();
		if ((token != null) && now.before(token.getExpires()))
		{
			// Async tasks need to be run on the UI thread
			m_Activity.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					// Graph path for our friends list
					String sGraphPathString  = "me/friends";

					Bundle params = new Bundle();
					params.putString("fields", sFields);

					// Create the request
					GraphRequest request = new GraphRequest(token, sGraphPathString, params, HttpMethod.GET, new GraphRequest.Callback()
					{
						@Override
						public void onCompleted(GraphResponse response)
						{
							String result = "";
							try
							{
								// Success - return the JSON friend data
								JSONObject json = response.getJSONObject();
								JSONArray matches = json.getJSONArray("data");
								JSONArray filtered = new JSONArray();
								JSONArray bystanders = new JSONArray();
								int total = 0;
								int index;

								// Gather installed users.
								for (index = 0; index < matches.length(); index++)
								{
									JSONObject entry = matches.getJSONObject(index);
									if (entry.optBoolean("installed", false))
									{
										if ((iLimit == 0) || (total < iLimit))
										{
											total++;
											filtered.put(entry);
										}
										else
										{
											// If we have a limit, stop
											// once we reach it.
											break;
										}
									}
									else
									{
										bystanders.put(entry);
									}
								}
								// If space permits, add non-players.
								for (index = 0; index < bystanders.length(); index++)
								{
									if ((iLimit == 0) || (total < iLimit))
									{
										filtered.put(bystanders.getJSONObject(index));
									}
									else
									{
										break;
									}
									total++;
								}

								// Replace the original data with the filtered set.
								json.put("data", filtered);
								result = json.toString();
							}
							catch (Exception _e)
							{
								FacebookRequestError fbre = response.getError();
								if (fbre != null)
								{
									if (AndroidLogConfig.ENABLED) { Log.e(AndroidLogConfig.TAG, "GraphPath Request Error: " + fbre.toString()); }
								}
								else
								{
									if (AndroidLogConfig.ENABLED) { Log.e(AndroidLogConfig.TAG, "Unknown Facebook Request Error: " + _e.getMessage()); }
								}
							}
							m_Activity.NativeOnReturnFriendsList(result);
						}
					});

					// Kick off the task
					request.executeAsync();
				}
			});
		}
		else
		{
			m_Activity.NativeOnReturnFriendsList("");
		}
	}

	/**
	 * Send a request with message sMessage.  To send it to a specific list of friends create a comma separated list of friend
	 *     IDs and pass it to sRecipients.  To allow the user to choose from their friends but suggesting a given set us sSuggestedFriends.
	 *     sData will be included as a data string when the user receives the request.
	 */
	public void FacebookSendRequest(final String sMessage, final String sTitle, final String sRecipients, final String sSuggestedFriends, final String sData)
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookSendRequest called prior to initializing Facebook."); }
			return;
		}

		GameRequestContent.Builder content = new GameRequestContent.Builder();

		if(!sMessage.isEmpty())
		{
			content.setMessage(sMessage);
		}

		if(!sTitle.isEmpty())
		{
			content.setTitle(sTitle);
		}

		if(!sRecipients.isEmpty())
		{
			content.setRecipients(new ArrayList<String>(Arrays.asList(sRecipients.split(","))));
		}
		else if(!sSuggestedFriends.isEmpty())
		{
			content.setSuggestions(new ArrayList<String>(Arrays.asList(sSuggestedFriends.split(","))));
		}

		if(!sData.isEmpty())
		{
			content.setData(sData);
		}

		// Turn on frictionless requests
		// TODO: Doesn't seem to exist in Facebook SDK 4, revisit
		// on next Facebook SDK update.
		//params.putString("frictionless", "1");

		// Show the dialog (if we need it)
		showRequestDialog(content.build(),sRecipients,sData);
	}

	public void FacebookDeleteRequest(final String sRequestID)
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookDeleteRequest called prior to initializing Facebook."); }
			return;
		}

		// Make sure we have a valid session
		final Date now = new Date();
		final AccessToken token = AccessToken.getCurrentAccessToken();
		if ((token != null) && now.before(token.getExpires()))
		{
			m_Activity.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					// Kick off the call to delete the request; ignore errors
					GraphRequest request = GraphRequest.newDeleteObjectRequest(token, sRequestID, null);
					request.executeAsync();
				}
			});
		}
	}

	public void FacebookSendPurchaseEvent(final double fLocalPrice, final String sCurrencyCode)
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookSendPurchaseEvent called prior to initializing Facebook."); }
			return;
		}

		m_Activity.runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				try
				{
					m_FacebookEventLogger.logPurchase(BigDecimal.valueOf(fLocalPrice),
				                                      Currency.getInstance(sCurrencyCode));
				}
				catch (Exception e)
				{
					if (AndroidLogConfig.ENABLED)
					{
						Log.e(AndroidLogConfig.TAG, "Error tracking facebook purchase event "
							+ " error: " + e.getMessage());
					}
				}
			}
		});
	}

	public void FacebookTrackEvent(final String sEventID)
	{
		if(!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookTrackEvent called prior to initializing Facebook."); }
			return;
		}

		m_Activity.runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				try
				{
					m_FacebookEventLogger.logEvent(sEventID);
				}
				catch (Exception e)
				{
					if (AndroidLogConfig.ENABLED)
					{
						Log.e(AndroidLogConfig.TAG, "Error tracking facebook event "
							+ sEventID + " error: " + e.getMessage());
					}
				}
			}
		});
	}

	public void FacebookShareLink(final String sContentURL)
	{
		if (!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookShareLink called prior to initializing Facebook."); }
			return;
		}

		ShareLinkContent.Builder contentBuilder = new ShareLinkContent.Builder()
			.setContentUrl(Uri.parse(sContentURL));

		final ShareLinkContent content = contentBuilder.build();
		final AndroidNativeActivity activity = m_Activity;
		m_Activity.runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				ShareDialog dialog = new ShareDialog(activity);
				dialog.registerCallback(m_FacebookCallback, new FacebookCallback<ShareDialog.Result>()
				{
					@Override public void onSuccess(ShareDialog.Result result)
					{
						m_Activity.NativeOnSentShareLink(true);
					}
					@Override public void onCancel()
					{
						// User cancelled - leave the screen
						m_Activity.NativeOnSentShareLink(false);
					}
					@Override public void onError(FacebookException e)
					{
						if (AndroidLogConfig.ENABLED) { Log.e(AndroidLogConfig.TAG, "Send Request Error: " + e.toString()); }
						m_Activity.NativeOnSentShareLink(false);
					}
				});

				dialog.show(content);
			}
		});
	}

	// Show a dialog (feed or request) without a notification bar (i.e. full screen)
	private void showRequestDialog(final GameRequestContent content, final String sRecipients, final String sData)
	{
		if (!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "showRequestDialog called prior to initializing Facebook."); }
			return;
		}

		final Date now = new Date();
		final AccessToken token = AccessToken.getCurrentAccessToken();
		if ((token != null) && now.before(token.getExpires()))
		{
			final AndroidNativeActivity activity = m_Activity;
			m_Activity.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					GameRequestDialog dialog = new GameRequestDialog(activity);
					dialog.registerCallback(m_FacebookCallback, new FacebookCallback<GameRequestDialog.Result>()
					{
						@Override public void onSuccess(GameRequestDialog.Result result)
						{
							// User actually sent request - so send the gift
							String sRequestID = result.getRequestId();
							m_Activity.NativeOnSentRequest(sRequestID, sRecipients, sData);
						}
						@Override public void onCancel()
						{
							// User cancelled - do nothing.
						}
						@Override public void onError(FacebookException e)
						{
							if (AndroidLogConfig.ENABLED) { Log.e(AndroidLogConfig.TAG, "Send Request Error: " + e.toString()); }
						}
					});

					dialog.show(content);
				}
			});
		}
	}

	public void FacebookRequestBatchUserInfo(final String[] aUserIDs)
	{
		if (!IsFacebookInitialized())
		{
			if (AndroidLogConfig.ENABLED) { Log.w(AndroidLogConfig.TAG, "FacebookRequestBatchUserInfo called prior to initializing Facebook."); }
			return;
		}

		final Date now = new Date();
		final AccessToken token = AccessToken.getCurrentAccessToken();
		if ((token != null) && now.before(token.getExpires()))
		{
			m_Activity.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					GraphRequestBatch requestBatch = new GraphRequestBatch();
					for(final String userId : aUserIDs)
					{
						requestBatch.add(new GraphRequest(token, userId, null, null, new GraphRequest.Callback()
						{
							public void onCompleted(GraphResponse response)
							{
								JSONObject json = response.getJSONObject();

								if(json != null && json.opt("id") != null)
								{
									m_Activity.NativeOnGetBatchUserInfo(json.optString("id"), json.optString("name"));
								}
								else
								{
									m_Activity.NativeOnGetBatchUserInfoFailed(userId);
								}
							}
						}));
					}
					requestBatch.executeAsync();
				}
			});
		}
	}
}
