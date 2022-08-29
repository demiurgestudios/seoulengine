/**
 * \file IOSCommerceManagerObjC.mm
 * \brief Implementation of the SKProductsRequestDelegate and
 * SKPaymentTransactionObserver API to interact with iOS first-party.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "JobsFunction.h"
#include "IOSCommerceManager.h"
#include "IOSEngine.h"
#include "Logger.h"
#include "Prereqs.h"
#include "ThreadId.h"

#import "IOSCommerceManagerObjC.h"

using namespace Seoul;

@implementation IOSCommerceManagerObjC

/** Mutex for product request. */
NSLock* m_Mutex = [[NSLock alloc] init];

/** Array of SKProductInfo of info about all of the known products. */
NSArray* m_Products = nil;

/** Reference to an active request, so we can cancel it if we're destroyed before it completes. */
SKProductsRequest* m_Request = nil;

/** Set to true when any of the request completion paths have been encountered. Indicates a new request can be started. */
BOOL m_bRequestComplete = NO;

/**
 * Dealloc hook to cancel a dangling request.
 */
- (void)dealloc
{
	[m_Mutex lock]; // Synchronize.

	// Cleanup our reference to products if we still have one.
	if (nil != m_Products)
	{
		[m_Products release];
		m_Products = nil;
	}

	// Cleanup remaining request object.
	if (nil != m_Request)
	{
		// Don't dangle - see also: https://discussions.apple.com/thread/5814621
		m_Request.delegate = nil;
		if (!m_bRequestComplete)
		{
			[m_Request cancel];
		}
		m_bRequestComplete = NO;
		[m_Request release];
		m_Request = nil;
	}

	// Release the mutex.
	[m_Mutex unlock]; // Synchronize.
	[m_Mutex release];
	m_Mutex = nil;

	[super dealloc];
}

/**
 * Initalizes our internal product list by making a request to the iTunes
 * Connect server for product info
 */
- (BOOL)requestProductList
{
	auto const& tItemInfo(CommerceManager::Get()->GetItemInfoTable());

	if (tItemInfo.IsEmpty())
	{
		SEOUL_LOG_COMMERCE("-[IOSCommerceManagerObjC initProductList]: No known products, skipping products request\n");
		return NO;
	}

	[m_Mutex lock]; // Synchronize.

	// Check for existing request, cleanup if possible Otherwise, fail.
	if (nil != m_Request)
	{
		// Early out if the request has not finished yet.
		if (!m_bRequestComplete)
		{
			SEOUL_LOG_COMMERCE("-[IOSCommerceManagerObjC initProductList]: Request already pending, skipping.");
			[m_Mutex unlock]; // Synchronize.
			return NO;
		}

		// Don't dangle - see also: https://discussions.apple.com/thread/5814621
		m_Request.delegate = nil;

		// Cleanup request state.
		m_bRequestComplete = NO;
		[m_Request release];
		m_Request = nil;
	}

	// Gather the list of all product IDs from the item info table
	NSMutableSet* pProductIDs = [NSMutableSet setWithCapacity:tItemInfo.GetSize()];
	for (auto const& pair : tItemInfo)
	{
		NSString* sProductID = pair.Second->m_ProductInfo.m_ProductID.m_sProductID.ToNSString();
		[pProductIDs addObject:sProductID];
	}

	SEOUL_LOG_COMMERCE("-[IOSCommerceManagerObjC initProductList]: Requesting product info for %u products\n", (UInt)[pProductIDs count]);

	// Setup the request.
	m_bRequestComplete = NO; // If we get here, we're about to start waiting for a new request to complete.
	m_Request = [[SKProductsRequest alloc] initWithProductIdentifiers:pProductIDs];
	m_Request.delegate = self;

	// Retain so we can safely start outside the mutex.
	SKProductsRequest* pRequest = m_Request;
	[pRequest retain];
	[m_Mutex unlock]; // Synchronize.

	// Now outside the mutex (in case [start] returns immediately),
	// start and release.
	[pRequest start];
	[pRequest release];

	// Done, started successful.
	return YES;
}

/**
 * Gets the SKProduct object corresponding to the given product ID
 */
- (SKProduct*)productForProductID:(NSString *)sProductID
{
	[m_Mutex lock]; // Synchronize

	// Early out if we have no products.
	if (nil == m_Products)
	{
		[m_Mutex unlock]; // Synchronize
		return nil;
	}

	// Iterate and search.
	for (SKProduct* product in m_Products)
	{
		if ([product.productIdentifier isEqualToString:sProductID])
		{
			[m_Mutex unlock]; // Synchronize
			return product;
		}
	}

	[m_Mutex unlock]; // Synchronize
	return nil;
}

/**
 * Callback called when we receive a response about our products info request
 */
- (void)productsRequest:(SKProductsRequest*)request didReceiveResponse:(SKProductsResponse*)response
{
#if SEOUL_LOGGING_ENABLED
	if (!IOSEngine::Get()->GetSettings().m_bEnterpriseBuild)
	{
		if ([response.invalidProductIdentifiers count] > 0)
		{
			String sWarnStr = String::Printf("IOSCommerceManager: %u invalid product identifiers:\n", (UInt)[response.invalidProductIdentifiers count]);
			for (NSString *sProductID in response.invalidProductIdentifiers)
			{
				sWarnStr.Append(String::Printf("  %s\n", [sProductID UTF8String]));
			}

			SEOUL_WARN("%s", sWarnStr.CStr());
		}
	}

	SEOUL_LOG_COMMERCE("Received product info for %u products\n", (UInt)[response.products count]);
#endif

	[m_Mutex lock]; // Synchronize

	// Cleanup existing products if we have them.
	if (nil != m_Products)
	{
		[m_Products release];
		m_Products = nil;
	}

	// Keep track of the products array.
	m_Products = response.products;
	[m_Products retain];

	// Request has now completed. Set this before we (potentially) escaped the selector.
	m_bRequestComplete = YES;

	// Retain for the call below.
	[response.products retain];

	[m_Mutex unlock]; // Synchronize

	// Tell the game to process the products list on the main thread
	Jobs::AsyncFunction(
		GetMainThreadId(),
		SEOUL_BIND_DELEGATE(&IOSCommerceManager::SetProductsInfo, IOSCommerceManager::Get().Get()),
		response.products);
}

