/**
 * \file DeveloperCommerceManager.h
 * \brief Specialization of CommerceManager for development
 * and testing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef DEVELOPER_COMMERCE_MANAGER_H
#define DEVELOPER_COMMERCE_MANAGER_H

#include "Atomic32.h"
#include "CommerceManager.h"
#include "SharedPtr.h"
namespace Seoul { class DeveloperCommerceManagerSimulatedFirstParty; }
namespace Seoul { class DeveloperCommerceManagerSimulatedFirstPartyTransactionObject; }
namespace Seoul { class DeveloperCommerceManagerSimulatedItemInfo; }

namespace Seoul
{

#if !SEOUL_SHIP
/**
 * Equivalent to NullCommerceManager for developer builds.
 * Implements basic behavior for automatically approving and
 * handling transactions and provides some hooks for testing.
 */
class DeveloperCommerceManager SEOUL_SEALED : public CommerceManager
{
public:
	// Automated/unit testing functionality.
	static void UnitTest_SetCanMakePayments(Bool b);
	static void UnitTest_SetEnableCompletionDelivery(Bool b);
	static void UnitTest_SetEnableFinalizeAccept(Bool b);
	static void UnitTest_SetEnableSilentFailure(Bool b);
	static void UnitTest_SetPaymentResult(Bool b);
	static void UnitTest_SetRefreshInterrupt(Bool b);
	// WARNING: DeveloperCommerceManager::Get() must be invalid when calling this function.
	static void UnitTest_ClearTransactions();
	Atomic32Type GetAlreadyOwnedPurchaseAttempts() const;
	Atomic32Type GetFailedPurchaseCount() const;
	UInt32 GetPendingTransactionsCount() const;
	Atomic32Type GetSilentlyFailedPurchaseCount() const;
	Atomic32Type GetSuccessfulPurchaseCount() const;
	Atomic32Type GetIgnoredFinalizeAttempts() const;
	Atomic32Type GetRefreshCount() const;
	void UnitTest_OverrideItemInfoRefreshIntervalSeconds(Double fSeconds) { m_fItemInfoRefreshIntervalSeconds = fSeconds; }
	// /Automated/unit testing functionality.

	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<DeveloperCommerceManager> Get()
	{
		if (CommerceManager::Get() && CommerceManager::Get()->GetType() == CommerceManagerType::kDev)
		{
			return (DeveloperCommerceManager*)CommerceManager::Get().Get();
		}
		else
		{
			return CheckedPtr<DeveloperCommerceManager>();
		}
	}

	DeveloperCommerceManager();
	~DeveloperCommerceManager();

	virtual CommerceManagerType GetType() const SEOUL_OVERRIDE
	{
		return CommerceManagerType::kDev;
	}

	virtual const String& GetStoreName() const SEOUL_OVERRIDE;
	virtual Bool SupportsSubscriptions() const SEOUL_OVERRIDE { return false; }

	// Callback functions from first party.

	// Callback called when a transaction has completed (successfully or
	// unsuccessfully)
	void OnTransactionCompleted(DeveloperCommerceManagerSimulatedFirstPartyTransactionObject* pTransaction);

	// Sets our product info table from our simulated first party.
	void SetProductsInfo(const Vector<DeveloperCommerceManagerSimulatedItemInfo, MemoryBudgets::Commerce>& v);

private:
	SEOUL_DISABLE_COPY(DeveloperCommerceManager);

	virtual void DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction) SEOUL_OVERRIDE;
	virtual void DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo) SEOUL_OVERRIDE;
	virtual void DoRefresh() SEOUL_OVERRIDE;
	virtual void DoTick() SEOUL_OVERRIDE;
	// Unit testing hook - allows interruption of OnItemPurchaseFinalized().
#if SEOUL_UNIT_TESTS
	virtual Bool UnitTestingHook_OnFinalizeAccept() SEOUL_OVERRIDE;
#endif // /SEOUL_UNIT_TESTS

	/** Shared pointer around a simualted first-party provider. Designed to mimic IOSCommerceManagerObjC. */
	SharedPtr<DeveloperCommerceManagerSimulatedFirstParty> m_pSimulatedFirstParty;

	CompletedTransaction* ConvertTransaction(DeveloperCommerceManagerSimulatedFirstPartyTransactionObject* pCompletedTransaction) const;
}; // class DeveloperCommerceManager
#endif // /#if !SEOUL_SHIP

} // namespace Seoul

#endif // include guard
