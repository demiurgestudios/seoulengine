/**
 * \file CommerceManager.cpp
 * \brief Singleton base class for abstracting per-platform in-app purchase
 * APIs. Handles tracking, submitting, and reporting purchase transactions.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CommerceManager.h"
#include "DataStoreParser.h"
#include "Engine.h"
#include "FilePath.h"
#include "Logger.h"
#include "ReflectionDefine.h"			// for SEOUL_PROPERTY_N, etc
#include "ReflectionEnum.h"				// for Enum
#include "SettingsManager.h"
#include "ToString.h"
#include "EngineVirtuals.h"

namespace Seoul
{

static const HString ksPriceInfoSectionName("USDCentsPrice");
static const HString ksExchangeRateSectionName("ExchangeRates");
static const Double kDefaultItemInfoRefreshIntervalSeconds = 5.0;

#if SEOUL_PLATFORM_LINUX
static const HString kAndroid("Android");
#else
static const HString kSamsung("Samsung");
static const HString kAmazon("Amazon");
#endif

static const String s_kNullCommerceManagerStoreName("Null");

#define SEOUL_TRACE(fmt, ...) SEOUL_LOG_COMMERCE("[CommerceManager]: %s: " fmt, __FUNCTION__, ##__VA_ARGS__)

SEOUL_BEGIN_ENUM(CommerceManager::ERefreshResult)
	SEOUL_ENUM_N("RefreshSuccess", CommerceManager::kRefreshSuccess)
	SEOUL_ENUM_N("RefreshFailure", CommerceManager::kRefreshFailure)
	SEOUL_ENUM_N("RefreshNoProducts", CommerceManager::kRefreshNoProducts)
SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(CommerceManager::EPurchaseResult)
	SEOUL_ENUM_N("ResultSuccess", CommerceManager::EPurchaseResult::kResultSuccess)
	SEOUL_ENUM_N("ResultFailure", CommerceManager::EPurchaseResult::kResultFailure)
	SEOUL_ENUM_N("ResultCanceled", CommerceManager::EPurchaseResult::kResultCanceled)
	SEOUL_ENUM_N("ResultNetworkError", CommerceManager::EPurchaseResult::kResultNetworkError)
	SEOUL_ENUM_N("ResultNotSignedInOnline", CommerceManager::EPurchaseResult::kResultNotSignedInOnline)
	SEOUL_ENUM_N("ResultSteamNotRunning", CommerceManager::EPurchaseResult::kResultSteamNotRunning)
	SEOUL_ENUM_N("ResultSteamOverlayDisabled", CommerceManager::EPurchaseResult::kResultSteamOverlayDisabled)
	SEOUL_ENUM_N("ResultCantMakePayments", CommerceManager::EPurchaseResult::kResultCantMakePayments)
	SEOUL_ENUM_N("ResultProductUnavailable", CommerceManager::EPurchaseResult::kResultProductUnavailable)
	SEOUL_ENUM_N("ResultItemNotOwned", CommerceManager::EPurchaseResult::kResultItemNotOwned)
	SEOUL_ENUM_N("ResultClientInvalid", CommerceManager::EPurchaseResult::kResultClientInvalid)
	SEOUL_ENUM_N("ResultPaymentInvalid", CommerceManager::EPurchaseResult::kResultPaymentInvalid)
	SEOUL_ENUM_N("ResultDuplicate", CommerceManager::EPurchaseResult::kResultDuplicate)
	SEOUL_ENUM_N("SeoulPurchaseInProgress", CommerceManager::EPurchaseResult::kSeoulPurchaseInProgress)
	SEOUL_ENUM_N("SeoulUnknownItem", CommerceManager::EPurchaseResult::kSeoulUnknownItem)
	SEOUL_ENUM_N("SeoulFailedToSetItem", CommerceManager::EPurchaseResult::kSeoulFailedToSetItem)
	SEOUL_ENUM_N("InternalPlatformError", CommerceManager::EPurchaseResult::kInternalPlatformError)
	SEOUL_ENUM_N("PlatformNotInitialized", CommerceManager::EPurchaseResult::kPlatformNotInitialized)
	SEOUL_ENUM_N("PlatformSpecificError1", CommerceManager::EPurchaseResult::kPlatformSpecificError1)
	SEOUL_ENUM_N("PlatformSpecificError2", CommerceManager::EPurchaseResult::kPlatformSpecificError2)
	SEOUL_ENUM_N("PlatformSpecificError3", CommerceManager::EPurchaseResult::kPlatformSpecificError3)
	SEOUL_ENUM_N("PlatformSpecificError4", CommerceManager::EPurchaseResult::kPlatformSpecificError4)
	SEOUL_ENUM_N("PlatformSpecificError5", CommerceManager::EPurchaseResult::kPlatformSpecificError5)
	SEOUL_ENUM_N("GameSpecificError1", CommerceManager::EPurchaseResult::kGameSpecificError1)
	SEOUL_ENUM_N("GameSpecificError2", CommerceManager::EPurchaseResult::kGameSpecificError2)
	SEOUL_ENUM_N("GameSpecificError3", CommerceManager::EPurchaseResult::kGameSpecificError3)
	SEOUL_ENUM_N("GameSpecificError4", CommerceManager::EPurchaseResult::kGameSpecificError4)

SEOUL_END_ENUM()

SEOUL_BEGIN_ENUM(CommerceManager::SubscriptionPeriodUnit)
	SEOUL_ENUM_N("Week", CommerceManager::kSubscriptionPeriodUnitWeek)
	SEOUL_ENUM_N("Month", CommerceManager::kSubscriptionPeriodUnitMonth)
	SEOUL_ENUM_N("Year", CommerceManager::kSubscriptionPeriodUnitYear)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(CommerceManager::SubscriptionPeriod)
	SEOUL_PROPERTY_N("Unit", Unit)
	SEOUL_PROPERTY_N("Value", Value)
SEOUL_END_TYPE()

SEOUL_BEGIN_ENUM(CommerceManager::ItemType)
	SEOUL_ENUM_N("Consumable", CommerceManager::kItemTypeConsumable)
	SEOUL_ENUM_N("DLC", CommerceManager::kItemTypeDLC)
	SEOUL_ENUM_N("Subscription", CommerceManager::kItemTypeSubscription)
SEOUL_END_ENUM()

SEOUL_BEGIN_TYPE(CommerceManager::ItemInfo)
	SEOUL_PROPERTY_PAIR_N("ProductID", GetProductID, SetProductID)
	SEOUL_PROPERTY_N("Type", m_eType)
	SEOUL_PROPERTY_N("USDCentsPrice", m_iUSDCentsPrice)
	SEOUL_PROPERTY_N("SubscriptionPeriod", m_SubscriptionPeriod)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("FreeTrialDuration", m_FreeTrialDuration)
		SEOUL_ATTRIBUTE(NotRequired)
	SEOUL_PROPERTY_N("Group", m_sGroup)
		SEOUL_ATTRIBUTE(NotRequired)
SEOUL_END_TYPE()

SEOUL_BEGIN_TYPE(CommerceManager::ProductInfo)
	SEOUL_PROPERTY_PAIR_N("sProductID", GetProductID, SetProductID)
	SEOUL_PROPERTY_N("sName", m_sName)
	SEOUL_PROPERTY_N("sDescription", m_sDescription)
	SEOUL_PROPERTY_N("sPrice", m_sPrice)
	SEOUL_PROPERTY_N("fPrice", m_fPrice)
	SEOUL_PROPERTY_N("sCurrencyCode", m_sCurrencyCode)
	SEOUL_PROPERTY_N("fUSDPrice", m_fUSDPrice)
SEOUL_END_TYPE()

/**
 * Constructs an invalid ProductID
 */
