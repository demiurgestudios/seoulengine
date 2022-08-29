/**
 * \file DeveloperCommerceManager.cpp
 * \brief Specialization of CommerceManager for development
 * and testing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Atomic32.h"
#include "DeveloperCommerceManager.h"
#include "EngineVirtuals.h"
#include "JobsFunction.h"
#include "Logger.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SettingsManager.h"
#include "SeoulUUID.h"
#include "SharedPtr.h"
#include "ThreadId.h"
namespace Seoul { class DeveloperCommerceManagerSimulatedFirstParty; }
#if !SEOUL_SHIP

namespace Seoul
{

static Atomic32Value<Bool> s_bCanMakePayments(true);
static Atomic32Value<Bool> s_bEnableCompletionDelivery(true);
static Atomic32Value<Bool> s_bEnableFinalizeAccept(true);
static Atomic32Value<Bool> s_bEnableSilentFailures(false);
static Atomic32Value<Bool> s_bPaymentResult(true);
static Atomic32Value<Bool> s_bRefreshInterrupt(false);

static Atomic32 s_AlreadyOwnedAttempts;
static Atomic32 s_Failed;
static Atomic32 s_SilentFailed;
static Atomic32 s_Succeeded;
static Atomic32 s_IgnoredFinalizeAttempts;
static Atomic32 s_RefreshCount;

// Constants for filling out receipt data.
static const String ksFakeDeveloperStore("FakeDeveloperStore");

// Poor man's persistence.
static Vector< SharedPtr<DeveloperCommerceManagerSimulatedFirstPartyTransactionObject>, MemoryBudgets::Commerce > s_vpTransactions;
static Mutex s_FirstPartyMutex;

void DeveloperCommerceManager::UnitTest_SetCanMakePayments(Bool b)
{
	s_bCanMakePayments = b;
}

void DeveloperCommerceManager::UnitTest_SetEnableFinalizeAccept(Bool b)
{
	s_bEnableFinalizeAccept = b;
}

void DeveloperCommerceManager::UnitTest_SetEnableCompletionDelivery(Bool b)
{
	s_bEnableCompletionDelivery = b;
}

void DeveloperCommerceManager::UnitTest_SetEnableSilentFailure(Bool b)
{
	s_bEnableSilentFailures = b;
}

void DeveloperCommerceManager::UnitTest_SetPaymentResult(Bool b)
{
	s_bPaymentResult = b;
}

void DeveloperCommerceManager::UnitTest_SetRefreshInterrupt(Bool b)
{
	s_bRefreshInterrupt = b;
}

// WARNING: DeveloperCommerceManager::Get() must be invalid when calling this function.
void DeveloperCommerceManager::UnitTest_ClearTransactions()
{
	SEOUL_ASSERT(!DeveloperCommerceManager::Get());
	Lock lock(s_FirstPartyMutex);
	s_vpTransactions.Clear();
}
// Unit testing hook - allows interruption of OnItemPurchaseFinalized().
#if SEOUL_UNIT_TESTS
Bool DeveloperCommerceManager::UnitTestingHook_OnFinalizeAccept()
{
	SEOUL_ASSERT(IsMainThread());

	// Early out if disabled.
	if (!s_bEnableFinalizeAccept)
	{
		++s_IgnoredFinalizeAttempts;
		return false;
	}

	return true;
}
#endif // /SEOUL_UNIT_TESTS

class DeveloperCommerceManagerSimulatedItemInfo SEOUL_SEALED
{
public:
	String m_sProductID{};
	String m_sDescription{};
	String m_sName{};
	String m_sPrice{};
	Float m_fPrice{};
	String m_sCurrencyCode{};
	Float m_fUSDPrice{};
};

class DeveloperCommerceManagerSimulatedFirstPartyTransactionObject SEOUL_SEALED
{
public:
	DeveloperCommerceManagerSimulatedFirstPartyTransactionObject(const String& sProductID)
		: m_sProductID(sProductID)
		, m_sTransactionID(UUID::GenerateV4().ToString())
		, m_bSuccess(s_bPaymentResult)
	{
	}

	void Finish()
	{
		m_bFinished = true;
	}

	Bool IsFinished() const
	{
		return m_bFinished;
	}

	Bool IsNotified() const
	{
		return m_bNotified;
	}

	void Notify()
	{
		m_bNotified = true;
	}

	void ResetNotify()
	{
		m_bNotified = false;
	}

	String const m_sProductID{};
	String const m_sTransactionID{};
	Bool const m_bSuccess{};

private:
	Atomic32Value<Bool> m_bNotified{ false };
	Atomic32Value<Bool> m_bFinished{ false };

	SEOUL_DISABLE_COPY(DeveloperCommerceManagerSimulatedFirstPartyTransactionObject);
	SEOUL_REFERENCE_COUNTED(DeveloperCommerceManagerSimulatedFirstPartyTransactionObject);
}; // class DeveloperCommerceManagerSimulatedFirstPartyTransactionObject

static inline Bool operator==(const SharedPtr<DeveloperCommerceManagerSimulatedFirstPartyTransactionObject>& a, const String& b)
{
	return (a->m_sProductID == b);
}

static void ResetPersistentData()
{
	s_bCanMakePayments = true;
	s_bEnableCompletionDelivery = true;
	s_bEnableFinalizeAccept = true;
	s_bEnableSilentFailures = false;
	s_bPaymentResult = true;
	s_bRefreshInterrupt = false;

	s_AlreadyOwnedAttempts.Reset();
	s_Failed.Reset();
	s_SilentFailed.Reset();
	s_Succeeded.Reset();
	s_IgnoredFinalizeAttempts.Reset();
	s_RefreshCount.Reset();

	Lock lock(s_FirstPartyMutex);
	for (auto& p : s_vpTransactions)
	{
		p->ResetNotify();
	}
}

struct DeveloperCommerceManagerSimulatedProduct SEOUL_SEALED
{
	DeveloperCommerceManagerSimulatedProduct(const String& sID)
		: m_sID(sID)
	{
	}

	String m_sID{};
};

class DeveloperCommerceManagerSimulatedRequestJob SEOUL_SEALED : public Jobs::Job
{
public:
	template <typename T>
	static T Clone(const T& t)
	{
		T ret;
		for (auto const& pair : t)
		{
			SEOUL_VERIFY(ret.Insert(pair.First, SEOUL_NEW(MemoryBudgets::Commerce) CommerceManager::ItemInfo(*pair.Second)).Second);
		}
		return ret;
	}

	DeveloperCommerceManagerSimulatedRequestJob(const CommerceManager::ItemInfoTable& tItemInfo)
		: m_tItemInfo(Clone(tItemInfo))
	{
	}

	~DeveloperCommerceManagerSimulatedRequestJob()
	{
		WaitUntilJobIsNotRunning();

		SafeDeleteTable(const_cast<CommerceManager::ItemInfoTable&>(m_tItemInfo));
	}

	void SetDelegate(DeveloperCommerceManagerSimulatedFirstParty* p)
	{
		Lock lock(s_FirstPartyMutex);
		m_pDelegate = p;
	}

private:
	SEOUL_DISABLE_COPY(DeveloperCommerceManagerSimulatedRequestJob);
	SEOUL_REFERENCE_COUNTED_SUBCLASS(DeveloperCommerceManagerSimulatedRequestJob);

	virtual void InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId) SEOUL_OVERRIDE;

	CheckedPtr<DeveloperCommerceManagerSimulatedFirstParty> m_pDelegate;
	const CommerceManager::ItemInfoTable m_tItemInfo;
}; // class DeveloperCommerceManagerSimulatedRequestJob

class DeveloperCommerceManagerSimulatedFirstParty SEOUL_SEALED
{
public:
	DeveloperCommerceManagerSimulatedFirstParty()
	{
	}

	~DeveloperCommerceManagerSimulatedFirstParty()
	{
		Lock lock(s_FirstPartyMutex);
		SafeDeleteVector(m_vPendingPurchases);
		if (m_pSimulatedRequest.IsValid())
		{
			m_pSimulatedRequest->SetDelegate(nullptr);
			m_pSimulatedRequest->WaitUntilJobIsNotRunning();
			m_pSimulatedRequest.Reset();
		}
	}

	void AddObserver(DeveloperCommerceManager* p)
	{
		SEOUL_ASSERT(m_vObservers.End() == m_vObservers.Find(p));

		Lock lock(s_FirstPartyMutex);
		m_vObservers.PushBack(p);
	}

	void AddPaymentWithProduct(DeveloperCommerceManagerSimulatedProduct* pProduct)
	{
		Lock lock(s_FirstPartyMutex);

		// Match behavior on our mobile platforms - if pProduct is a request
		// for a consumable that is still pending finalization, ignore the request
		// and just return the one in the queue.
		if (s_vpTransactions.Contains(pProduct->m_sID))
		{
			SEOUL_LOG_COMMERCE("AddPaymentWithProduct: pProduct %s is a request for a consumable that is still pending finalization, ignore the request.", pProduct->m_sID.CStr());
			++s_AlreadyOwnedAttempts;
			SafeDelete(pProduct);
		}
		else
		{
			SEOUL_LOG_COMMERCE("AddPaymentWithProduct: Adding %s to pending purchases", pProduct->m_sID.CStr());
			m_vPendingPurchases.PushBack(pProduct);
		}
	}

	Bool CanMakePayments() const
	{
		return s_bCanMakePayments;
	}

	void InitProductList()
	{
		auto const& tItemInfo(CommerceManager::Get()->GetItemInfoTable());

		if (tItemInfo.IsEmpty())
		{
			SEOUL_LOG_COMMERCE("%s: No known products, skipping products request", __FUNCTION__);
			return;
		}

		if (m_pSimulatedRequest.IsValid())
		{
			if (m_pSimulatedRequest->IsJobRunning())
			{
				SEOUL_LOG_COMMERCE("%s: Request already pending, skipping.", __FUNCTION__);
				return;
			}

			m_pSimulatedRequest.Reset();
		}

		SEOUL_LOG_COMMERCE("%s: Requesting product info for %u products", __FUNCTION__, (UInt)tItemInfo.GetSize());
		m_pSimulatedRequest.Reset(SEOUL_NEW(MemoryBudgets::Commerce) DeveloperCommerceManagerSimulatedRequestJob(
			tItemInfo));
		m_pSimulatedRequest->SetDelegate(this);
		m_pSimulatedRequest->StartJob();
	}

	void Poll()
	{
		{
			Lock lock(s_FirstPartyMutex);

			// Consume.
			{
				for (auto& pIn : m_vPendingPurchases)
				{
					auto p = SEOUL_NEW(MemoryBudgets::Commerce) DeveloperCommerceManagerSimulatedFirstPartyTransactionObject(pIn->m_sID);
					SafeDelete(pIn);

					s_vpTransactions.PushBack(SharedPtr<DeveloperCommerceManagerSimulatedFirstPartyTransactionObject>(p));
				}
				m_vPendingPurchases.Clear();
			}

			// Now report.
			if (s_bEnableCompletionDelivery)
			{
				for (auto i = s_vpTransactions.Begin(); s_vpTransactions.End() != i; ++i)
				{
					if (!(*i)->IsNotified())
					{
						(*i)->Notify();
						for (auto const& pOut : m_vObservers)
						{
							pOut->OnTransactionCompleted(i->GetPtr());
						}
					}
				}

				// Now do a second pass to remove any transactions that were finished.
			}

			// Last step, handle finalized transactions.
			{
				for (auto i = s_vpTransactions.Begin(); s_vpTransactions.End() != i; )
				{
					// Remove and finish.
					if ((*i)->IsFinished())
					{
						auto const bSuccess = (*i)->m_bSuccess;

						// Remove.
						i = s_vpTransactions.Erase(i);

						// When finalizing is complete, we can now fully count
						// this transaction as a success or failure.
						if (bSuccess)
						{
							++s_Succeeded;
						}
						else
						{
							++s_Failed;
						}
					}
					else
					{
						++i;
					}
				}
			}
		}
	}

	void RemoveObserver(DeveloperCommerceManager* p)
	{
		Lock lock(s_FirstPartyMutex);

		SEOUL_ASSERT(m_vObservers.End() != m_vObservers.Find(p));
		m_vObservers.Erase(m_vObservers.Find(p));
	}

private:
	SEOUL_DISABLE_COPY(DeveloperCommerceManagerSimulatedFirstParty);
	SEOUL_REFERENCE_COUNTED(DeveloperCommerceManagerSimulatedFirstParty);

	friend class DeveloperCommerceManagerSimulatedRequestJob;
	void OnReceiveProductsInfo(const Vector<DeveloperCommerceManagerSimulatedItemInfo, MemoryBudgets::Commerce>& v)
	{
		Lock lock(s_FirstPartyMutex);
		for (auto const& p : m_vObservers)
		{
			p->SetProductsInfo(v);
		}
	}

	Vector<DeveloperCommerceManager*, MemoryBudgets::Cooking> m_vObservers;
	SharedPtr<DeveloperCommerceManagerSimulatedRequestJob> m_pSimulatedRequest;
	Vector<DeveloperCommerceManagerSimulatedProduct*, MemoryBudgets::Cooking> m_vPendingPurchases;
}; // class DeveloperCommerceManagerSimulatedFirstParty

void DeveloperCommerceManagerSimulatedRequestJob::InternalExecuteJob(Jobs::State& reNextState, ThreadId& rNextThreadId)
{
	if (rNextThreadId == GetMainThreadId())
	{
		Vector<DeveloperCommerceManagerSimulatedItemInfo, MemoryBudgets::Commerce> v;
		v.Reserve(m_tItemInfo.GetSize());
		for (auto const& pair : m_tItemInfo)
		{
			DeveloperCommerceManagerSimulatedItemInfo info;

			Int32 iCents = pair.Second->m_iUSDCentsPrice;
			info.m_sProductID = String(pair.Second->m_ProductInfo.m_ProductID.m_sProductID);
			info.m_sDescription = info.m_sProductID;
			info.m_sName = info.m_sProductID;
			info.m_fPrice = (Float)iCents / 100.0f;
			info.m_sPrice = String::Printf("$%.2f", info.m_fPrice);
			info.m_fUSDPrice = info.m_fPrice;
			info.m_sCurrencyCode = String("USD");
			v.PushBack(info);
		}

		{
			Lock lock(s_FirstPartyMutex);
			if (m_pDelegate)
			{
				m_pDelegate->OnReceiveProductsInfo(v);
			}
		}

		reNextState = Jobs::State::kComplete;
	}
	else
	{
		// Switch to the main thread. We insert this intentional
		// switch to delay the results a bit.
		rNextThreadId = GetMainThreadId();
	}
}

Atomic32Type DeveloperCommerceManager::GetAlreadyOwnedPurchaseAttempts() const
{
	return s_AlreadyOwnedAttempts;
}

Atomic32Type DeveloperCommerceManager::GetFailedPurchaseCount() const
{
	return s_Failed;
}

UInt32 DeveloperCommerceManager::GetPendingTransactionsCount() const
{
	UInt32 uReturn = 0u;
	{
		Lock lock(s_FirstPartyMutex);
		uReturn = s_vpTransactions.GetSize();
	}
	return uReturn;
}

Atomic32Type DeveloperCommerceManager::GetSilentlyFailedPurchaseCount() const
{
	return s_SilentFailed;
}

Atomic32Type DeveloperCommerceManager::GetSuccessfulPurchaseCount() const
{
	return s_Succeeded;
}

Atomic32Type DeveloperCommerceManager::GetIgnoredFinalizeAttempts() const
{
	return s_IgnoredFinalizeAttempts;
}

Atomic32Type DeveloperCommerceManager::GetRefreshCount() const
{
	return s_RefreshCount;
}

DeveloperCommerceManager::DeveloperCommerceManager()
	: m_pSimulatedFirstParty(nullptr)
{
	SEOUL_ASSERT(IsMainThread());

	// Reset state stored in static variables.
	ResetPersistentData();

	// Initialize our commerce object.
	m_pSimulatedFirstParty.Reset(SEOUL_NEW(MemoryBudgets::Commerce) DeveloperCommerceManagerSimulatedFirstParty);
	m_pSimulatedFirstParty->AddObserver(this);
}

DeveloperCommerceManager::~DeveloperCommerceManager()
{
	SEOUL_ASSERT(IsMainThread());

	// Cleanup commerce object.
	m_pSimulatedFirstParty->RemoveObserver(this);
	m_pSimulatedFirstParty.Reset();
}

const String& DeveloperCommerceManager::GetStoreName() const
{
	return ksFakeDeveloperStore;
}

/**
 * Callback called when a transaction has completed successfully
 */
