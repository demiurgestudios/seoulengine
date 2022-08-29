package com.demiurgestudios.seoul;

import android.app.Activity;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

import com.samsung.android.sdk.iap.lib.helper.IapHelper;
import com.samsung.android.sdk.iap.lib.helper.HelperDefine;
import com.samsung.android.sdk.iap.lib.listener.OnConsumePurchasedItemsListener;
import com.samsung.android.sdk.iap.lib.listener.OnGetOwnedListListener;
import com.samsung.android.sdk.iap.lib.listener.OnGetProductsDetailsListener;
import com.samsung.android.sdk.iap.lib.listener.OnPaymentListener;
import com.samsung.android.sdk.iap.lib.vo.ConsumeVo;
import com.samsung.android.sdk.iap.lib.vo.ErrorVo;
import com.samsung.android.sdk.iap.lib.vo.OwnedProductVo;
import com.samsung.android.sdk.iap.lib.vo.ProductVo;
import com.samsung.android.sdk.iap.lib.vo.PurchaseVo;

/**
 * Implementation of Android commerce API for Samsung purchasing.
 */
public final class AndroidCommerceSamsungApi extends AndroidCommerceApi implements OnPaymentListener, OnConsumePurchasedItemsListener, OnGetProductsDetailsListener, OnGetOwnedListListener
{
	final Listener m_Listener;
	final boolean m_bDebugEnabled;

	private IapHelper m_IapHelper = null;

	// The pass through parameter is userdata that is returned
	// to the OnPaymentListener.  We could use this, for example,
	// to identify a specific item to deliver for a microtransaction
	// id that is beung reused.
	private String mPassThroughParam = "TEMP_PASS_THROUGH";