CommerceManager::ProductID::ProductID()
	: m_bIsValid(false)
	, m_sProductID()
{
}

/**
 * Constructs an ProductID from the given product ID string
 */
CommerceManager::ProductID::ProductID(const String& sProductID)
	: m_bIsValid(true)
{
	m_sProductID = sProductID;
}

/**
 * Tests this ProductID for equality with another ProductID
 */
Bool CommerceManager::ProductID::operator == (const ProductID& other) const
{
	if (m_bIsValid != other.m_bIsValid)
	{
		return false;
	}

	if (!m_bIsValid && !other.m_bIsValid)
	{
		return true;
	}

	return (m_sProductID.CompareASCIICaseInsensitive(other.m_sProductID) == 0);
}

/**
 * Tests this ProductID for inequality with another ProductID
 */
Bool CommerceManager::ProductID::operator != (const ProductID& other) const
{
	return !(*this == other);
}

CommerceManager::CommerceManager()
	: m_fItemInfoRefreshIntervalSeconds(kDefaultItemInfoRefreshIntervalSeconds)
{
	m_JsonFilePath = FilePath::CreateConfigFilePath("microtransactions.json");
}

CommerceManager::~CommerceManager()
{
	SEOUL_ASSERT(m_tItemInfo.IsEmpty());
}

/**
 * Loads basic item info about purchasable items from an json file.
 */
