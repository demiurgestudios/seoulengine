/**
 * \file AndroidAppsFlyerImpl.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

// System
import android.content.SharedPreferences;

import java.util.HashMap;
import java.util.Map;

import android.net.Uri;
import android.util.Log;

// AppsFlyer
import com.appsflyer.AppsFlyerConversionListener;
import com.appsflyer.AppsFlyerLib;
import com.appsflyer.AppsFlyerProperties;

public final class AndroidAppsFlyerImpl
{
	/** SharedPreferences key for install attribution propagation. */
	private static final String ksCampaignInstallAttributionKey = "AndroidTrackingInfoCampaignInstallAttribution";
	private static final String ksMediaSourceInstallAttributionKey = "AndroidTrackingInfoMediaSourceInstallAttribution";

	protected AndroidNativeActivity m_Activity;

	private String m_sDeepLinkCampaignScheme;

	public AndroidAppsFlyerImpl(final AndroidNativeActivity activity)
	{
		m_Activity = activity;

		// Default to empty string; we don't want to parse
		// deep links until AppsFlyer is initialized.
		m_sDeepLinkCampaignScheme = "";
	}

	public void AppsFlyerTrackEvent(
		final String sEventID,
		final boolean bLiveBuild)
	{
		final AndroidNativeActivity self = m_Activity;
		m_Activity.runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				try
				{
					final HashMap<String, Object> tProperties = new HashMap<String, Object>();
					tProperties.put("af_is_dev_build", !bLiveBuild);

					AppsFlyerLib.getInstance().trackEvent(
						self.getApplicationContext(),
						sEventID,
						tProperties);
				}
				catch (Exception e)
				{
					e.printStackTrace();
				}
			}
		});
	}

	public void AppsFlyerSessionChange(
		final boolean bIsSessionStart,
		final String sSessionUUID,
		final long iTimeStampMicrosecondsUnixUTC,
		final long iDurationMicroseconds,
		final boolean bLiveBuild)
	{
		final AndroidNativeActivity self = m_Activity;
		final String sEventID = (bIsSessionStart ? "af_session_start" : "af_session_end");
		final HashMap<String, Object> tProperties = new HashMap<String, Object>();
		if (!bIsSessionStart)
		{
			tProperties.put("af_session_duration", (iDurationMicroseconds / 1000000));
		}
		tProperties.put("af_session_id", sSessionUUID);
		tProperties.put("af_is_dev_build", !bLiveBuild);

		m_Activity.runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				try
				{
					AppsFlyerLib.getInstance().trackEvent(
						self.getApplicationContext(),
						sEventID,
						tProperties);
				}
				catch (Exception e)
				{
					e.printStackTrace();
				}
			}
		});
	}



	public void handleDeepLinking(Map<String, String> tData)
	{
		try
		{
			// We get one of:
			// - Data mapped for us by AppsFlyer
			// - A single key 'link' which corresponds to the entire
			//   deep link for us to parse ourselves
			//
			// A deep link will look something like:
			// - An 'af_dp' parameter which has the scheme in it
			// - A 'scheme' parameter which has the scheme, and
			//   an 'af_deeplink' parameter should be 'true'
			// - Both should also include the campaign name
			//   separately
			String sShouldDeeplink = tData.get("af_deeplink");
			String sScheme = tData.get("scheme");
			String sAfdp = tData.get("af_dp");
			String sCampaignName = tData.get("campaign");

			// If we didn't get the campaign name from the map,
			// then we need to try and pull it out of the link
			if (sCampaignName == null)
			{
				// For app links we need to parse out the link ourselves
				String sLink = tData.get("link");
				if (sLink == null)
				{
					// If we can't get the campaign name, either in the
					// map or in the link in the map, we can't do
					// anything anyways.
					return;
				}
				Uri linkUri = Uri.parse(sLink);
				// Note that it can be one of these two keys
				sCampaignName = linkUri.getQueryParameter("c") == null ?
						linkUri.getQueryParameter("campaign") : linkUri.getQueryParameter("c");
				if (sCampaignName == null)
				{
					// Still null, break out
					return;
				}

				// Also pull af_dp, af_deeplink, and scheme out of the link.
				sAfdp = linkUri.getQueryParameter("af_dp");
				sShouldDeeplink = linkUri.getQueryParameter("af_deeplink");
				sScheme = linkUri.getScheme();
			}

			// If this parameter exists and is false, don't deep link
			if (sShouldDeeplink != null && sShouldDeeplink.equals("false"))
			{
				return;
			}

			if (sScheme == null || !sScheme.equals(m_sDeepLinkCampaignScheme))
			{
				// If scheme does not exist, or is not equal to our deep link
				// scheme, try the af_dp
				if (sAfdp != null)
				{
					Uri afdpUri = Uri.parse(sAfdp);
					if (!afdpUri.getScheme().equals(m_sDeepLinkCampaignScheme))
					{
						return;
					}
				}
				else
				{
					return;
				}
			}

			AndroidNativeActivity.NativeDeepLinkCampaignDelegate(sCampaignName);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public void AppsFlyerInitialize(
		final SharedPreferences prefs,
		final String sUniqueUserID,
		final String sID,
		final String sDeepLinkCampaignScheme,
		final String sChannel,
		final boolean bIsUpdate,
		final boolean bEnableDebugLogging,
		final boolean bLiveBuild)
	{
		m_sDeepLinkCampaignScheme = sDeepLinkCampaignScheme;
		final AndroidNativeActivity self = m_Activity;
		m_Activity.runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				try
				{
					// Setup deep linking; this has to be done in each activity that handles AppsFlyer intents
					AppsFlyerLib.getInstance().sendDeepLinkData(self);

					// Enable/disable debug mode as specified.
					AppsFlyerLib.getInstance().setDebugLog(bEnableDebugLogging);

					// Set whether this install is an update or not.
					AppsFlyerLib.getInstance().setIsUpdate(bIsUpdate);

					// Setup device ID gathering.
					AppsFlyerLib.getInstance().setCollectAndroidID(true);
					AppsFlyerLib.getInstance().setCollectIMEI(false);

					if (sChannel != null && sChannel.length() != 0)
					{
						if (bEnableDebugLogging)
						{
							if (AndroidLogConfig.ENABLED)
							{
								Log.d(AndroidLogConfig.TAG, "[AndroidAppsFlyerImpl]: Set AppsFlyer channel: " + sChannel);
							}
						}
						AppsFlyerProperties.getInstance().set(AppsFlyerProperties.CHANNEL, sChannel);
					}

					// Setup our conversion listener.
					final AppsFlyerConversionListener listener = new AppsFlyerConversionListener()
					{

						void handleInstallAttributionData(Map<String, String> tData)
						{
							try
							{
								// Acquire.
								String sCampaign = tData.get("campaign");
								String sMediaSource = tData.get("media_source");

								// Store.
								final SharedPreferences.Editor editor = prefs.edit();

								// If campaign is not null, update it, otherwise only update it if the existing
								// value is null, in which case, set it to "organic".
								if (null != sCampaign)
								{
									editor.putString(ksCampaignInstallAttributionKey, sCampaign);
								}
								else if (!prefs.contains(ksCampaignInstallAttributionKey))
								{
									editor.putString(ksCampaignInstallAttributionKey, "organic");
								}

								// If media_source is not null, update it, otherwise only update it if the existing
								// value is null, in which case, set it to "organic".
								if (null != sMediaSource)
								{
									editor.putString(ksMediaSourceInstallAttributionKey, sMediaSource);
								}
								else if (!prefs.contains(ksMediaSourceInstallAttributionKey))
								{
									editor.putString(ksMediaSourceInstallAttributionKey, "organic");
								}

								// Commit changes that have been made.
								editor.apply();

								// now set the values back to native
								sCampaign = prefs.getString(ksCampaignInstallAttributionKey, "");
								sMediaSource = prefs.getString(ksMediaSourceInstallAttributionKey, "");

								AndroidNativeActivity.NativeSetAttributionData(sCampaign, sMediaSource);
							}
							catch (Exception e)
							{
								e.printStackTrace();
							}
						}

						public void onInstallConversionDataLoaded(Map<String, String> tConversionData)
						{
							// After first install, this data is cached and should always be the same.
							handleInstallAttributionData(tConversionData);
							handleDeepLinking(tConversionData);
						}

						public void onInstallConversionFailure(String sErrorMessage)
						{
							// Nop
						}

						public void onAppOpenAttribution(Map<String, String> tAttributionData)
						{
							// We do not set attribution data here so that we do not override the original
							// install's attribution data.
							handleDeepLinking(tAttributionData);
						}

						public void onAttributionFailure(String sErrorMessage)
						{
							// Nop
						}
					};

					// Initialize AppsFlyer - note that this weird dance with "waitForCustomerUserId" is now
					// necessary. At some point on Android, the AppsFlyer SDK changed such that if startTracking
					// is called after the application's onResume() lifespan callback, the SDK will not attempt
					// to submit attribution/install tracking data to the server until the user puts the app
					// into the background and then foregrounds it again later.
					//
					// setCustomerIdAndTrack() kicks it, so it will attempt it immediately upon call of that method.
					AppsFlyerLib.getInstance().waitForCustomerUserId(true);
					AppsFlyerLib.getInstance().init(sID, listener, self.getApplication().getApplicationContext());
					AppsFlyerLib.getInstance().startTracking(self.getApplication());
					AppsFlyerLib.getInstance().setCustomerIdAndTrack(sUniqueUserID, self.getApplication());
					AppsFlyerLib.getInstance().trackAppLaunch(self.getApplication(), sID);

					// Pass the AppsFlyer ID back to native code.
					m_Activity.NativeSetExternalTrackingUserID(AppsFlyerLib.getInstance().getAppsFlyerUID(self.getApplicationContext()));

					// Initial event.
					AppsFlyerTrackEvent("af_app_open", bLiveBuild);
				}
				catch (Exception e)
				{
					e.printStackTrace();
				}
			}
		});
	}

	public void AppsFlyerShutdown()
	{
		final AndroidNativeActivity self = m_Activity;
		m_Activity.runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				try
				{
					// TODO: Cleanup.
				}
				catch (Exception e)
				{
					e.printStackTrace();
				}
			}
		});
	}
}