	private static final String CONSUME_STATUS_SUCCESS  = "0";  //success

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
			Log.d(AndroidLogConfig.TAG, "[AndroidCommerceSamsungApi]: " + sMessage);
		}
	}

	public AndroidCommerceSamsungApi(final Activity activity, final Listener listener, final boolean bDebugEnabled)
	{
		synchronized (this)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("Created AndroidCommerceSamsungApi"); }
			m_Listener = listener;
			m_bDebugEnabled = bDebugEnabled;
			m_IapHelper = IapHelper.getInstance(activity);
			m_IapHelper.setOperationMode(bDebugEnabled ? HelperDefine.OperationMode.OPERATION_MODE_TEST : HelperDefine.OperationMode.OPERATION_MODE_PRODUCTION);

			// Query the INAPP purchases inventory and resume any pending purchases.
			m_IapHelper.getOwnedList(IapHelper.PRODUCT_TYPE_ALL, this);
		}
	}

	// AndroidCommerceApi implementations.

	@Override
	public synchronized void Dispose()
	{
		m_IapHelper.dispose();
	}

	@Override
	public synchronized void AcknowledgeItem(final String sProductID, final String sTransactionID)
	{
		// TODO: implement acknowledgement if supported
		if (AndroidLogConfig.ENABLED) { LogMessage("AcknowledgeItem " + sProductID + " (TransactionID: " + sTransactionID + ") with Samsung."); }

		// Notify listener
		m_Listener.OnAcknowledgeComplete(AndroidCommerceResult.kResultSuccess, sProductID);
	}

	@Override
	public synchronized void ConsumeItem(final String sProductID, final String sTransactionID)
	{
		// Fulfill with Samsung.
		if (AndroidLogConfig.ENABLED) { LogMessage("ConsumeItem " + sProductID + " (TransactionID: " + sTransactionID + ") with Samsung."); }
		m_IapHelper.consumePurchasedItems(sTransactionID, this);

		// Notify the listener immediately.  If the consumption fails, this will be handled
		// the next time we call getOwnedList.  We are protected from double-awarding
		// the goods by the game code that handles redemption.
		m_Listener.OnConsumeComplete(AndroidCommerceResult.kResultSuccess, sProductID);
	}

	@Override
	public synchronized void PurchaseItem(final String sProductID)
	{
		// Purchase with Samsung
		if (AndroidLogConfig.ENABLED) { LogMessage("PurchaseItem " + sProductID + " with Samsung."); }
		m_IapHelper.startPayment(sProductID, mPassThroughParam, true, this);
	}

	@Override
	public synchronized void RefreshProductInfo(final String[] skus)
	{
		// RefreshProductInfo with Samsung
		if (AndroidLogConfig.ENABLED) { LogMessage("RefreshProductInfo with Samsung."); }

		boolean bFirstIteration = true;
		StringBuilder skus_csv = new StringBuilder();
		for (String sku : skus)
		{
			if (!bFirstIteration)
			{
				skus_csv.append(",");
			}
			skus_csv.append(sku);
			bFirstIteration = false;
		}

		m_IapHelper.getProductsDetails(skus_csv.toString(), this);
	}

	@Override
	public PurchaseData GetPurchaseData(final String sProductID)
	{
		// NLS - The only way to determine the PurchaseData for a Product is with an asynchronous call to
		// IAPHelper.getOwnedList, and searching the OnGetOwnedListener response for one that matches.
		// This doesn't map cleanly to the AndroidCommerceApi.
		// This function is only needed in response to calling OnPurchaseComplete with products where none
		// are the AndroidCommerceManager's InProgress Product.
		// TODO: Further evaluate if this is needed. It looks like a flow that should not happen.
		return null;
	}

	// OnGetOwnedListListener implementations

	@Override
	public void onGetOwnedProducts(ErrorVo errorVo, ArrayList<OwnedProductVo> ownedList )
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("onGetOwnedProducts with Samsung"); }
		try
		{
			ArrayList<PurchaseData> outList = new ArrayList<PurchaseData>();
			if (errorVo != null && errorVo.getErrorCode() == IapHelper.IAP_ERROR_NONE)
			{
				if (ownedList != null)
				{
					for (int i = 0; i < ownedList.size(); i++) {
						OwnedProductVo ownedProductVo = ownedList.get(i);
						if (AndroidLogConfig.ENABLED) { LogMessage("ownedProductVo: " + ownedProductVo.getJsonString()); }
						outList.add(ConvertOwnedProduct(ownedProductVo));
					}
				}
			}
			if(outList.size() > 0)
			{
				PurchaseData[] outArray = new PurchaseData[outList.size()];
				outArray = outList.toArray(outArray);
				m_Listener.OnReceivedCompletedConsumableTransactions(outArray);
			}
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
	}

	// OnPaymentListener implementations

	@Override
	public void onPayment(ErrorVo errorVo, PurchaseVo purchaseVo)
	{
		if (AndroidLogConfig.ENABLED) { LogMessage("onPayment with Samsung"); }
		ArrayList<PurchaseData> outList = new ArrayList<PurchaseData>();
		AndroidCommerceResult eResult = AndroidCommerceResult.kResultFailure;
		try
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("errorVo " + errorVo.dump()); }
			final String purchaseVoStr = purchaseVo == null ? "<NULL>" : purchaseVo.getJsonString();
			if (AndroidLogConfig.ENABLED) { LogMessage("purchaseVo: " + purchaseVoStr); }
			eResult = ToCommerceResult(errorVo);

			if (purchaseVo != null)
			{
				outList.add(ConvertPurchase(purchaseVo));
			}

		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
		finally
		{
			// Notify listener
			m_Listener.OnPurchaseComplete(eResult, outList);
		}
	}

	// OnConsumePurchasedItemsListener implementations

	@Override
	public void onConsumePurchasedItems(ErrorVo errorVo, ArrayList<ConsumeVo> consumeList)
	{
		// Note: We notified the listener already (at the same time we asked Samsung to
		// execute the consumption.  If the consumption fails, this will be handled
		// the next time we call getOwnedList.  We are protected from double-awarding
		// the goods by the game code.
		if (AndroidLogConfig.ENABLED) { LogMessage("onConsumePurchasedItems with Samsung, errorVo " + errorVo.dump()); }

		if (m_bDebugEnabled) // log out the list of consumed iaps
		{
			try {
				if (consumeList != null)
				{
					for (ConsumeVo consumeVo : consumeList)
					{
						if (AndroidLogConfig.ENABLED) { LogMessage("consumeVo: " + consumeVo.getJsonString()); }
					}
				}
			}catch (Exception e)
			{
				e.printStackTrace();
			}
		}
	}

	// OnGetProductsDetailsListener implementations
	@Override
	public void onGetProducts(ErrorVo errorVo, ArrayList<ProductVo> productList )
	{
		boolean bSuccess = false;
		ProductInfo[] outArray = new ProductInfo[0];
		try
		{
			List<ProductInfo> productInfos = new ArrayList<ProductInfo>();
			if (AndroidLogConfig.ENABLED) { LogMessage("onGetProducts with Samsung, errorVo.getErrorCode() : " + errorVo.dump()); }
			if (errorVo.getErrorCode() == IapHelper.IAP_ERROR_NONE)
			{
				if (productList != null)
				{
					for (ProductVo productVo : productList) {
						if (AndroidLogConfig.ENABLED) { LogMessage("productVo: " + productVo.getJsonString()); }

						long lItemPriceMicros = (long)(productVo.getItemPrice() * 1000000.0);

						// Add the ProductInfo to the List of received Products
						productInfos.add(new ProductInfo(
							AndroidCommerceProductType.kProductTypeConsumable, // TODO add support for subscriptions
							productVo.getItemId(),
							productVo.getItemName(),
							productVo.getItemDesc(),
							productVo.getItemPriceString(),
							productVo.getCurrencyUnit(),
							lItemPriceMicros));
					}
					bSuccess = true;
				}
			}

			outArray = new ProductInfo[productInfos.size()];
			outArray = productInfos.toArray(outArray);
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
		finally
		{
			// Notify listener
			m_Listener.OnRefreshProductInfoComplete(bSuccess, outArray);
		}
	}

	// Helper methods

	private static AndroidCommerceResult ToCommerceResult(ErrorVo errorVo)
	{
		if (errorVo == null)
		{
			return AndroidCommerceResult.kResultFailure;
		}

		switch (errorVo.getErrorCode())
		{
		case HelperDefine.IAP_ERROR_NONE: return AndroidCommerceResult.kResultSuccess;
		case HelperDefine.IAP_PAYMENT_IS_CANCELED: return AndroidCommerceResult.kResultCanceled;
		case HelperDefine.IAP_ERROR_NEED_APP_UPGRADE: return AndroidCommerceResult.kResultClientInvalid;
		// NOTE: This is the desired transformation in all our use cases - an already owned item
		// is equivalent to purchasing it for the first time. We want to consume it either way.
		case HelperDefine.IAP_ERROR_ALREADY_PURCHASED: return AndroidCommerceResult.kResultSuccess;
		case HelperDefine.IAP_ERROR_PRODUCT_DOES_NOT_EXIST: return AndroidCommerceResult.kResultProductUnavailable;
		case HelperDefine.IAP_ERROR_ITEM_GROUP_DOES_NOT_EXIST: return AndroidCommerceResult.kResultProductUnavailable;
		case HelperDefine.IAP_ERROR_NETWORK_NOT_AVAILABLE: return AndroidCommerceResult.kResultNetworkError;
		case HelperDefine.IAP_ERROR_SOCKET_TIMEOUT: return AndroidCommerceResult.kResultNetworkError;
		case HelperDefine.IAP_ERROR_CONNECT_TIMEOUT: return AndroidCommerceResult.kResultNetworkError;
		case HelperDefine.IAP_ERROR_NOT_EXIST_LOCAL_PRICE: return AndroidCommerceResult.kResultProductUnavailable;
		case HelperDefine.IAP_ERROR_NOT_AVAILABLE_SHOP: return AndroidCommerceResult.kResultProductUnavailable;
		case HelperDefine.IAP_ERROR_NEED_SA_LOGIN: return AndroidCommerceResult.kResultNotSignedInOnline;
		default:
			return AndroidCommerceResult.kResultFailure;
		}
	}

	// Utility, input PurchaseVo to output PurchaseData array.
	private PurchaseData ConvertPurchase(PurchaseVo purchaseVo)
	{
		// Assemble list.
		if (null != purchaseVo)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("ConvertPurchase: '" + purchaseVo.getItemId() + "'."); }
			return new PurchaseData(
				purchaseVo.getItemId(),
				purchaseVo.getJsonString(),
				purchaseVo.getPurchaseId(),
				purchaseVo.getPaymentId());
		}

		return null;
	}

	// Utility, input OwnedProductVo to output PurchaseData array.
	private PurchaseData ConvertOwnedProduct(OwnedProductVo ownedProductVo)
	{
		// Assemble list.
		if (null != ownedProductVo)
		{
			if (AndroidLogConfig.ENABLED) { LogMessage("ConvertOwnedProduct: '" + ownedProductVo.getItemId() + "'."); }
			return new PurchaseData(
				ownedProductVo.getItemId(),
				ownedProductVo.getJsonString(),
				ownedProductVo.getPurchaseId(),
				ownedProductVo.getPaymentId());
		}

		return null;
	}
}