void CommerceManager::LoadItemInfoFromJson()
{
	SEOUL_ASSERT(IsMainThread());

	auto pSettingsManager = SettingsManager::Get();
	SharedPtr<DataStore> pSettings = pSettingsManager->WaitForSettings(m_JsonFilePath);
	if (!pSettings.IsValid())
	{
		SEOUL_TRACE("Unable to load %s\n", m_JsonFilePath.CStr());
		return;
	}

	DataNode platformSection;
	HString sPlatformName = GetCommercePlatformId();
	if (!pSettings->GetValueFromTable(pSettings->GetRootNode(), sPlatformName, platformSection))
	{
		SEOUL_TRACE("No section in %s for platform %s\n", m_JsonFilePath.CStr(), sPlatformName.CStr());
		return;
	}

	// Read in ItemInfo from the section
	{
		auto const iBegin = pSettings->TableBegin(platformSection);
		auto const iEnd = pSettings->TableEnd(platformSection);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			auto pItemInfo = SEOUL_NEW(MemoryBudgets::Commerce) ItemInfo();

			HString const sItemID(i->First);
			Reflection::DeserializeObject(m_JsonFilePath, *pSettings, i->Second, pItemInfo);
			pItemInfo->m_sID = sItemID;

			auto itemInfo = m_tItemInfo.Insert(sItemID, pItemInfo);
			SEOUL_ASSERT(itemInfo.Second);
		}
	}

	// Now get the Exchange Rates.
	// TODO: The app server should control these values and automate updating them periodically.
	DataNode exchangeRateSection;
	if (!pSettings->GetValueFromTable(pSettings->GetRootNode(), ksExchangeRateSectionName, exchangeRateSection))
	{
		SEOUL_TRACE("No section in %s for %s\n", m_JsonFilePath.CStr(), ksExchangeRateSectionName.CStr());
		return;
	}

	{
		auto const iBegin = pSettings->TableBegin(exchangeRateSection);
		auto const iEnd = pSettings->TableEnd(exchangeRateSection);
		for (auto i = iBegin; iEnd != i; ++i)
		{
			HString const sCountryCode(i->First);
			Float fRate = 1.f;
			if (!pSettings->AsFloat32(i->Second, fRate))
			{
#if SEOUL_LOGGING_ENABLED
				String sValue;
				pSettings->ToString(i->Second, sValue);
				SEOUL_TRACE("Exchange rate in %s for '%s' is an invalid value: %s\n", m_JsonFilePath.CStr(), sCountryCode.CStr(), sValue.CStr());
#endif // SEOUL_LOGGING_ENABLED
				continue;
			}
			m_tExchangeRates.Insert(sCountryCode, fRate);
		}
	}

	SEOUL_TRACE("Loaded item info for %u items from microtransactions.json\n", m_tItemInfo.GetSize());
}

/**
 * Refreshes the product info cache.  May be a no-op on some platforms.
 */
void CommerceManager::Refresh()
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_TRACE("Asked to refresh product info.");

	// If a refresh is in progress or if we already have received
	// info, ignore the request.
	if (m_bReceivedProductsInfo || m_bPendingProductInfo)
	{
		SEOUL_TRACE("Not refreshing product info, already have up-to-date info.");
		return;
	}

	SEOUL_TRACE("Dispatching to first-party to refresh product info.");

	// prevent re-entry
	m_bPendingProductInfo = true;

	// Implementation specific handling.
	DoRefresh();
}

void CommerceManager::ReloadItemInfoTable()
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_TRACE("Asked to reload item info table from microtransactions.json.");

	// Release current item info.
	SafeDeleteTable(m_tItemInfo);
	m_tExchangeRates.Clear();

	// Load new.
	LoadItemInfoFromJson();

	// Force a refresh on new info.
	// Don't mess with pending here, in case a refresh is currently active.
	m_bReceivedProductsInfo = false;
	m_iNextRefreshTimeInTicks = 0;
}

