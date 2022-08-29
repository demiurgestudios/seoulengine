/**
 * \file IOSCommerceManagerObjC.h
 * \brief Implementation of the SKProductsRequestDelegate and
 * SKPaymentTransactionObserver API to interact with iOS first-party.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IOS_COMMERCE_MANAGER_OBJC_H
#define IOS_COMMERCE_MANAGER_OBJC_H

#import <StoreKit/StoreKit.h>
#include "CommerceManager.h"

/**
 * Objective-C class for managing iOS commerce
 */
@interface IOSCommerceManagerObjC : NSObject<SKProductsRequestDelegate, SKPaymentTransactionObserver>
{
}

// Requests the list of known products from the iTunes store
- (BOOL)requestProductList;

// Gets the SKProduct object corresponding to the given product ID
- (SKProduct *)productForProductID:(NSString *)sProductID;

// SKProductsRequestDelegate methods
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response;

// SKRequestDelegate methods
- (void)requestDidFinish:(SKRequest *)request;
- (void)request:(SKRequest *)request didFailWithError:(NSError *)error;

// SKPaymentTransactionObserver methods
- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions;
- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray *)transactions;
- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error;
- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue;

// Utilities.
- (Seoul::CommerceManager::EPurchaseResult)skErrorToPurchaseResult:(NSError *)error;
@end

#endif // include guard
