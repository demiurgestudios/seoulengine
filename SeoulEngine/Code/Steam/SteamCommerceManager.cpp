/**
 * \file SteamCommerceManager.cpp
 * \brief Steam microtransaction API implementation
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EngineVirtuals.h"
#include "LocManager.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "ReflectionDeserialize.h"
#include "SteamCommerceManager.h"
#include "SteamEngine.h"
#include "SteamPrivateApi.h"
#include "ThreadId.h"

#if SEOUL_WITH_STEAM

namespace Seoul
{

SEOUL_SPEC_TEMPLATE_TYPE(Vector<SteamCommerceManager::OneProductInfoResponse, 48>)
SEOUL_BEGIN_TYPE(SteamCommerceManager::OneProductInfoResponse)
	SEOUL_PROPERTY_N("id", m_uProductID)
	SEOUL_PROPERTY_N("name", m_sLocalizedName)
	SEOUL_PROPERTY_N("description", m_sLocalizedDescription)
	SEOUL_PROPERTY_N("currency", m_sCurrency)
	SEOUL_PROPERTY_N("price", m_uPriceInSmallestUnits)
	SEOUL_PROPERTY_N("type", m_sType)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(SteamCommerceManager::AllProductInfoResponse)
	SEOUL_PROPERTY_N("products", m_vProductInfo)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(SteamCommerceManager::InitTransactionResponse)
	SEOUL_PROPERTY_N("orderid", m_uOrderID)
	SEOUL_PROPERTY_N("msg", m_sErrorMessage)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(SteamCommerceManager::FinalizeTransactionResponse)
	SEOUL_PROPERTY_N("orderid", m_uOrderID)
	SEOUL_PROPERTY_N("msg", m_sErrorMessage)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(SteamCommerceManager::FinalizeTransactionFailureResponse)
	SEOUL_PROPERTY_N("error_code", m_uErrorCode)
	SEOUL_PROPERTY_N("msg", m_sErrorMessage)
SEOUL_END_TYPE()

// Server-provided product types
static const HString kTypeConsumable("consumable");
static const HString kTypeDLC("dlc");

static const String s_kSteamStore("SteamStore");

/** Converts a String product Id to a UInt for Steam's App Ids. */
UInt32 SteamCommerceManager::ProductIDStringToUInt32(const String& sProductID)
{
	UInt32 uProductId;
	if (SSCANF(sProductID.CStr(), "%u", &uProductId) != 1)
	{
		SEOUL_ASSERT_MESSAGE(false, String::Printf("Could not parse Product Id as a UInt32: %s", sProductID.CStr()).CStr());
		return 0;
	}
	return uProductId;
}

/** Converts a Steam UInt App Id into a String for Commercemanager. */
String SteamCommerceManager::ProductIDUInt32ToString(UInt32 uProductID)
{
	return ToString(uProductID);
}

/** Format a purchase receipt. */
String SteamCommerceManager::FormatReceipt(UInt64 uOrderID) const
{
	return String::Printf("%d|%llu", m_uAppID, uOrderID);
}

class SteamCommerceManagerPimpl SEOUL_SEALED
{
public:
	SteamCommerceManagerPimpl(SteamCommerceManager& r)
		: m_r(r)
#pragma warning(disable:4355)  // 'this': used in base member initializer list
		, m_CallbackMicroTxnAuthorizationResponse(this, &SteamCommerceManagerPimpl::OnMicroTxnAuthorizationResponse)
#pragma warning(default:4355)
	{
	}

	~SteamCommerceManagerPimpl()
	{
	}

private:
	SEOUL_DISABLE_COPY(SteamCommerceManagerPimpl);

	SteamCommerceManager& m_r;

	// Steam callback when the user has responded to a microtransaction authorization request
	STEAM_CALLBACK(SteamCommerceManagerPimpl, OnMicroTxnAuthorizationResponse, MicroTxnAuthorizationResponse_t, m_CallbackMicroTxnAuthorizationResponse);
};

/**
 * Callback when Steam has presented the player with a microtransaction UI and
 * the player has approved or canceled it.
 */
void SteamCommerceManagerPimpl::OnMicroTxnAuthorizationResponse(MicroTxnAuthorizationResponse_t *pAuthorization)
{
	m_r.OnMicroTxnAuthorizationResponse(
		/* double bang */ !!pAuthorization->m_bAuthorized,
		pAuthorization->m_ulOrderID,
		pAuthorization->m_unAppID);
}