void CommerceManager::Tick()
{
	SEOUL_ASSERT(IsMainThread());

	// Check if we need to issue a new product info refresh.
	auto iNowTicks = SeoulTime::GetGameTimeInTicks();
	if (!m_bReceivedProductsInfo && m_iNextRefreshTimeInTicks < iNowTicks)
	{
		if (!m_bPendingProductInfo)
		{
			SEOUL_TRACE("Refreshing product info after waiting %.2f seconds", m_fItemInfoRefreshIntervalSeconds);

			Refresh();

			// schedule the next item into refresh attempt
			m_iNextRefreshTimeInTicks = iNowTicks + SeoulTime::ConvertSecondsToTicks(m_fItemInfoRefreshIntervalSeconds);
		}
	}

	// Subclass tick.
	DoTick();

	// Dispatch any completed transactions.
	if (g_pEngineVirtuals->CanHandlePurchasedItems())
	{
		auto pNext = m_CompletedTransactions.Pop();
		while (nullptr != pNext)
		{
			// Get internal item ID.
			auto const sItemID(GetItemIDForProduct(pNext->m_ProductID));

			// If no mapping, leave in the first-party queue without Finishing.
			// This allows a transaction to hold until (for example)
			// a patch provides a new mapping table to resolve the lookup.
			if (sItemID.IsEmpty())
			{
				// Advance.
				DoDestroyTransactionObject(pNext);
				SafeDelete(pNext);
				pNext = m_CompletedTransactions.Pop();
				continue;
			}

			SEOUL_TRACE("Delivering completed purchase ('%s', '%s') to app.", sItemID.CStr(), pNext->m_sTransactionID.CStr());

			// Send out result.
			g_pEngineVirtuals->OnItemPurchased(sItemID, pNext->m_eResult, pNext->m_pPurchaseReceiptData.Get());

			// If the result has a transaction id and was successful, wait for finalizing.
			// Otherwise, complete immediately.
			if (pNext->m_sTransactionID.IsEmpty() || EPurchaseResult::kResultSuccess != pNext->m_eResult)
			{
				SEOUL_TRACE("Completed purchase '%s' was not successful, finishing immediately.", sItemID.CStr());

				// Finish transaction object.
				DoFinishTransactionObject(pNext);

				// Cleanup.
				SafeDelete(pNext);
			}
			else
			{
				SEOUL_TRACE("Completed purchase '%s' was successful, waiting for app to finish.", sItemID.CStr());

				// Put into finalizing.
				m_vFinalizingTransactions.PushBack(pNext);
			}

			// Atomic release of purchased item, only if the
			// one just completed.
			(void)m_sItemBeingPurchased.CompareAndSet(HString().GetHandleValue(), sItemID.GetHandleValue());

			// Advance.
			pNext = m_CompletedTransactions.Pop();
		}
	}
}

/**
 * Must be called by client code when a user has been granted awards (and that new state
 * has been saved to persistence). This tells the first-party layer to remove the
 * record of the (consumable) purchase.
 */
void CommerceManager::OnItemPurchaseFinalized(HString sItemID, const String& sFirstPartyTransactionID)
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_TRACE("Received request to finish purchase ('%s', '%s').", sItemID.CStr(), sFirstPartyTransactionID.CStr());

	// Unit testing hook - allows interruption of OnItemPurchaseFinalized().
#if SEOUL_UNIT_TESTS
	if (!UnitTestingHook_OnFinalizeAccept()) { return; }
#endif // /SEOUL_UNIT_TESTS

	auto i = m_vFinalizingTransactions.Find(sFirstPartyTransactionID);
	if (m_vFinalizingTransactions.End() != i)
	{
		SEOUL_TRACE("Transaction ('%s', '%s') found to finalize, dispatching to first party.", sItemID.CStr(), sFirstPartyTransactionID.CStr());

		// Remove From Finalizing transactions
		auto p = *i;
		m_vFinalizingTransactions.Erase(i);

		// Finish the Transaction
		// For consumables this consumes the item with the first party and deallocates first-party allocated data.
		// For subscriptions this acknowledges the subscription, but keeps the transaction around.
		DoFinishTransactionObject(p);

		// If this is a Subscription purchase, add it to the Inventory.
		auto pItemInfo = GetItemInfoForProduct(p->m_ProductID);
		if (nullptr != pItemInfo && kItemTypeSubscription == pItemInfo->m_eType)
		{
			SEOUL_TRACE("Adding finalized Subscription Transaction ('%s', '%s') to Inventory.", sItemID.CStr(), sFirstPartyTransactionID.CStr());
			m_vInventory.PushBack(p);
		}

		// If this is not a subscription purchase, finish and clean up the transaction.
		else
		{
			SafeDelete(p);
		}
	}
}

/**
 * Begins the automated purchase of the given item.  When the purchase is
 * completed or canceled, the delegate is called.  Depending on the
 * platform, this may also show a platform-specific confirmation dialog.
 */
void CommerceManager::PurchaseItem(HString sItemID)
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_TRACE("Asked to purchase item '%s'.", sItemID.CStr());

	// Currently, at most one purchase can be in progress at once
	if (!GetItemBeingProcessed().IsEmpty())
	{
		SEOUL_TRACE("Another purchase is currently in progress, cannot purchase '%s'", sItemID.CStr());
		g_pEngineVirtuals->OnItemPurchased(sItemID, EPurchaseResult::kSeoulPurchaseInProgress, nullptr);
		return;
	}

	// Invalid item id.
	ItemInfo* pItemInfo = nullptr;
	if (!m_tItemInfo.GetValue(sItemID, pItemInfo) || nullptr == pItemInfo)
	{
		SEOUL_TRACE("Unknown item '%s', cannot purchase.", sItemID.CStr());
		g_pEngineVirtuals->OnItemPurchased(sItemID, EPurchaseResult::kSeoulUnknownItem, nullptr);
		return;
	}

	// Track the item - compare and set for sanity (although we force main thread interactions
	// everywhere, so this should never happen).
	if (HString().GetHandleValue() != m_sItemBeingPurchased.CompareAndSet(sItemID.GetHandleValue(), HString().GetHandleValue()))
	{
		SEOUL_TRACE("Failed to apply item '%s' for purchase, purchase already in progress?", sItemID.CStr());
		g_pEngineVirtuals->OnItemPurchased(sItemID, EPurchaseResult::kSeoulFailedToSetItem, nullptr);
		return;
	}

	// Pass to platform specific handling for further processing.
	DoPurchaseItem(sItemID, *pItemInfo);
}

