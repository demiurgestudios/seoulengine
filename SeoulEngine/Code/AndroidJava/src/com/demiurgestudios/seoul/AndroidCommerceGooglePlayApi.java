/**
 * \file AndroidCommerceGooglePlayApi.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

import android.app.Activity;
import android.util.Log;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClient.BillingResponse;
import com.android.billingclient.api.BillingClient.FeatureType;
import com.android.billingclient.api.BillingClient.SkuType;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.ConsumeResponseListener;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.SkuDetails;
import com.android.billingclient.api.SkuDetailsParams;
import com.android.billingclient.api.SkuDetailsResponseListener;

import java.text.DateFormat;		// For debug logging
import java.text.SimpleDateFormat;	// For debug logging
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;				// For debug logging
import java.util.List;
import java.util.Locale;
import org.json.JSONObject;

/**
 * Implementation of Android commerce API for Google Play purchasing.
 */
public final class AndroidCommerceGooglePlayApi extends AndroidCommerceApi implements PurchasesUpdatedListener
{
	final Activity m_Activity;
	final Listener m_Listener;
	final boolean m_bDebugEnabled;
	final BillingClient m_Client;
	boolean m_bConnecting = false;
	boolean m_bConnected = false;
	List<ProductInfo> m_productInfos = new ArrayList<ProductInfo>();

	/** Logging utility. */
	void LogMessage(String sMessage)
	{
		// Early out if not logging.
		if (!m_bDebugEnabled)
		{
			return;
		}

		if (AndroidLogConfig.ENABLED)
		{
			Log.d(AndroidLogConfig.TAG, "[AndroidCommerceGooglePlayApi]: " + sMessage);
		}
	}

	/** Convenience - runs runnable if not null. Eats any exception generated. */
	void checkedRun(final Runnable runnable)
	{
		final AndroidCommerceGooglePlayApi self = this;
		try
		{
			// Synchronization. Re-entrant.
			synchronized(self)
			{
				if (null != runnable) { runnable.run(); }
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	/**
	 * If necessary, connect billing services. Run runOnSuccess if
	 * connection is successful or if already connected. Otherwise,
	 * runs runOnFailure.
	 */
	synchronized void connect(final Runnable runOnSuccess, final Runnable runOnFailure)
	{
		// For synchronization.
		final AndroidCommerceGooglePlayApi self = this;

		if (AndroidLogConfig.ENABLED) { LogMessage("connect:"); }

		// If already connected, run the success runnable.
		if (m_bConnected)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("connect: already connected, success."); }
			checkedRun(runOnSuccess);
			return;
		}
		// Otherwise, if connecting, run the failure runnable.
		else if (m_bConnecting)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("connect: connection in progress, fail."); }
			checkedRun(runOnFailure);
			return;
		}

