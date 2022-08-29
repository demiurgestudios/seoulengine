/**
 * \file CommerceManager.h
 * \brief Singleton base class for abstracting per-platform in-app purchase
 * APIs. Handles tracking, submitting, and reporting purchase transactions.
 *
 * TODO: Determine best method for updating Inventory based on first-party behaviors.
 * When a purchase of a subscription IAP is made, the Inventory should reflect this product.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COMMERCE_MANAGER_H
#define COMMERCE_MANAGER_H

#include "Atomic32.h"
#include "AtomicRingBuffer.h"
#include "Delegate.h"
#include "FilePath.h"
#include "HashSet.h"
#include "HashTable.h"
#include "ReflectionDeclare.h"		// for SEOUL_REFLECTION_FRIENDSHIP
#include "ScopedPtr.h"
#include "SeoulTypes.h"
#include "SeoulTypeTraits.h"		// for SEOUL_SPECIALIZE_IS_ENUM
#include "SeoulAssert.h"
#include "Singleton.h"
#include "StandardPlatformIncludes.h"
#include "StringUtil.h"
#include "Vector.h"
namespace Seoul { struct PurchaseReceiptData; }

#include <stdint.h>

namespace Seoul
{

enum class CommerceManagerType
{
	kAndroid,
	kIOS,
	kDev,
	kNull,
	kSteam,
};

class CommerceManager SEOUL_ABSTRACT : public Singleton<CommerceManager>
{
public:
	SEOUL_DELEGATE_TARGET(CommerceManager);

	struct ProductID SEOUL_SEALED
	{
		// Constructs an invalid ProductID
		ProductID();
		ProductID(const String& sProductID);

		// Compares this item ID with another
		Bool operator == (const ProductID& other) const;
		Bool operator != (const ProductID& other) const;

		/** True if we represent a valid item ID */
		Bool m_bIsValid{};
		String m_sProductID{};
	};

	/**
	 * First party data about a purchaseable item.
	 */
	struct ProductInfo SEOUL_SEALED
	{
		/** Platform-specific identifier */
		ProductID m_ProductID{};

		/** Item's localized name */
		String m_sName{};

		/** Item's short localized description */
		String m_sDescription{};

		/** Item's cost in local currency */
		String m_sPrice{};

		/** Items's cost as raw number */
		Float m_fPrice{};

		/** 3-letter uppercase ISO 4217 currency code of the product's cost */
		String m_sCurrencyCode{};

		/** Item's cost in USD, determined via exchange rate table */
		Float m_fUSDPrice{};

	private:
		SEOUL_REFLECTION_FRIENDSHIP(ProductInfo);
		/** Methods for Reflection to ease deserialization. */
		void SetProductID(const String& sProductID)
		{
			m_ProductID = ProductID(sProductID);
		}
		const String& GetProductID() const
		{
			return m_ProductID.m_sProductID;
		}
	};

	/** Units of time for Subscription Periods */
	enum SubscriptionPeriodUnit
	{
		kSubscriptionPeriodUnitWeek,
		kSubscriptionPeriodUnitMonth,
		kSubscriptionPeriodUnitYear
	};

	/** Subscription Period with Unit and Value */
	struct SubscriptionPeriod SEOUL_SEALED
	{
		SubscriptionPeriodUnit Unit{};
		UInt32 Value{};
	};

	/**
	 * Item type.
	 * Not all platforms may support all item types.
	 */
	enum ItemType
	{
		kItemTypeConsumable,
		kItemTypeDLC,
		kItemTypeSubscription
	};

	/**
	 * Information about a purchaseable item.
	 * This includes any needed information that is not available on all platforms,
	 * including ItemType and subscription information.
	 */
	struct ItemInfo SEOUL_SEALED
	{
		/** Platform-agnostic unique identifier, to be used by game code */
		HString m_sID{};

		/** Item type */
		ItemType m_eType{};

		/** Item's cost in USD cents */
		Int32 m_iUSDCentsPrice{};

		/** Subscription billing period */
		SubscriptionPeriod m_SubscriptionPeriod{};
		/** Free Trial duration */
		SubscriptionPeriod m_FreeTrialDuration{};
		/** Item Group for subscription tiers */
		HString m_sGroup;

		/** First party data. */
		ProductInfo m_ProductInfo{};

	private:
		SEOUL_REFLECTION_FRIENDSHIP(ItemInfo);
		/** Methods for Reflection to ease deserialization. */
		void SetProductID(const String& sProductID)
		{
			m_ProductInfo.m_ProductID = ProductID(sProductID);
		}
		const String& GetProductID() const
		{
			return m_ProductInfo.m_ProductID.m_sProductID;
		}
	};

	/**
	 * Enumeration of result codes for Refresh()
	 */
	enum ERefreshResult
	{
		/** Refresh succeeded with at least one valid product */
		kRefreshSuccess,
		/** Unspecified failure */
		kRefreshFailure,
		/** Refresh succeeded but no products are available */
		kRefreshNoProducts
	};

	/**
	 * Enumeration of result codes for PurchaseItem()
	 *
	 * NOTE: This must be kept in sync with the EPurchaseResult
	 * enum in Java land and in Script
	 */
	enum class EPurchaseResult
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

	CommerceManager();
	virtual ~CommerceManager();

	virtual CommerceManagerType GetType() const = 0;

	void ReloadItemInfoTable();
	void Tick();

	/**
	 * Tests if we know all of the item info (may require fetching info from a
	 * platform-specific server on certain platforms like iOS)
	 */
	Bool HasAllItemInfo() const { return !m_bPendingProductInfo && m_bReceivedProductsInfo; }
	// Checks if we have received in the Inventory from the first-party.
	Bool HasReceivedSubscriptions() const { return m_bReceivedInventory; }

	/** Table type mapping internal item ID to item info */
	typedef HashTable<HString, ItemInfo*, MemoryBudgets::Commerce> ItemInfoTable;
	typedef Vector<ProductInfo, MemoryBudgets::Commerce> ProductInfoVector;
	typedef Vector<ProductID, MemoryBudgets::Commerce> ProductIDVector;

	// Gets the item info table for all items
	const ItemInfoTable& GetItemInfoTable() const;

	// Gets the list of all known platform-specific item IDs
	void GetAllPlatformItemIDs(Vector<ProductID>& vOutItemIDs) const;

	// Gets the list of all owned DLC platform-specific item IDs
	const ProductIDVector& GetOwnedDLCPlatformItemIDs() const;

	// Gets item info about a particular item
	const ItemInfo *GetItemInfo(HString sItemID) const;

	// Gets the price of the given item in the user's local currency
	Bool GetItemPrice(HString sItemID, String& sOutPrice) const;

	// Get information about the first finalizing transaction
	Bool GetFirstFinalizingTransaction(HString& itemID, EPurchaseResult& eResult, PurchaseReceiptData const*& pReceiptData) const;

	// Gets information related to a Subscription purchase for a given Item Id.
	Bool GetSubscription(HString itemID, EPurchaseResult& eResult, PurchaseReceiptData const*& pReceiptData) const;

	// Convenience method to determine if an item exists and is of a given type.
	Bool IsItemOfType(HString itemID, ItemType eType) const;

	// When an item purchase has completed, it will be passed to
	// the environment for finalizing. This is expected to reward
	// the item and commit the reward to persistent storage. Once
	// that process has completed, this method should be called to
	// tell the commerce system that the purchase can be removed
	// and the purchase flow is completed.
	void OnItemPurchaseFinalized(HString sItemID, const String& sFirstPartyTransactionID);

	// Begins the automated purchase of the given item.  When the purchase is
	// completed or canceled, the delegate is called.  Depending on the
	// platform, this may also show a platform-specific confirmation dialog.
	void PurchaseItem(HString sItemID);

	// Formats the given price using the given currency's symbol (currency
	// must be a 3-letter ISO 4217 currency code).  Optionally also converts
	// the price into a floating-point number in the currency's real units.
	static String FormatPrice(UInt64 uPriceInSmallestUnits, const String& sCurrency, Float *pOutPrice);

	/**
	 * @return FilePath to the configuration JSON of CommerceManager.
	 */
	FilePath GetJsonFilePath() const
	{
		return m_JsonFilePath;
	}

	/**
	 * @return true if a purchase is pending, false otherwise.
	 */
	Bool IsPurchaseInProgress() const
	{
		return !GetItemBeingProcessed().IsEmpty();
	}

	// Gets the name of the platform the CommerceManager uses for its microtransaction products.
	HString GetCommercePlatformId() const;

	/**
	 * Sets the URLs to be used for requesting product info and for initiating
	 * and finalizing transactions with a non-first-party server.
	 */
	virtual void SetTransactionServerURLs(
		const String& sProductInfoURL,
		const String& sInitTransactionURL,
		const String& sFinalizeTransactionURL) {}

	// String name of the Store for identifying which Store receipts originated from.
	virtual const String& GetStoreName() const = 0;

	// Whether or not this store support Subscription purchases
	virtual Bool SupportsSubscriptions() const = 0;

	/** Data we care about from a completed transaction. */
	class CompletedTransaction SEOUL_SEALED
	{
	public:
		CompletedTransaction();
		~CompletedTransaction();

		/** Platform dependent first-party identifier for the product. */
		ProductID m_ProductID{};

		/** Unique guid for the transaction. */
		String m_sTransactionID{};

		/** Success/failure and specifics thereof. */
		EPurchaseResult m_eResult{};

		/** Collected purchase receipt data for purchase verification. */
		ScopedPtr<PurchaseReceiptData> m_pPurchaseReceiptData{};

		/** Backend dependent first-party transaction object. */
		void* m_pTransactionObject{};

	private:
		SEOUL_DISABLE_COPY(CompletedTransaction);
	}; // struct CompletedTransaction

	/** Vector of transactions for all owned items. */
	typedef Vector<CheckedPtr<CompletedTransaction>, MemoryBudgets::Commerce> Inventory;