/**
 * Convenience that generates a CompletedTransaction object for failure cases, with
 * no other data.
 */
CommerceManager::CompletedTransaction* CommerceManager::MakeFailureObject(const ProductID& productID, CommerceManager::EPurchaseResult eResult) const
{
	// Allocate and return.
	auto pReturn = SEOUL_NEW(MemoryBudgets::Commerce) CompletedTransaction;
	pReturn->m_eResult = eResult;
	pReturn->m_pPurchaseReceiptData.Reset();
	pReturn->m_ProductID = productID;
	pReturn->m_pTransactionObject = nullptr;
	pReturn->m_sTransactionID = String();
	return pReturn;
}


/**
 * Gets the Item ID for a given ProductID.
 *
 * @param[in] Platform-specific item ID
 *
 * @return The game-specific item ID corresponding to the given Product, or the
 *         empty HString if the item is unknown.
 */
HString CommerceManager::GetItemIDForProduct(const ProductID& productID) const
{
	auto pItemInfo = GetItemInfoForProduct(productID);
	if (nullptr == pItemInfo)
	{
		return HString();
	}
	return pItemInfo->m_sID;
}


/**
 * Gets the ItemInfo for a given ProductID.
 *
 * @param[in] Platform-specific item ID
 *
 * @return The ItemInfo for the given Product, or nullptr if the item is unknown.
 */
const CommerceManager::ItemInfo* CommerceManager::GetItemInfoForProduct(ProductID const& productID) const
{
	for (auto const& pair : m_tItemInfo)
	{
		if (pair.Second->m_ProductInfo.m_ProductID == productID)
		{
			return pair.Second;
		}
	}
	return nullptr;
}

/** Must be called by first party when a purchase has completed (success or failure). */
void CommerceManager::OnCompletedTransaction(CompletedTransaction* pCompletedTransaction)
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_TRACE("Received completed transaction ('%s', '%s', %d).",
		pCompletedTransaction->m_ProductID.m_sProductID.CStr(),
		pCompletedTransaction->m_sTransactionID.CStr(),
		(Int)pCompletedTransaction->m_eResult);

	// This can get called on startup if there was a purchase which was
	// never finalized, e.g. due to a crash after the purchase completed but
	// before DoFinishTransactionObject() was called.
	if (GetItemBeingProcessed().IsEmpty())
	{
		// Try to set - only overwrite the empty value, but
		// otherwise ignore the result (we just want there to
		// be an active item before continuing).
		auto const sItemID(GetItemIDForProduct(pCompletedTransaction->m_ProductID));
		if (!sItemID.IsEmpty())
		{
			(void)m_sItemBeingPurchased.CompareAndSet(sItemID.GetHandleValue(), HString().GetHandleValue());
		}
	}

	// TODO: Add to m_vInventory when subscribing. Where?
	// Push the completed transaction into the processing queue
	// for dispatch.
	m_CompletedTransactions.Push(pCompletedTransaction);
}

/** Must be called by first party when new product info has been received. */
void CommerceManager::OnReceiveProductInfo(const ProductInfoVector& v)
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_TRACE("Received %u items of product info.", v.GetSize());

	// Fill in product info.
	Bool bSuccess = false;
	for (auto const& e : v)
	{
		// Get the internal item id - if this fails, don't consume the product info.
		auto const sItemID(GetItemIDForProduct(e.m_ProductID));
		if (sItemID.IsEmpty())
		{
			continue;
		}

		// Acquire the item - if this fails, don't consume the product info.
		ItemInfo* pInfo = nullptr;
		if (!m_tItemInfo.GetValue(sItemID, pInfo) || nullptr == pInfo)
		{
			continue;
		}

		// Update.
		pInfo->m_ProductInfo = e;
		bSuccess = true;
	}

	SEOUL_TRACE("Product info refresh was %s.", (bSuccess ? "successful" : "a failure"));

	// Status.
	m_bReceivedProductsInfo = bSuccess;
	m_bPendingProductInfo = false;

	// Report.
	g_pEngineVirtuals->OnItemInfoRefreshed(bSuccess ? kRefreshSuccess : kRefreshFailure);
}

/**  Must be called by first party when owned DLC products are discovered. */
void CommerceManager::OnReceiveOwnedDLCProducts(const ProductIDVector& v)
{
	SEOUL_ASSERT(IsMainThread());
	m_vOwnedDLC = v;
}