void DeveloperCommerceManager::OnTransactionCompleted(DeveloperCommerceManagerSimulatedFirstPartyTransactionObject* pTransaction)
{
	SEOUL_ASSERT(IsMainThread());

	// Convert the transaction into a tracking object.
	auto pCompletedTransaction = ConvertTransaction(pTransaction);

	// Dispatch.
	OnCompletedTransaction(pCompletedTransaction);
}

/**
 * Called when product info has been received by first party.
 */
void DeveloperCommerceManager::SetProductsInfo(const Vector<DeveloperCommerceManagerSimulatedItemInfo, MemoryBudgets::Commerce>& v)
{
	SEOUL_ASSERT(IsMainThread());

	UInt32 const uSize = v.GetSize();
	ProductInfoVector vProductInfo;
	vProductInfo.Reserve(uSize);
	for (auto const& product : v)
	{
		// Set the product info.
		ProductInfo info;
		info.m_fPrice = product.m_fPrice;
		info.m_ProductID = ProductID(product.m_sProductID);
		info.m_sDescription = product.m_sDescription;
		info.m_sName = product.m_sName;
		info.m_sPrice = product.m_sPrice;
		vProductInfo.PushBack(info);
	}

	// Deliver.
	OnReceiveProductInfo(vProductInfo);
}

