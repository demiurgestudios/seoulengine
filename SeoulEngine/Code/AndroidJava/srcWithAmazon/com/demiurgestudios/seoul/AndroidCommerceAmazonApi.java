package com.demiurgestudios.seoul;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import com.amazon.device.iap.PurchasingListener;
import com.amazon.device.iap.PurchasingService;
import com.amazon.device.iap.model.FulfillmentResult;
import com.amazon.device.iap.model.Product;
import com.amazon.device.iap.model.ProductDataResponse;
import com.amazon.device.iap.model.PurchaseResponse;
import com.amazon.device.iap.model.PurchaseUpdatesResponse;
import com.amazon.device.iap.model.Receipt;
import com.amazon.device.iap.model.RequestId;
import com.amazon.device.iap.model.UserData;
import com.amazon.device.iap.model.UserDataResponse;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Implementation of Android commerce API for Amazon purchasing.
 */
public final class AndroidCommerceAmazonApi extends AndroidCommerceApi implements PurchasingListener
{
	/** Debug Only Locale Management */
	private static final HashMap<String, Locale> s_tIso2CountryToLocale = new HashMap<> ();
	static
	{
		Locale[] locales = Locale.getAvailableLocales();
		for(Locale l : locales)
			s_tIso2CountryToLocale.put(l.getCountry(), l);
	}

	final Listener m_Listener;
	final boolean m_bDebugEnabled;

	/** the token refers to an Amazon user, but is not the user's name/ID */
	protected String m_sAmazonUserToken = null;
	/** market is the ISO 3166-1-alpha-2 region, like "US" */
	protected String m_sAmazonMarketName = null;
	/** ISO3 4217 CurrencyCode */
	protected String m_sAmazonUserCurrencyCode = null;

	/** Active Request Ids - All types */
	protected HashSet<RequestId> m_Requests = new HashSet<RequestId>();
	/** Receipts from paginated getPurchaseUpdates response. */
	protected ArrayList<Receipt> m_Receipts = new ArrayList<Receipt>();

