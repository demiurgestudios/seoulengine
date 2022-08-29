/**
 * \file AndroidCommerceManager.cpp
 * \brief Native code side of IAP support on Android. Backend
 * is written in Java and may be driven by either Google Play,
 * Samsung, or Amazon IAPs.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidCommerceManager.h"
#include "AndroidMainThreadQueue.h"
#include "AndroidPrereqs.h"
#include "EngineVirtuals.h"
#include "FromString.h"
#include "JobsFunction.h"
#include "JobsManager.h"
#include "Logger.h"
#include "ReflectionUtil.h"
#include "SeoulTime.h"
#include "SeoulTypes.h"

namespace Seoul
{

// Bindings used by JNI hooks, see below.
static void HandleTransactionCompleted(
	CommerceManager::ProductID productID,
	String sTransactionID,
	String sReceiptData,
	String sPurchaseToken,
	CommerceManager::EPurchaseResult eResult)
{
	if (AndroidCommerceManager::Get())
	{
		AndroidCommerceManager::Get()->OnTransactionCompleted(
			productID,
			sTransactionID,
			sReceiptData,
			sPurchaseToken,
			(CommerceManager::EPurchaseResult)eResult);
	}
}

static void HandleSetProductsInfo(Bool bSuccess, CommerceManager::ProductInfoVector v)
{
	if (AndroidCommerceManager::Get())
	{
		AndroidCommerceManager::Get()->SetProductUSDPrices(v);
		AndroidCommerceManager::Get()->SetProductsInfo(bSuccess, v);
	}
	else
	{
		SEOUL_LOG_COMMERCE("%s: NULL AndroidCommerceManager", __FUNCTION__);
	}
}

static void HandleInventoryUpdated(Vector<AndroidCommerceManager::PurchaseData> v)
{
	if (AndroidCommerceManager::Get())
	{
		AndroidCommerceManager::Get()->OnInventoryUpdated(v);
	}
	else
	{
		SEOUL_LOG_COMMERCE("%s: NULL AndroidCommerceManager", __FUNCTION__);
	}
}

// /Bindings used by JNI hooks, see below.

} // namespace Seoul

// JNI hooks for callbacks from Java into C++ code.
extern "C"
{

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidCommerceManager_NativeSetProductsInfo(
	JNIEnv* pJniEnvironment,
	jclass,
	jboolean bSuccess,
	jobjectArray productInfo)
{
	using namespace Seoul;

	auto const uLength = (UInt32)Max(pJniEnvironment->GetArrayLength(productInfo), 0);

	CommerceManager::ProductInfoVector v;
	v.Reserve(uLength);
	for (UInt32 i = 0u; i < uLength; ++i)
	{
		auto const in = pJniEnvironment->GetObjectArrayElement(productInfo, i);

		String sProductID;
		SetStringFromJavaObjectField(pJniEnvironment, in, "ProductID", sProductID);

		// TODO: All Android CommerceManagers can determine the product type.
		//
		// The CommerceManager however still gets the type from microtransactions.json
		// as other platforms cannot determine this information.
		/*
		Int iAndroidCommerceManagerProductType;
		GetEnumOrdinalFromJavaObjectField(
			pJniEnvironment,
			in,
			"Type",
			"Lcom/demiurgestudios/seoul/AndroidCommerceProductType;",
			iAndroidCommerceManagerProductType);
		*/

		CommerceManager::ProductInfo info;
		info.m_ProductID = CommerceManager::ProductID(sProductID);
		SetStringFromJavaObjectField(pJniEnvironment, in, "Name", info.m_sName);
		SetStringFromJavaObjectField(pJniEnvironment, in, "Description", info.m_sDescription);
		SetStringFromJavaObjectField(pJniEnvironment, in, "PriceString", info.m_sPrice);
		SetStringFromJavaObjectField(pJniEnvironment, in, "CurrencyCode", info.m_sCurrencyCode);

		Int64 lPriceMicros = GetInt64FromJavaObjectField(pJniEnvironment, in, "PriceMicros");
		info.m_fPrice = ((Float)lPriceMicros) / 1000000.0;

		v.PushBack(info);

		// Clean up JVM references used in this loop iteration to avoid hitting the max.
		// See https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#local_references
		pJniEnvironment->DeleteLocalRef(in);
	}

	RunOnMainThread(&HandleSetProductsInfo, (Bool)(!!bSuccess), v);
}

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidCommerceManager_NativeInventoryUpdated(
	JNIEnv* pJniEnvironment,
	jclass,
	jobjectArray inventory)
{
	using namespace Seoul;

	auto const uLength = (UInt32)Max(pJniEnvironment->GetArrayLength(inventory), 0);

	Vector<AndroidCommerceManager::PurchaseData> v;
	v.Reserve(uLength);
	for (UInt32 i = 0u; i < uLength; ++i)
	{
		auto const in = pJniEnvironment->GetObjectArrayElement(inventory, i);

		AndroidCommerceManager::PurchaseData p;
		SetStringFromJavaObjectField(pJniEnvironment, in, "ProductID", p.m_sProductID);
		SetStringFromJavaObjectField(pJniEnvironment, in, "ReceiptData", p.m_sReceiptData);
		SetStringFromJavaObjectField(pJniEnvironment, in, "TransactionID", p.m_sTransactionID);
		SetStringFromJavaObjectField(pJniEnvironment, in, "PurchaseToken", p.m_sPurchaseToken);
		v.PushBack(p);

		pJniEnvironment->DeleteLocalRef(in);
	}

	RunOnMainThread(&HandleInventoryUpdated, v);
}

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidCommerceManager_NativeTransactionCompleted(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring javaProductID,
	jstring transactionID,
	jstring receiptData,
	jstring purchaseToken,
	jint eResult)
{
	using namespace Seoul;

	CommerceManager::ProductID productID;
	SetProductIDFromJava(pJniEnvironment, javaProductID, productID);

	String sTransactionID;
	SetStringFromJava(pJniEnvironment, transactionID, sTransactionID);

	String sReceiptData;
	SetStringFromJava(pJniEnvironment, receiptData, sReceiptData);

	String sPurchaseToken;
	SetStringFromJava(pJniEnvironment, purchaseToken, sPurchaseToken);

	RunOnMainThread(
		&HandleTransactionCompleted,
		productID,
		sTransactionID,
		sReceiptData,
		sPurchaseToken,
		(CommerceManager::EPurchaseResult)eResult);
}

} // extern "C"