		// Otherwise, mark that we're connecting and start that now.
		try
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("connect: starting connection attempt."); }
			m_bConnecting = true;
			m_Client.startConnection(new BillingClientStateListener()
			{
				@Override
				public void onBillingSetupFinished(@BillingResponse int iResponse)
				{
					synchronized (self)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("connect: onBillingSetupFinished: " + String.valueOf(iResponse)); }

						// Track whether we were successful in connecting or not.
						if (BillingResponse.OK == iResponse)
						{
							m_bConnected = true;
						}

						// No longer connecting.
						m_bConnecting = false;

						// Dispatch as appropriate.
						if (m_bConnected)
						{
							if (AndroidLogConfig.ENABLED) { LogMessage("connect: running OnConnected Runnable."); }
							checkedRun(m_runOnConnected);
							if (AndroidLogConfig.ENABLED) { LogMessage("connect: running OnSuccess Runnable."); }
							checkedRun(runOnSuccess);
						}
						else
						{
							if (AndroidLogConfig.ENABLED) { LogMessage("connect: running OnFailure Runnable."); }
							checkedRun(runOnFailure);
						}
					}
				}

				@Override
				public void onBillingServiceDisconnected()
				{
					synchronized (self)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("connect: onBillingServiceDisconnected"); }

						// Tracking.
						m_bConnected = false;
					}
				}
			});
		}
		catch (Exception e)
		{
			e.printStackTrace();
			m_bConnecting = false;
			checkedRun(runOnFailure);
		}
	}

	/**
	 * Execute runOnSuccess or runOnFailure
	 * depending on whether we successfully connect to billing services or not.
	 */
	synchronized void run(final Runnable runOnSuccess, final Runnable runOnFailure)
	{
		connect(runOnSuccess, runOnFailure);
	}

	/**
	 * Convert a purchase blob into an encoding JSON object of receipt data for
	 * later receipt verification.
	 */
	static String getReceiptData(final Purchase purchase)
	{
		try
		{
			// Gather.
			final String sOriginalJson = purchase.getOriginalJson();
			final String sSignature = purchase.getSignature();

			// Encode.
			final JSONObject receiptData = new JSONObject();
			receiptData.putOpt("json", sOriginalJson);
			receiptData.putOpt("signature", sSignature);

			return receiptData.toString();
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return "";
		}
	}

	/** Convert billing codes into our custom response flags. */
	public static AndroidCommerceResult ToCommerceResult(@BillingResponse int iResult)
	{
		switch (iResult)
		{
		case BillingResponse.OK: return AndroidCommerceResult.kResultSuccess;
		case BillingResponse.USER_CANCELED: return AndroidCommerceResult.kResultCanceled;
		case BillingResponse.BILLING_UNAVAILABLE: return AndroidCommerceResult.kResultCantMakePayments;
		case BillingResponse.SERVICE_DISCONNECTED: return AndroidCommerceResult.kResultNetworkError;
		case BillingResponse.SERVICE_UNAVAILABLE: return AndroidCommerceResult.kResultNetworkError;
		case BillingResponse.FEATURE_NOT_SUPPORTED: return AndroidCommerceResult.kResultClientInvalid;
		case BillingResponse.ITEM_UNAVAILABLE: return AndroidCommerceResult.kResultProductUnavailable;
		case BillingResponse.DEVELOPER_ERROR: return AndroidCommerceResult.kResultPaymentInvalid;
		case BillingResponse.ERROR: return AndroidCommerceResult.kInternalPlatformError;
		// NOTE: This is the desired transformation in all our use cases - an already owned item
		// is equivalent to purchasing it for the first time. We want to consume it either way.
		case BillingResponse.ITEM_ALREADY_OWNED: return AndroidCommerceResult.kResultSuccess;
		case BillingResponse.ITEM_NOT_OWNED: return AndroidCommerceResult.kResultItemNotOwned;
		default:
			return AndroidCommerceResult.kResultFailure;
		}
	}

	// Utility, input Purchase to output PurchaseData.
	synchronized PurchaseData Convert(Purchase purchase)
	{
		final String sReceiptData = getReceiptData(purchase);
		final PurchaseData purchaseData = new PurchaseData(
			purchase.getSku(),
			sReceiptData,
			purchase.getOrderId(),
			purchase.getPurchaseToken());

		if (AndroidLogConfig.ENABLED) { LogMessage("Convert: '" + purchaseData.ProductID + "'."); }

		return purchaseData;
	}

	// Utility, input Purchase ArrayList to output PurchaseData array.
	synchronized ArrayList<PurchaseData> Convert(List<Purchase> purchases)
	{
		if (null == purchases) { purchases = new ArrayList<Purchase>(); } // Sanity.

		// Assemble list.
		final ArrayList<PurchaseData> outList = new ArrayList<PurchaseData>();
		for (Purchase purchase : purchases)
		{
			try
			{
				outList.add(Convert(purchase));
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
		}

		return outList;
	}

	/**
	 * Checks if subscriptions are supported for current client
	 * This method does not automatically retry for RESULT_SERVICE_DISCONNECTED.
	 */
	public boolean AreSubscriptionsSupported()
	{
		int responseCode = m_Client.isFeatureSupported(FeatureType.SUBSCRIPTIONS);
		if (responseCode != BillingResponse.OK)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("AreSubscriptionsSupported: ResponseCode: '" + responseCode + "'"); }
		}
		return responseCode == BillingResponse.OK;
	}

	// Gets a Purchase for the given Product Id with a synchronous query
	synchronized Purchase GetPurchase(final String sProductID, final String sSkuType)
	{
		// Query and assign result if possible.
		final Purchase.PurchasesResult result = m_Client.queryPurchases(sSkuType);
		final @BillingResponse int iResponseCode = result.getResponseCode();
		if (BillingResponse.OK == iResponseCode)
		{
			final List<Purchase> purchases = result.getPurchasesList();
			if (null != purchases)
			{
				for (Purchase purchase : purchases)
				{
					if (sProductID.equals(purchase.getSku()))
					{
						return purchase;
					}
				}
			}
		}
		return null;
	}

	// Gets a Purchase for the given Product Id with a synchronous query
	synchronized Purchase GetPurchase(final String sProductID)
	{
		// Try to get the Purchase from the INAPP (i.e. consumable) inventory
		Purchase purchase = GetPurchase(sProductID, SkuType.INAPP);
		if (null != purchase)
		{
			return purchase;
		}
		// Try to get the Purchase from the SUBS (i.e. subscription) inventory
		purchase = GetPurchase(sProductID, SkuType.SUBS);
		return purchase;
	}

	/**
	 * Private helper method for RefreshProductInfo
	 * Performs a querySkuDetailsAsync query for a given SkuType for the given Sku list
	 * If given a Runnable, will run it when complete. Otherwise, it will inform the Listener
	 */
	protected void QuerySkus(final AndroidCommerceGooglePlayApi self, final @SkuType String billingType, List<String> skusList, final Runnable executeWhenFinished)
	{
		// Listener to handle the response of the querySkuDetailsAsync call below.
		final SkuDetailsResponseListener listener = new SkuDetailsResponseListener()
		{
			@Override
			public void onSkuDetailsResponse(@BillingResponse int iResponse, List<SkuDetails> skuDetails)
			{
				synchronized (self)
				{
					final boolean bSuccess = (BillingResponse.OK == iResponse);
					try
					{
						if (null == skuDetails) { skuDetails = new ArrayList<SkuDetails>(); } // Sanity.

						if (AndroidLogConfig.ENABLED) { LogMessage("RefreshProductInfo (" + billingType + "): onSkuDetailsResponse: " + String.valueOf(iResponse) + " for " + String.valueOf(skuDetails.size()) + " results."); }

						for (SkuDetails details : skuDetails)
						{
							// Determine type
							// Currently not used by CommerceManager as it determines the type from
							// microtransactions.json. Ideally it should come from the first-party
							// commerce system, but not all first parties provide this information.
							AndroidCommerceProductType eType = AndroidCommerceProductType.kProductTypeConsumable;
							String subPeriod = details.getSubscriptionPeriod();
							if (null != subPeriod && subPeriod.length() > 0)
							{
								eType = AndroidCommerceProductType.kProductTypeSubscription;
							}

							// TODO: On Android we do not send any subscription details. These details are
							// not available from every first party system, so the CommerceManager gets them
							// from data we maintain in microtransactions.json

							// Add the ProductInfo to the List of received Products
							if (bSuccess)
							{
								m_productInfos.add(new ProductInfo(
									eType,
									details.getSku(),
									details.getTitle(),
									details.getDescription(),
									details.getPrice(),
									details.getPriceCurrencyCode(),
									details.getPriceAmountMicros()));
							}
						}
					}
					catch (Exception e)
					{
						e.printStackTrace();
					}

					// If we need to query another type of Product, run that Runnable.
					if (bSuccess && executeWhenFinished != null)
					{
						executeWhenFinished.run();
					}
					// If there is nothing left to run, inform the Listener
					else
					{
						ProductInfo[] outArray = new ProductInfo[m_productInfos.size()];
						outArray = m_productInfos.toArray(outArray);
						m_Listener.OnRefreshProductInfoComplete(bSuccess, outArray);
						m_productInfos = new ArrayList<ProductInfo>();
					}
				}
			}
		};

		// Query the SKU details for the given type
		final SkuDetailsParams params = SkuDetailsParams.newBuilder()
			.setSkusList(skusList)
			.setType(billingType)
			.build();
		m_Client.querySkuDetailsAsync(params, listener);
	}

	// PurchasesUpdatedListener implementations.
	/**
	 * Listener interface for purchase updates which happen when, for example, the user buys
	 * something within the app or by initiating a purchase from Google Play Store.
	 *
	 * Unknowns:
	 *   Does this get called for renewals? (e.g. GPA.1234-5678-9012-34567..0)
	 *
	 * In practice the following behavior is observed:
	 * If a Subscription product is purchased, this method is invoked however purchases is empty.
	 *   This seems like a bug. AndroidCommerceManager (java) handles this case by asking the
	 *   AndroidCommerceAPI for the Purchase via GetPurchaseData. The Purchase, in practice, is
	 *   in the user's Inventory at this time.
	 * If a Consumable product is purchased, this method is invoked with the expected Purchase in
	 *   in the purchases parameter.
	 * If there are existing Subscription Purchases in the user's Inventory, they are not present
	 *   in the purchases parameter.
	 */
	public synchronized void onPurchasesUpdated(int iResult, List<Purchase> purchases)
	{
		// Convert
		final AndroidCommerceResult eResult = ToCommerceResult(iResult);
		final ArrayList<PurchaseData> outList = Convert(purchases);

		// Logging
		if (m_bDebugEnabled)
		{
			try
			{
				DateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss:SSS Z", Locale.getDefault());
				if (AndroidLogConfig.ENABLED) { LogMessage("onPurchasesUpdated: eResult: '" + String.valueOf(eResult) + "'"); }
				if (AndroidLogConfig.ENABLED) { LogMessage("onPurchasesUpdated: " + String.valueOf(null != purchases ? purchases.size() : 0) + " Purchases received."); }
				if (null != purchases)
				{
					for (Purchase p : purchases)
					{
						Date d = new Date(p.getPurchaseTime());
						if (AndroidLogConfig.ENABLED) { LogMessage("onPurchasesUpdated: Purchase: Product: " + p.getSku() + ", Purchase Time: " + sdf.format(d)); }
					}
				}
				if (AndroidLogConfig.ENABLED) { LogMessage("onPurchasesUpdated: " + String.valueOf(outList.size()) + " Output PurchaseData."); }
				for (PurchaseData p : outList)
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("onPurchasesUpdated: Output PurchaseData: Product: " + p.ProductID + ", Transaction ID: " + p.TransactionID); }
				}
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
		}

		// Dispatch
		try
		{
			m_Listener.OnPurchaseComplete(eResult, outList);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}
	// /PurchasesUpdatedListener implementations.

	/**
	 * Runnable for running when connecting to the BillingClient successfully.
	 * Queries Subscription Purchases and reports them to the Listener.
	 */
	protected Runnable m_runOnConnected = new Runnable()
	{
		@Override
		public void run()
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("Running OnConnected Operations."); }
			checkedRun(m_runQuerySubscriptionPurchases);
		}
	};

	/**
	 * Runnable for Querying Subscription Purchases
	 * Reports Subscription Inventory to the Listener in all cases.
	 */
	protected Runnable m_runQuerySubscriptionPurchases = new Runnable()
	{
		@Override
		public void run()
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("<Query SUBS>: Running Query Subscriptions."); }
			PurchaseData[] outArray = null;
			try
			{
				ArrayList<Purchase> purchasesSubs = new ArrayList<Purchase>();
				// If Subscriptions are supported, query them and add their results to the above result.
				if (AreSubscriptionsSupported())
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("<Query SUBS>: querying existing SUBS Sku inventory."); }
					Purchase.PurchasesResult subscriptionResult = m_Client.queryPurchases(SkuType.SUBS);
					final List<Purchase> subscriptionPurchases = subscriptionResult.getPurchasesList();
					final @BillingResponse int iSubscriptionResponseCode = subscriptionResult.getResponseCode();
					if (BillingResponse.OK == iSubscriptionResponseCode)
					{
						if (null != subscriptionPurchases)
						{
							if (AndroidLogConfig.ENABLED)
							{
								LogMessage("<Query SUBS>: SUBS query successful, "
									+ String.valueOf(subscriptionPurchases.size()) + " purchases in inventory.");
							}

							// Record purchases
							purchasesSubs.addAll(subscriptionPurchases);

							if (AndroidLogConfig.ENABLED)
							{
								LogMessage("<Query SUBS>: " + String.valueOf(purchasesSubs.size())
									+ " SUBS purchases in inventory.");
							}
						}
						else
						{
							if (AndroidLogConfig.ENABLED)
							{
								LogMessage("<Query SUBS>: SUBS query successful, but purchases null, not using result.");
							}
						}
					}
					else
					{
						if (AndroidLogConfig.ENABLED)
						{
							LogMessage("<Query SUBS>: SUBS query failed with code "
								+ String.valueOf(iSubscriptionResponseCode));
						}
					}
				}
				else
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("<Query SUBS>: Subscriptions not supported - skipping querying for SUBS Sku inventory."); }
				}

				final ArrayList<PurchaseData> outList = Convert(purchasesSubs);
				outArray = new PurchaseData[outList.size()];
				outArray = outList.toArray(outArray);
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
			if (AndroidLogConfig.ENABLED) { LogMessage("<Query SUBS>: Reporting to Listener."); }
			m_Listener.OnInventoryUpdated(outArray);
		}
	};

	/**
	 * Runnable for Querying In App Consumable Purchases.
	 * If Consumable purchases exist, the Listener will be informed via
	 * OnReceivedCompletedConsumableTransactions.
	 */
	private Runnable m_runQueryInAppPurchases = new Runnable()
	{
		@Override
		public void run()
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("<Query INAPP>: Running Query In-App Purchases."); }
			try
			{
				ArrayList<Purchase> purchasesInApp = new ArrayList<Purchase>();

				// Process INAPP responses
				if (AndroidLogConfig.ENABLED) { LogMessage("<Query INAPP>: querying existing INAPP Sku inventory."); }
				Purchase.PurchasesResult inAppResult = m_Client.queryPurchases(SkuType.INAPP);
				final @BillingResponse int iResponseCode = inAppResult.getResponseCode();
				if (BillingResponse.OK == iResponseCode)
				{
					final List<Purchase> inappPurchases = inAppResult.getPurchasesList();
					if (null != inappPurchases)
					{
						if (AndroidLogConfig.ENABLED)
						{
							LogMessage("<Query INAPP>: INAPP query successful, "
								+ String.valueOf(inappPurchases.size()) + " purchases in inventory.");
						}

						// Record purchases
						purchasesInApp.addAll(inappPurchases);
					}
					else
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("<Query INAPP>: INAPP query successful, but purchases null, not using result."); }
					}
				}
				else
				{
					if (AndroidLogConfig.ENABLED)
					{
						LogMessage("<Query INAPP>: INAPP query failed with code "
							+ String.valueOf(iResponseCode));
					}
				}

				if(purchasesInApp.size() > 0)
				{
					final ArrayList<PurchaseData> outList = Convert(purchasesInApp);
					PurchaseData[] outArray = new PurchaseData[outList.size()];
					outArray = outList.toArray(outArray);
					m_Listener.OnReceivedCompletedConsumableTransactions(outArray);
				}
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
		}
	};

	public AndroidCommerceGooglePlayApi(final Activity activity, final Listener listener, final boolean bDebugEnabled)
	{
		m_Activity = activity;
		m_Listener = listener;
		m_bDebugEnabled = bDebugEnabled;
		m_Client = BillingClient.newBuilder(m_Activity).setListener(this).build();

		// Attempt the initial connection to the BillingClient
		// When connecting successfully, the SUBS purchases inventory is queried automatically.
		// If successful, query the INAPP purchases inventory and resume those pending purchases.
		// If the connection fails, report the empty inventory to the listener.
		run(
			// Successful case
			m_runQueryInAppPurchases,
			// Failure case
			new Runnable()
			{
				@Override
				public void run()
				{
					if (AndroidLogConfig.ENABLED) { LogMessage("Failure Connecting at Startup. Reporting Empty Inventory to Listener."); }
					m_Listener.OnInventoryUpdated(null);
				}
			}
		);
	}

	public synchronized void Dispose()
	{
		try
		{
			if (null != m_Client && m_Client.isReady())
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("Dispose:"); }
				m_Client.endConnection();
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	// AndroidCommerceApi implementations.
	@Override
	public synchronized void AcknowledgeItem(final String sProductID, final String sTransactionID)
	{
		// TOOD: In Google Play Billing version 2.0.0, implement acknowledgement.
		// Currently, there is no way to acknowledge a subscription.
		m_Listener.OnAcknowledgeComplete(AndroidCommerceResult.kResultSuccess, sProductID);
	}

	@Override
	public synchronized void ConsumeItem(final String sProductID, final String sTransactionID)
	{
		// For synchronization.
		final AndroidCommerceGooglePlayApi self = this;

		// Early out.
		if (null == m_Client)
		{
			try
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("ConsumeItem: failure, no client."); }

				// Error code to indicate permanent failure.
				m_Listener.OnConsumeComplete(AndroidCommerceResult.kResultCantMakePayments, sProductID);
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}

			return;
		}

		final ConsumeResponseListener listener = new ConsumeResponseListener()
		{
			@Override
			public void onConsumeResponse(@BillingResponse int iResponse, String sPurchaseToken)
			{
				synchronized (self)
				{
					try
					{
						if (null == sPurchaseToken) { sPurchaseToken = ""; } // Sanity.

						if (AndroidLogConfig.ENABLED)
						{
							LogMessage("ConsumeItem: onConsumeResponse: " + String.valueOf(iResponse) + " for product id '" + sProductID +
								" and purchase token '" + sPurchaseToken + "'.");
						}

						// Convert and report.
						final AndroidCommerceResult eResult = ToCommerceResult(iResponse);
						m_Listener.OnConsumeComplete(eResult, sProductID);
					}
					catch (Exception e)
					{
						e.printStackTrace();
					}
				}
			}
		};

		run(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized (self)
				{
					// Early out.
					if (null == m_Client)
					{
						try
						{
							if (AndroidLogConfig.ENABLED) { LogMessage("ConsumeItem: client set to null during run."); }

							// Error code to indicate permanent failure.
							m_Listener.OnConsumeComplete(AndroidCommerceResult.kResultCantMakePayments, sProductID);
						}
						catch (Exception e)
						{
							e.printStackTrace();
						}
						return;
					}

					try
					{
						// Get the purchase data for the given item ID.
						final Purchase purchase = GetPurchase(sProductID, SkuType.INAPP);

						// Can't continue if no data.
						if (null == purchase)
						{
							if (AndroidLogConfig.ENABLED) { LogMessage("ConsumeItem: no purchase data for product id '" + sProductID + "', cannot consume!"); }

							// Error code to indicate permanent failure.
							m_Listener.OnConsumeComplete(AndroidCommerceResult.kResultItemNotOwned, sProductID);
						}
						// Consume.
						else
						{
							m_Client.consumeAsync(purchase.getPurchaseToken(), listener);
						}
					}
					catch (Exception e)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("ConsumeItem: exception from consumeAsync."); }

						e.printStackTrace();

						try
						{
							// Unknown whether failure was temporary or not,
							// so assume permanent.
							m_Listener.OnConsumeComplete(AndroidCommerceResult.PlatformSpecificError3, sProductID);
						}
						catch (Exception ie)
						{
							ie.printStackTrace();
						}
						return;
					}
				}
			}
		}, new Runnable()
		{
			@Override
			public void run()
			{
				synchronized (self)
				{
					try
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("ConsumeItem: failure to run consumeAsync runnable."); }

						// Unknown whether failure was temporary or not,
							// so assume permanent.
						m_Listener.OnConsumeComplete(AndroidCommerceResult.PlatformSpecificError4, sProductID);
					}
					catch (Exception e)
					{
						e.printStackTrace();
					}
					return;
				}
			}
		});
	}

	@Override
	public synchronized void PurchaseItem(final String sProductID)
	{
		// For synchronization.
		final AndroidCommerceGooglePlayApi self = this;

		final Runnable runOnFailure = new Runnable()
		{
			@Override public void run()
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("PurchaseItem: failure, unknown case."); }
				final PurchaseData data = new PurchaseData(sProductID, "", "", "");
				final ArrayList<PurchaseData> list = new ArrayList<PurchaseData>();
				list.add(data);

				m_Listener.OnPurchaseComplete(AndroidCommerceResult.PlatformSpecificError2, list);
			}
		};

		// Early out.
		if (null == m_Client)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("PurchaseItem: failure, null client."); }
			checkedRun(runOnFailure);
			return;
		}

		run(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized (self)
				{
					// Early out.
					if (null == m_Client)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("PurchaseItem: failure, null client during run."); }
						checkedRun(runOnFailure);
						return;
					}

					try
					{
						final BillingFlowParams params = BillingFlowParams.newBuilder().setSku(sProductID).setType(SkuType.INAPP).build();
						m_Client.launchBillingFlow(m_Activity, params);
					}
					catch (Exception e)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("PurchaseItem: exception from launch billing flow."); }

						e.printStackTrace();
					}
				}
			}
		}, runOnFailure);
	}

	@Override
	public synchronized void RefreshProductInfo(final String[] skus)
	{
		// For synchronization.
		final AndroidCommerceGooglePlayApi self = this;
		m_productInfos = new ArrayList<ProductInfo>();

		final Runnable runOnFailure = new Runnable()
		{
			@Override public void run()
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("RefreshProductInfo: failure, unknown case."); }
				m_Listener.OnRefreshProductInfoComplete(false, new ProductInfo[0]);
			}
		};

		// Early out.
		if (null == m_Client)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("RefreshProductInfo: failure, null client."); }
			checkedRun(runOnFailure);
			return;
		}

		run(new Runnable()
		{
			@Override
			public void run()
			{
				synchronized (self)
				{
					// Early out.
					if (null == m_Client)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("RefreshProductInfo: failure, null client during run."); }
						checkedRun(runOnFailure);
						return;
					}

					try
					{
						// The BillingClient can only query one type of product at a time.
						// To query multiple types we need to query multiple times. This is
						// achieved by chaining together Runnables with each Runnable making
						// a query for one type and appending the results to m_productInfos.
						// When the last one completes, we inform the Listener.
						// This is complex, but follows Google's example code from their
						// TrivialDrive_v2 sample.
						final List<String> skuList = Arrays.asList(skus);
						QuerySkus(self, SkuType.INAPP, skuList, new Runnable()
							{
								@Override
								public void run()
								{
									QuerySkus(self, SkuType.SUBS, skuList, null);
								}
							});
					}
					catch (Exception e)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("RefreshProductInfo: failure, querySkuDetailsAsync call."); }
						e.printStackTrace();
						checkedRun(runOnFailure);
					}
				}
			}
		}, runOnFailure);
	}

	@Override
	public synchronized PurchaseData GetPurchaseData(final String sProductID)
	{
		// Early out case.
		if (null == sProductID)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("GetPurchaseData: called with null sProductID, returning null."); }
			return null;
		}

		try
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("GetPurchaseData:"); }

			// Get the purchase.
			final Purchase purchase = GetPurchase(sProductID);
			if (null == purchase)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("GetPurchaseData: failed to find purchase or purchases inventory was null or empty."); }
			}
			else
			{
				return Convert(purchase);
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}

		return null;
	}
	// /AndroidCommerceApi implementations.
}