void DeveloperCommerceManager::DoDestroyTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	SEOUL_ASSERT(IsMainThread());

	DeveloperCommerceManagerSimulatedFirstPartyTransactionObject* pTransactionObject = (DeveloperCommerceManagerSimulatedFirstPartyTransactionObject*)pCompletedTransaction->m_pTransactionObject;
	pCompletedTransaction->m_pTransactionObject = nullptr;

	if (nullptr != pTransactionObject)
	{
		// See comment on DoDestroyTransactionObject() - do not finish. Shutdown, want to leave for processing next run.
		SeoulGlobalDecrementReferenceCount(pTransactionObject);
		pTransactionObject = nullptr;
	}
}

void DeveloperCommerceManager::DoFinishTransactionObject(CompletedTransaction* pCompletedTransaction)
{
	SEOUL_ASSERT(IsMainThread());

	DeveloperCommerceManagerSimulatedFirstPartyTransactionObject* pTransactionObject = (DeveloperCommerceManagerSimulatedFirstPartyTransactionObject*)pCompletedTransaction->m_pTransactionObject;
	pCompletedTransaction->m_pTransactionObject = nullptr;

	if (nullptr != pTransactionObject)
	{
		pTransactionObject->Finish();
		SeoulGlobalDecrementReferenceCount(pTransactionObject);
		pTransactionObject = nullptr;
	}
}

