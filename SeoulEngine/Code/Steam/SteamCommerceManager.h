/**
 * \file SteamCommerceManager.h
 * \brief Steam microtransaction API
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef STEAM_COMMERCE_MANAGER_H
#define STEAM_COMMERCE_MANAGER_H

#include "CommerceManager.h"
#include "HTTPManager.h"
#include "List.h"

#if SEOUL_WITH_STEAM
namespace Seoul { class SteamCommerceManagerPimpl; }

namespace Seoul
{

/**
 * Implementation of CommerceManager for the Steam platform.
 */
class SteamCommerceManager SEOUL_SEALED : public CommerceManager
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<SteamCommerceManager> Get()
	{
		if (CommerceManager::Get() && CommerceManager::Get()->GetType() == CommerceManagerType::kSteam)
		{
			return (SteamCommerceManager*)CommerceManager::Get().Get();
		}
		else
		{
			return CheckedPtr<SteamCommerceManager>();
		}
	}

	SteamCommerceManager(UInt32 uAppID, UInt64 uSteamID);
	~SteamCommerceManager();

	virtual CommerceManagerType GetType() const SEOUL_OVERRIDE
	{
		return CommerceManagerType::kSteam;
	}

	virtual const String& GetStoreName() const SEOUL_OVERRIDE;
	virtual Bool SupportsSubscriptions() const SEOUL_OVERRIDE { return false; }

	virtual void SetTransactionServerURLs(
		const String& sProductInfoURL,
		const String& sInitTransactionURL,
		const String& sFinalizeTransactionURL) SEOUL_OVERRIDE;

	/** Info from server about one product */
	struct OneProductInfoResponse
	{
		OneProductInfoResponse() : m_uProductID(0) , m_uPriceInSmallestUnits(0) { }

		UInt32 m_uProductID;
		String m_sLocalizedName;
		String m_sLocalizedDescription;
		String m_sCurrency;
		UInt64 m_uPriceInSmallestUnits;
		HString m_sType;
	};

	/** Info from server about all products */
	struct AllProductInfoResponse
	{
		Vector<OneProductInfoResponse> m_vProductInfo;
	};

	/** Response to the InitTransaction() API call */
	struct InitTransactionResponse
	{
		InitTransactionResponse() : m_uOrderID(0) { }

		UInt64 m_uOrderID;
		String m_sErrorMessage;
	};

	/** Response to the FinalizeTransaction() API call */
	struct FinalizeTransactionResponse
	{
		FinalizeTransactionResponse() : m_uOrderID(0) { }

		UInt64 m_uOrderID;
		String m_sErrorMessage;
	};

	/** Response to the FinalizeTransaction() API call */
	struct FinalizeTransactionFailureResponse
	{
		FinalizeTransactionFailureResponse() : m_uErrorCode(0) { }

		Int m_uErrorCode;
		String m_sErrorMessage;
	};

	void OnMicroTxnAuthorizationResponse(
		Bool bAuthorized,
		UInt64 uOrderID,
		UInt32 uAppID);

private:
	SEOUL_DISABLE_COPY(SteamCommerceManager);

	virtual void DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo) SEOUL_OVERRIDE;
	virtual void DoRefresh() SEOUL_OVERRIDE;
	virtual void DoPopulateOwnedDLCProducts() SEOUL_OVERRIDE;

	static UInt32 ProductIDStringToUInt32(const String& sProductID);
	static String ProductIDUInt32ToString(UInt32 uProductID);
	String FormatReceipt(UInt64 uOrderID) const;

	/** Info about a user-requested order */
	struct Order
	{
		// Orders link a product with a callback upon completion.
		Order() {};
		ProductID m_ProductID;
	};

	/** An invoice combines an order ID with product and callback */
	class Invoice SEOUL_SEALED
	{
	public:
		// Settling the invoice ends the transaction and de-allocates the
		// invoice instance.
		static HTTP::CallbackResult::Enum Settle(void* pVoidInvoice, HTTP::Result::Enum eResult, HTTP::Response *pResponse);

		// Invoices link an order ID with an order.
		Invoice(UInt32 uAppID, UInt64 uOrderID, const Order& order);

		// Paying an invoice will tell our server about the completed
		// purchase. Eventually, the invoice will go through the
		// Settle() call for self destruction.
		void Submit(HTTP::RequestList* pRequestList, const String& sURL);

		UInt32 const m_uAppID;
		UInt64 const m_uOrderID;
		ProductID const m_ProductID;

	private:
		SEOUL_DISABLE_COPY(Invoice);
	};

	// Bind a Steam order to an in-game item. Once you bind an order ID
	// to an item, you can resolve the order to deliver the actual item.
	//
	// Note: you may want to Flush() after binding if the binding
	// completes an order.
	void BindProduct(UInt64 uOrderID, const ProductID& productID);

	// Billing Flow
	void TryStartBilling();
	void TryStartBilling(UInt64 uOrderID);
	void StartBilling(UInt64 uOrderID);

	// HTTP callback for requesting product info
	static HTTP::CallbackResult::Enum InternalOnReceivedProductInfo(HTTP::Result::Enum eResult, HTTP::Response *pResponse);

	// HTTP callback for initating a transaction
	static HTTP::CallbackResult::Enum InternalOnTransactionInitiated(void* pVoidProductId, HTTP::Result::Enum eResult, HTTP::Response *pResponse);

	UInt32 const m_uAppID;
	UInt64 const m_uSteamID;
	ScopedPtr<SteamCommerceManagerPimpl> m_pImpl;

	// Create a CompeltedTransaction object for a billed purchase.
	CompletedTransaction* ConvertTransaction(UInt64 uOrderID, const ProductID& productID) const;

	String m_sProductInfoURL;
	String m_sInitTransactionURL;
	String m_sFinalizeTransactionURL;

	/** Mutex to protect m_lPendingTransactions */
	Mutex m_PendingTransactionsMutex;

	/** Queue of incomplete purchase requests (user requested) */
	typedef Vector<Order> OrderQueue;
	OrderQueue m_vOrders;

	/** Queue of Steam-authorized orders */
	typedef Vector<UInt64> AuthorizationQueue;
	AuthorizationQueue m_vAuthorizations;

	/** Map binding authorizations to product id */
	typedef HashTable<UInt64, ProductID> BindingTable;
	BindingTable m_tBindings;

	/** RequestList for managing Steam IAP HTTP requests. */
	HTTP::RequestList m_RequestList;
}; // class SteamCommerceManager

} // namespace Seoul

#endif // /#if SEOUL_WITH_STEAM

#endif // include guard