/** Must be called by first party when owned products are discovered. */
void CommerceManager::OnReceiveInventory(const Inventory& v)
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_TRACE("Received %i Transactions in Inventory.", v.GetSize());

	// Update Inventory
	for (auto& p : m_vInventory)
	{
		DoDestroyTransactionObject(p);
		SafeDelete(p);
	}
	m_vInventory.Clear();
	m_vInventory = v;

	m_bReceivedInventory = true;

	g_pEngineVirtuals->OnSubscriptionsReceived();
}

static inline Bool operator==(CommerceManager::CompletedTransaction const* a, const String& sTransactionID)
{
	return (a->m_sTransactionID == sTransactionID);
}

CommerceManager::CompletedTransaction::CompletedTransaction() = default;
CommerceManager::CompletedTransaction::~CompletedTransaction()
{
	// Must have been finished prior to our destruction.
	SEOUL_ASSERT(nullptr == m_pTransactionObject);
}

/**
 * Gets the table containing information about all known items
 */
const CommerceManager::ItemInfoTable& CommerceManager::GetItemInfoTable() const
{
	return m_tItemInfo;
}

/**
 * Gets the list of all known platform-specific item IDs
 *
 * @param[out] vOutItemIDs Receives the list of all known platform-specific
 *               item IDs
 */
void CommerceManager::GetAllPlatformItemIDs(Vector<ProductID>& vOutItemIDs) const
{
	vOutItemIDs.Clear();
	vOutItemIDs.Reserve(m_tItemInfo.GetSize());

	for (auto const& pair : m_tItemInfo)
	{
		vOutItemIDs.PushBack(pair.Second->m_ProductInfo.m_ProductID);
	}
}

/**
 * Gets the list of all owned DLC platform-specific item IDs
 *
 * @return Vector of all owned DLC Product Ids
 */
const CommerceManager::ProductIDVector& CommerceManager::GetOwnedDLCPlatformItemIDs() const
{
	return m_vOwnedDLC;
}

/**
 * Gets information about the given item
 *
 * @param[in] sItemID Game-specific item ID
 *
 * @return A pointer to item-specific information, or nullptr if the item is not
 *         valid.  Not all of the fields in the returned object are necessarily
 *         valid, depending on the current platform.
 */
const CommerceManager::ItemInfo *CommerceManager::GetItemInfo(HString sItemID) const
{
	ItemInfo* pReturn = nullptr;
	(void)m_tItemInfo.GetValue(sItemID, pReturn);
	return pReturn;
}

/**
 * Gets the price of the given item in the user's local currency
 *
 * @param[in] sItemID Game-specific item ID
 * @param[out] sOutPrice Receives the price of the item in the user's local
 *               currency
 *
 * @return True if the item is valid and its price is known, or false otherwise
 */
Bool CommerceManager::GetItemPrice(HString sItemID, String& rsOutPrice) const
{
	auto const pItemInfo = GetItemInfo(sItemID);
	if (pItemInfo != nullptr)
	{
		if (!pItemInfo->m_ProductInfo.m_sPrice.IsEmpty())
		{
			rsOutPrice = pItemInfo->m_ProductInfo.m_sPrice;
			return true;
		}
	}

	return false;
}

/**
 * Gets information about the first finalizing transaction, if one exists.
 * Through out parameters, sets information provided by the OnItemPurchased call.
 *
 * @param[out] itemID Item ID of the purchased item.
 * @param[out] eResult Purchase result.
 * @param[out] pReceitData PurchaseReceitData containing information from the
 *               first-party store regarding the purchase.
 *
 * @return Whether or not there is a finalizing transaction.
 */
Bool CommerceManager::GetFirstFinalizingTransaction(HString& itemID, EPurchaseResult& eResult, PurchaseReceiptData const*& pReceiptData) const
{
	SEOUL_ASSERT(IsMainThread());

	if (m_vFinalizingTransactions.GetSize() == 0)
	{
		return false;
	}
	auto pTransaction = m_vFinalizingTransactions.Front();
	if (nullptr == pTransaction)
	{
		return false;
	}

	itemID = GetItemIDForProduct(pTransaction->m_ProductID);
	eResult = pTransaction->m_eResult;
	pReceiptData = pTransaction->m_pPurchaseReceiptData.Get();

	return true;
}

/**
 * Gets information relating to a Subscription purchase for the given Item Id.
 *
 * @param[in] itemID The If of the Item to check the Subscription purchase information for.
 * @param[out] eResult The the result of the Subscription transaction, if a subscription exists.
 * @param[out] pReceiptData The PurcahseReceiptData of the Subscription transaction, if a subscription exists.
 *
 * @return Whether or not a Subscription is active for the given Item Id.
 */