SteamCommerceManager::SteamCommerceManager(UInt32 uAppID, UInt64 uSteamID)
	: m_uAppID(uAppID)
	, m_uSteamID(uSteamID)
	, m_pImpl()
{
	m_pImpl.Reset(SEOUL_NEW(MemoryBudgets::Commerce) SteamCommerceManagerPimpl(*this));
}

SteamCommerceManager::~SteamCommerceManager()
{
	m_tBindings.Clear();
	m_vAuthorizations.Clear();
	m_vOrders.Clear();

	m_RequestList.BlockingCancelAll();
}

const String& SteamCommerceManager::GetStoreName() const
{
	return s_kSteamStore;
}

/**
 * Sets the URLs to be used for requesting product info and for initiating and
 * finalizing transactions with a non-first-party server.
 */
void SteamCommerceManager::SetTransactionServerURLs(
	const String& sProductInfoURL,
	const String& sInitTransactionURL,
	const String& sFinalizeTransactionURL)
{
	m_sProductInfoURL = sProductInfoURL;
	m_sInitTransactionURL = sInitTransactionURL;
	m_sFinalizeTransactionURL = sFinalizeTransactionURL;

	// Refresh when the Product Info URL becomes available.
	DoRefresh();
}


/**
 * Checks for which items have been purchased by the steam user who is
 * currently signed in.
 */
void SteamCommerceManager::DoPopulateOwnedDLCProducts()
{
	ProductIDVector v;

	auto pSteamApps = SteamApps();
	if (nullptr == pSteamApps)
	{
		OnReceiveOwnedDLCProducts(v);
		return;
	}

	// Only one steam user can be signed in so only update for the first local user.
	// Search through all items and check if the user owns them
	auto& itemInfoTable = GetItemInfoTable();
	for (auto itemInfoItr : itemInfoTable)
	{
		if (itemInfoItr.Second->m_eType != kItemTypeDLC)
		{
			continue;
		}

		auto& sProductId = itemInfoItr.Second->m_ProductInfo.m_ProductID.m_sProductID;
		auto uProductId = ProductIDStringToUInt32(sProductId);
		if (pSteamApps->BIsSubscribedApp(uProductId))
		{
			v.PushBack(ProductID(sProductId));
		}
	}

	// Inform CommerceManager
	OnReceiveOwnedDLCProducts(v);
}

/**
 * Refreshes the product info by asking our server for product information.
 * Will invoke InternalOnReceivedProductInfo as the request callback.
 */
void SteamCommerceManager::DoRefresh()
{
	// If the product info URL is not set, the request cannot be made.
	// If the user is not signed into Steam, our production app server will not send
	// product information. Avoid the unnecessary round trip.
	if (m_sProductInfoURL.IsEmpty()
#if SEOUL_SHIP && !defined(SEOUL_PROFILING_BUILD)
		|| nullptr == SteamApps()
#endif // SEOUL_SHIP && !defined(SEOUL_PROFILING_BUILD)
		)
	{
		OnReceiveProductInfo(ProductInfoVector());
		return;
	}

	auto& r = HTTP::Manager::Get()->CreateRequest(&m_RequestList);

	// Set the URL for requesting the product info and include the user's ID
	// and language so that the server can localize the results properly and
	// return the prices in the correct currency.
	auto pEngine(Engine::Get());
	String sURL = String::Printf(
		"%s/user/%lld/",
		m_sProductInfoURL.CStr(),
		m_uSteamID);

	r.SetURL(sURL);
	r.AddHeader("Accept-Language", pEngine->GetSystemLanguageCode());
	r.SetCallback(SEOUL_BIND_DELEGATE(&SteamCommerceManager::InternalOnReceivedProductInfo));
	r.Start();
}

/**
 * HTTP callback for requesting product info
 * Parse product information and deliver to base CommerceManager
 */