/**
 * Callback called after the products request finishes, either after success or
 * after failure.
 */
- (void)requestDidFinish:(SKRequest *)request
{
	SEOUL_LOG_COMMERCE("Products request finished\n");

	// Request has now completed.
	[m_Mutex lock]; // Synchronize.
	m_bRequestComplete = YES;
	[m_Mutex unlock]; // Synchronize.
}

/**
 * Callback called if an error occurs during the products request
 */
- (void)request:(SKRequest*)request didFailWithError:(NSError*)error
{
	SEOUL_LOG_COMMERCE("Products request failed: %s\n", [[error localizedDescription] UTF8String]);

	[m_Mutex lock]; // Synchronize.

	// Cleanup existing products if we have them.
	if (nil != m_Products)
	{
		[m_Products release];
		m_Products = nil;
	}

	// Keep track of the products array.
	NSArray* pEmptyProductList = @[];
	m_Products = pEmptyProductList;
	[m_Products retain];

	// Request has now completed. Set this before we (potentially) escaped the selector.
	m_bRequestComplete = YES;

	// Retain for the call below.
	[pEmptyProductList retain];

	[m_Mutex unlock]; // Synchronize.

	// Pass an empty array of product info to the commerce manager, which we
	// process on the main thread.
	Jobs::AsyncFunction(
		GetMainThreadId(),
		SEOUL_BIND_DELEGATE(&IOSCommerceManager::SetProductsInfo, IOSCommerceManager::Get().Get()),
		pEmptyProductList);
}

/**
 * Callback called when one or more transactions have been updated
 */
- (void)paymentQueue:(SKPaymentQueue*)queue updatedTransactions:(NSArray*)transactions
{
	auto pCommerceManager(IOSCommerceManager::Get());

	// Inform the commerce manager about each updated transaction
	for (SKPaymentTransaction* transaction in transactions)
	{
		switch (transaction.transactionState)
		{
			// TODO: For SKPaymentTransactionStateDeferred, we are supposed
			// to allow the user to continue using the application.
		case SKPaymentTransactionStateDeferred: // fall-through
		case SKPaymentTransactionStatePurchasing:
			// No-op
			break;

		case SKPaymentTransactionStatePurchased:
			pCommerceManager->OnTransactionCompleted(transaction, true);
			break;
		case SKPaymentTransactionStateFailed:
			pCommerceManager->OnTransactionCompleted(transaction, false);
			break;

		case SKPaymentTransactionStateRestored:
			SEOUL_WARN("[Commerce] Restoring transactions is not yet supported");
			[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
			break;

		default:
			SEOUL_WARN("Invalid transaction state: %d", (Int)transaction.transactionState);
			break;
		}
	}
}

/**
 * Callback called when one or more transactions have been removed from the
 * queue
 */
- (void)paymentQueue:(SKPaymentQueue*)queue removedTransactions:(NSArray*)transactions
{
	// Nop.
}

/**
 * Callback called when an error occurred while restoring transactions.
 */
- (void)paymentQueue:(SKPaymentQueue*)queue restoreCompletedTransactionsFailedWithError:(NSError *)error
{
	SEOUL_WARN("-[IOSCommerceManagerObjC paymentQueue:restoreCompletedTransactionsFailedWithError:] not implemented");
}

/**
 * Callback called when the payment queue has finished sending restored
 * transactions.
 */
- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue*)queue
{
	SEOUL_WARN("-[IOSCommerceManagerObjC paymentQueueRestoreCompletedTransactionsFinished:] not implemented");
}

/**
 * Convert the SKError code that we get because of a transaction issue into something our engine can
 * interpret.
 *
 * @param error NSError from a SKPaymentTransaction
 * @return corresponding CommerceManager::EPurchaseResult for the given error.
 */
- (CommerceManager::EPurchaseResult)skErrorToPurchaseResult:(NSError*)error
{
	if (error != nil)
	{
		switch(error.code)
		{
			case SKErrorUnknown:
				return CommerceManager::EPurchaseResult::kInternalPlatformError;
			case SKErrorClientInvalid:
				return CommerceManager::EPurchaseResult::kResultClientInvalid;
			case SKErrorPaymentCancelled:
				return CommerceManager::EPurchaseResult::kResultCanceled;
			case SKErrorPaymentInvalid:
				return CommerceManager::EPurchaseResult::kResultPaymentInvalid;
			case SKErrorPaymentNotAllowed:
				return CommerceManager::EPurchaseResult::kResultCantMakePayments;
			case SKErrorStoreProductNotAvailable:
				return CommerceManager::EPurchaseResult::kResultProductUnavailable;
			default:
				return CommerceManager::EPurchaseResult::kResultFailure;
		}
	}

	return CommerceManager::EPurchaseResult::kResultFailure;
}

@end  // @implementation IOSCommerceManagerObjC
