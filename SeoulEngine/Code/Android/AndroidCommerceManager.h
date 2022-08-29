/**
 * \file AndroidCommerceManager.h
 * \brief Native code side of IAP support on Android. Backend
 * is written in Java and may be driven by either Google Play
 * or Amazon IAPs.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_COMMERCE_MANAGER_H
#define ANDROID_COMMERCE_MANAGER_H

#include "CommerceManager.h"
#include "Mutex.h"
#include "PlatformFlavor.h"
#include "Queue.h"

// Forward declarations
struct ANativeActivity;

namespace Seoul
{

struct AndroidCommerceManagerSettings
{
	AndroidCommerceManagerSettings()
		: m_pNativeActivity()
		, m_eDevicePlatformFlavor(PlatformFlavor::kGooglePlayDevelopment)
	{
	}

	CheckedPtr<ANativeActivity> m_pNativeActivity;
	PlatformFlavor m_eDevicePlatformFlavor;
};

/**
 * Implementation of CommerceManager for the Android platform.
 */
class AndroidCommerceManager SEOUL_SEALED : public CommerceManager
{
public:
	/** 
	 * Purchase Data provided from Java.
	 */
	struct PurchaseData
	{
		String m_sProductID;
		String m_sReceiptData;
		String m_sTransactionID;
		String m_sPurchaseToken;
	};

	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<AndroidCommerceManager> Get()
	{
		if (CommerceManager::Get() && CommerceManager::Get()->GetType() == CommerceManagerType::kAndroid)
		{
			return (AndroidCommerceManager*)CommerceManager::Get().Get();
		}
		else
		{
			return CheckedPtr<AndroidCommerceManager>();
		}
	}

	AndroidCommerceManager(const AndroidCommerceManagerSettings& settings);
	~AndroidCommerceManager();

	virtual CommerceManagerType GetType() const SEOUL_OVERRIDE
	{
		return CommerceManagerType::kAndroid;
	}

	virtual const String& GetStoreName() const SEOUL_OVERRIDE;
	virtual Bool SupportsSubscriptions() const SEOUL_OVERRIDE { return true; }

	// Callback functions from first party.

	// Callback called when a transaction has completed (successfully or
	// unsuccessfully)
	void OnTransactionCompleted(
		const ProductID& productID,
		const String& sTransactionID,
		const String& sReceiptData,
		const String& sPurchaseToken,
		EPurchaseResult eResult);

	// Determined the estimated USD prices for products returned from Java.
	void SetProductUSDPrices(ProductInfoVector& vProductsInfo);
	// List of product info returned from Java.
	void SetProductsInfo(Bool bSuccess, const ProductInfoVector& vProductsInfo);
	// Inventory received from Java.
	void OnInventoryUpdated(Vector<PurchaseData>& vInventory);

private:
	SEOUL_DISABLE_COPY(AndroidCommerceManager);

	virtual void DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo) SEOUL_OVERRIDE;
	virtual void DoRefresh() SEOUL_OVERRIDE;

	/** Configuration of AndroidCommerceManager */
	AndroidCommerceManagerSettings const m_Settings;

	CompletedTransaction* ConvertTransaction(
		const ProductID& productID,
		const String& sTransactionID,
		const String& sReceiptData,
		const String& sPurchaseToken,
		EPurchaseResult eResult) const;
}; // class AndroidCommerceManager

} // namespace Seoul

#endif // include guard
