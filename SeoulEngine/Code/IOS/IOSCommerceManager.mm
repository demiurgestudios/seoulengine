/**
 * \file IOSCommerceManager.mm
 * \brief Specialization of CommerceManager for the IOS platform.
 * Wraps iOS platform IAP functionality and binds SeoulEngine into
 * the AppStore.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EngineVirtuals.h"
#include "JobsFunction.h"
#include "IOSCommerceManager.h"
#include "IOSEngine.h"
#include "Logger.h"
#include "MemoryBarrier.h"

#import "IOSCommerceManagerObjC.h"

namespace Seoul
{

// Constants for filling out receipt data.
static const String ksAppleAppStore("AppleAppStore");

IOSCommerceManager::IOSCommerceManager()
	: m_pCommerceObjC(nil)
{
	SEOUL_ASSERT(IsMainThread());

	// Initialize our commerce object.
	m_pCommerceObjC = [[IOSCommerceManagerObjC alloc] init];
	[[SKPaymentQueue defaultQueue] addTransactionObserver:m_pCommerceObjC];
}

IOSCommerceManager::~IOSCommerceManager()
{
	SEOUL_ASSERT(IsMainThread());

	// Cleanup commerce object.
	[[SKPaymentQueue defaultQueue] removeTransactionObserver:m_pCommerceObjC];
	[m_pCommerceObjC release];
	m_pCommerceObjC = nil;
}

const String& IOSCommerceManager::GetStoreName() const
{
	return ksAppleAppStore;
}

/**
 * Callback called when a transaction has completed successfully
 */
void IOSCommerceManager::OnTransactionCompleted(SKPaymentTransaction* pTransaction, Bool bSuccess)
{
	SEOUL_ASSERT(IsMainThread());

	// Convert the transaction into a tracking object.
	auto pCompletedTransaction = ConvertTransaction(pTransaction, bSuccess);

	// Dispatch.
	OnCompletedTransaction(pCompletedTransaction);
}

/**
 * Sets our product info table from an NSArray of SKProduct instances
 */
void IOSCommerceManager::SetProductsInfo(NSArray *aProductsInfo)
{
	SEOUL_ASSERT(IsMainThread());

	UInt32 const uSize = (UInt32)([aProductsInfo count]);
	ProductInfoVector vProductInfo;
	vProductInfo.Reserve(uSize);
	for (SKProduct *product in aProductsInfo)
	{
		// Format the item price
		//
		// TODO: Use the ISO code (e.g. "USD") instead of the symbol ("$") if
		// we want to guarantee that we have the right characters in our font
		// for all currencies
		//
		// Allocate.
		NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
		[numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
		[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
		[numberFormatter setLocale:product.priceLocale];

		// Acquire.
		NSString *sPrice = [numberFormatter stringFromNumber:product.price];

		String sCurrencyCode;
		NSString *sNSCurrencyCode = [product.priceLocale objectForKey:NSLocaleCurrencyCode];
		if (sNSCurrencyCode != nil)
		{
			sCurrencyCode = String([sNSCurrencyCode UTF8String]).ToUpperASCII();
		}
		
		// Set the product info.
		ProductInfo info;
		
		// Apple only added subscription information in SDK 11.2. Prior to this version Subscriptions
		// existed, but it is up to the developer to determine and display the appropriate information.
		// Product Type and Subscription information are defined in microtransactions.json.
		
		info.m_fPrice = (Float)[product.price doubleValue]; // store raw price. Used for sorting in UI display
		info.m_ProductID = ProductID([product.productIdentifier UTF8String]);
		info.m_sDescription = String([product.localizedDescription UTF8String]);
		info.m_sName = String([product.localizedDescription UTF8String]);
		info.m_sPrice = String([sPrice UTF8String]);
		info.m_sCurrencyCode = sCurrencyCode;
		info.m_fUSDPrice = EstimateUSDPrice(info.m_ProductID, info.m_fPrice, HString(info.m_sCurrencyCode));
		vProductInfo.PushBack(info);

		// Cleanup.
		[numberFormatter release];
	}

	// Deliver.
	OnReceiveProductInfo(vProductInfo);

	// Cleanup.
	[aProductsInfo release];
}

void IOSCommerceManager::DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	SEOUL_ASSERT(IsMainThread());

	SKPaymentTransaction* pTransactionObject = (SKPaymentTransaction*)pCompletedTransaction->m_pTransactionObject;
	pCompletedTransaction->m_pTransactionObject = nullptr;

	if (nil != pTransactionObject)
	{
		// See comment on DoDestroyTransactionObject() - do not finish. Shutdown, want to leave for processing next run.
		[pTransactionObject release];
		pTransactionObject = nil;
	}
}

void IOSCommerceManager::DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	SEOUL_ASSERT(IsMainThread());

	SKPaymentTransaction* pTransactionObject = (SKPaymentTransaction*)pCompletedTransaction->m_pTransactionObject;
	pCompletedTransaction->m_pTransactionObject = nullptr;

	if (nil != pTransactionObject)
	{
		[[SKPaymentQueue defaultQueue] finishTransaction:pTransactionObject];
		[pTransactionObject release];
		pTransactionObject = nil;
	}
}