HTTP::CallbackResult::Enum SteamCommerceManager::InternalOnReceivedProductInfo(HTTP::Result::Enum eResult, HTTP::Response *pResponse)
{
	CheckedPtr<SteamCommerceManager> pSteamCommerceManager(SteamCommerceManager::Get());
	if (!pSteamCommerceManager.IsValid())
	{
		SEOUL_LOG_COMMERCE("SteamCommerceManager Failed to receive product info. SteamCommerceManager is null.");
		return HTTP::CallbackResult::kSuccess;;
	}

	// Check if the HTTP call succeeded
	if (eResult != HTTP::Result::kSuccess || pResponse->GetStatus() != HTTP::Status::kOK)
	{
		SEOUL_LOG_COMMERCE("SteamCommerceManager Failed to receive product info: result=%d status=%d", eResult, pResponse->GetStatus());
		pSteamCommerceManager->OnReceiveProductInfo(ProductInfoVector());
		return HTTP::CallbackResult::kSuccess;;
	}

	// Deserialize the product data
	AllProductInfoResponse allProductInfo;
	if (!Reflection::DeserializeFromString((const Byte *)pResponse->GetBody(), pResponse->GetBodySize(), &allProductInfo))
	{
		SEOUL_LOG_COMMERCE("SteamCommerceManager Failed to deserialize product info");
		pSteamCommerceManager->OnReceiveProductInfo(ProductInfoVector());
		return HTTP::CallbackResult::kSuccess;;
	}

	UInt uNumProducts = allProductInfo.m_vProductInfo.GetSize();
	SEOUL_LOG_COMMERCE("SteamCommerceManager Received product info for %u products\n", uNumProducts);

	ProductInfoVector vProductInfo;
	vProductInfo.Reserve(uNumProducts);
	for (auto& product : allProductInfo.m_vProductInfo)
	{
		// Create a ProductInfo entry for this product
		ProductInfo info;
		info.m_ProductID = ProductID(ProductIDUInt32ToString(product.m_uProductID));
		info.m_sName = product.m_sLocalizedName;
		info.m_sDescription = product.m_sLocalizedDescription;
		info.m_sPrice = FormatPrice(product.m_uPriceInSmallestUnits, product.m_sCurrency, &info.m_fPrice);
		info.m_sCurrencyCode = product.m_sCurrency;
		info.m_fUSDPrice = pSteamCommerceManager->EstimateUSDPrice(info.m_ProductID, info.m_fPrice, HString(info.m_sCurrencyCode));
		vProductInfo.PushBack(info);
	}

	// Deliver.
	pSteamCommerceManager->OnReceiveProductInfo(vProductInfo);

#if !SEOUL_SHIP
	// Check if we didn't receive data for any products. Other stores take an explicit list.
	// In our Steam implementation we ask for everything.
	Int nNumMissingProducts = 0;
	String sWarnStr;
	auto const& itemInfo = pSteamCommerceManager->GetItemInfoTable();
	for (auto itr : itemInfo)
	{
		if (itr.Second->m_ProductInfo.m_sName.IsEmpty())
		{
			nNumMissingProducts++;
			sWarnStr += String::Printf("%s (%s)\n", itr.First.CStr(), itr.Second->m_ProductInfo.m_ProductID.m_sProductID.CStr());
		}
	}
	if (nNumMissingProducts > 0)
	{
		SEOUL_WARN("SteamCommerceManager Did not receive product info for %d items:\n%s", nNumMissingProducts, sWarnStr.CStr());
	}
#endif

	return HTTP::CallbackResult::kSuccess;
}

/**
 * Begins the purchase flow for the given item.
 * Will invoke OnCompletedTransaction when completed.
 */
void SteamCommerceManager::DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo)
{
	// Make sure Steam has been initialized
	if (SteamApps() == nullptr)
	{
		SEOUL_LOG_COMMERCE("Cannot purchase items, Steam has not been initialized\n");
		OnCompletedTransaction(MakeFailureObject(itemInfo.m_ProductInfo.m_ProductID, EPurchaseResult::kResultSteamNotRunning));
		return;
	}

	if (!SteamUtils()->IsOverlayEnabled())
	{
		SEOUL_LOG_COMMERCE("Cannot purchase items, Steam overlay is disabled\n");
		OnCompletedTransaction(MakeFailureObject(itemInfo.m_ProductInfo.m_ProductID, EPurchaseResult::kResultSteamOverlayDisabled));
		return;
	}

	// Verify that we were configured properly
	if (m_sInitTransactionURL.IsEmpty())
	{
		SEOUL_WARN("Must call CommerceManager::SetTransactionServerURLs() before initiating Steam microtransactions");
		OnCompletedTransaction(MakeFailureObject(itemInfo.m_ProductInfo.m_ProductID, EPurchaseResult::kPlatformNotInitialized));
		return;
	}

	// Create an Order entry for this purchase
	Order order;
	Lock lock(m_PendingTransactionsMutex);
	order.m_ProductID = itemInfo.m_ProductInfo.m_ProductID;
	m_vOrders.PushBack(order);

	// Set up the HTTP request to initiate the transaction on the server
	auto& r = HTTP::Manager::Get()->CreateRequest(&m_RequestList);
	r.SetURL(m_sInitTransactionURL);
	r.SetMethod(HTTP::Method::ksPost);
	r.AddPostData("appid", ToString(m_uAppID));
	r.AddPostData("user", ToString(m_uSteamID));
	r.AddPostData("lang", LocManager::GetLanguageCode(LocManager::Get()->GetCurrentLanguage()));
	r.AddPostData("item", String(order.m_ProductID.m_sProductID));
	r.AddPostData("quantity", "1");

	ProductID* pProductIdParameter = SEOUL_NEW(MemoryBudgets::Commerce) ProductID;
	*pProductIdParameter = order.m_ProductID;
	r.SetCallback(SEOUL_BIND_DELEGATE(&SteamCommerceManager::InternalOnTransactionInitiated, pProductIdParameter));

	r.Start();
}