void DeveloperCommerceManager::DoPurchaseItem(HString sItemID, const ItemInfo& itemInfo)
{
	SEOUL_ASSERT(IsMainThread());

	// If s_bEnableSilentFailures is true,
	// then we note but otherwise stop processing
	// the purchase. This is meant to simulate a crash.
	if (s_bEnableSilentFailures)
	{
		SEOUL_LOG_COMMERCE("PurchaseItem: Failed due to enable of silent failures. Not reporting.\n");
		++s_SilentFailed;
		return;
	}

	if (!m_pSimulatedFirstParty->CanMakePayments())
	{
		SEOUL_LOG_COMMERCE("PurchaseItem: User is not authorized to make payments\n");
		OnCompletedTransaction(MakeFailureObject(itemInfo.m_ProductInfo.m_ProductID, EPurchaseResult::kResultCantMakePayments));
		++s_Failed;
		return;
	}

	// Get the SKProduct for the item.
	auto product = SEOUL_NEW(MemoryBudgets::Commerce) DeveloperCommerceManagerSimulatedProduct(itemInfo.m_ProductInfo.m_ProductID.m_sProductID);
	if (nullptr == product)
	{
		SEOUL_LOG_COMMERCE("Can't retrieve purchase info for product %s from first party\n", itemInfo.m_ProductInfo.m_ProductID.m_sProductID.CStr());
		OnCompletedTransaction(MakeFailureObject(itemInfo.m_ProductInfo.m_ProductID, EPurchaseResult::kResultProductUnavailable));
		++s_Failed;
		return;
	}

	// Queue up a payment
	m_pSimulatedFirstParty->AddPaymentWithProduct(product);
}

