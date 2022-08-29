/**
 * \file ScriptEngineCommerceManager.cpp
 * \brief Binder instance for exposing CommerceManager functions into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ScriptEngineCommerceManager.h"
#include "ReflectionDefine.h"
#include "ScriptFunctionInterface.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineCommerceManager, TypeFlags::kDisableCopy)
	SEOUL_METHOD(PurchaseItem)
	SEOUL_METHOD(GetItemPrice)
	SEOUL_METHOD(FormatPrice)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(string, double)", "double iAmountInSmallestUnits, string sCurrencyName")
	SEOUL_METHOD(HasAllItemInfo)
	SEOUL_METHOD(OnItemPurchaseFinalized)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "string sItemID, string sFirstPartyTransactionID")
	SEOUL_METHOD(GetCommercePlatformId)
	SEOUL_METHOD(GetProductInfo)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "SlimCS.Table", "string sItemID")
SEOUL_END_TYPE()

ScriptEngineCommerceManager::ScriptEngineCommerceManager()
{
}

ScriptEngineCommerceManager::~ScriptEngineCommerceManager()
{
}

void ScriptEngineCommerceManager::PurchaseItem(HString sItemID)
{
	CommerceManager::Get()->PurchaseItem(sItemID);
}

String ScriptEngineCommerceManager::GetItemPrice(HString sItemID) const
{
	String sPrice;
	CommerceManager::Get()->GetItemPrice(sItemID, sPrice);
	return sPrice;
}


void ScriptEngineCommerceManager::FormatPrice(Script::FunctionInterface* pInterface) const
{
	Int32 amountInSmallestUnits;
	if (!pInterface->GetInteger(1u, amountInSmallestUnits))
	{
		pInterface->RaiseError(1u, "invalid target type, must be convertible to integer.");
		return;
	}

	HString currencyName;
	if (!pInterface->GetString(2u, currencyName))
	{
		pInterface->RaiseError(2u, "invalid event name, must be convertible to string.");
		return;
	}

	Float pOutPrice;
	String sRet = CommerceManager::Get()->FormatPrice(amountInSmallestUnits, currencyName.CStr(), &pOutPrice);

	pInterface->PushReturnString(sRet);
	pInterface->PushReturnNumber(pOutPrice);
}


Bool ScriptEngineCommerceManager::HasAllItemInfo() const
{
	return CommerceManager::Get()->HasAllItemInfo();
}

void ScriptEngineCommerceManager::OnItemPurchaseFinalized(HString sItemID, const String& sFirstPartyTransactionID)
{
	CommerceManager::Get()->OnItemPurchaseFinalized(sItemID, sFirstPartyTransactionID);
}

HString ScriptEngineCommerceManager::GetCommercePlatformId() const
{
	return CommerceManager::Get()->GetCommercePlatformId();
}

void ScriptEngineCommerceManager::GetProductInfo(Script::FunctionInterface* pInterface) const
{
	HString sItemID;
	if (!pInterface->GetString(1u, sItemID))
	{
		pInterface->RaiseError(1u, "invalid item id, must be convertible to string.");
		return;
	}

	auto pItemInfo = CommerceManager::Get()->GetItemInfo(sItemID);
	if (nullptr == pItemInfo)
	{
		pInterface->RaiseError(1u, "item id %s not found.", sItemID.CStr());
		return;
	}

	auto& productInfo = pItemInfo->m_ProductInfo;
	pInterface->PushReturnAsTable(productInfo);
}

} // namespace Seoul
