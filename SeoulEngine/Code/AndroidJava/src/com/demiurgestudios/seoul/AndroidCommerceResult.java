/**
 * \file AndroidCommerceResult.java
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

package com.demiurgestudios.seoul;

/**
 * Result codes for commerce purchase/consume operations
 *
 * NOTE: Keep this in sync with the CommerceManager::EPurchaseResult enum
 */
public enum AndroidCommerceResult
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
	/** Steam is not running (PC-only)*/
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

	/** Client only (we'll never return this) - indicates a purchase was already awarded. */
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
	PlatformSpecificError2,

	/**
		Platform specific, see notes/usage
		On Android: Exception in consume
		On Steam: HTTP failure in settle
	*/
	PlatformSpecificError3,

	/**
		Platform specific, see notes/usage
		On Android: Exception in consume runnable
		On Steam: Failure to parse settle response
	*/
	PlatformSpecificError4,

	/**
		Platform specific, see notes/usage
		On Android: No purchase data in OnPurchaseComplete
	*/
	PlatformSpecificError5,

	/**
		Game specific, see notes/usage
	*/
	GameSpecificError1,

	/**
		Game specific, see notes/usage
	*/
	GameSpecificError2,

	/**
		Game specific, see notes/usage
	*/
	GameSpecificError3,

	/**
		Game specific, see notes/usage
	*/
	GameSpecificError4,
}