/**
 * HTTP callback for initating a transaction to purchase an item
 */
HTTP::CallbackResult::Enum SteamCommerceManager::InternalOnTransactionInitiated(void* pVoidProductId, HTTP::Result::Enum eResult, HTTP::Response *pResponse)
{
	CheckedPtr<SteamCommerceManager> pCommerce(SteamCommerceManager::Get());
	ProductID* pProductId = (ProductID*)pVoidProductId;
	ProductID productID = *pProductId;
	SEOUL_DELETE pProductId;

	if (eResult != HTTP::Result::kSuccess || pResponse->GetStatus() != HTTP::Status::kOK)
	{
		// If we failed to initiate the transaction, abort
		SEOUL_LOG_COMMERCE("Failed to initate transaction: result=%d status=%d\n%.*s\n",
			eResult,
			pResponse->GetStatus(),
			pResponse->GetBodySize(),
			(const char *)pResponse->GetBody());

		EPurchaseResult ePurchaseResult = (eResult != HTTP::Result::kSuccess ? EPurchaseResult::kResultNetworkError : EPurchaseResult::kPlatformSpecificError1);
		pCommerce->OnCompletedTransaction(pCommerce->MakeFailureObject(productID, ePurchaseResult));
		return HTTP::CallbackResult::kSuccess;
	}

	// Deserialize the response
	InitTransactionResponse response;
	if (!Reflection::DeserializeFromString((const Byte *)pResponse->GetBody(), pResponse->GetBodySize(), &response))
	{
		SEOUL_LOG_COMMERCE("Failed to deserialize InitTransaction response\n");
		pCommerce->OnCompletedTransaction(pCommerce->MakeFailureObject(productID, EPurchaseResult::kResultNetworkError));
	}
	else
	{
		// Success - bind the order ID to an item, then attempt to bill the authorized order.
		pCommerce->BindProduct(response.m_uOrderID, productID);
		pCommerce->TryStartBilling();
	}

	return HTTP::CallbackResult::kSuccess;
}

/** Record the server-generated order Id for the desired product being purchased */
void SteamCommerceManager::BindProduct(UInt64 uOrderID, const ProductID& productID)
{
	Lock lock(m_PendingTransactionsMutex);
	m_tBindings.Overwrite(uOrderID, productID);
}

/**
 * Try to start billing for the next item in the authorization queue.
 */
void SteamCommerceManager::TryStartBilling()
{
	Lock lock(m_PendingTransactionsMutex);
	AuthorizationQueue vBatch;

	// Move the authorization queue into a temporary list. We'll
	// return authorizations to the queue if we can't fulfill them.
	vBatch.Swap(m_vAuthorizations);

	// Attempt to resolve all authorizations in the batch.
	AuthorizationQueue::Iterator iCurrent = vBatch.Begin();
	AuthorizationQueue::Iterator iEnd = vBatch.End();
	while (iCurrent != iEnd)
	{
		// Note: resolving will put authorizations back in the
		// authoriation queue if the resolve fails.
		TryStartBilling(*iCurrent);
		++iCurrent;
	}
}

/**
 * Try to start billing for a specific order.
 * If the order is not bound to a product, then the order will be put into an authorization queue for a future attempt.
 *
 * @param uOrderID Order Id to attempt to start the billing flow for.
 */