namespace Seoul
{

// Constants for filling out receipt data.
static const String ksGooglePlay("GooglePlay");
static const String ksAmazon("Amazon");
static const String ksSamsung("Samsung");

AndroidCommerceManager::AndroidCommerceManager(const AndroidCommerceManagerSettings& settings)
	: m_Settings(settings)
{
	SEOUL_ASSERT(IsMainThread());

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"AndroidCommerceManagerInitialize",
		"(Z)V",
		!SEOUL_SHIP);
}

AndroidCommerceManager::~AndroidCommerceManager()
{
	SEOUL_ASSERT(IsMainThread());

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"AndroidCommerceManagerShutdown",
		"()V");
}

const String& AndroidCommerceManager::GetStoreName() const
{
	if (IsAmazonPlatformFlavor(m_Settings.m_eDevicePlatformFlavor))
	{
		return ksAmazon;
	}
	else if (IsSamsungPlatformFlavor(m_Settings.m_eDevicePlatformFlavor))
	{
		return ksSamsung;
	}
	else
	{
		return ksGooglePlay;
	}
}

void AndroidCommerceManager::OnTransactionCompleted(
	const ProductID& productID,
	const String& sTransactionID,
	const String& sReceiptData,
	const String& sPurchaseToken,
	EPurchaseResult eResult)
{
	SEOUL_ASSERT(IsMainThread());

	SEOUL_LOG_COMMERCE("%s: Received purchase ('%s', '%s', '%s', %d)",
		__FUNCTION__,
		productID.m_sProductID.CStr(),
		sTransactionID.CStr(),
		sPurchaseToken.CStr(),
		(Int)eResult);

	// Convert the transaction into a tracking object.
	auto pCompletedTransaction = ConvertTransaction(productID, sTransactionID, sReceiptData, sPurchaseToken, eResult);

	// Dispatch.
	OnCompletedTransaction(pCompletedTransaction);
}