void IOSCommerceManager::DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo)
{
	SEOUL_ASSERT(IsMainThread());

	if (![SKPaymentQueue canMakePayments])
	{
		SEOUL_LOG_COMMERCE("PurchaseItem: User is not authorized to make payments\n");
		OnCompletedTransaction(MakeFailureObject(itemInfo.m_ProductInfo.m_ProductID, EPurchaseResult::kResultCantMakePayments));
		return;
	}

	// Get the SKProduct for the item.
	SKProduct* product = [m_pCommerceObjC productForProductID:[NSString stringWithUTF8String:itemInfo.m_ProductInfo.m_ProductID.m_sProductID.CStr()]];
	if (nil == product)
	{
		SEOUL_LOG_COMMERCE("Can't retrieve purchase info for product %s from first party\n", itemInfo.m_ProductInfo.m_ProductID.m_sProductID.CStr());
		OnCompletedTransaction(MakeFailureObject(itemInfo.m_ProductInfo.m_ProductID, EPurchaseResult::kPlatformSpecificError1));
		return;
	}

	// Queue up a payment
	SKPayment* payment = [SKPayment paymentWithProduct:product];
	[[SKPaymentQueue defaultQueue] addPayment:payment];
}

void IOSCommerceManager::DoRefresh()
{
	SEOUL_ASSERT(IsMainThread());

	// Populate our product list. Immediate failure
	// if we fail to start the refresh.
	if (![m_pCommerceObjC requestProductList])
	{
		OnReceiveProductInfo(ProductInfoVector());
	}
}