Bool CommerceManager::GetSubscription(HString itemID, EPurchaseResult& eResult, PurchaseReceiptData const*& pReceiptData) const
{
	SEOUL_ASSERT(IsMainThread());
	if (!IsItemOfType(itemID, kItemTypeSubscription))
	{
		return false;
	}
	for (auto p : m_vInventory)
	{
		if (itemID == GetItemIDForProduct(p->m_ProductID))
		{
			eResult = p->m_eResult;
			pReceiptData = p->m_pPurchaseReceiptData.Get();
			return true;
		}
	}
	return false;
}

/**
 * Convenience method to determine if an item exists and is of a given type.
 *
 * @param[in] itemID The Id of the item to check the tpe of.
 * @param[in] eType The type to check.
 *
 * @return Whether or not an item with given Id exists and is of the given type.
 */
Bool CommerceManager::IsItemOfType(HString itemID, ItemType eType) const
{
	auto pItemInfo = GetItemInfo(itemID);
	return nullptr != pItemInfo && pItemInfo->m_eType == eType;
}

/**
 * Gets an estimated USD price for a product given its local price and ISO 4217 currency code.
 * Determines value based on ExchangeRates table.
 *
 * @param[in] productID first-party product Id
 * @param[in] fLocalPrice price in local currency
 * @param[in] currencyCode ISO 4217 local currency code
 *
 * @return estimated USD price. Returns 0.f if no price could be determined.
 */
Float CommerceManager::EstimateUSDPrice(ProductID const& productID, Float fLocalPrice, HString currencyCode) const
{
	// Attempt to find the exchange rate for the given currency code
	const Float* pExchangeRate = m_tExchangeRates.Find(currencyCode);
	if (nullptr != pExchangeRate)
	{
		return *pExchangeRate * fLocalPrice;
	}

	// Failing that, attempt to use the ItemInfo's USD value
	HString itemID = GetItemIDForProduct(productID);
	ItemInfo* const* ppItemInfo = m_tItemInfo.Find(itemID);
	if (nullptr != ppItemInfo)
	{
		return 0.f;
	}

	ItemInfo* const pItemInfo = *ppItemInfo;
	return pItemInfo->m_iUSDCentsPrice * 100.f;
}

/**
 * Formats the given price using the given currency's symbol
 *
 * @param[in] uPriceInSmallestUnits Price amount to format, measured in the
 *              smallest subunit of the given currency (e.g. cents for USD)
 * @param[in] sCurrency 3-letter ISO 4217 currency code of the currency to use
 * @param[out] pOutPrice If non-null, receives the floating-point value of the
 *               given currency amount in terms of the base unit (e.g. converts
 *               cents into USD)
 *
 * @return The given currency amount formatted as appropriate for the given
 *         currency, e.g. "$123.45" if the currency is USD
 */