void DeveloperCommerceManager::DoRefresh()
{
	SEOUL_ASSERT(IsMainThread());

	// Track.
	++s_RefreshCount;

	// Populate our product list.
	m_pSimulatedFirstParty->InitProductList();

	// If enabled, trigger this now.
	if (s_bRefreshInterrupt)
	{
		s_bRefreshInterrupt = false;
		ReloadItemInfoTable();
	}
}

void DeveloperCommerceManager::DoTick()
{
	SEOUL_ASSERT(IsMainThread());

	// Process simulated first party.
	m_pSimulatedFirstParty->Poll();
}

DeveloperCommerceManager::CompletedTransaction* DeveloperCommerceManager::ConvertTransaction(
	DeveloperCommerceManagerSimulatedFirstPartyTransactionObject* pCompletedTransaction) const
{
	SEOUL_ASSERT(IsMainThread());

	// Acquire result.
	auto eResult = (!pCompletedTransaction || !pCompletedTransaction->m_bSuccess) ? EPurchaseResult::kResultSuccess : EPurchaseResult::kInternalPlatformError;

	// Populate receipt data on success.
	String sTransactionID;
	ScopedPtr<PurchaseReceiptData> pPurchaseReceiptData;
	if (eResult == EPurchaseResult::kResultSuccess)
	{
		// Gather transaction ID.
		sTransactionID = pCompletedTransaction->m_sTransactionID;

		// Gather and assign receipt data.
		if (!sTransactionID.IsEmpty())
		{
			// Fill out receipt data and assign for submission.
			pPurchaseReceiptData.Reset(SEOUL_NEW(MemoryBudgets::Commerce) PurchaseReceiptData);

			// Fill out.
			pPurchaseReceiptData->m_sPayload = String();
			pPurchaseReceiptData->m_sPayload2 = String();
			pPurchaseReceiptData->m_sStore = GetStoreName();
			pPurchaseReceiptData->m_sTransactionID = sTransactionID;
			pPurchaseReceiptData->m_sPurchaseToken = String();
		}
	}

	// Allocate and return.
	auto pReturn(SEOUL_NEW(MemoryBudgets::Commerce) CompletedTransaction);
	pReturn->m_eResult = eResult;
	pReturn->m_pPurchaseReceiptData.Swap(pPurchaseReceiptData);
	pReturn->m_ProductID = ProductID(pCompletedTransaction->m_sProductID);
	SeoulGlobalIncrementReferenceCount(pCompletedTransaction);
	pReturn->m_pTransactionObject = pCompletedTransaction;
	pReturn->m_sTransactionID = sTransactionID;

	// Done.
	return pReturn;
}

} // namespace Seoul

#endif // /#if !SEOUL_SHIP