IOSCommerceManager::CompletedTransaction* IOSCommerceManager::ConvertTransaction(
	SKPaymentTransaction* pCompletedTransaction,
	Bool bSuccess) const
{
	SEOUL_ASSERT(IsMainThread());

	// Acquire result.
	auto eResult = EPurchaseResult::kResultSuccess;
	if (bSuccess && pCompletedTransaction.transactionState == SKPaymentTransactionStatePurchased)
	{
		SEOUL_LOG_COMMERCE("Transaction succeeded\n");
		eResult = EPurchaseResult::kResultSuccess;
	}
	else if (pCompletedTransaction.error.code == SKErrorPaymentCancelled)
	{
		SEOUL_LOG_COMMERCE("Transaction was canceled by the user\n");
		eResult = EPurchaseResult::kResultCanceled;
	}
	else
	{
		SEOUL_LOG_COMMERCE("Transaction failed: %s\n", [[pCompletedTransaction.error localizedDescription] UTF8String]);
		eResult = [m_pCommerceObjC skErrorToPurchaseResult:pCompletedTransaction.error];
	}

	// Populate receipt data on success.
	String sTransactionID;
	ScopedPtr<PurchaseReceiptData> pPurchaseReceiptData;
	if (eResult == EPurchaseResult::kResultSuccess)
	{
		// Gather transaction ID.
		sTransactionID = String(pCompletedTransaction.transactionIdentifier);

		// Gather receipt data.

		// As of December 2018, receipt data returned for "Ask to Buy" purchases has an empty "in_app" property. As such,
		// our server cannot distinguish these purchases from invalid/cheater purchases.
		//
		// We pinged Apple support but were told (in short) "this is not a bug report, please file a bug report" with no
		// helpful info beyond what they've said publically:
		// - https://forums.developer.apple.com/thread/104536
		// - https://forums.developer.apple.com/thread/85380
		//
		// As such, we've decided not to file a bug report (given that Unity, at least, did so back in 2017
		// and nothing has since been fixed or changed), and also as such, we've decided to workaround this
		// issue in the same way that Unity Technologies (apparently) has:
		// https://forum.unity.com/threads/solved-ask-to-buy-empty-in_app-array-and-refreshing-receipt-flow.498704/
		//
		// Even though it's deprecated, we send the old transactionReceipt field as a "Payload2" to the server. The
		// server will then fall back to this receipt if the primary receipt fails due to a missing "in_app" property.

		// As above comment, always store the (now deprecated) transactionReceipt in payload 2.
#		pragma GCC diagnostic push
#		pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		String sPayload2;
		if (nil != pCompletedTransaction.transactionReceipt)
		{
			sPayload2 = Base64Encode(
				(Byte const*)pCompletedTransaction.transactionReceipt.bytes,
				(UInt32)pCompletedTransaction.transactionReceipt.length);
		}
#		pragma GCC diagnostic pop

		String sPayload;

		// If we support less than 7, check if below 7. For environments less than 7,
		// payload 1 is just equal to payload 2.
#if __IPHONE_OS_VERSION_MIN_REQUIRED < 70000 // Support for projects targeting less than iOS 7.0.
		if ([[[UIDevice currentDevice] systemVersion] floatValue] < 7.0)
		{
			sPayload = sPayload2;
		}
		else
#endif // /#if __IPHONE_OS_VERSION_MIN_REQUIRED < 70000
		{
			// If at or above 7, the primary payload is the newer "Grand Unified Receipt"
			// available through the appStoreReceiptURL property.

			// pReceiptURL is auto-released.
			NSURL* pReceiptURL = [[NSBundle mainBundle] appStoreReceiptURL];

			// pReceiptData is auto-released. see http://stackoverflow.com/questions/6514319/
			NSData* pReceiptData = [NSData dataWithContentsOfURL:pReceiptURL];

			// Get order token.
			if (nil != pReceiptData)
			{
				// Result of baes64EncodedStringWithOptions is auto-released.
				sPayload.Assign([[pReceiptData base64EncodedStringWithOptions:0] UTF8String]);
			}
		}

		// Gather and assign receipt data.
		if (!sTransactionID.IsEmpty())
		{
			// Fill out receipt data and assign for submission.
			pPurchaseReceiptData.Reset(SEOUL_NEW(MemoryBudgets::Commerce) PurchaseReceiptData);

			// Fill out.
			pPurchaseReceiptData->m_sPayload = sPayload;
			pPurchaseReceiptData->m_sPayload2 = sPayload2;
			pPurchaseReceiptData->m_sStore = GetStoreName();
			pPurchaseReceiptData->m_sTransactionID = sTransactionID;
		}
	}

	// Allocate and return.
	auto pReturn(SEOUL_NEW(MemoryBudgets::Commerce) CompletedTransaction);
	pReturn->m_eResult = eResult;
	pReturn->m_pPurchaseReceiptData.Swap(pPurchaseReceiptData);
	pReturn->m_ProductID = ProductID([pCompletedTransaction.payment.productIdentifier UTF8String]);
	[pCompletedTransaction retain];
	pReturn->m_pTransactionObject = pCompletedTransaction;
	pReturn->m_sTransactionID = sTransactionID;

	// Done.
	return pReturn;
}

} // namespace Seoul