String CommerceManager::FormatPrice(UInt64 uPriceInSmallestUnits, const String& sCurrency, Float *pOutPrice)
{
	// TODO: Format according to the appropriate locale (i.e. use the proper
	// decimal separator & thousands separator, etc.)
	static const struct CurrencyInfo
	{
		const char *sCurrencyCode;
		const char *sCurrencySymbol;
		bool bSymbolBeforeAmount;
		int nDigitsAfterDecimalPoint;
	} kaCurrencyInfo[] =
	  {
		  {"AUD", "$",            true,  2},  // Australian dollar
		  {"BRL", "R$",           true,  2},  // Brazilian real
		  {"CAD", "$",            true,  2},  // Canadian dollar
		  {"EUR", "\xE2\x82\xAC", true,  2},  // Euro (U+20AC)
		  {"GBP", "\xC2\xA3",     true,  2},  // Pound sterling (U+00A3)
		  {"JPY", "\xC2\xA5",     true,  0},  // Japanese yen (U+00A5)
		  {"MXN", "$",            true,  2},  // Mexican peso
		  {"NZD", "$",            true,  2},  // New Zealand dollar
		  {"RUB", " RUB",         false, 2},  // Russian ruble
		  {"USD", "$",            true,  2},  // United States dollar
		  // TODO: Support more currencies
	  };

	// Look up the currency in our data table
	const CurrencyInfo *pCurrencyInfo = nullptr;
	for (UInt i = 0; i < SEOUL_ARRAY_COUNT(kaCurrencyInfo); i++)
	{
		if (sCurrency.CompareASCIICaseInsensitive(kaCurrencyInfo[i].sCurrencyCode) == 0)
		{
			pCurrencyInfo = &kaCurrencyInfo[i];
			break;
		}
	}

	if (pCurrencyInfo == nullptr)
	{
		SEOUL_WARN("Unknown or unsupported currency: %s", sCurrency.CStr());

		// Assume two decimal places if we don't support the currency
		if (pOutPrice != nullptr)
		{
			*pOutPrice = (Float)uPriceInSmallestUnits / 100.0f;
		}
		return String::Printf("%" PRIu64 ".%02u %s", uPriceInSmallestUnits / 100, (int)(uPriceInSmallestUnits % 100), sCurrency.CStr());
	}

	// Format the price according to the number of units of the currency's
	// subunit in the base unit
	static const UInt kaPowersOf10[] = {1, 10, 100, 1000, 10000};

	SEOUL_ASSERT(pCurrencyInfo->nDigitsAfterDecimalPoint < (Int)SEOUL_ARRAY_COUNT(kaPowersOf10));
	UInt kPowerOf10 = kaPowersOf10[pCurrencyInfo->nDigitsAfterDecimalPoint];
	UInt64 uBaseUnits = uPriceInSmallestUnits / kPowerOf10;
	UInt uSubunits = (UInt)(uPriceInSmallestUnits % kPowerOf10);

	if (pOutPrice != nullptr)
	{
		*pOutPrice = (Float)uPriceInSmallestUnits / (Float)kPowerOf10;
	}

	if (pCurrencyInfo->bSymbolBeforeAmount)
	{
		if (pCurrencyInfo->nDigitsAfterDecimalPoint > 0)
		{
			return String::Printf(
				"%s%" PRIu64 ".%02u",
				pCurrencyInfo->sCurrencySymbol,
				uBaseUnits,
				uSubunits);
		}
		else
		{
			return String::Printf(
				"%s%" PRIu64,
				pCurrencyInfo->sCurrencySymbol,
				uBaseUnits);
		}
	}
	else
	{
		if (pCurrencyInfo->nDigitsAfterDecimalPoint > 0)
		{
			return String::Printf(
				"%" PRIu64 ".%02u%s",
				uBaseUnits,
				uSubunits,
				pCurrencyInfo->sCurrencySymbol);
		}
		else
		{
			return String::Printf(
				"%" PRIu64 "%s",
				uBaseUnits,
				pCurrencyInfo->sCurrencySymbol);
		}
	}
}

HString CommerceManager::GetCommercePlatformId() const
{
#if SEOUL_PLATFORM_LINUX // TODO: Annoying to need to keep making this exception.
	return kAndroid;
#else
	if (Engine::Get()->IsSamsungPlatformFlavor())
	{
		return kSamsung;
	}
	else if (Engine::Get()->IsAmazonPlatformFlavor())
	{
		return kAmazon;
	}
	else
	{
		return HString(GetCurrentPlatformName());
	}
#endif // /#if SEOUL_PLATFORM_LINUX
}

/**
 * Handle vtable - constructor called outside of the constructor.
 */
void CommerceManager::Initialize()
{
	SEOUL_ASSERT(IsMainThread());

	m_bReceivedInventory = false;

	// Load our microtransaction data and force
	// a refresh the next time Tick() is called.
	LoadItemInfoFromJson();
	m_iNextRefreshTimeInTicks = 0;

	// Let first-party populate Owned DLC Products
	DoPopulateOwnedDLCProducts();
}

/**
 * Handle vtable - destructor called outside of the destructor.
 */
void CommerceManager::Shutdown()
{
	SEOUL_ASSERT(IsMainThread());

	// Cleanup.
	SafeDeleteTable(m_tItemInfo);

	// Release any not delivered transactions.
	auto pNext = m_CompletedTransactions.Pop();
	while (nullptr != pNext)
	{
		// Cleanup.
		DoDestroyTransactionObject(pNext);
		SafeDelete(pNext);

		// Advance.
		pNext = m_CompletedTransactions.Pop();
	}

	// Release any unfinalized transactions.
	for (auto& p : m_vFinalizingTransactions)
	{
		// Cleanup.
		DoDestroyTransactionObject(p);
		SafeDelete(p);
	}
	m_vFinalizingTransactions.Clear();

	// Release inventory
	for (auto& p : m_vInventory)
	{
		// Cleanup.
		DoDestroyTransactionObject(p);
		SafeDelete(p);
	}
	m_vInventory.Clear();
}

const String& NullCommerceManager::GetStoreName() const
{
	return s_kNullCommerceManagerStoreName;
}

void NullCommerceManager::DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	pCompletedTransaction->m_pTransactionObject = nullptr;
}

void NullCommerceManager::DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	pCompletedTransaction->m_pTransactionObject = nullptr;
}

void NullCommerceManager::DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo)
{
	OnCompletedTransaction(MakeFailureObject(itemInfo.m_ProductInfo.m_ProductID, EPurchaseResult::kResultFailure));
}

void NullCommerceManager::DoRefresh()
{
	// Default implementation is a refresh failure
	OnReceiveProductInfo(ProductInfoVector());
}

} // namespace Seoul