void AndroidCommerceManager::SetProductUSDPrices(CommerceManager::ProductInfoVector& v)
{
	for (auto & e : v)
	{
		e.m_fUSDPrice = EstimateUSDPrice(e.m_ProductID, e.m_fPrice, HString(e.m_sCurrencyCode));
	}
}

void AndroidCommerceManager::SetProductsInfo(Bool bSuccess, const CommerceManager::ProductInfoVector& v)
{
	SEOUL_ASSERT(IsMainThread());

#if SEOUL_LOGGING_ENABLED
	SEOUL_LOG_COMMERCE("%s: Received %u products, %s", __FUNCTION__, v.GetSize(), (bSuccess ? "success" : "failure"));
	for (auto const& e : v)
	{
		SEOUL_LOG_COMMERCE("%s: Product ('%s', %s, '%s', '%s', %f, %f USD, %s)",
			__FUNCTION__,
			e.m_ProductID.m_sProductID.CStr(),
			e.m_sName.CStr(),
			e.m_sDescription.CStr(),
			e.m_sPrice.CStr(),
			e.m_fPrice,
			e.m_fUSDPrice,
			e.m_sCurrencyCode.CStr());
	}
#endif

	// Update fields in items from the commerce data.
	if (bSuccess)
	{
		OnReceiveProductInfo(v);
	}
	// Otherwise, just report an empty vector.
	else
	{
		OnReceiveProductInfo(ProductInfoVector());
	}
}

void AndroidCommerceManager::OnInventoryUpdated(Vector<PurchaseData>& vInventory)
{
	SEOUL_ASSERT(IsMainThread());

	// Convert.
	CommerceManager::Inventory vCommerceInventory;
	vCommerceInventory.Reserve(vInventory.GetSize());
	for (auto const& p : vInventory)
	{
		auto pCompletedTransaction = ConvertTransaction(
			p.m_sProductID,
			p.m_sTransactionID,
			p.m_sReceiptData,
			p.m_sPurchaseToken,
			EPurchaseResult::kResultSuccess);
		vCommerceInventory.PushBack(pCompletedTransaction);
	}

	// Dispatch.
	OnReceiveInventory(vCommerceInventory);
}

void AndroidCommerceManager::DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	SEOUL_ASSERT(IsMainThread());

	String* pTransactionObject = (String*)pCompletedTransaction->m_pTransactionObject;
	pCompletedTransaction->m_pTransactionObject = nullptr;

	if (nullptr != pTransactionObject)
	{
		// See comment on DoDestroyTransactionObject() - do not finish. Shutdown, want to leave for processing next run.

		// Cleanup.
		SafeDelete(pTransactionObject);
	}
}

