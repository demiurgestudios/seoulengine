/**
 * \file IOSCommerceManager.h
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

#pragma once
#ifndef IOS_COMMERCE_MANAGER_H
#define IOS_COMMERCE_MANAGER_H

#include "Atomic32.h"
#include "CommerceManager.h"
#include "SharedPtr.h"
#ifdef __OBJC__
@class NSArray;
@class SKPaymentTransaction;
@class IOSCommerceManagerObjC;
#else
typedef struct NSArray NSArray;
typedef struct SKPaymentTransaction SKPaymentTransaction;
typedef struct IOSCommerceManagerObjC IOSCommerceManagerObjC;
#endif

namespace Seoul
{

/**
 * Implementation of CommerceManager for the iOS platform.
 */
class IOSCommerceManager SEOUL_SEALED : public CommerceManager
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<IOSCommerceManager> Get()
	{
		if (CommerceManager::Get() && CommerceManager::Get()->GetType() == CommerceManagerType::kIOS)
		{
			return (IOSCommerceManager*)CommerceManager::Get().Get();
		}
		else
		{
			return CheckedPtr<IOSCommerceManager>();
		}
	}

	IOSCommerceManager();
	~IOSCommerceManager();

	virtual CommerceManagerType GetType() const SEOUL_OVERRIDE
	{
		return CommerceManagerType::kIOS;
	}

	virtual const String& GetStoreName() const SEOUL_OVERRIDE;
	virtual Bool SupportsSubscriptions() const SEOUL_OVERRIDE { return false; } // TODO: Implement Subscriptions

	// Callback functions from first party.

	// Callback called when a transaction has completed (successfully or
	// unsuccessfully)
	void OnTransactionCompleted(SKPaymentTransaction* pTransaction, Bool bSuccess);

	// Sets our product info table from an NSArray of SKProduct instances
	void SetProductsInfo(NSArray SEOUL_NS_CONSUMED* aProductsInfo);

private:
	SEOUL_DISABLE_COPY(IOSCommerceManager);

	virtual void DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo) SEOUL_OVERRIDE;
	virtual void DoRefresh() SEOUL_OVERRIDE;

	/** Objective-C helper object */
	IOSCommerceManagerObjC* m_pCommerceObjC;

	CompletedTransaction* ConvertTransaction(SKPaymentTransaction* pCompletedTransaction, Bool bSuccess) const;
}; // class IOSCommerceManager

} // namespace Seoul

#endif // include guard