protected:
	// Must be implemented by specializations - cleanup the transaction
	// object but don't finalize/finish it. Used at shutdown if finalization
	// has not occurred for a pending object.
	virtual void DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction) = 0;

	// Must be implemented by specializations.
	// Handles finalization of a transaction.
	// For a consumable purchase:
	//	This is expected to remove any record of it from first party.
	//	Also assumed to perform the equivalent of DoDestroyTransactionObject().
	// For a subscription purchase:
	//	This is expected to acknowledge the subscription transaction with the first party.
	//	Assumed to NOT destroy (DoDestroyTransactionObject()) as a subscription is ongoing.
	// Is not called against for a finished transaction object.
	virtual void DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction) = 0;

	// Must be implemented by specializations - actually initiate
	// purchase for an item with first party.
	virtual void DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo) = 0;

	// Must be implemented by specializations - handle product
	// info refresh. Must send m_bPendingProductInfo to false
	// on completion and send out an engine virtual event
	// with the result.
	virtual void DoRefresh() = 0;

	// Optional hook that allows subclasses to implement custom per-frame logic. Run
	// prior to dispatch of completed or finalized transactions.
	virtual void DoTick() {}

	// Optional hook for subclasses to populated owned DLC products. Not all first-parties
	// support this Product type. It is expected that the subclass will invoke OnReceiveOwnedDLCProducts
	// when owned DLC products have been determined.
	virtual void DoPopulateOwnedDLCProducts() {}

	// Unit testing hook - allows interruption of OnItemPurchaseFinalized().
