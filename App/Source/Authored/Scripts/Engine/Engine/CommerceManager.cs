// Access to the native CommerceManager singleton.
//
// Copyright (c) Demiurge Studios, Inc.
// 
// This source code is licensed under the MIT license.
// Full license details can be found in the LICENSE file
// in the root directory of this source tree.

using static SlimCS;

public static class CommerceManager
{
	/// <summary>
	/// Enumeration of result codes for PurchaseItem()
	/// </summary>
	/// <remarks>
	/// NOTE: This must be kept in sync with the EPurchaseResult
	/// enum in Java land and in CPP
	/// </remarks>
	public enum EPurchaseResult
	{
		/** Purchase succeeded */
		kResultSuccess,

		/** Unspecified failure */
		kResultFailure,

		/** Purchase was canceled by the user */
		kResultCanceled,

		/** Error communicating with server */
		kResultNetworkError,

		/** User is not signed into an online profile */
		kResultNotSignedInOnline,

		/** Steam is not running (PC-only) */
		kResultSteamNotRunning,

		/** Steam settings are disabling the overlay (PC-only) */
		kResultSteamOverlayDisabled,

		/** User is not authorized to make payments */
		kResultCantMakePayments,

		/** Requested product is not available for purchase */
		kResultProductUnavailable,

		/** Failure to consume since item is not owned */
		kResultItemNotOwned,

		/**
			On iOS: Indicates that the client is not allowed to perform the attempted action.
			On Google Play: Indicates a response code of FEATURE_NOT_SUPPORTED
			On Amazon: Indicates a response code of NOT_SUPPORTED
		*/
		kResultClientInvalid,

		/**
			On iOS: Indicates that one of the payment parameters was not recognized by the Apple App Store.
			On Google Play: Indicates a response code of DEVELOPER_ERROR
			On Amazon: Indicates a response code of ALREADY_PURCHASED
		*/
		kResultPaymentInvalid,

		/** Raised by client handling if we already have a record of the given transaction id */
		kResultDuplicate,

		/** Raised by the engine if there is already a purchase in flight */
		kSeoulPurchaseInProgress,

		/** Raised by the engine if the item is not recognized maybe idk */
		kSeoulUnknownItem,

		/** Raised by the engine if we somehow fail to record the new item */
		kSeoulFailedToSetItem,

		/** Raised by platform-specific code when the 1st party API returns an error */
		kInternalPlatformError,

		/**
			Raised by platform-specific code when initialization was not completed
			On Android: no commerce manager was created
			On Steam: steam not initialized
		*/
		kPlatformNotInitialized,

		/**
			Platform specific, see notes/usage
			On iOS: Failed to find product in DoPurchaseItem
			On Android: Exception in purchase
			On Steam: HTTTP failure in purchase
			On developer: Product not found
		*/
		kPlatformSpecificError1,

		/**
			Platform specific, see notes/usage
			On Android: Exception in purchase runnable
			On Steam: Failure to parse purchase response
		*/
		kPlatformSpecificError2,

		/**
			Platform specific, see notes/usage
			On Android: Exception in consume
			On Steam: HTTP failure in settle
		*/
		kPlatformSpecificError3,

		/**
			Platform specific, see notes/usage
			On Android: Exception in consume runnable
			On Steam: Failure to parse settle response
		*/
		kPlatformSpecificError4,

		/**
			Platform specific, see notes/usage
			On Android: No purchase data in OnPurchaseComplete
		*/
		kPlatformSpecificError5,

		/**
			Game specific, see notes/usage
		*/
		kGameSpecificError1,

		/**
			Game specific, see notes/usage
		*/
		kGameSpecificError2,

		/**
			Game specific, see notes/usage
		*/
		kGameSpecificError3,

		/**
			Game specific, see notes/usage
		*/
		kGameSpecificError4,
	};

	/// <summary>
	/// Enumeration of result codes for OnItemInfoRefreshed()
	/// </summary>
	/// <remarks>
	/// NOTE: This must be kept in sync with the ERefreshResult
	/// enum in CPP
	/// </remarks>
	public enum ERefreshResult
	{
		/// <summary>
		/// &amp;lt; Refresh succeeded with at least one valid product
		/// </summary>
		kRefreshSuccess,