void AndroidCommerceManager::DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	SEOUL_ASSERT(IsMainThread());

	String* pTransactionObject = (String*)pCompletedTransaction->m_pTransactionObject;
	String sTransactionID = pCompletedTransaction->m_sTransactionID;
	pCompletedTransaction->m_pTransactionObject = nullptr;

	if (nullptr != pTransactionObject)
	{
		Bool bDestroyTransaction = true;

		if (pCompletedTransaction->m_eResult == EPurchaseResult::kResultSuccess)
		{
			// Attach a Java environment to the current thread.
			ScopedJavaEnvironment scope;
			JNIEnv* pEnvironment = scope.GetJNIEnv();

			auto pItemInfo = GetItemInfoForProduct(pCompletedTransaction->m_ProductID);
			if (nullptr == pItemInfo)
			{
				SEOUL_LOG_COMMERCE("%s: No ItemInfo found for Product %s. Cannot determine whether to consume or acknowledge.",
					__FUNCTION__, pCompletedTransaction->m_ProductID.m_sProductID.CStr());
			}
			else
			{
				switch (pItemInfo->m_eType)
				{
					case kItemTypeConsumable:
					{
						// Consume the product.
						Java::Invoke<void>(
							pEnvironment,
							m_Settings.m_pNativeActivity->clazz,
							"AndroidCommerceManagerConsumeItem",
							"(Ljava/lang/String;Ljava/lang/String;)V",
							*pTransactionObject,
							sTransactionID);
						break;
					}
					case kItemTypeSubscription:
					{
						// Do not destroy subscription transactions
						bDestroyTransaction = false;

						// Acknowledge the product.
						Java::Invoke<void>(
							pEnvironment,
							m_Settings.m_pNativeActivity->clazz,
							"AndroidCommerceManagerAcknowledgeItem",
							"(Ljava/lang/String;Ljava/lang/String;)V",
							*pTransactionObject,
							sTransactionID);
						break;
					}
					default:
					{
						SEOUL_LOG_COMMERCE("%s: Unsupported ItemType %s for Product %s. Cannot finish Transaction.",
							__FUNCTION__, EnumToString<ItemType>(pItemInfo->m_eType), pCompletedTransaction->m_ProductID.m_sProductID.CStr());
						break;
					}
				}
			}
		}

		// Cleanup.
		if (bDestroyTransaction)
		{
			SafeDelete(pTransactionObject);
		}
	}
}

void AndroidCommerceManager::DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo)
{
	SEOUL_ASSERT(IsMainThread());

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"AndroidCommerceManagerPurchaseItem",
		"(Ljava/lang/String;)V",
		itemInfo.m_ProductInfo.m_ProductID.m_sProductID);
}

void AndroidCommerceManager::DoRefresh()
{
	// Assemble list of active SKUs.
	Vector<ProductID> vIds;
	GetAllPlatformItemIDs(vIds);

	Vector<String> v;
	v.Reserve(vIds.GetSize());
	for (auto const& e : vIds) { v.PushBack(e.m_sProductID); }

	// Attach a Java environment to the current thread.
	ScopedJavaEnvironment scope;
	JNIEnv* pEnvironment = scope.GetJNIEnv();

	// Dispatch to Java.
	Java::Invoke<void>(
		pEnvironment,
		m_Settings.m_pNativeActivity->clazz,
		"AndroidCommerceManagerRefreshProductInfo",
		"([Ljava/lang/String;)V",
		v);
}

CommerceManager::CompletedTransaction* AndroidCommerceManager::ConvertTransaction(
	const ProductID& productID,
	const String& sTransactionID,
	const String& sReceiptData,
	const String& sPurchaseToken,
	EPurchaseResult eResult) const
{
	// Make a receipt data object.
	ScopedPtr<PurchaseReceiptData> pPurchaseReceiptData(SEOUL_NEW(MemoryBudgets::Commerce) PurchaseReceiptData);
	pPurchaseReceiptData->m_sPayload = sReceiptData;
	pPurchaseReceiptData->m_sStore = GetStoreName();
	pPurchaseReceiptData->m_sTransactionID = sTransactionID;
	pPurchaseReceiptData->m_sPurchaseToken = sPurchaseToken;

	// Allocate and return.
	auto pReturn(SEOUL_NEW(MemoryBudgets::Commerce) CompletedTransaction);
	pReturn->m_eResult = eResult;
	pReturn->m_pPurchaseReceiptData.Swap(pPurchaseReceiptData);
	pReturn->m_ProductID = productID;
	pReturn->m_pTransactionObject = SEOUL_NEW(MemoryBudgets::Commerce) String(productID.m_sProductID);
	pReturn->m_sTransactionID = sTransactionID;

	// Done.
	return pReturn;
}

} // namespace Seoul