	/** Test file for sandbox IAP. Used for correcting currency codes. */
	protected JSONObject m_SDKTesterJSON = null;

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
			Log.d(AndroidLogConfig.TAG, "[AndroidCommerceAmazonApi]: " + sMessage);
		}
	}

	// PurchasingObserver implementations.
	/**
	 * Handle a response from PurchasingService.getProductData().
	 *
	 * @param response the response content
	 */
	@Override
	public void onProductDataResponse(ProductDataResponse response)
	{
		if (AndroidLogConfig.ENABLED)
		{
			LogMessage("onProductDataResponse: requestId (" + response.getRequestId()
				+ ") RequestStatus: "
				+ response.getRequestStatus()
				+ ")");
		}

		if (!ResolveRequest(response.getRequestId()))
		{
			return;
		}

		final ProductDataResponse.RequestStatus status = response.getRequestStatus();
		switch (status)
		{
			case SUCCESSFUL:
			{
				final ProductInfo[] productInfo = new ProductInfo[response.getProductData().size()];
				int iOut = 0;

				Iterator it = response.getProductData().entrySet().iterator();
				while (it.hasNext())
				{
					Map.Entry<String, Product> pair = (Map.Entry<String, Product>)it.next();
					String sku = pair.getKey();
					Product p = pair.getValue();

					String price = p.getPrice();

					// Sandbox builds get data from the SDKTester, which
					// is known to report only USD values, though formatted
					// for other currencies. Manually grab the right value and reformat it.
					if (!BuildConfig.BUILD_TYPE.equals("ship") && (m_SDKTesterJSON != null) && (m_sAmazonMarketName != null))
					{
						// If the marketplace ISO code is not known then we cannot look up
						// into the SDKTester map. Skip for now and update products on the next
						// call.
						if(m_sAmazonMarketName == null)
						{
							continue;
						}
						// If an overriding JSON file exists in a non-ship build, use it for currency data.
						try
						{
							// Get the numeric value for the right locale
							JSONObject item = m_SDKTesterJSON.getJSONObject(sku);
							JSONObject pricemap = item.getJSONObject("currencyPriceMap");
							double value = pricemap.getDouble(m_sAmazonMarketName);

							// Format that number based on the locale formatting rules.
							Locale locale = s_tIso2CountryToLocale.get(m_sAmazonMarketName);
							if(locale != null)
							{
								price = NumberFormat.getCurrencyInstance(locale).format(value);
							}
							else
							{
								price = NumberFormat.getCurrencyInstance().format(value);
							}

							if (AndroidLogConfig.ENABLED) { LogMessage("amazon.sdktester.json " + sku + " price " + price); }
						}
						catch (JSONException jsone)
						{
							if (AndroidLogConfig.ENABLED) { LogMessage("Missing or invalid amazon.sdktester.json file for sku " + sku + ": " + jsone.getMessage()); }
						}
					}

					long lPrice = TryParsePriceMicros(price);

					// Determine type
					// Currently not used by CommerceManager as it determines the type from
					// microtransactions.json. Ideally it should come from the first-party
					// commerce system, but not all first parties provide this information.
					AndroidCommerceProductType eType = AndroidCommerceProductType.kProductTypeConsumable;
					switch(p.getProductType())
					{
						case CONSUMABLE:		eType = AndroidCommerceProductType.kProductTypeConsumable;		break;
						case ENTITLED:			eType = AndroidCommerceProductType.kProductTypeDLC;				break;
						case SUBSCRIPTION:		eType = AndroidCommerceProductType.kProductTypeSubscription;	break;
					}

					// TODO: On Android we do not send any subscription details. These details are
					// not available from every first party system, so the CommerceManager gets them
					// from data we maintain in microtransactions.json.
					// Amazon does not have access to the subscription details.

					// Add a new ProductInfo entry for this product
					productInfo[iOut++] = new ProductInfo(
						eType,
						p.getSku(),
						p.getTitle(),
						p.getDescription(),
						price,
						m_sAmazonUserCurrencyCode,
						lPrice);
				}

				if (AndroidLogConfig.ENABLED)
				{
					for (String sUnavailableSku : response.getUnavailableSkus())
					{
						LogMessage("AmazonCommerce: onProductDataResponse returned unavailable SKU: " + sUnavailableSku);
					}
				}

				// Inform the listener of the products
				m_Listener.OnRefreshProductInfoComplete(true, productInfo);
				break;
			}

			case FAILED:
			case NOT_SUPPORTED:
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("onProductDataResponse failed, status code is " + status); }

				// Inform the listener of the failure
				m_Listener.OnRefreshProductInfoComplete(false, new ProductInfo[0]);
				break;
			}
		}
	}

	/**
	 * Handle a response from PurchasingService.getPurchaseUpdates().
	 *
	 * @param response the response content
	 */
	@Override
	public void onPurchaseUpdatesResponse(PurchaseUpdatesResponse response)
	{
		if (AndroidLogConfig.ENABLED)
		{
			LogMessage("onPurchaseUpdatesResponse: requestId (" + response.getRequestId()
				+ ") RequestStatus: "
				+ response.getRequestStatus()
				+ ")");
		}

		synchronized(this)
		{
			PurchaseData[] outInventory = null;
			try
			{
				// TODO: I am suspicious of this early-out. I can see us wanting to always accept the results
				// of a getPurchaseUpdates request.
				if (!ResolveRequest(response.getRequestId()))
				{
					return;
				}

				final PurchaseUpdatesResponse.RequestStatus status = response.getRequestStatus();
				switch (status)
				{
					case SUCCESSFUL:
					{
						UpdateUserData(response.getUserData());

						// Record Receipts
						m_Receipts.addAll(response.getReceipts());

						// This response is paginated and may have more receipts.
						if (response.hasMore())
						{
							// Keep requesting receipts until we flush them out.
							m_Requests.add(PurchasingService.getPurchaseUpdates(false));
							return;
						}

						// Finished receiving receipts. Report to listener.
						ArrayList<PurchaseData> outListConsumables = new ArrayList<PurchaseData>();
						ArrayList<PurchaseData> outListSubscriptions = new ArrayList<PurchaseData>();

						for(Receipt receipt : m_Receipts)
						{
							switch(receipt.getProductType())
							{
								case CONSUMABLE:	outListConsumables.add(Convert(receipt));	break;
								case SUBSCRIPTION:	outListSubscriptions.add(Convert(receipt));	break;
								default: break;
							}
						}

						// Gather inventory of subscriptions, even if there are not, to report to CommerceManager
						outInventory = new PurchaseData[outListSubscriptions.size()];
						outInventory = outListSubscriptions.toArray(outInventory);

						// TODO: If getPurchaseUpdates is called outside of startup flow, do not
						// call OnReceivedCompletedConsumableTransactions
						if (outListConsumables.size() > 0)
						{
							PurchaseData[] outArray = new PurchaseData[outListConsumables.size()];
							outArray = outListConsumables.toArray(outArray);
							m_Listener.OnReceivedCompletedConsumableTransactions(outArray);
						}

						m_Receipts.clear();
						break;
					}

					case FAILED:
					case NOT_SUPPORTED:
						if (AndroidLogConfig.ENABLED) { LogMessage("onPurchaseUpdatesResponse failed, status code is " + status); }
						break;
				}
			}
			catch(Exception e)
			{
				e.printStackTrace();
			}

			// Always inform listener of Inventory if this response if a request we issued and it does not need
			// to wait for another page of results from the paginated response.
			m_Listener.OnInventoryUpdated(outInventory);
		}
	}

	/**
	 * Handle a response from PurchasingService.purchase().
	 *
	 * @param response the response content
	 */
	@Override
	public void onPurchaseResponse(PurchaseResponse response)
	{
		if (AndroidLogConfig.ENABLED)
		{
			LogMessage("onPurchaseResponse: requestId (" + response.getRequestId()
				+ ") RequestStatus: "
				+ response.getRequestStatus()
				+ ")");
		}

		if (ResolveRequest(response.getRequestId()))
		{
			final PurchaseResponse.RequestStatus status = response.getRequestStatus();

			List<PurchaseData> purchaseData = new ArrayList<PurchaseData>();
			if (response.getReceipt() != null)
			{
				purchaseData.add(Convert(response.getReceipt()));
			}

			AndroidCommerceResult result;
			switch (status)
			{
				case SUCCESSFUL:
				{
					UpdateUserData(response.getUserData());

					// Inform listener of successful purchase.
					m_Listener.OnPurchaseComplete(AndroidCommerceResult.kResultSuccess, purchaseData);
					break;
				}
				case ALREADY_PURCHASED:
				{
					UpdateUserData(response.getUserData());

					// Inform listener of failure.
					m_Listener.OnPurchaseComplete(AndroidCommerceResult.kResultPaymentInvalid, purchaseData);
					break;
				}

				case INVALID_SKU:
				{
					UpdateUserData(response.getUserData());

					// Inform listener of failure.
					m_Listener.OnPurchaseComplete(AndroidCommerceResult.kResultProductUnavailable, purchaseData);
					break;
				}

				case FAILED:
				{
					// Inform listener of failure.
					m_Listener.OnPurchaseComplete(AndroidCommerceResult.kInternalPlatformError, purchaseData);
					break;
				}

				case NOT_SUPPORTED:
				{
					// Inform listener of failure.
					m_Listener.OnPurchaseComplete(AndroidCommerceResult.kResultClientInvalid, purchaseData);
					break;
				}
			}
		}
	}

	/**
	 * Handle a response from PurchasingService.getUserData().
	 *
	 * @param response the response content
	 */
	@Override
	public void onUserDataResponse(UserDataResponse response)
	{
		if (AndroidLogConfig.ENABLED)
		{
			LogMessage("onGetUserDataResponse: requestId (" + response.getRequestId()
				+ ") userIdRequestStatus: "
				+ response.getRequestStatus()
				+ ")");
		}

		if (ResolveRequest(response.getRequestId()))
		{
			final UserDataResponse.RequestStatus status = response.getRequestStatus();
			switch (status)
			{
			case SUCCESSFUL:
				UpdateUserData(response.getUserData());
				break;

			case FAILED:
			case NOT_SUPPORTED:
				if (AndroidLogConfig.ENABLED) { LogMessage("onUserDataResponse failed, status code is " + status); }
				break;
			}
		}
	}

	// /PurchasingObserver implementations.

	// AndroidCommerceApi implementations.

	public AndroidCommerceAmazonApi(final Activity activity, final Listener listener, final boolean bDebugEnabled)
	{
		m_Listener = listener;
		m_bDebugEnabled = bDebugEnabled;

		if (!BuildConfig.BUILD_TYPE.equals("ship"))
		{
			File testerfile = new File(Environment.getExternalStorageDirectory(), "amazon.sdktester.json");
			if (testerfile.exists())
			{
				// Sandbox builds get data from the SDKTester, which
				// is known to report only USD values, though formatted
				// for other currencies.
				//
				// https://forums.developer.amazon.com/questions/16890/testing-international-currency.html
				//
				// We'll parse out the localized currencies from the
				// amazon.sdktester.json files directly in order to allow
				// testers to see what the players would see.
				try
				{
					FileInputStream stream = new FileInputStream(testerfile);
					try
					{
						BufferedReader reader = new BufferedReader(new InputStreamReader(stream, "utf-8"));
						StringBuffer buffer = new StringBuffer((int) testerfile.length());
						char[] block = new char[512];
						for (int count = 0; count >= 0; count = reader.read(block, 0, block.length))
						{
							buffer.append(block, 0, count);
						}
						m_SDKTesterJSON = new JSONObject(buffer.toString());
					}
					catch (FileNotFoundException fnfe)
					{
						// Ignore - we SDKTester JSON file not found.
						m_SDKTesterJSON = null;
					}
					catch (UnsupportedEncodingException uce)
					{
						// Ignore - bad file.
						m_SDKTesterJSON = null;
					}
					catch (JSONException jsone)
					{
						// Ignore - bad file.
						m_SDKTesterJSON = null;
					}
					finally
					{
						// Always close the input stream.
						stream.close();
					}
				}
				catch (IOException ioe)
				{
					// Errors mean JSON should be null.
					m_SDKTesterJSON = null;
				}
			}
		}

		synchronized (this)
		{
			// Register the listener. This must be registered before other PurchasingService calls can be made.
			PurchasingService.registerListener(activity.getApplicationContext(), this);
			if (PurchasingService.IS_SANDBOX_MODE)
			{
				if (AndroidLogConfig.ENABLED) { LogMessage("Amazon Store Running in Sandbox Mode!"); }
			}

			// Retrieve the app-specific ID and marketplace for the user who is currently logged on.
			m_Requests.add(PurchasingService.getUserData());
			// Get all unfulfilled Consumable purchases, and all Subscription and Entitlement purchases.
			m_Receipts.clear();
			m_Requests.add(PurchasingService.getPurchaseUpdates(false));
		}
	}

	@Override
	public void Dispose()
	{
		// No-op
	}

	@Override
	public synchronized void AcknowledgeItem(final String sProductID, final String sTransactionID)
	{
		// TOOD: implement acknowledgement if supported
		m_Listener.OnAcknowledgeComplete(AndroidCommerceResult.kResultSuccess, sProductID);
	}

	@Override
	public void ConsumeItem(final String sProductID, final String sTransactionID)
	{
		final String sReceiptID = sTransactionID;

		// Fulfill with Amazon.
		if (AndroidLogConfig.ENABLED) { LogMessage("Fulfilling product " + sProductID + " (Receipt: " + sReceiptID + ") with Amazon."); }
		PurchasingService.notifyFulfillment(sReceiptID, FulfillmentResult.FULFILLED);

		// Notify listener
		m_Listener.OnConsumeComplete(AndroidCommerceResult.kResultSuccess, sProductID);
	}

	@Override
	public void PurchaseItem(final String sProductID)
	{
		synchronized (this)
		{
			// Initiate a request to purchase a product.
			m_Requests.add(PurchasingService.purchase(sProductID));
		}
	}

	@Override
	public void RefreshProductInfo(final String[] skus)
	{
		synchronized (this)
		{
			// Initiate a request for product data
			m_Requests.add(PurchasingService.getProductData(new HashSet<String>(Arrays.asList(skus))));
		}
	}

	@Override
	public PurchaseData GetPurchaseData(final String sProductID)
	{
		// TODO: The only way to determine the PurchaseData for a Product is with an asynchronous call to
		// getPurchaseUpdates, and searching the PurchaseUpdatesResponse receipts for one that matches.
		// This doesn't map cleanly to the AndroidCommerceApi.
		// This function is only needed in response to calling OnPurchaseComplete with products where none
		// are the AndroidCommerceManager's InProgress Product.
		// TODO: Further evaluate if this is needed. It looks like a flow that should not happen.
		return null;
	}

	// /AndroidCommerceApi implementations.

	/**
	 * Convert a purchase blob into an encoding JSON object of receipt data for
	 * later receipt verification.
	 */
	String getReceiptData(Receipt receipt)
	{
		String sReference;
		try
		{
			// Create a JSON object with the receipt data and the Amazon
			// store account ID.
			JSONObject json = receipt.toJSON();
			json.put("amazon_id", (m_sAmazonUserToken != null) ? m_sAmazonUserToken : "*UNKNOWN*");
			sReference = json.toString();
		}
		catch (Exception e)
		{
			// On JSON error, just return the receipt as a string.
			// It will be missing the user information.
			sReference = receipt.toString();
		}

		return sReference;
	}

	// Utility, input Receipt to output PurchaseData.
	PurchaseData Convert(Receipt receipt)
	{
		if (receipt == null)
		{
			return null;
		}

		final String sReceiptData = getReceiptData(receipt);
		final PurchaseData purchaseData = new PurchaseData(
			receipt.getSku(),
			sReceiptData,
			receipt.getReceiptId(),
			new String());

		if (AndroidLogConfig.ENABLED) { LogMessage("Convert: '" + purchaseData.ProductID + "'."); }

		return purchaseData;
	}

	// Utility, input Receipts to output PurchaseData array.
	ArrayList<PurchaseData> Convert(ArrayList<Receipt> receipts)
	{
		if (null == receipts) { receipts = new ArrayList<Receipt>(); } // Sanity.

		// Assemble list.
		final ArrayList<PurchaseData> outList = new ArrayList<PurchaseData>();
		for (Receipt r : receipts)
		{
			try
			{
				outList.add(Convert(r));
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
		}

		return outList;
	}

	/**
	 * Close out an active request. Resolve BEFORE processing a response. If
	 * we return false, then the request was not active to begin with, and
	 * you should ignore the response.
	 *
	 * @param id a request ID
	 * @return true if we resolved the request (and removed it from the queue)
	 */
	protected synchronized boolean ResolveRequest(RequestId id)
	{
		return m_Requests.remove(id);
	}

	/**
	 * Update user information for this particular Amazon user.
	 *
	 * @param payload user data, normally from a response
	 */
	protected synchronized void UpdateUserData(UserData payload)
	{
		String sOldMarketName = m_sAmazonMarketName;
		m_sAmazonUserToken = payload.getUserId();
		m_sAmazonMarketName = payload.getMarketplace();

		// Try to convert the ISO-2 country code to an ISO-3 currency code for our purchase analytics.
		m_sAmazonUserCurrencyCode = AndroidIsoCodeMap.s_tIso2CountryToIso3CurrencyCodes.get(m_sAmazonMarketName);
		if (m_sAmazonUserCurrencyCode == null)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("Failure getting ISO-3 currency code for country " + m_sAmazonMarketName + ". Defaulting to USD."); }
			m_sAmazonUserCurrencyCode = "USD";
		}

		if (AndroidLogConfig.ENABLED) { LogMessage(new StringBuffer().append("Amazon user id (").append(m_sAmazonUserToken).append("), marketplace (").append(m_sAmazonMarketName).append(')').toString()); }
	}

	/**
	 * Attempt to parse the price string from the Product price string.
	 * The price string is formatted according to the Amazon User's Marketplace region.
	 *
	 * @return The formatted price string sPrice as a long representation.
	 */
	private long TryParsePriceMicros(String sPrice)
	{
		// Remove any whitespace, including non-breaking space characters.
		sPrice = sPrice.replaceAll(" \t\u00A0\u1680\u180e\u2000-\u200a\u202f\u205f\u3000","");

		double dPrice = 0.0;
		StringBuilder sb = new StringBuilder();

		// Treat the last period or comma as the decimal marker
		// This will fail if the string has no decimal marker and only a thousands marker but in
		// practice we do not see values like that.
		char decimal = '.';
		for (int i = sPrice.length() - 1; i >= 0; i--)
		{
			char c = sPrice.charAt(i);
			if ('.' == c || ',' == c)
			{
				decimal = c;
				break;
			}
		}

		for (int i = 0; i < sPrice.length(); ++i)
		{
			char c = sPrice.charAt(i);
			// Only append number characters and the decimal point '.'
			if (c >= '0' && c <= '9')
				sb.append(c);
			else if (c == decimal)
				sb.append('.');
		}

		try
		{
			dPrice = Double.parseDouble(sb.toString());
		}
		catch (Exception e)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("TryParsePriceMicros: Failed to use parseDouble to parse price " + sPrice + ". Exception was: " + e.getMessage()); }
		}

		long lPrice = (long) (dPrice * 1000000.0);
		if (AndroidLogConfig.ENABLED) { LogMessage("TryParsePriceMicros: micros " + lPrice); }
		return lPrice;
	}
}