void SteamCommerceManager::TryStartBilling(UInt64 uOrderID)
{
	Lock lock(m_PendingTransactionsMutex);
	ProductID sProductID;

	// Only resolve orders with bindings (or else we don't know which product to consume).
	if (m_tBindings.GetValue(uOrderID, sProductID))
	{
		// Start the billing flow.
		StartBilling(uOrderID);

		// Discard the authorization (don't put it in the queue).
		return;
	}

	// If we didn't find a request with a matching order, then we're
	// not ready to complete the order yet.
	m_vAuthorizations.PushBack(uOrderID);
}

/**
 * Start the billing flow for a specific bound order.
 * Will submit an invoice and remove the Order and Binding for this Order Id.
 *
 * @param uOrderID Order Id to start the billing flow for.
 */
void SteamCommerceManager::StartBilling(UInt64 uOrderID)
{
	Lock lock(m_PendingTransactionsMutex);
	ProductID productID;

	if (m_tBindings.GetValue(uOrderID, productID))
	{
		// Find the first item in the request queue.
		OrderQueue::Iterator iCurrent = m_vOrders.Begin();
		OrderQueue::Iterator iEnd = m_vOrders.End();
		while (iCurrent != iEnd)
		{
			if (iCurrent->m_ProductID == productID)
			{
				Invoice* pInvoice = SEOUL_NEW(MemoryBudgets::Commerce) Invoice(m_uAppID, uOrderID, *iCurrent);

				// Remove the request and the binding.
				m_vOrders.Erase(iCurrent);
				m_tBindings.Erase(uOrderID);

				// Submit the invoice and bill the player for the purchase
				pInvoice->Submit(&m_RequestList, m_sFinalizeTransactionURL);

				// Complete only one at a time.
				return;
			}
			++iCurrent;
		}
	}

	SEOUL_WARN("SteamCommerceManager Could not start billing for order %lu - no known Product", uOrderID);
}

SteamCommerceManager::Invoice::Invoice(UInt32 uAppID, UInt64 uOrderID, const Order& order)
	: m_uAppID(uAppID)
	, m_uOrderID(uOrderID)
	, m_ProductID(order.m_ProductID)
{
}

void SteamCommerceManager::Invoice::Submit(HTTP::RequestList* pRequestList, const String& sURL)
{
	// Set up the HTTP request to initiate the transaction on the server
	auto& r = HTTP::Manager::Get()->CreateRequest(pRequestList);

	r.SetURL(sURL);
	r.SetMethod(HTTP::Method::ksPost);

	// Set various POST parameters
	r.AddPostData("appid", ToString(m_uAppID));
	r.AddPostData("orderid", ToString(m_uOrderID));

	r.SetCallback(SEOUL_BIND_DELEGATE(&SteamCommerceManager::Invoice::Settle, (void*)this));

	r.Start();
}

/**
 * Callback for handling a settled purchase
 * At this point the player has been billed for the purchase.
 */
HTTP::CallbackResult::Enum SteamCommerceManager::Invoice::Settle(void* pVoidInvoice, HTTP::Result::Enum eResult, HTTP::Response *pResponse)
{
	CheckedPtr<SteamCommerceManager> pCommerce(SteamCommerceManager::Get());
	Invoice* pInvoice = (Invoice*) pVoidInvoice;
	ProductID productID = pInvoice->m_ProductID;
	UInt64 uOrderID = pInvoice->m_uOrderID;

	// We must always de-allocate settled invoices.
	SEOUL_DELETE pInvoice;

	if (eResult != HTTP::Result::kSuccess || pResponse->GetStatus() != HTTP::Status::kOK)
	{
		EPurchaseResult outcome = (eResult != HTTP::Result::kSuccess ? EPurchaseResult::kResultNetworkError : EPurchaseResult::kPlatformSpecificError3);

		// If we failed to finalize the transaction, abort
		SEOUL_LOG_COMMERCE("Failed to finalize transaction, result=%d status=%d\n%.*s\n",
			eResult,
			pResponse->GetStatus(),
			pResponse->GetBodySize(),
			(const char *)pResponse->GetBody());

		if(EPurchaseResult::kPlatformSpecificError3 == outcome)
		{
			FinalizeTransactionFailureResponse response;
			if (Reflection::DeserializeFromString((const Byte *)pResponse->GetBody(), pResponse->GetBodySize(), &response))
			{
				// Transaction has been denied by user
				if(response.m_uErrorCode == 10)
				{
					outcome = EPurchaseResult::kResultCanceled;
				}
			}
		}

		pCommerce->OnCompletedTransaction(pCommerce->MakeFailureObject(productID, outcome));
		return HTTP::CallbackResult::kSuccess;
	}

	// Deserialize the response
	FinalizeTransactionResponse response;
	if (!Reflection::DeserializeFromString((const Byte *)pResponse->GetBody(), pResponse->GetBodySize(), &response))
	{
		SEOUL_LOG_COMMERCE("Failed to deserialize FinalizeTransaction response\n");
		pCommerce->OnCompletedTransaction(pCommerce->MakeFailureObject(productID, EPurchaseResult::kResultNetworkError));
		return HTTP::CallbackResult::kSuccess;
	}

	SEOUL_LOG_COMMERCE("Transaction succeeded\n");

	// Convert the transaction into a tracking object.
	auto pCompletedTransaction = pCommerce->ConvertTransaction(uOrderID, productID);

	// Dispatch.
	pCommerce->OnCompletedTransaction(pCompletedTransaction);
	return HTTP::CallbackResult::kSuccess;
}