		/// <summary>
		/// &amp;lt; Unspecified failure
		/// </summary>
		kRefreshFailure,

		/// <summary>
		/// &amp;lt; Refresh succeeded but no products are available
		/// </summary>
		kRefreshNoProducts
	}

	delegate void OnItemIntoRefreshedHandler(ERefreshResult eResult);

	public static (string, string) GetErrorDisplayText(EPurchaseResult eResult)
	{
		switch (eResult)
		{
			case EPurchaseResult.kResultNetworkError:
				return ("IAPError_NetworkError_Title", "IAPError_NetworkError_Body");

			case EPurchaseResult.kResultNotSignedInOnline:
				return ("IAPError_NotSignedInOnline_Title", "IAPError_NotSignedInOnline_Body");

			case EPurchaseResult.kResultCantMakePayments:
				return ("IAPError_CantMakePayments_Title", "IAPError_CantMakePayments_Body");

			case EPurchaseResult.kResultProductUnavailable:
				return ("IAPError_ProductUnavailable_Title", "IAPError_ProductUnavailable_Body");

			case EPurchaseResult.kResultItemNotOwned:
				return ("IAPError_ItemNotOwned_Title", "IAPError_ItemNotOwned_Body");

			case EPurchaseResult.kResultClientInvalid:
				return ("IAPError_ClientInvalid_Title", "IAPError_ClientInvalid_Body");

			default:
				return ("IAPError_Failure_Title", "IAPError_Failure_Body");
		}
	}

	static readonly Native.ScriptEngineCommerceManager udCommerceManager = null;

	delegate bool CanHandlePurchasedItemsDelegate();

	static CommerceManager()
	{
		udCommerceManager = CoreUtilities.NewNativeUserData<Native.ScriptEngineCommerceManager>();
		if (null == udCommerceManager)
		{
			error("Failed instantiating ScriptEngineCommerceManager.");
		}

		// Native to script invoke when the list of purchaseable IAPs via CommerceManager
		// has been updated.
		_G["HANDLER_GlobalOnItemInfoRefreshed"] = (OnItemIntoRefreshedHandler)((eResult) =>
		{
			switch (eResult)
			{
				case ERefreshResult.kRefreshSuccess:
					CoreNative.LogChannel(LoggerChannel.Commerce, "Refreshed IAP item info, inform UIs");
					UIManager.BroadcastEvent("HANDLER_OnItemInfoRefreshed");
					break;

				case ERefreshResult.kRefreshFailure:
					CoreNative.LogChannel(LoggerChannel.Commerce, "Failed to refresh IAP item info");
					break;

				case ERefreshResult.kRefreshNoProducts:
					CoreNative.LogChannel(LoggerChannel.Commerce, "Refresh of IAP item info found no products");
					break;

				default:
					CoreNative.LogChannel(
						LoggerChannel.Commerce,
						"Received invalid result from item info refresh " +
						eResult +
						", did you update CommerceManager ERefreshResult in Native and fail to update script?");
					break;
			}
		});
	}

	public static string GetItemPrice(string sItemID)
	{
		var sRet = udCommerceManager.GetItemPrice(sItemID);
		return sRet;
	}

	public static (string, double) FormatPrice(double amount, string currencyString)
	{
		(var sRet, var dRet) = udCommerceManager.FormatPrice(amount, currencyString);
		return (sRet, dRet);
	}

	public static bool HasAllItemInfo()
	{
		var bRet = udCommerceManager.HasAllItemInfo();
		return bRet;
	}

	public static void PurchaseItem(string sItemID)
	{
		udCommerceManager.PurchaseItem(sItemID);
	}

	public static void OnItemPurchaseFinalized(string sItemID, string sFirstPartyTransactionID)
	{
		udCommerceManager.OnItemPurchaseFinalized(sItemID, sFirstPartyTransactionID);
	}

	public static string GetCommercePlatformId()
	{
		return udCommerceManager.GetCommercePlatformId();
	}

	public static Table GetProductInfo(string sItemID)
	{
		return udCommerceManager.GetProductInfo(sItemID);
	}
}