#if SEOUL_UNIT_TESTS
	virtual Bool UnitTestingHook_OnFinalizeAccept() { return true; }
#endif // /SEOUL_UNIT_TESTS

	// Attempt to estimated a USD price for a given local price and currency code.
	Float EstimateUSDPrice(ProductID const& productID, Float fLocalPrice, HString currencyCode) const;

	// Convenience that generates a CompletedTransaction object for failure cases, with
	// no other data.
	CompletedTransaction* MakeFailureObject(const ProductID& productID, EPurchaseResult eResult) const;

	// Gets the Item ID for a given ProductID
	HString GetItemIDForProduct(const ProductID& productID) const;

	// Gets the ItemInfo for a given ProductID
	const ItemInfo* GetItemInfoForProduct(ProductID const& productID) const;

	// Must be called by first party when a purchase has completed (success or failure).
	void OnCompletedTransaction(CompletedTransaction* pCompletedTransaction);

	// Must be called by first party when new product info has been received.
	void OnReceiveProductInfo(const ProductInfoVector& v);

	// Must be called by first party when owned DLC products are discovered.
	void OnReceiveOwnedDLCProducts(const ProductIDVector& v);

	// Must be called by first party when owned products are discovered.
	void OnReceiveInventory(const Inventory& v);

	// Can be overriden by subclasses as needed.
	Double m_fItemInfoRefreshIntervalSeconds;