/**
 * Create a CompletedTransaction object for tracking the completed purchase.
 * SteamCommerceManager does not support the CommerceManager concept of "finalizing" a completed transaction.
 * We will not be able to handled rewarding purchases that are interrupted after billing and before saving the rewards.
 */
CommerceManager::CompletedTransaction* SteamCommerceManager::ConvertTransaction(UInt64 uOrderID, const ProductID& productID) const
{
	SEOUL_ASSERT(IsMainThread());

	// For now, use the order Id as the Transaction Id.
	String sTransactionID = String::Printf("%lu", uOrderID);

	// Allocate and return.
	auto pReturn(SEOUL_NEW(MemoryBudgets::Commerce) CompletedTransaction);
	pReturn->m_eResult = EPurchaseResult::kResultSuccess;
	pReturn->m_ProductID = productID;
	pReturn->m_pTransactionObject = nullptr;
	pReturn->m_sTransactionID = sTransactionID;

	ScopedPtr<PurchaseReceiptData> pPurchaseReceiptData;
	if (!sTransactionID.IsEmpty())
	{
		// Fill out receipt data and assign for submission.
		pPurchaseReceiptData.Reset(SEOUL_NEW(MemoryBudgets::Commerce) PurchaseReceiptData);

		// Fill out.
		pPurchaseReceiptData->m_sPayload = FormatReceipt(uOrderID);
		pPurchaseReceiptData->m_sStore = GetStoreName();
		pPurchaseReceiptData->m_sTransactionID = sTransactionID;
		pPurchaseReceiptData->m_sPurchaseToken = String();
	}

	pReturn->m_pPurchaseReceiptData.Swap(pPurchaseReceiptData);

	// Done.
	return pReturn;
}

void SteamCommerceManager::OnMicroTxnAuthorizationResponse(
	Bool bAuthorized,
	UInt64 uOrderID,
	UInt32 uAppID)
{
	// I don't think this can happen, but guard against it just in case, since
	// some other Steam APIs are documented as capable of receiving spurious
	// callback notifications for other apps.
	if (uAppID != m_uAppID)
	{
		SEOUL_LOG_COMMERCE("Got MicroTxnAuthorizationResponse_t callback for wrong app ID (%u)\n", uAppID);
		return;
	}

	// Canceled purchase
	if (!bAuthorized)
	{
		// Canceled for an expected product
		CommerceManager::ProductID productID;
		if (m_tBindings.GetValue(uOrderID, productID))
		{
			m_tBindings.Erase(uOrderID);
			OnCompletedTransaction(MakeFailureObject(productID, EPurchaseResult::kResultCanceled));
			return;
		}
		// Canceled for an unexpected product. This should never happen.
		else
		{
			SEOUL_WARN("SteamCommerceManager received unauthorized response for microtransaction order %lu with no known Product.",
				uOrderID);
		}
		return;
	}

	// Try to start the billing process for the authorized order.
	// If we have not yet bound the order with a product, this will add the order to an authorized list for a future attempt.
	TryStartBilling(uOrderID);
}

void SteamCommerceManager::DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	// no-op
	// Currently nothing is allocated on the CompletedTransaction by SteamCommerceManager.
}

void SteamCommerceManager::DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	// no-op
	// TODO: Implement finalizing completed orders.
}

} // namespace Seoul

#endif // /SEOUL_WITH_STEAM