private:
	SEOUL_DISABLE_COPY(CommerceManager);

	// Call to request updated product info.
	void Refresh();

	/** True if we've ever successfully fetched any product info */
	Atomic32Value<Bool> m_bReceivedProductsInfo{};

	/** True if a product info update is underway */
	Atomic32Value<Bool> m_bPendingProductInfo{};

	/** True if we've ever successfully fetched the Inventory. */
	Atomic32Value<Bool> m_bReceivedInventory{};

	/** Internal item ID of the item being purchased, if any. When this is non-empty, it indicates a purchase is in progress. */
	Atomic32Value<HStringData::InternalIndexType> m_sItemBeingPurchased{};
	HString GetItemBeingProcessed() const
	{
		HString ret;
		ret.SetHandleValue((HStringData::InternalIndexType)m_sItemBeingPurchased);
		return ret;
	}

	/** Ring buffer of completed transaction data. */
	typedef AtomicRingBuffer<CompletedTransaction*, MemoryBudgets::Commerce> CompletedTransactions;
	CompletedTransactions m_CompletedTransactions{};
	/** Queue of transactions that have been completed and need to be finalized. */
	typedef Vector<CompletedTransaction*, MemoryBudgets::Commerce> FinalizingTransactions;
	FinalizingTransactions m_vFinalizingTransactions{};

	/** Inventory of owned Items. */
	Inventory m_vInventory;

	// Loads purchasable item info from an JSON file
	void LoadItemInfoFromJson();

	/** File path to microtransactions.json */
	FilePath m_JsonFilePath{};

	/** Table of known items which can be purchased by a user */
	ItemInfoTable m_tItemInfo{};

	/** Used to periodically re-attempt a call to Refresh() for populating the product list. */
	Int64 m_iNextRefreshTimeInTicks{};

	/** Owned DLC Products */
	ProductIDVector m_vOwnedDLC{};

	/** Table keyed by Currency Code with the value being the exchange rate from the given currency to USD. */
	typedef HashTable<HString, Float> ExchangeRateTable;
	ExchangeRateTable m_tExchangeRates{};

	// Handle vtable and initialization.
	friend class Engine;
	void Initialize();
	void Shutdown();
}; // class CommerceManager

/**
 * Default CommerceManager implementation which does not succeed at doing
 * anything useful.  Used by platforms which do not support commerce
 * operations.
 */
class NullCommerceManager SEOUL_SEALED : public CommerceManager
{
public:
	NullCommerceManager()
	{
	}

	~NullCommerceManager()
	{
	}

	virtual CommerceManagerType GetType() const SEOUL_OVERRIDE { return CommerceManagerType::kNull; }
	virtual const String& GetStoreName() const SEOUL_OVERRIDE;
	virtual Bool SupportsSubscriptions() const SEOUL_OVERRIDE { return false; }

private:
	SEOUL_DISABLE_COPY(NullCommerceManager);

	virtual void DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo) SEOUL_OVERRIDE;
	virtual void DoRefresh() SEOUL_OVERRIDE;
}; // class NullCommerceManager

} // namespace Seoul

#endif // include guard
